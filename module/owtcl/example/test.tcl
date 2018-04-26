#!/usr/bin/env tclsh
# -*- Tcl -*-
# File: test.tcl
#
# Start script with this command:  tclsh test.tcl
#

package require ow 0.2

puts "Version information:"
puts [ ::OW::version ]

::OW::init --fake 28 --fake 10

puts "\nDirectory-listing of / (return string)"
puts [ ::OW::get "/" ]

puts "\nDirectory-listing of / (return list)"
set list [ ::OW::get / -list ]
puts $list

puts "\nFirst object in directory list"
puts [ lindex $list 0 ]

puts "\nRead /10.67C6697351FF/temperature"
puts [ ::OW::get "/10.67C6697351FF/temperature" ]

puts "\nRead /10.67C6697351FF/type"
puts [ ::OW::get "/10.67C6697351FF/type" ]

puts "\nRead bus.0/interface/settings/name"
puts [ ::OW::get "bus.0/interface/settings/name" ]

::OW::finish
