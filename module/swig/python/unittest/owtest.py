#! /usr/bin/env python

"""
::BOH
$HeadURL: http://subversion/stuff/svn/owfs/trunk/unittest/owtest.py $

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

Run all the tests from the modules in the current directory.
"""


import sys
import os
import glob
import unittest


__version__ = '0.0'


def Suite( ):
    """
    Function used by unittest.TextTestRunner to create the suite of
    test cases that are found in the current directory. All modules
    except the current one are considered test cases and will be added
    to the list of test suites to be run.
    """
    this_module_name = os.path.basename( __file__ )
    test_suites = unittest.TestSuite( )
    for name in glob.glob( '*.py' ):
        if name == this_module_name:
            continue
        name = name[:-3]
        try:
            exec 'import ' + name
            exec 'load = ' + name + '.load'
            if load:
                exec 'suite = ' + name + '.Suite( )'
                test_suites = unittest.TestSuite( ( test_suites, suite ) )
        except AttributeError:
            print 'skipping ' + name + ' no Suite function found.'
            pass
    return test_suites


suite = Suite( )


if __name__ == "__main__":
    unittest.TextTestRunner( verbosity=2 ).run( Suite( ) )
