#! /usr/bin/env python

"""
::BOH
$Id$
$HeadURL: http://subversion/stuff/svn/owfs/trunk/examples/tree.py $

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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::EOH

Print the address and type of all sensors on a 1-wire network.
"""


import sys
import ow


__version__ = '0.0-%s' % '$Id$'.split( )[ 2 ]


def tree( sensor ):
    print '%7s - %s' % ( sensor._type, sensor._path )
    for next in sensor.sensors( ):
        if next._type in [ 'DS2409', ]:
            tree( next )
        else:
            print '%7s - %s' % ( next._type, next._path )


if __name__ == "__main__":
    if len( sys.argv ) == 1:
        print 'usage: tree.py u|serial_port_path'
        sys.exit( 1 )
    else:
        ow.init( sys.argv[ 1 ] )
        tree( ow.Sensor( '/' ) )
