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

Create an XML-RPC server for a 1-wire network.

Run xmlrpc_client.py to see the server in action.

Or point a browser at http://localhost:8765 to see some documentation.
"""


import sys
import ow
from DocXMLRPCServer import DocXMLRPCServer, DocXMLRPCRequestHandler
from SocketServer import ThreadingMixIn


class owr:
    """
    A wrapper class is needed around the ow.Sensor class since the
    XML-RPC protocol doesn't know anything about generators, Python
    objects and such. XML-RPC is a pretty simple protocol that deals
    pretty well with basic types. So that's what it'll get.
    """
    def entries( self, path ):
        """List a sensor's attributes."""
        return [entry for entry in ow.Sensor( path ).entries( )]


    def sensors( self, path ):
        """List all the sensors that exist in a particular path."""
        return [sensor._path for sensor in ow.Sensor( path ).sensors( )]


    def attr( self, path, attr ):
        """Lookup a specific sensor attribute."""
        sensor = ow.Sensor( path )
        exec 'val = sensor.' + attr
        return val


class ThreadingServer( ThreadingMixIn, DocXMLRPCServer ):
    pass


# Initialize ow for a USB controller or for a serial port.
ow.init( 'u' )
#ow.init( '/dev/ttyS0' )

# Allow connections for the localhost on port 8765.
serveraddr = ( '', 8765 )
srvr = ThreadingServer( serveraddr, DocXMLRPCRequestHandler )
srvr.set_server_title( '1-wire network' )
srvr.set_server_documentation( 'Welcome to the world of 1-wire networks.' )
srvr.register_instance( owr( ) )
srvr.register_introspection_functions( )
srvr.serve_forever( )
