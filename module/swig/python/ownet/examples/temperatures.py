#! /usr/bin/env python

import sys
import ownet

if len(sys.argv) != 3:
    print 'temperatures.py server port'
    sys.exit(1)

r = ownet.Sensor('/', server=sys.argv[1], port=int(sys.argv[2]))
e = r.entryList()
s = r.sensorList()
print 'r:', r
print 'r.entryList():', e
print 'r.sensorList():', s

for x in r.sensors():
    if hasattr(x, 'temperature'):
        print x, x.temperature
