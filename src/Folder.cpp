/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "File.h"
#include "Folder.h"
#include "Device.h"
#include "Media.h"

#include "database/SqliteTools.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IDevice.h"
#include "utils/Filename.h"

#include <unordered_map>

namespace medialibrary
{

namespace policy
{
    const std::string FolderTable::Name = "Folder";
    const std::string FolderTable::PrimaryKeyColumn = "id_folder";
    int64_t Folder::* const FolderTable::PrimaryKey = &Folder::m_id;
}

Folder::Folder( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_path
        >> m_parent
        >> m_isBlacklisted
        >> m_deviceId
        >> m_isPresent
        >> m_isRemovable;
}

Folder::Folder(MediaLibraryPtr ml, const std::string& path, int64_t parent, int64_t deviceId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_path( path )
    , m_parent( parent )
    , m_isBlacklisted( false )
    , m_deviceId( deviceId )
    , m_isPresent( true )
    , m_isRemovable( isRemovable )
{
}

bool Folder::createTable( DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name +
            "("
            "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT,"
            "parent_id UNSIGNED INTEGER,"
            "is_blacklisted BOOLEAN NOT NULL DEFAULT 0,"
            "device_id UNSIGNED INTEGER,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "is_removable BOOLEAN NOT NULL,"
            "FOREIGN KEY (parent_id) REFERENCES " + policy::FolderTable::Name +
            "(id_folder) ON DELETE CASCADE,"
            "FOREIGN KEY (device_id) REFERENCES " + policy::DeviceTable::Name +
            "(id_device) ON DELETE CASCADE,"
            "UNIQUE(path, device_id) ON CONFLICT FAIL"
            ")";
    std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_device_present AFTER UPDATE OF is_present ON "
            + policy::DeviceTable::Name +
            " BEGIN"
            " UPDATE " + policy::FolderTable::Name + " SET is_present = new.is_present WHERE device_id = new.id_device;"
            " END";
    return sqlite::Tools::executeRequest( connection, req ) &&
            sqlite::Tools::executeRequest( connection, triggerReq );
}

std::shared_ptr<Folder> Folder::create( MediaLibraryPtr ml, const std::string& fullPath,
                                        int64_t parentId, Device& device, fs::IDevice& deviceFs )
{
    std::string path;
    if ( device.isRemovable() == true )
        path = utils::file::removePath( fullPath, deviceFs.mountpoint() );
    else
        path = fullPath;
    auto self = std::make_shared<Folder>( ml, path, parentId, device.id(), device.isRemovable() );
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, parent_id, device_id, is_removable) VALUES(?, ?, ?, ?)";
    if ( insert( ml, self, req, path, sqlite::ForeignKey( parentId ), device.id(), device.isRemovable() ) == false )
        return nullptr;
    if ( device.isRemovable() == true )
    {
        self->m_deviceMountpoint = deviceFs.mountpoint();
        self->m_fullPath = self->m_deviceMountpoint.get() + path;
    }
    return self;
}

bool Folder::blacklist( MediaLibraryPtr ml, const std::string& fullPath )
{
    // Ensure we delete the existing folder if any & blacklist the folder in an "atomic" way
    auto t = ml->getConn()->newTransaction();

    auto f = fromPath( ml, fullPath, BannedType::Any );
    if ( f != nullptr )
    {
        // No need to blacklist a folder twice
        if ( f->m_isBlacklisted == true )
            return true;
        // Let the foreign key destroy everything beneath this folder
        destroy( ml, f->id() );
    }
    auto fsFactory = ml->fsFactoryForPath( fullPath );
    if ( fsFactory == nullptr )
        return false;
    auto folderFs = fsFactory->createDirectory( fullPath );
    if ( folderFs == nullptr )
        return false;
    auto deviceFs = folderFs->device();
    auto device = Device::fromUuid( ml, deviceFs->uuid() );
    if ( device == nullptr )
        device = Device::create( ml, deviceFs->uuid(), deviceFs->isRemovable() );
    std::string path;
    if ( deviceFs->isRemovable() == true )
        path = utils::file::removePath( fullPath, deviceFs->mountpoint() );
    else
        path = fullPath;
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, parent_id, is_blacklisted, device_id, is_removable) VALUES(?, ?, ?, ?, ?)";
    auto res = sqlite::Tools::executeInsert( ml->getConn(), req, path, nullptr, true, device->id(), deviceFs->isRemovable() ) != 0;
    t->commit();
    return res;
}

