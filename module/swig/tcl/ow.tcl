# -*- Tcl -*-
# File: ow.tcl
#
# Created: Tue Jan 11 20:34:11 2005
#
# $Id$
#
proc ::ow {interface args} {
    if {$args == ""} {
	::OW::init $interface
    } else {
	::OW::init $interface $args
    }
}

namespace eval ::OW {
}
