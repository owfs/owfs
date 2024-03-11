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

Simple test to ensure that the ow module can be loaded and an instance
of ow.Sensor created.
"""


import unittest
import sys
import os
import configparser


__version__ = '0.0'
load    = True


class OWLoad( unittest.TestCase ):
    def setUp( self ):
        curr_dir = os.path.dirname(os.path.realpath(__file__))

        if not os.path.exists(os.path.join(curr_dir, 'owtest.ini')):
            raise IOError(os.path.join(curr_dir, 'owtest.ini'))

        self.config = configparser.ConfigParser( )
        self.config.read(os.path.join(curr_dir, 'owtest.ini'))

    def testImport( self ):
        import ow
        ow.init( self.config.get( 'General', 'interface' ) )
        s = ow.Sensor( '/' )


def Suite( ):
    suite = unittest.TestSuite()
    suite.addTest(OWLoad('testImport'))
    return suite


if __name__ == "__main__":
    if len( sys.argv ) > 1:
        unittest.main( )
    else:
        unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
