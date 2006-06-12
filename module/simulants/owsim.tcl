#!/usr/bin/wish

option add *highlightThickness 0
tk_setPalette gray60

source rnotebook.tcl

###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

source ow.tcl

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}

wm geometry . 500x400

set dlist [list 0x10 0x10 0x10 0x01 0x20]
foreach d $dlist {
    lappend devlist [SetAddress $d]
}

foreach d $devlist {
    lappend devname $chip($d.name)
}

Rnotebook:create .n -tabs $devname -borderwidth 2

set i 0
foreach d $devlist {
    set w [Rnotebook:frame .n [incr i] ]
    $chip($d.process) $d $w
}

pack .n -fill both -expand 1 -padx 10 -pady 10
