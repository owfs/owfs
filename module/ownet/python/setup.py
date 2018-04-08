"""
::BOH
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
::EOH
ownet: access 1-wire sensors

ownet is a standalone python module for accessing 1-wire sensors
through an owserver. The ownet module does not use the core ow
libraries. Instead, it impliments the wire protocol for owserver
communication. This means that the ow core libraries do no have to be
installed on all systems, just on the system running owserver. The
ownet module can be installed on any system with Python.
"""


from distutils.core import setup, Extension


classifiers = """
Development Status :: 4 - Beta
Environment :: Console
Intended Audience :: Developers
Intended Audience :: System Administrators
License :: OSI Approved :: GNU General Public License (GPL)
Operating System :: MacOS
Operating System :: Microsoft
Operating System :: Microsoft :: Windows :: Windows NT/2000
Operating System :: Other OS
Operating System :: POSIX
Operating System :: Unix
Programming Language :: Python
Topic :: System :: Hardware
Topic :: System :: Networking :: Monitoring
Topic :: Utilities
"""

doclines = __doc__.split('::EOH')[1].split('\n')[1:]
version  = '0.3'

setup(
    name             = 'ownet',
    description      = doclines[0],
    version          = version,
    author           = 'Peter Kropf',
    author_email     = 'pkropf@gmail.com',
    maintainer       = 'Peter Kropf',
    maintainer_email = 'pkropf@gmail.com',
    url              = 'http://www.owfs.org/',
    license          = 'GPL',
    platforms        = ['any'],
    long_description = '\n'.join(doclines),
    classifiers      = filter( None, classifiers.split( '\n' ) ),
    packages         = ['ownet'],
    download_url     = 'http://cheeseshop.python.org/packages/source/o/ownet/ownet-%s.tar.gz' % version
    )
