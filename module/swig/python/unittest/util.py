#! /usr/bin/env python

"""
::BOH
$Id$

Copyright (c) 2005 Peter Kropf. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::EOH

Utility functions for unittests.
"""

__version__ = '0.0-%s' % '$Id$'.split( )[ 2 ]

load = False


def find_config( config, type ):
    """
    Returns a list of sensor path names for all the sensors found in
    the various sections of a ConfigParser .ini file.

    [ '/29.400001001234', '/1F.440701000000/main/29.400900000000' ]
    """
    sensors = [ ]
    for section in config.sections( ):
        if config.has_option( section, 'type' ):
            if config.get( section, 'type' ) == type:
                parent = config.get( section, 'parent' )
                if parent == '/':
                    sensors.append( '/' + section )
                else:
                    sensors.append( parent + '/' + config.get( section, 'branch' ) + '/' + section )
    return sensors
