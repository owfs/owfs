#!/bin/sh
# the next line restarts using wish \
exec wish "$0" "$@"

option add *highlightThickness 0
tk_setPalette gray60

source rnotebook.tcl

#################################
# The following code implements an example of using the
# notebook widget.
#

wm geometry . 400x220

frame .un -borderwidth 2 -relief raised
frame .deux -borderwidth 2 -relief raised 

pack .un -side top -fill both -expand 1
pack .deux -side top -fill x

button .deux.xit -text "quit" -command exit
button .deux.conf -text "reconfigure" -command reconf

pack .deux.xit .deux.conf -side left -padx 10 -pady 5

set nn .un.n 

Rnotebook:create $nn -tabs {on two three} -borderwidth 2

pack $nn -fill both -expand 1 -padx 10 -pady 10

set frm [Rnotebook:frame $nn 1]
label $frm.l1 -text "Welcome frame 1 !" 
pack $frm.l1 -fill both -expand 1

set frm [Rnotebook:frame $nn 2]
label $frm.l2 -text "Good Morning frame 2 !" 
pack $frm.l2 -fill both -expand 1

set frm [Rnotebook:frame $nn 3]
label $frm.l3 -text "Hello frame 3 !" 
pack $frm.l3 -fill both -expand 1

proc reconf {} {
    set frm [Rnotebook:button .un.n 1]
    $frm configure -text "page one"
}
