#! /usr/bin/env python

"""
::BOH

Copyright (c) 2004 Peter Kropf. All rights reserved.

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
::EOH

Find all temperature sensors (DS18S20) and print out their current
temperature reading.
"""


import sys
import ow


def temperature( ):
    root = ow.Sensor( '/' )
    for sensor in root.find( type = 'DS18S20' ):
        print sensor._path, sensor.temperature


if __name__ == "__main__":
    if len( sys.argv ) == 1:
        print 'usage: temperature.py u|serial_port_path'
        sys.exit( 1 )
    else:
        ow.init( sys.argv[ 1 ] )
        temperature( )
