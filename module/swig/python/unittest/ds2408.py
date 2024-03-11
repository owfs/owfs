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
import configparser
import ow
import util


__version__ = '0.0'

curr_dir = os.path.dirname(os.path.realpath(__file__))

if not os.path.exists(os.path.join(curr_dir, 'owtest.ini')):
    raise IOError(os.path.join(curr_dir, 'owtest.ini'))

config = configparser.ConfigParser( )
config.read(os.path.join(curr_dir, 'owtest.ini'))

sensors = util.find_config( config, 'DS2408' )
if len( sensors ):
    load = True
else:
    load = False


class DS2408( unittest.TestCase ):
    def setUp( self ):
        ow.init( config.get( 'General', 'interface' ) )


    def testAttributes( self ):
        self.assertNotEqual( len( sensors ), 0 )
        for name in sensors:
            sensor = ow.Sensor( name )
            family, id = name.split( '/' )[ -1 ].split( '.' )

            self.assertEqual( family + id,                      sensor.address[ :-2 ] )
            #self.failUnlessEqual( config.get( name, 'crc8' ),       sensor.crc8 )
            self.assertEqual( family,                           sensor.family )
            self.assertEqual( id,                               sensor.id )
            self.assertEqual( '1',                              sensor.present )
            self.assertEqual( config.get( name.split( '/' )[ -1 ], 'type' ), sensor.type )

            self.assertEqual( hasattr( sensor, 'PIO_0' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_1' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_2' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_3' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_4' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_5' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_6' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_7' ),       True )
            self.assertEqual( hasattr( sensor, 'PIO_ALL' ),     True )
            self.assertEqual( hasattr( sensor, 'PIO_BYTE' ),    True )
            self.assertEqual( hasattr( sensor, 'latch_0' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_1' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_2' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_3' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_4' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_5' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_6' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_7' ),     True )
            self.assertEqual( hasattr( sensor, 'latch_ALL' ),   True )
            self.assertEqual( hasattr( sensor, 'latch_BYTE' ),  True )
            self.assertEqual( hasattr( sensor, 'power' ),       True )
            self.assertEqual( hasattr( sensor, 'reset' ),       True )
            self.assertEqual( hasattr( sensor, 'sensed_0' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_1' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_2' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_3' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_4' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_5' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_6' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_7' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_ALL' ),  True )
            self.assertEqual( hasattr( sensor, 'sensed_BYTE' ), True )
            self.assertEqual( hasattr( sensor, 'set_alarm' ),   True )
            self.assertEqual( hasattr( sensor, 'strobe' ),      True )


    def testSet( self ):
        self.assertNotEqual( len( sensors ), 0 )
        for name in sensors:
            sensor = ow.Sensor( name )

            sensor.PIO_0 = '0'
            #print sensor._path + '/PIO.0'
            self.assertEqual( ow._OW.get( sensor._path + '/PIO.0' ), '0' )
            self.assertEqual( sensor.PIO_0, '0' )
            self.assertEqual( 'PIO_0' in dir( sensor ), False )

            sensor.PIO_0 = '1'
            self.assertEqual( sensor.PIO_0, '1' )
            self.assertEqual( 'PIO_0' in dir( sensor ), False )


            sensor.PIO_1 = '0'
            self.assertEqual( sensor.PIO_1, '0' )
            self.assertEqual( 'PIO_1' in dir( sensor ), False )

            sensor.PIO_1 = '1'
            self.assertEqual( sensor.PIO_1, '1' )
            self.assertEqual( 'PIO_1' in dir( sensor ), False )


            sensor.PIO_2 = '0'
            self.assertEqual( sensor.PIO_2, '0' )
            self.assertEqual( 'PIO_2' in dir( sensor ), False )

            sensor.PIO_2 = '1'
            self.assertEqual( sensor.PIO_2, '1' )
            self.assertEqual( 'PIO_2' in dir( sensor ), False )


            sensor.PIO_3 = '0'
            self.assertEqual( sensor.PIO_3, '0' )
            self.assertEqual( 'PIO_3' in dir( sensor ), False )

            sensor.PIO_3 = '1'
            self.assertEqual( sensor.PIO_3, '1' )
            self.assertEqual( 'PIO_3' in dir( sensor ), False )


            sensor.PIO_4 = '0'
            self.assertEqual( sensor.PIO_4, '0' )
            self.assertEqual( 'PIO_4' in dir( sensor ), False )

            sensor.PIO_4 = '1'
            self.assertEqual( sensor.PIO_4, '1' )
            self.assertEqual( 'PIO_4' in dir( sensor ), False )


            sensor.PIO_5 = '0'
            self.assertEqual( sensor.PIO_5, '0' )
            self.assertEqual( 'PIO_5' in dir( sensor ), False )

            sensor.PIO_5 = '1'
            self.assertEqual( sensor.PIO_5, '1' )
            self.assertEqual( 'PIO_5' in dir( sensor ), False )


            sensor.PIO_6 = '0'
            self.assertEqual( sensor.PIO_6, '0' )
            self.assertEqual( 'PIO_6' in dir( sensor ), False )

            sensor.PIO_6 = '1'
            self.assertEqual( sensor.PIO_6, '1' )
            self.assertEqual( 'PIO_6' in dir( sensor ), False )


            sensor.PIO_7 = '0'
            self.assertEqual( sensor.PIO_7, '0' )
            self.assertEqual( 'PIO_7' in dir( sensor ), False )

            sensor.PIO_7 = '1'
            self.assertEqual( sensor.PIO_7, '1' )
            self.assertEqual( 'PIO_7' in dir( sensor ), False )



def Suite( ):
    suite = unittest.TestSuite()
    suite.addTest(DS2408('testAttributes'))
    suite.addTest(DS2408('testSet'))
    return suite

if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
