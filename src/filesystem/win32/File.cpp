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

#include <sys/types.h>
#include <sys/stat.h>

#include <stdexcept>

namespace medialibrary
{

namespace fs
{

File::File( const std::string &filePath )
    : CommonFile( filePath )
{
}

unsigned int File::lastModificationDate() const
{
    if ( m_lastModificationDate == 0 )
    {
        struct _stat s;
        if ( _stat( m_fullPath.c_str(), &s ) != 0 )
        {
            LOG_ERROR( "Failed to get ", m_fullPath, " stats" );
            throw std::system_error( GetLastError(), std::generic_category(), "Failed to get stats" );
        }

        m_lastModificationDate = s.st_mtime;
    }
    return m_lastModificationDate;
}

}

}