std::shared_ptr<Folder> Folder::fromPath( MediaLibraryPtr ml, const std::string& fullPath )
{
    return fromPath( ml, fullPath, BannedType::No );
}

std::shared_ptr<Folder> Folder::blacklistedFolder( MediaLibraryPtr ml, const std::string& fullPath )
{
    return fromPath( ml, fullPath, BannedType::Yes );
}

std::shared_ptr<Folder> Folder::fromPath( MediaLibraryPtr ml, const std::string& fullPath, BannedType bannedType )
{
    auto fsFactory = ml->fsFactoryForPath( fullPath );
    if ( fsFactory == nullptr )
        return nullptr;
    auto folderFs = fsFactory->createDirectory( fullPath );
    if ( folderFs == nullptr )
        return nullptr;
    auto deviceFs = folderFs->device();
    if ( deviceFs == nullptr )
    {
        LOG_ERROR( "Failed to get device containing an existing folder: ", folderFs->path() );
        return nullptr;
    }
    if ( deviceFs->isRemovable() == false )
    {
        std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE path = ? AND is_removable = 0";
        if ( bannedType == BannedType::Any )
            return fetch( ml, req, folderFs->path() );
        req += " AND is_blacklisted = ?";
            return fetch( ml, req, folderFs->path(), bannedType == BannedType::Yes ? true : false );
    }

    auto device = Device::fromUuid( ml, deviceFs->uuid() );
    // We are trying to find a folder. If we don't know the device it's on, we don't know the folder.
    if ( device == nullptr )
        return nullptr;
    auto path = utils::file::removePath( folderFs->path(), deviceFs->mountpoint() );
    std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE path = ? AND device_id = ?";
    std::shared_ptr<Folder> folder;
    if ( bannedType == BannedType::Any )
    {
        folder = fetch( ml, req, path, device->id() );
    }
    else
    {
        req += " AND is_blacklisted = ?";
        folder = fetch( ml, req, path, device->id(), bannedType == BannedType::Yes ? true : false );
    }
    if ( folder == nullptr )
        return nullptr;
    folder->m_deviceMountpoint = deviceFs->mountpoint();
    folder->m_fullPath = folder->m_deviceMountpoint.get() + path;
    return folder;
}

int64_t Folder::id() const
{
    return m_id;
}

const std::string& Folder::path() const
{
    if ( m_isRemovable == false )
        return m_path;

    auto lock = m_deviceMountpoint.lock();
    if ( m_deviceMountpoint.isCached() == true )
        return m_fullPath;

    auto device = Device::fetch( m_ml, m_deviceId );
    auto fsFactory = m_ml->fsFactoryForPath( m_fullPath );
    assert( fsFactory != nullptr );
    auto deviceFs = fsFactory->createDevice( device->uuid() );
    m_deviceMountpoint = deviceFs->mountpoint();
    m_fullPath = m_deviceMountpoint.get() + m_path;
    return m_fullPath;
}

std::vector<std::shared_ptr<File>> Folder::files()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +
        " WHERE folder_id = ?";
    return File::fetchAll<File>( m_ml, req, m_id );
}

std::vector<std::shared_ptr<Folder>> Folder::folders()
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE parent_id = ? AND is_blacklisted = 0 AND is_present = 1";
    return DatabaseHelpers::fetchAll<Folder>( m_ml, req, m_id );
}

std::shared_ptr<Folder> Folder::parent()
{
    return fetch( m_ml, m_parent );
}

int64_t Folder::deviceId() const
{
    return m_deviceId;
}

bool Folder::isPresent() const
{
    return m_isPresent;
}

bool Folder::isRootFolder() const
{
    return m_parent == 0;
}

std::vector<std::shared_ptr<Folder>> Folder::fetchRootFolders( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE parent_id IS NULL AND is_blacklisted = 0 AND is_present = 1";
    return DatabaseHelpers::fetchAll<Folder>( ml, req );
}

}
