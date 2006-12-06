"""
::BOH
$Id$
$HeadURL: http://subversion/stuff/svn/owfs/trunk/setup.py $

Copyright (c) 2006 Peter Kropf. All rights reserved.

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

distutils setup file for use in creating the ownet Python module to
interact with an owserver via socket communcations.
"""


from distutils.core import setup, Extension


classifiers = """
Development Status :: 4 - Alpha
Environment :: Console
Intended Audience :: Developers
Intended Audience :: System Administrators
Operating System :: POSIX :: Linux
Programming Language :: Python
Topic :: System :: Hardware
Topic :: Utilities
"""

setup(
    name             = 'ownet',
    description      = '1-wire owserver interface.',
    version          = '0.2',
    author           = 'Peter Kropf',
    author_email     = 'pkropf@gmail.com',
    url              = 'http://owfs.sourceforge.net/',
    license          = 'GPL',
    platforms        = 'Linux',
    long_description = 'Interface with 1-wire owserver and sensors from Python.',
    classifiers      = filter( None, classifiers.split( '\n' ) ),
    packages         = [ 'ownet' ],
    )
