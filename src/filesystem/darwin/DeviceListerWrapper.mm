/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2017 Hugo Beauzée-Luyssen, Videolabs
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
#import "DeviceListerWrapper.h"
#import "DeviceLister.h"

#include <vector>
#include <iostream>

#pragma mark - Wrapper

@interface DeviceListerWrapper()
{
    medialibrary::fs::DeviceLister *_deviceListerCpp;
}

@end

@implementation DeviceListerWrapper

- (id)init
{
    self = [super init];
    if (self)
    {
        NSLog(@"Init DeviceLister");
        _deviceListerCpp = new medialibrary::fs::DeviceLister;
        if (!_deviceListerCpp)
            NSLog(@"Failed to init DeviceLister");
        NSLog(@"After Init device lister");
    }
return self;
}

- (void)dealloc
{
    NSLog(@"Dealloc device lister");
    if (_deviceListerCpp)
        delete _deviceListerCpp;
    [super dealloc];
}

- (NSArray *)devices
{
    NSArray *res;
    NSMutableArray *mRes;

    auto deviceInfo =  _deviceListerCpp->devices();

    for ( const auto& it : deviceInfo )
    {
//        std::cout << it << std::endl;
    }    

    return nil;
}

@end