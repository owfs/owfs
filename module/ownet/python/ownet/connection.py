# -*- coding: utf-8 -*-
"""
::BOH
$Id$
$HeadURL: http://subversion/stuff/svn/owfs/trunk/ow/__init__.py $

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

OWFS is an open source project developed by Paul Alfille and hosted at
http://www.owfs.org
"""


import sys
import os
import socket
import struct
import re


__author__ = 'Peter Kropf'
__email__ = 'pkropf@gmail.com'
__version__ = '$Id$'.split()[2]


class exError(Exception):
    """base exception for all one wire raised exceptions."""


class exErrorValue(exError):
    """Base exception for all one wire raised exceptions with a value."""
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class exInvalidMessage(exErrorValue):
    """Exception raised when trying to unpack a message that doesn't meet specs."""


class exShortRead(exError):
    """Exception raised when too few bytes are received from the owserver."""


class OWMsg:
    """
    Constants for the owserver api message types.
    """
    error    = 0
    nop      = 1
    read     = 2
    write    = 3
    dir      = 4
    size     = 5
    presence = 6


class Connection(object):
    """
    A Connection provides access to a owserver without the standard
    core ow libraries. Instead, it impliments the wire protocol for
    communicating with the owserver. This allows Python programs to
    interact with the ow sensors on any platform supported by Python.
    """

    def __init__(self, server, port):
        """
        Create a new connection object.
        """
        #print 'Connection.__init__(%s, %i)' % (server, port)

        self._server = server
        self._port   = port


    def __str__(self):
        """
        Print a string representation of the Connection in the form of:

        server:port
        """

        #print 'Connection.__str__'
        return "%s:%i" % (self._server, self._port)


    def __repr__(self):
        """
        Print a representation of the Connection in the form of:

        Connection(server, port)

        Example:

            >>> Connection('xyzzy', 9876)
            Connection(server="xyzzy", port=9876)
        """

        #print 'Connection.__repr__'
        return 'Connection("%s", %i)' % (self._server, self._port)


    def read(self, path):
        """
        """

        #print 'Connection.read("%s", %i, "%s")' % (path)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((self._server, self._port))

        smsg = self.pack(OWMsg.read, len(path) + 1, 8192)
        s.sendall(smsg)
        s.sendall(path + '\x00')

        while 1:
            data = s.recv(24)

            if len(data) is not 24:
                raise exShortRead

            ret, payload_len, data_len = self.unpack(data)

            if payload_len >= 0:
                data = s.recv(payload_len)
                rtn = self.toNumber(data[:data_len])
                break
            else:
                # ping response
                rtn = None
                break

        s.close()
        return rtn


    def write(self, path, value):
        """
        """

        #print 'Connection.write("%s", "%s")' % (path, str(value))
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((self._server, self._port))

        value = str(value)
        smsg = self.pack(OWMsg.write, len(path) + 1 + len(value) + 1, len(value) + 1)
        s.sendall(smsg)
        s.sendall(path + '\x00' + value + '\x00')

        data = s.recv(24)

        if len(data) is not 24:
            raise exShortRead

        ret, payload_len, data_len = self.unpack(data)

        s.close()
        return ret


    def dir(self, path):
        """
        """

        #print 'Connection.dir("%s")' % (path)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((self._server, self._port))

        smsg = self.pack(OWMsg.dir, len(path) + 1, 0)
        s.sendall(smsg)
        s.sendall(path + '\x00')

        fields = []
        while 1:
            data = s.recv(24)

            if len(data) is not 24:
                raise exShortRead

            ret, payload_len, data_len = self.unpack(data)

            if payload_len > 0:
                data = s.recv(payload_len)
                fields.append(data[:data_len])
            else:
                # end of dir list or 'ping' response
                break

        s.close()
        return fields


    def pack(self, function, payload_len, data_len):
        """
        """

        #print 'Connection.pack(%i, %i, %i)' % (function, payload_len, data_len)
        return struct.pack('iiiiii',
                           socket.htonl(0),           #version
                           socket.htonl(payload_len), #payload length
                           socket.htonl(function),    #type of function call
                           socket.htonl(258),         #format flags -- 266 for alias upport
                           socket.htonl(data_len),    #size of data element for read or write
                           socket.htonl(0),           #offset for read or write
                           )


    def unpack(self, msg):
        """
        """

        #print 'Connection.unpack("%s")' % msg
        if len(msg) is not 24:
            raise exInvalidMessage, msg

        val          = struct.unpack('iiiiii', msg)
        version      = socket.ntohl(val[0])
        payload_len  = socket.ntohl(val[1])
        try:
            ret_value    = socket.ntohl(val[2])
        except OverflowError:
            ret_value = 0
        format_flags = socket.ntohl(val[3])
        data_len     = socket.ntohl(val[4])
        offset       = socket.ntohl(val[5])

        return ret_value, payload_len, data_len


    def toNumber(self, str):
        """
        """

        stripped = str.strip()
        if re.compile('^-?\d+$').match(stripped) :
            return int(stripped)

        if re.compile('^-?\d*\.\d*$').match(stripped) :	# Could crash if it matched '.' - let it.
            return float(stripped)

        return str
