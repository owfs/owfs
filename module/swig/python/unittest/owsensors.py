#! /usr/bin/env python

"""
::BOH
$Id$
$HeadURL: http://subversion/stuff/svn/owfs/trunk/unittest/owsensors.py $

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
import ConfigParser
import ow


__version__ = '0.0-%s' % '$Id$'.split( )[ 2 ]
load    = True


class OWSensors( unittest.TestCase ):
    def setUp( self ):
        #print 'OWSensors.setup'
        if not os.path.exists( 'owtest.ini' ):
            raise IOError, 'owtest.ini'

        self.config = ConfigParser.ConfigParser( )
        self.config.read( 'owtest.ini' )

        ow.init( self.config.get( 'General', 'interface' ) )

        self.entries = self.config.get( 'Root', 'entries' ).split( ' ' )
        self.entries.sort( )

        self.sensors = [ '/' + name for name in self.config.get( 'Root', 'sensors' ).split( ' ' ) ]
        self.sensors.sort( )


    def testRootNameType( self ):
        #print 'OWSensors.testRootNameType'
        path, name = str( ow.Sensor( '/' ) ).split( ' - ' )
        self.failUnlessEqual( path, '/' )
        self.failUnlessEqual( name, self.config.get( 'Root', 'type' ) )


    def testRootEntries( self ):
        #print 'OWSensors.testRootEntries'
        entries = list( ow.Sensor( '/' ).entries( ) )
        entries.sort( )

        self.failUnlessEqual( entries, self.entries )


    def testSensors( self ):
        #print 'OWSensors.testSensors'
        sensors = [ str( sensor ).split( ' - ' )[ 0 ] for sensor in ow.Sensor( '/' ).sensors( ) ]
        sensors.sort( )

        self.failUnlessEqual( sensors, self.sensors )


    def testBaseAttributes( self ):
        #print 'OWSensors.testBaseAttributes'
        for sensor in ow.Sensor( '/' ).sensors( ):
            name = str( sensor ).split( ' - ' )[ 0 ][ 1: ]
            family, id = name.split( '.' )

            self.failUnlessEqual( family + id,                     sensor.address[ :-2 ] )
            #self.failUnlessEqual( self.config.get( name, 'crc8' ), sensor.crc8 )
            self.failUnlessEqual( family,                          sensor.family )
            self.failUnlessEqual( id,                              sensor.id )
            self.failUnlessEqual( self.config.get( name, 'type' ), sensor.type )


    def testFindAnyValue( self ):
        #print 'OWSensors.testFindAnyValue'
        type_list = { }

        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            id_type = self.config.get( id, 'type' )
            try:
                type_list[ id_type ] += 1
            except KeyError:
                type_list[ id_type ] = 1

        for id_type in type_list:
            sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( type = id_type ) ]
            self.failUnlessEqual( len( sensor_list ), type_list[ id_type ] )


    def testFindAnyNoValue( self ):
        #print 'OWSensors.testFindAnyNoValue'
        type_count = len( self.config.get( 'Root', 'sensors' ).split( ' ' ) )
        sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( type = None ) ]
        self.failUnlessEqual( len( sensor_list ), type_count )


    def testFindAnyNone( self ):
        #print 'OWSensors.testFindNone'
        sensor_list = [ sensor for sensor in ow.Sensor( '/' ).find( xyzzy = None ) ]
        self.failUnlessEqual( len( sensor_list ), 0 )


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
            self.failUnlessEqual( len( sensor_list ), list[ pair ] )


    def testFindAllNone( self ):
        #print 'OWSensors.testFindAllNone'
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
            self.failUnlessEqual( len( sensor_list ), 0 )


    def testCacheUncached( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            c = ow.Sensor( '/' + id )
            u = ow.Sensor( '/uncached/' + id )
            self.failUnlessEqual( c._path, u._path )

            ce = [ entry for entry in c.entries( ) ]
            ue = [ entry for entry in u.entries( ) ]
            self.failUnlessEqual( ce, ue )

            cs = [ sensor for sensor in c.sensors( ) ]
            us = [ sensor for sensor in u.sensors( ) ]
            self.failUnlessEqual( cs, us )


    def testCacheSwitch( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            s = ow.Sensor( '/' + id )
            ce = s.entryList( )
            cs = s.sensorList( )

            s.useCache( False )

            ue = s.entryList( )
            us = s.sensorList( )

            self.failUnlessEqual( ce, ue )
            self.failUnlessEqual( cs, us )


    def testRootCache( self ):
        r = ow.Sensor( '/' )
        ce = r.entryList( )
        cs = r.sensorList( )

        r.useCache( False )
        ue = r.entryList( )
        us = r.sensorList( )

        self.failUnlessEqual( cs, us )
        self.failUnlessEqual( ue, [ 'alarm', 'simultaneous' ] )


    def testEntryList( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            c = ow.Sensor( '/' + id )
            u = ow.Sensor( '/uncached/' + id )

            ce = [ entry for entry in c.entries( ) ]
            self.failUnlessEqual( ce, c.entryList( ) )

            ue = [ entry for entry in u.entries( ) ]
            self.failUnlessEqual( ue, u.entryList( ) )

            self.failUnlessEqual( ce, ue )


    def testSensorList( self ):
        for id in self.config.get( 'Root', 'sensors' ).split( ' ' ):
            c = ow.Sensor( '/' + id )
            u = ow.Sensor( '/uncached/' + id )

            cs = [ sensor for sensor in c.sensors( ) ]
            self.failUnlessEqual( cs, c.sensorList( ) )

            us = [ sensor for sensor in u.sensors( ) ]
            self.failUnlessEqual( us, u.sensorList( ) )

            self.failUnlessEqual( cs, us )


    def testEqual( self ):
        s1 = ow.Sensor( '/' )
        s2 = ow.Sensor( '/' )
        s3 = ow.Sensor( self.config.get( 'Root', 'sensors' ).split( ' ' )[ 0 ] )
        self.failUnlessEqual( s1, s2 )
        self.failIfEqual( s1, s3 )
        self.failIfEqual( s2, s3 )


def Suite( ):
    return unittest.makeSuite( OWSensors, 'test' )


if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
