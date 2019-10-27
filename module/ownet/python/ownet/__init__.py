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

1-wire sensor network interface. ownet provides standalone access to a
owserver without the need to compile the core ow libraries on the
local system. As a result, ownet can run on almost any platform that
support Python.

OWFS is an open source project developed by Paul Alfille and hosted at
http://www.owfs.org
"""

from __future__ import generators, absolute_import

import sys
import os
from .connection import Connection

__author__ = 'Peter Kropf'
__email__ = 'pkropf@gmail.com'
__version__ = '0.3'


#
# exceptions used and thrown by the ownet classes
#

class exError(Exception):
    """base exception for all one wire raised exceptions."""


class exErrorValue(exError):
    """Base exception for all one wire raised exceptions with a value."""
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class exNoController(exError):
    """Exception raised when a controller cannot be initialized."""


class exNotInitialized(exError):
    """Exception raised when a controller has not been initialized."""


class exUnknownSensor(exErrorValue):
    """Exception raised when a specified sensor is not found."""



#
# _server and _port are the default server and port values to be used
# if a Sensor is initialized without specifying a server and port.
#

_server      = None
_port        = None


#
# Initialize and cleanup the _server and _port default values.
#

def init(iface):
    """
    Initialize the interface mechanism to be used for communications
    to the 1-wire network. Only socket connections to owserver are
    supported.

    Examples:

        ownet.init('remote_system:3003')

    Will initialize the 1-wire interface to use the owserver running
    on remote_system on port 3003.
    """

    #print 'ownet.init(%s)' % iface
    global _server
    global _port
    pair = iface.split(':')
    if len(pair) != 2:
        raise exNoController
    _server      = pair[0]
    _port        = pair[1]


def finish():
    """
    Cleanup the OWFS library, freeing any used resources.
    """

    #print 'ownet.finish()'
    global _server
    global _port
    _server = None
    _port   = None



#
# 1-wire sensors
#

class Sensor(object):
    """
    A Sensor is the basic component of a 1-wire network. It represents
    a individual 1-wire element as it exists on the network.
    """

    def __init__(self, path, server = None, port = None, connection=None):
        """
        Create a new Sensor as it exists at the specified path.
        """
        #print 'Sensor.__init__(%s, server="%s", port=%s)' % (path, str(server), str(port))

        # setup the connection to use for connunication with the owsensor server
        if connection:
            self._connection = connection
        elif not server or not port:
            global _server
            global _port
            if not _server or not _port:
                raise exNotInitialized
            else:
                self._connection = Connection(server, port)
        else:
            self._connection = Connection(server, port)

        self._attrs = {}

        if path == '/':
            self._path    = path
            self._useCache = True
        elif path == '/uncached':
            self._path    = '/'
            self._useCache = False
        else:
            if path[:len('/uncached')] == '/uncached':
                self._path     = path[len('/uncached'):]
                self._useCache = False
            else:
                self._path     = path
                self._useCache = True

        self.useCache(self._useCache)


    def __str__(self):
        """
        Print a string representation of the Sensor in the form of:

        server:port/path - type

        Example:
        
            >>> print Sensor('/')
            xyzzy:9876/ - DS9490
        """

        #print 'Sensor.__str__'
        return "%s%s - %s" % (str(self._connection), self._usePath, self._type)


    def __repr__(self):
        """
        Print a representation of the Sensor in the form of:

        Sensor(path)

        Example:

            >>> Sensor('/')
            Sensor("/", server="xyzzy", port=9876)
        """

        #print 'Sensor.__repr__'
        return 'Sensor("%s", server="%s", port=%i)' % (self._usePath, self._connection._server, self._connection._port)


    def __eq__(self, other):
        """
        Two sensors are considered equal if their paths are
        equal. This is done by comparing their _path attributes so
        that cached and uncached Sensors compare equal.

        Examples:
        
            >>> Sensor('/') == Sensor('/1F.440701000000')
            False

            >>> Sensor('/') == Sensor('/uncached')
            True
        """

        #print 'Sensor.__eq__(%s)' % str(other)
        return self._path == other._path


    def __hash__(self):
        """
        Return a hash for the Sensor object's name. This allows
        Sensors to be used better in sets.Set.
        """

        #print 'Sensor.__hash__'
        return hash(self._path)


    def __getattr__(self, name):
        """
        Retreive an attribute from the sensor. __getattr__ is called
        only if the named item doesn't exist in the Sensor's
        namespace. If it's not in the namespace, look for the attribute
        on the physical sensor.

        Usage:

            s = ownet.Sensor('/1F.5D0B01000000')
            print s.family, s.PIO_0

        will result in the family and PIO.0 values being read from the
        sensor and printed. In this example, the family would be 1F
        and thr PIO.0 might be 1.
        """

        try:
            #print 'Sensor.__getattr__(%s)' % name
            attr = self._connection.read(object.__getattribute__(self, '_attrs')[name])
        except Exception as exc:
            raise AttributeError(name) from exc

        return attr


    def __setattr__(self, name, value):
        """
        Set the value of a sensor attribute. This is accomplished by
        first determining if the physical sensor has the named
        attribute. If it does, then the value is written to the
        name. Otherwise, the Sensor's dictionary is updated with the
        name and value.

        Usage:

            s = ownet.Sensor('/1F.5D0B01000000')
            s.PIO_1 = '1'

        will set the value of PIO.1 to 1.
        """

        #print 'Sensor.__setattr__(%s, %s)' % (name, value)

        # Life can get tricky when using __setattr__. Self doesn't
        # have an _attrs atribute when it's initially created. _attrs
        # is only there after it's been set in __init__. So we can
        # only reference it if it's already been added.
        if hasattr(self, '_attrs'):
            if name in self._attrs:
                self._connection.write(self._attrs[name], value)
            else:
                self.__dict__[name] = value
        else:
            self.__dict__[name] = value


    def useCache(self, use_cache):
        """
        Set the sensor to use the underlying owfs cache (or not)
        depending on the use_cache parameter.

        Usage:

            s = ownet.Sensor('/1F.5D0B01000000')
            s.useCache(False)

        will set the internal sensor path to /uncached/1F.5D0B01000000.

        Also:

            s = ownet.Sensor('/uncached/1F.5D0B01000000')
            s.useCache(True)

        will set the internal sensor path to /1F.5D0B01000000.
        """

        #print 'Sensor.useCache(%s)' % str(use_cache)
        self._useCache = use_cache
        if self._useCache:
            self._usePath = self._path
        else:
            if self._path == '/':
                self._usePath = '/uncached'
            else:
                self._usePath = '/uncached' + self._path

        if self._path == '/':
            self._type    = self._connection.read('/system/adapter/name.0')
        else:
            self._type  = self._connection.read('%s/type' % self._usePath)

        self._attrs = dict([(n.replace('.', '_'), self._usePath + '/' + n) for n in self.entries()])


    def entries(self):
        """
        Generator which yields the attributes of a sensor.
        """
        #print 'Sensor.entries()'
        list = self._connection.dir(self._usePath)
        if self._path == '/':
            for entry in list:
                if not '/' in entry:
                    yield entry
        else:
            for entry in list:
                yield entry.split('/')[-1]


    def entryList(self):
        """
        List of the sensor's attributes.

        Example:

            >>> Sensor("/10.B7B64D000800").entryList()
            ['address', 'crc8', 'die', 'family', 'id', 'power',
            'present', 'temperature', 'temphigh', 'templow',
            'trim', 'trimblanket', 'trimvalid', 'type']
        """
        #print 'Sensor.entryList()'
        return [e for e in self.entries()]


    def sensors(self, names = ['main', 'aux']):
        """
        Generator which yields all the sensors that are associated
        with the current sensor.

        In the event that the current sensor is the adapter (such as a
        DS9490 USB adapter) the list of sensors directly attached to
        the 1-wire network will be yielded.

        In the event that the current sensor is a microlan controller
        (such as a DS2409) the list of directories found in the names
        list parameter will be searched and any sensors found will be
        yielded. The names parameter defaults to ['main', 'aux'].
        """

        #print 'Sensor.sensors(%s)' % str(names)
        if self._type == 'DS2409':
            for branch in names:
                path = self._usePath + '/' + branch
                list = filter(lambda x: '/' in x, self._connection.dir(path))
                if list:
                    namelist = ','.join(list)
                    #print 'Sensor.sensors namelist(%s)' % str(namelist)
                    for branch_entry in namelist.split(','):
                        # print 'branch_entry(%s)' % str(branch_entry)
                        try:
                            self._connection.read(branch_entry + '/type')
                        except exUnknownSensor as ex:
                            continue
                        yield Sensor(branch_entry, connection=self._connection)

        else:
            list = self._connection.dir(self._usePath)
            if self._path == '/':
                for entry in list:
                    if '/' in entry:
                        yield Sensor(entry, connection=self._connection)
                

    def sensorList(self, names = ['main', 'aux']):
        """
        List of all the sensors that are associated with the current
        sensor.

        In the event that the current sensor is the adapter (such as a
        DS9490 USB adapter) the list of sensors directly attached to
        the 1-wire network will be yielded.

        In the event that the current sensor is a microlan controller
        (such as a DS2409) the list of directories found in the names
        list parameter will be searched and any sensors found will be
        yielded. The names parameter defaults to ['main', 'aux'].

        Example:

            >>> Sensor("/1F.440701000000").sensorList()
            [Sensor("/1F.440701000000/main/29.400900000000")]
        """

        #print 'Sensor.sensorList(%s)' % str(names)
        return [s for s in self.sensors()]


    def find(self, **keywords):
        """
        Generator which yields all the sensors whose attributes match
        those past in. By default, any matched attribute will yield a
        sensor. If the parameter all is passed to the find call, then
        all the supplied attributes must match to yield a sensor.

        Usage:

            for s in Sensor('/').find(type = 'DS2408'):
                print s

        will print all the sensors whose type is DS2408.

            root = Sensor('/')
            print len([s for s in root.find(all = True,
                                               family = '1F',
                                               type = 'DS2409')])

        will print the count of sensors whose family is 1F and whose
        type is DS2409.
        """
        #print 'Sensor.find', keywords
        #recursion = keywords.pop('recursion', False)
        all       = keywords.pop('all',       False)

        for sensor in self.sensors():
            match = 0
            for attribute in keywords:
                if hasattr(sensor, attribute):
                    if keywords[attribute]:
                        if getattr(sensor, attribute) == keywords[attribute]:
                            match = match + 1
                    else:
                        if hasattr(sensor, attribute):
                            match = match + 1

            if not all and match:
                yield sensor
            elif all and match == len(keywords):
                yield sensor
