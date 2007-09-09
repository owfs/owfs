#! /usr/bin/env python

"""
::BOH
$Id$

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

Example showing direct access to the underlying owfs libraries.
"""


import sys
from ow import _OW


def tree( path, indent = 0 ):
    raw = _OW.get( path )
    if raw:
        entries = raw.split( ',' )
        for entry in entries:
            print ' ' * indent, entry
            if entry[ -1 ] == '/':
                tree( entry, indent + 4 )


if __name__ == "__main__":
    if len( sys.argv ) == 1:
        print 'usage: tree.py u|serial_port_path|localhost:4304'
        sys.exit( 1 )
    else:
        if not _OW.init( sys.argv[ 1 ] ):
	    print 'problem initializing the 1-wire controller'
	    sys.exit( 1 )


tree( '/' )
