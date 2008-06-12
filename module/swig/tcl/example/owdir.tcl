#!/usr/bin/env tclsh
# -*- Tcl -*-
# File: owdir.tcl
#
# $Id$
#
# Start script with this command:  tclsh owdir.tcl
#

package require ow 0.2

puts [ ::OW::version ]

if { $argc == 0 } {
    puts "Usage:"
    puts "\towdir.tcl 1wire-adapter \[dir\]"
    puts "  1wire-adapter (required):"
    puts "\t'u' for USB -or-"
    puts "\t--fake=10,28 for a fake-adapter with DS18S20 and DS18B20-sensor -or-"
    puts "\t/dev/ttyS0 (or whatever) serial port -or-"
    puts "\t4304 for remote-server at port 4304"
    puts "\t192.168.1.1:4304 for remote-server at 192.168.1.1:4304"
    exit;
}


set adapter [ lindex $argv 0 ];

if { $argc > 1 } {
    set dir [ lindex $argv 1 ];
} else {
    set dir "/";
}
    
::OW::init $adapter
puts [ ::OW::get $dir ]
::OW::finish
