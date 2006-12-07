ownet
=====

ownet is a standalone python module for accessing 1-wire sensors
through an owserver. The ownet module does not use the core ow
libraries. Instead, it impliments the wire protocol for owserver
communication. This means that the ow core libraries do no have to be
installed on all systems, just on the system running owserver. The
ownet module can be installed on any system with Python.


Installation
------------

Installation of the ownet module follows the standard Python distutils
installation.

$ cd module/swig/python/ownet
$ sudo python setup.py install

There should now be a module called ownet in the site-packages
directory of the python installation.


Examples
--------

There are two sample programs in the examples directory - check_ow.py
and temperatures.py.

The check_ow.py program impliments a Nagios (http://www.nagios.org)
plugin to monitor the value of a sensor field, like the
temperature. Usage is beyond the scope of this document. See the
Nagios documentation for configuring and monitoring via plugins.

The temperatures.py will print out some details of the sensors along
with the temperatures found on any sensor that has a field called
temperature. Running it should print out something like:

$ python ./examples/temperatures.py  kuro2 9999
r: kuro2:9999/ - DS9490
r.entryList(): ['bus.0', 'settings', 'system', 'statistics']
r.sensorList(): [Sensor("/10.B7B64D000800", server="kuro2",
port="9999"), Sensor("/26.AF2E15000000", server="kuro2", port="9999"),
Sensor("/81.A44C23000000", server="kuro2", port="9999")]
kuro2:9999/10.B7B64D000800 - DS18S20      22.4375
kuro2:9999/26.AF2E15000000 - DS2438      21.0938
$

where "kuro2" should be the host name where owserver is running and
"9999" is the port to be used to communicate with the owserver.
