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

Test suite for basic reality checks on a 1-wire network.

PIO.0/       PIO.1/     PIO.2/    PIO.3/    PIO.4/     PIO.5/
PIO.6/       PIO.7/     PIO.ALL/  PIO.BYTE/ address/   crc8/
family/      id/        latch.0/  latch.1/  latch.2/   latch.3/
latch.4/     latch.5/   latch.6/  latch.7/  latch.ALL/ latch.BYTE/
power/       present/   reset/    sensed.0/ sensed.1/  sensed.2/
sensed.3/    sensed.4/  sensed.5/ sensed.6/ sensed.7/  sensed.ALL/
sensed.BYTE/ set_alarm/ strobe/   type/
"""


import unittest
import sys
import os
import ConfigParser
import ow
import util


__version__ = '0.0'


if not os.path.exists( 'owtest.ini' ):
    raise IOError, 'owtest.ini'

config = ConfigParser.ConfigParser( )
config.read( 'owtest.ini' )

sensors = util.find_config( config, 'DS2408' )
if len( sensors ):
    load = True
else:
    load = False


class DS2408( unittest.TestCase ):
    def setUp( self ):
        ow.init( config.get( 'General', 'interface' ) )


    def testAttributes( self ):
        self.failIfEqual( len( sensors ), 0 )
        for name in sensors:
            sensor = ow.Sensor( name )
            family, id = name.split( '/' )[ -1 ].split( '.' )

            self.failUnlessEqual( family + id,                      sensor.address[ :-2 ] )
            #self.failUnlessEqual( config.get( name, 'crc8' ),       sensor.crc8 )
            self.failUnlessEqual( family,                           sensor.family )
            self.failUnlessEqual( id,                               sensor.id )
            self.failUnlessEqual( '1',                              sensor.present )
            self.failUnlessEqual( config.get( name.split( '/' )[ -1 ], 'type' ), sensor.type )

            self.failUnlessEqual( hasattr( sensor, 'PIO_0' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_1' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_2' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_3' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_4' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_5' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_6' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_7' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_ALL' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'PIO_BYTE' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'latch_0' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_1' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_2' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_3' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_4' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_5' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_6' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_7' ),     True )
            self.failUnlessEqual( hasattr( sensor, 'latch_ALL' ),   True )
            self.failUnlessEqual( hasattr( sensor, 'latch_BYTE' ),  True )
            self.failUnlessEqual( hasattr( sensor, 'power' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'reset' ),       True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_0' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_1' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_2' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_3' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_4' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_5' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_6' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_7' ),    True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_ALL' ),  True )
            self.failUnlessEqual( hasattr( sensor, 'sensed_BYTE' ), True )
            self.failUnlessEqual( hasattr( sensor, 'set_alarm' ),   True )
            self.failUnlessEqual( hasattr( sensor, 'strobe' ),      True )


    def testSet( self ):
        self.failIfEqual( len( sensors ), 0 )
        for name in sensors:
            sensor = ow.Sensor( name )

            sensor.PIO_0 = '0'
            #print sensor._path + '/PIO.0'
            self.failUnlessEqual( ow._OW.get( sensor._path + '/PIO.0' ), '0' )
            self.failUnlessEqual( sensor.PIO_0, '0' )
            self.failUnlessEqual( 'PIO_0' in dir( sensor ), False )

            sensor.PIO_0 = '1'
            self.failUnlessEqual( sensor.PIO_0, '1' )
            self.failUnlessEqual( 'PIO_0' in dir( sensor ), False )


            sensor.PIO_1 = '0'
            self.failUnlessEqual( sensor.PIO_1, '0' )
            self.failUnlessEqual( 'PIO_1' in dir( sensor ), False )

            sensor.PIO_1 = '1'
            self.failUnlessEqual( sensor.PIO_1, '1' )
            self.failUnlessEqual( 'PIO_1' in dir( sensor ), False )


            sensor.PIO_2 = '0'
            self.failUnlessEqual( sensor.PIO_2, '0' )
            self.failUnlessEqual( 'PIO_2' in dir( sensor ), False )

            sensor.PIO_2 = '1'
            self.failUnlessEqual( sensor.PIO_2, '1' )
            self.failUnlessEqual( 'PIO_2' in dir( sensor ), False )


            sensor.PIO_3 = '0'
            self.failUnlessEqual( sensor.PIO_3, '0' )
            self.failUnlessEqual( 'PIO_3' in dir( sensor ), False )

            sensor.PIO_3 = '1'
            self.failUnlessEqual( sensor.PIO_3, '1' )
            self.failUnlessEqual( 'PIO_3' in dir( sensor ), False )


            sensor.PIO_4 = '0'
            self.failUnlessEqual( sensor.PIO_4, '0' )
            self.failUnlessEqual( 'PIO_4' in dir( sensor ), False )

            sensor.PIO_4 = '1'
            self.failUnlessEqual( sensor.PIO_4, '1' )
            self.failUnlessEqual( 'PIO_4' in dir( sensor ), False )


            sensor.PIO_5 = '0'
            self.failUnlessEqual( sensor.PIO_5, '0' )
            self.failUnlessEqual( 'PIO_5' in dir( sensor ), False )

            sensor.PIO_5 = '1'
            self.failUnlessEqual( sensor.PIO_5, '1' )
            self.failUnlessEqual( 'PIO_5' in dir( sensor ), False )


            sensor.PIO_6 = '0'
            self.failUnlessEqual( sensor.PIO_6, '0' )
            self.failUnlessEqual( 'PIO_6' in dir( sensor ), False )

            sensor.PIO_6 = '1'
            self.failUnlessEqual( sensor.PIO_6, '1' )
            self.failUnlessEqual( 'PIO_6' in dir( sensor ), False )


            sensor.PIO_7 = '0'
            self.failUnlessEqual( sensor.PIO_7, '0' )
            self.failUnlessEqual( 'PIO_7' in dir( sensor ), False )

            sensor.PIO_7 = '1'
            self.failUnlessEqual( sensor.PIO_7, '1' )
            self.failUnlessEqual( 'PIO_7' in dir( sensor ), False )



def Suite( ):
    return unittest.makeSuite( DS2408, 'test' )


if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
