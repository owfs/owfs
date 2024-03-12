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
"""


import unittest
import sys
import os
import configparser
import ow


__version__ = '0.0'
load    = True


class OWSensors( unittest.TestCase ):
    def setUp( self ):
        curr_dir = os.path.dirname(os.path.realpath(__file__))

        if not os.path.exists(os.path.join(curr_dir, 'owtest.ini')):
            raise IOError(os.path.join(curr_dir, 'owtest.ini'))

        self.config = configparser.ConfigParser( )
        self.config.read(os.path.join(curr_dir, 'owtest.ini'))

        ow.init( self.config.get( 'General', 'interface' ) )

        self.entries = self.config.get( 'Root', 'entries' ).split( ' ' )
        self.entries.sort( )

        self.sensors = [ '/' + name for name in self.config.get( 'Root', 'sensors' ).split( ' ' ) ]
        self.sensors.sort( )


    def testRootNameType( self ):
        path, name = str( ow.Sensor( '/' ) ).split( ' - ' )
        self.assertEqual( path, '/' )
        self.assertEqual( name, self.config.get( 'Root', 'type' ) )


    def testRootEntries( self ):
        entries = list( ow.Sensor( '/' ).entries( ) )
        entries.sort( )

        self.assertEqual( entries, self.entries )


    def testSensors( self ):
        sensors = [ str( sensor ).split( ' - ' )[ 0 ] for sensor in ow.Sensor( '/' ).sensors( ) ]
        sensors.sort( )

        self.assertEqual( sensors, self.sensors )


    def testBaseAttributes( self ):
        for sensor in ow.Sensor( '/' ).sensors( ):
            name = str( sensor ).split( ' - ' )[ 0 ][ 1: ]
            family, id = name.split( '.' )

            self.assertEqual( family + id,                     sensor.address[ :-2 ] )
            #self.assertEqual( self.config.get( name, 'crc8' ), sensor.crc8 )
            self.assertEqual( family,                          sensor.family )
            self.assertEqual( id,                              sensor.id )
            self.assertEqual( self.config.get( name, 'type' ), sensor.type )


    def testFindAnyValue( self ):
        type_list = { }

        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            id_type = self.config.get( id, 'type' )
            try:
                type_list[ id_type ] += 1
            except KeyError:
                type_list[ id_type ] = 1

        for id_type in type_list:
            sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( type = id_type ) ]
            self.assertEqual( len( sensor_list ), type_list[ id_type ] )


    def testFindAnyNoValue( self ):
        #print 'OWSensors.testFindAnyNoValue'
        type_count = len( self.config.get( 'Root', 'sensors' ).split( ' ' ) )
        sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( type = None ) ]
        self.assertEqual( len( sensor_list ), type_count )


    def testFindAnyNone( self ):
        #print 'OWSensors.testFindNone'
        sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( xyzzy = None ) ]
        self.assertEqual( len( sensor_list ), 0 )


    def testFindAll( self ):
        #print 'OWSensors.testFindAll'
        list = { }

        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            family = id.split( '.' )[ 0 ]
            id_type = self.config.get( id, 'type' )
            try:
                list[ ( family, id_type ) ] += 1
            except KeyError:
                list[ ( family, id_type ) ] = 1

        for pair in list:
            sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( all = True,
                                                                        family = pair[ 0 ],
                                                                        type = pair[ 1 ] ) ]
            self.assertEqual( len( sensor_list ), list[ pair ] )


    def testFindAllNone( self ):
        list = { }

        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            family = id.split( '.' )[ 0 ]
            id_type = self.config.get( id, 'type' )
            try:
                list[ ( family, id_type ) ] += 1
            except KeyError:
                list[ ( family, id_type ) ] = 1

        for pair in list:
            sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( all = True,
                                                                        xyzzy = True,
                                                                        family = pair[ 0 ],
                                                                        type = pair[ 1 ] ) ]
            self.assertEqual( len( sensor_list ), 0 )


    def testCacheUncached( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            c = ow.Sensor( '/' + id )
            u = ow.Sensor( '/uncached/' + id )
            self.assertEqual( c._path, u._path )

            ce = [ entry for entry in c.entries( ) ]
            ue = [ entry for entry in u.entries( ) ]
            self.assertEqual( ce, ue )

            cs = [ sensor for sensor in c.sensors( ) ]
            us = [ sensor for sensor in u.sensors( ) ]
            self.assertEqual( cs, us )


    def testCacheSwitch( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            s = ow.Sensor( '/' + id )
            ce = s.entryList( )
            cs = s.sensorList( )

            s.useCache( False )

            ue = s.entryList( )
            us = s.sensorList( )

            self.assertEqual( ce, ue )
            self.assertEqual( cs, us )


    def testRootCache( self ):
        r = ow.Sensor( '/' )
        ce = r.entryList( )
        cs = r.sensorList( )

        r.useCache( False )
        ue = r.entryList( )
        us = r.sensorList( )

        self.assertEqual( cs, us )
        self.assertEqual( ue, [ 'alarm', 'simultaneous' ] )


    def testEntryList( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            c = ow.Sensor( '/' + id )
            u = ow.Sensor( '/uncached/' + id )

            ce = [ entry for entry in c.entries( ) ]
            self.assertEqual( ce, c.entryList( ) )

            ue = [ entry for entry in u.entries( ) ]
            self.assertEqual( ue, u.entryList( ) )

            self.assertEqual( ce, ue )


    def testSensorList( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            c = ow.Sensor( '/' + id )
            u = ow.Sensor( '/uncached/' + id )

            cs = [ sensor for sensor in c.sensors( ) ]
            self.assertEqual( cs, c.sensorList( ) )

            us = [ sensor for sensor in u.sensors( ) ]
            self.assertEqual( us, u.sensorList( ) )

            self.assertEqual( cs, us )


    def testEqual( self ):
        s1 = ow.Sensor( '/' )
        s2 = ow.Sensor( '/' )
        s3 = ow.Sensor( self.config.get( 'Root', 'sensors' ).split( ' ' )[ 0 ] )
        self.assertEqual( s1, s2 )
        self.assertNotEqual( s1, s3 )
        self.assertNotEqual( s2, s3 )


def Suite( ):
    suite = unittest.TestSuite()
    suite.addTest(OWSensors('testRootNameType'))
    suite.addTest(OWSensors('testRootEntries'))
    suite.addTest(OWSensors('testSensors'))
    suite.addTest(OWSensors('testBaseAttributes'))
    suite.addTest(OWSensors('testFindAnyValue'))
    suite.addTest(OWSensors('testFindAnyNoValue'))
    suite.addTest(OWSensors('testFindAnyNone'))
    suite.addTest(OWSensors('testFindAll'))
    suite.addTest(OWSensors('testFindAllNone'))
    suite.addTest(OWSensors('testCacheUncached'))
    suite.addTest(OWSensors('testCacheSwitch'))
    suite.addTest(OWSensors('testRootCache'))
    suite.addTest(OWSensors('testEntryList'))
    suite.addTest(OWSensors('testSensorList'))
    suite.addTest(OWSensors('testEqual'))
    return suite


if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
