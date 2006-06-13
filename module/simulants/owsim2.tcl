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
source ow2.tcl
source ow3.tcl
source ow4.tcl

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}

wm geometry . 500x400

CommandLineParsing

foreach d $dlist {
    SetAddress $d
}

set pan [frame .p -bg cyan]
pack $pan -fill both -expand 1

set status [StatusFrame $pan]
set pane [panedwindow $pan.pane]
pack $pane -side top -expand yes -fill both

SetupPanels $pane

$status insert end "Hello\nthere"
