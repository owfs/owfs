# -*- Tcl -*-
# File: owdir.tcl
#
# $Id$
#
# Start script with this command:  tclsh owdir.tcl
#

package require ow 0.2

puts [ ::OW::version ]

::OW::init u
if {$argv == ""} {
     puts [ ::OW::get "/" ]
} else {
     puts [ ::OW::get $argv ]
}
::OW::finish
