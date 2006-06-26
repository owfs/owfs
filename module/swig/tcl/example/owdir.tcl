# -*- Tcl -*-
# File: owdir.tcl
#
# $Id$
#

package require ow 0.1

puts [ ::OW::version ]

::OW::init u
if {$argv == ""} {
     puts [ ::OW::get "/" ]
} else {
     puts [ ::OW::get $argv ]
}
::OW::finish
