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

#pragma once

#include <atomic>
#include "compat/ConditionVariable.h"
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "compat/Mutex.h"
#include "compat/Thread.h"
#include "discoverer/IDiscoverer.h"

namespace medialibrary
{

class DiscovererWorker
{
public:
    DiscovererWorker( MediaLibraryPtr ml );
    ~DiscovererWorker();
    void addDiscoverer( std::unique_ptr<IDiscoverer> discoverer );
    void stop();

    bool discover( const std::string& entryPoint );
    void reload();
    void reload( const std::string& entryPoint );

private:
    void enqueue( const std::string& entryPoint, bool reload );
    void run();

private:
    struct Task
    {
        Task() = default;
        Task( const std::string& entryPoint, bool reload )
            : entryPoint( entryPoint ), reload( reload ) {}
        std::string entryPoint;
        bool reload;
    };

    compat::Thread m_thread;
    std::queue<Task> m_tasks;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    std::atomic_bool m_run;
    std::vector<std::unique_ptr<IDiscoverer>> m_discoverers;
    IMediaLibraryCb* m_cb;
};

}
