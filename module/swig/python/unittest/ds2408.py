#! /usr/bin/env python

"""
::BOH
$Id$
$HeadURL: http://subversion/stuff/svn/owfs/trunk/unittest/ds2408.py $

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

Test suite for basic reality checks on a 1-wire nerwork.

PIO.0/  PIO.3/  PIO.6/    PIO.BYTE/  family/   latch.1/  latch.4/  latch.7/     power/    sensed.0/  sensed.3/  sensed.6/    sensed.BYTE/  type/
PIO.1/  PIO.4/  PIO.7/    address/   id/       latch.2/  latch.5/  latch.ALL/   present/  sensed.1/  sensed.4/  sensed.7/    set_alarm/
PIO.2/  PIO.5/  PIO.ALL/  crc8/      latch.0/  latch.3/  latch.6/  latch.BYTE/  reset/    sensed.2/  sensed.5/  sensed.ALL/  strobe/
"""


import unittest
import sys
import os
import ConfigParser
import ow


__version__ = '0.0-%s' % '$Id$'.split( )[ 2 ]

if not os.path.exists( 'owtest.ini' ):
    raise IOError, 'owtest.ini'

config = ConfigParser.ConfigParser( )
config.read( 'owtest.ini' )

sensors = [ name for name in config.get( 'Root', 'sensors' ).split( ' ' ) ]
sensors = [ '/' + name for name in sensors if config.get( name, 'type' ) == 'DS2408' ]
if len( sensors ):
    load = True
else:
    load = False


class DS2408( unittest.TestCase ):
    def setUp( self ):
        ow.init( config.get( 'General', 'interface' ) )


    def testAttributes( self ):
        for name in sensors:
            sensor = ow.Sensor( name )
            family, id = name[ 1: ].split( '.' )

            self.failUnlessEqual( family + id,                      sensor.address[ :-2 ] )
            #self.failUnlessEqual( config.get( name, 'crc8' ),       sensor.crc8 )
            self.failUnlessEqual( family,                           sensor.family )
            self.failUnlessEqual( id,                               sensor.id )
            self.failUnlessEqual( '1',                              sensor.present )
            self.failUnlessEqual( config.get( name[ 1: ], 'type' ), sensor.type )

            self.failUnlessEqual( hasattr( sensor, 'PIO.0' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.1' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.2' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.3' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.4' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.5' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.6' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.7' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.ALL' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'PIO.BYTE' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'latch.0' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.1' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.2' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.3' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.4' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.5' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.6' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.7' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch.ALL' ),   True )
            self.failUnlessEqual( hasattr( sensor, 'latch.BYTE' ),  True )
            self.failUnlessEqual( hasattr( sensor, 'power' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'reset' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.0' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.1' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.2' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.3' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.4' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.5' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.6' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.7' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.ALL' ),  True )
            self.failUnlessEqual( hasattr( sensor, 'sensed.BYTE' ), True )
            self.failUnlessEqual( hasattr( sensor, 'set_alarm' ),   True )
            self.failUnlessEqual( hasattr( sensor, 'strobe' ),      True )
                


def Suite( ):
    return unittest.makeSuite( DS2408, 'test' )


if __name__ == "__main__":
    #unittest.main()
    unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
