# -*- Tcl -*-
# File: ow.tcl
#
# Created: Tue Jan 11 20:34:11 2005
#
# $Id$
#

namespace eval ::OW {
  proc ::OW { args } { return [eval ::OW::init $args] }
}

proc ::OW::init {args} {
  array set oldopts {
    -format -f
    -celsius -C
    -fahenheit -F
    -kelvin -K
    -rankine -R
    -readonly -r
    -cache -t
  }
  set sargs ""
  foreach arg $args {
	if {[info exists oldopts($arg)]} {
		append sargs $oldopts($arg) { }
	} else {
		append sargs $arg { }
	}
  }
  set sargs [string trim $sargs]
  return [::OW::_init $sargs]
}
