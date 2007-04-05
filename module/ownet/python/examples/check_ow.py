#! /usr/bin/env python

"""
$Id$
$HeadURL: http://void/svn/peter/playground/nud/categories.py $

Copyright (c) 2006 Peter Kropf. All rights reserved.

Nagios (http://nagios.org) plugin to check the value of a 1-wire sensor.
"""


import ownet
import sys
import os
from optparse import OptionParser
import socket


class nagios:
    ok       = (0, 'OK')
    warning  = (1, 'WARNING')
    critical = (2, 'CRITICAL')
    unknown  = (3, 'UNKNOWN')


# FIXME: need to add more help text in the init_string and sensor_path parameters
parser = OptionParser(usage='usage: %prog [options] server port sensor_path',
                      version='%prog ' + ownet.__version__)
parser.add_option('-v', dest='verbose', action='count',             help='multiple -v increases the level of debugging output.')
parser.add_option('-w', dest='warning',                 type='int', help='warning level.')
parser.add_option('-c', dest='critical',                type='int', help='critical level.')
parser.add_option('-f', dest='field',                               help='sensor field to be used for monitoring.', default='temperature')
options, args = parser.parse_args()

if len(args) != 3:
    print 'OW ' + nagios.unknown[1] + ' - ' + 'missing command line arguments'
    parser.print_help()
    sys.exit(nagios.unknown[0])

server = args[0]
port   = int(args[1])
path   = args[2]

try:
    s = ownet.Sensor(path, server=server, port=port)
except ownet.exUnknownSensor, ex:
    print 'OW ' + nagios.unknown[1] + ' - unknown sensor ' + str(ex)
    sys.exit(nagios.unknown[0])
except socket.error, ex:
    print 'OW ' + nagios.unknown[1] + ' - communication error ' + str(ex)
    sys.exit(nagios.unknown[0])

if options.verbose == 1:
    print s
elif options.verbose > 1:
    print s
    print 'entryList:', s.entryList()
    print 'sensorList:', s.sensorList()

if not hasattr(s, options.field):
    print 'OW ' + nagios.unknown[1] + ' - ' + 'unknown field: ' + options.field
    sys.exit(nagios.unknown[0])

val = getattr(s, options.field)

if val >= options.critical:
    status    = nagios.critical[1]
    exit_code = nagios.critical[0]
elif val >= options.warning:
    status    = nagios.warning[1]
    exit_code = nagios.warning[0]
else:
    status    = nagios.ok[1]
    exit_code = nagios.ok[0]

print 'OW %s - %s %s: %i| %s/%s=%i' % (status, path, options.field, val, path, options.field, val)
sys.exit(exit_code)
