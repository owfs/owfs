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

Create an XML-RPC client for a 1-wire network.
"""


import xmlrpclib
import code


ow_sensor = xmlrpclib.ServerProxy( 'http://localhost:8765/' )
print 'Entries at the root:', ow_sensor.entries( '/' )

print 'List of sensors:'
sensors = ow_sensor.sensors( '/' )
for sensor in sensors:
    sensor_type = ow_sensor.attr( sensor, 'type' )
    if sensor_type == 'DS18S20':
        print '   ', sensor, '-', sensor_type, '- temperature:', ow_sensor.attr( sensor, 'temperature' ).strip( )
    else:
        print '   ', sensor, '-', sensor_type
