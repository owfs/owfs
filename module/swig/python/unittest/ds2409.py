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

Test suite for basic reality checks on a 1-wire nerwork.

address/  branch.0/  branch.ALL/   control/  discharge/  event.1/    event.BYTE/  id/    present/   sensed.1/    sensed.BYTE/
aux/      branch.1/  branch.BYTE/  crc8/     event.0/    event.ALL/  family/      main/  sensed.0/  sensed.ALL/  type/
"""


import unittest
import sys
import os
import configparser
import ow


__version__ = '0.0'

curr_dir = os.path.dirname(os.path.realpath(__file__))

if not os.path.exists(os.path.join(curr_dir, 'owtest.ini')):
    raise IOError(os.path.join(curr_dir, 'owtest.ini'))

config = configparser.ConfigParser( )
config.read(os.path.join(curr_dir, 'owtest.ini'))

sensors = [ name for name in config.get( 'Root', 'sensors' ).split( ' ' ) ]
sensors = [ '/' + name for name in sensors if config.get( name, 'type' ) == 'DS2409' ]
if len( sensors ):
    load = True
else:
    load = False

# There seems to be an odd interaction when ds2409 is run from
# owtest.py. Once the aux or main attributes are checked, calls to the
# controller no longer return the sensors that are there. This is seen
# when owsensors.py tests are run. At this point, I'm not sure why
# this is so ds2409 is forced not to run from owtest.py. Running it by
# itself works just fine. But more testing is needed to understand
# what's going on.

load = False


class DS2409( unittest.TestCase ):
    def setUp( self ):
        ow.init( config.get( 'General', 'interface' ) )


    def testAttributes( self ):
        for name in sensors:
            sensor = ow.Sensor( name )
            family, id = name[ 1: ].split( '.' )

            self.assertEqual( family + id,                      sensor.address[ :-2 ] )
            #self.assertEqual( config.get( name, 'crc8' ),       sensor.crc8 )
            self.assertEqual( family,                           sensor.family )
            self.assertEqual( id,                               sensor.id )
            self.assertEqual( '1',                              sensor.present )
            self.assertEqual( config.get( name[ 1: ], 'type' ), sensor.type )

            self.assertEqual( hasattr( sensor, 'branch_0' ),    True )
            self.assertEqual( hasattr( sensor, 'branch_1' ),    True )
            self.assertEqual( hasattr( sensor, 'branch_ALL' ),  True )
            self.assertEqual( hasattr( sensor, 'branch_BYTE' ), True )
            self.assertEqual( hasattr( sensor, 'control' ),     True )
            #self.assertEqual( hasattr( sensor, 'discharge' ),   True )
            self.assertEqual( hasattr( sensor, 'event_0' ),     True )
            self.assertEqual( hasattr( sensor, 'event_1' ),     True )
            self.assertEqual( hasattr( sensor, 'event_ALL' ),   True )
            self.assertEqual( hasattr( sensor, 'event_BYTE' ),  True )
            self.assertEqual( hasattr( sensor, 'sensed_0' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_1' ),    True )
            self.assertEqual( hasattr( sensor, 'sensed_ALL' ),  True )
            self.assertEqual( hasattr( sensor, 'sensed_BYTE' ), True )
            

def Suite( ):
    suite = unittest.TestSuite()
    suite.addTest(DS2409('testAttributes'))
    return suite


if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
