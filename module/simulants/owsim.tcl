#!/usr/bin/wish

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

set pan [frame .p -bg cyan]
pack $pan -fill both -expand 1

StatusFrame $pan
set pane [panedwindow $pan.pane]
pack $pane -side top -expand yes -fill both

SetupPanels $pane

SetupServer
