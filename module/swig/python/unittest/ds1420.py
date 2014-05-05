#! /usr/bin/env python

"""
::BOH
$Id$
$HeadURL: http://subversion/stuff/svn/owfs/trunk/unittest/ds1420.py $

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

Test suite for basic reality checks on a 1-wire nerwork.

address/  crc8/  family/  id/  present/  type/
"""


import unittest
import sys
import os
import ConfigParser
import ow


__version__ = '0.0'

if not os.path.exists( 'owtest.ini' ):
    raise IOError, 'owtest.ini'

config = ConfigParser.ConfigParser( )
config.read( 'owtest.ini' )

sensors = [ name for name in config.get( 'Root', 'sensors' ).split( ' ' ) ]
sensors = [ '/' + name for name in sensors if config.get( name, 'type' ) == 'DS1420' ]
if len( sensors ):
    load = True
else:
    load = False


class DS1420( unittest.TestCase ):
    def setUp( self ):
        ow.init( config.get( 'General', 'interface' ) )


    def testAttributes( self ):
        self.failIfEqual( len( sensors ), 0 )
        for name in sensors:
            sensor = ow.Sensor( name )
            family, id = name[ 1: ].split( '.' )

            self.failUnlessEqual( family + id,                      sensor.address[ :-2 ] )
            #self.failUnlessEqual( config.get( name, 'crc8' ),       sensor.crc8 )
            self.failUnlessEqual( family,                           sensor.family )
            self.failUnlessEqual( id,                               sensor.id )
            self.failUnlessEqual( config.get( name[ 1: ], 'type' ), sensor.type )


def Suite( ):
    return unittest.makeSuite( DS1420, 'test' )


if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
