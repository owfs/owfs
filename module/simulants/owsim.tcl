#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

source owaddress.tcl
source owaggregate.tcl
source ow2401.tcl
source ow18xx.tcl
source owcmdlin.tcl
source owdisplay.tcl
source owmenu.tcl
source owlog.tcl
source owabout.tcl
source owhelp.tcl
source ownet.tcl
source owhandler.tcl

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}

wm geometry . 500x400

CommandLineParsing

set pan [frame .p -bg #996633]
pack $pan -fill both -expand 1
SetupDisplay $pan

SetupMenu

SetupServer
