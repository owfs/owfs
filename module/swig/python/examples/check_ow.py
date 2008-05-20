#! /usr/bin/env python

"""
Nagios plugin to check the value of a 1-wire sensor. Nagios is a host, service
and network monitoring program available at http://nagios.org.

Copyright (c) 2008 Peter Kropf. All rights reserved.

Examples:

This example doesn't really do all that much, must verifies that the
specified sensor exists on the USB adapter.
    
    ./check_ow.py u /10.54BA4D000800

Check the temperature of the sensor on the USB adapter. Give a warning
if the temperature is greater than 29 degrees celsius and make it
critical if above 35.
    
    ./check_ow.py -c 35 -w 29 u /10.54BA4D000800/temperature
"""


import ow
import sys
from optparse import OptionParser


class nagios:
    ok       = (0, 'OK')
    warning  = (1, 'WARNING')
    critical = (2, 'CRITICAL')
    unknown  = (3, 'UNKNOWN')


def exit(status, message):
    print 'ow ' + status[1] + ' - ' + message
    sys.exit(status[0])


# FIXME: need to add more help text in the init_string and sensor_path parameters
parser = OptionParser(usage='usage: %prog [options] init_string sensor_path',
                      version='%prog ' + ow.__version__)
parser.add_option('-v', dest='verbose',     action='count', help='multiple -v increases the level of debugging output.')
parser.add_option('-w', dest='warning',     type='float',   help='warning level.')
parser.add_option('-c', dest='critical',    type='float',   help='critical level.')
parser.add_option('-t', dest='temperature', choices=['C', 'F', 'R', 'K'], help='set the temperature scale: C -  celsius, F - fahrenheit, R - rankine or K - kelvin.')
options, args = parser.parse_args()

if len(args) != 2:
    exit(nagios.unknown, 'missing command line arguments')

init = args[0]
sensor_path = args[1]

if options.temperature:
    ow.opt(options.temperature)

try:
    ow.error_print(ow.error_print.suppressed) # needed to exclude debug output which confuses nagios
    ow.init(init)
except ow.exUnknownSensor, ex:
    exit(nagios.unknown[0], 'unable to initialize sensor adapter ' + str(ex))


pieces = [x for x in sensor_path.split('/') if x]
if len(pieces):
    sensor = '/' + pieces[0]
else:
    sensor = '/'


try:
    s = ow.Sensor(sensor)
except ow.exUnknownSensor, ex:
    exit(nagios.unknown, 'unknown sensor ' + str(ex))


if options.verbose == 1:
    print s
elif options.verbose > 1:
    print s
    print 'entryList:', s.entryList()
    print 'sensorList:', s.sensorList()

if len(pieces) > 1:
    field = pieces[1]
    
    try:
        v = float(getattr(s, field))
    except AttributeError:
	    exit(nagios.unknown, 'unknown field - ' + repr(s) + '.' + field)

    if options.critical:
        if v > options.critical:
            exit(nagios.critical, repr(s) + '.' + field + ' %f' % v)
	
    if options.warning:
        if v > options.warning:
            exit(nagios.warning, repr(s) + '.' + field + ' %f' % v)
	
    exit(nagios.ok, repr(s) + '.' + field + ' %f' % v)

else: # len(pieces) > 1
    exit(nagios.ok, repr(s))
