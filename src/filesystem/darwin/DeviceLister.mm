/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2016 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *          Felix Paul Kühne <fkuehne@videolan.org>
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

#import <Foundation/Foundation.h>
#import "DeviceLister.h"

# include "logging/Logger.h"
# include <vector>
# include <iostream>

#pragma mark - C++

std::vector<std::tuple<std::string, std::string, bool>> medialibrary::fs::DeviceLister::devices() const
{
    std::vector<std::tuple<std::string, std::string, bool>> res;
    try
    {
        NSArray *keys = [NSArray arrayWithObjects:NSURLVolumeUUIDStringKey, NSURLVolumeIsRemovableKey, nil];
        NSArray *mountPoints = [[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:keys options:0];

        if ( mountPoints.count == 0 )
        {
            LOG_WARN( "Failed to detect any mountpoint" );
            return res;
        }

        for ( NSURL *url in mountPoints )
        {
            bool isRemovable;
            std::string uuid;
            NSString *tmp;
            NSDictionary<NSURLResourceKey, id> *deviceInfo = [url resourceValuesForKeys:keys error:nil];

            tmp = deviceInfo[NSURLVolumeUUIDStringKey];
            uuid = ( tmp != nil ) ? [tmp UTF8String] : "";
            isRemovable = ( ( NSNumber * )deviceInfo[NSURLVolumeIsRemovableKey] ).boolValue;

           res.emplace_back( std::make_tuple( uuid, [[url absoluteString] UTF8String], isRemovable ) );
        }
    }
    catch( std::runtime_error& ex )
    {
        LOG_WARN( "Failed to list devices: ", ex.what(), ". Falling back to a dummy device containing '/'");
        res.emplace_back( std::make_tuple( "{dummy-device}", "file:///", false ) );
    }
    return res;
}
