#!/usr/bin/wish
# $Id$
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

source owaddress.tcl
source ow2401.tcl
source ow18xx.tcl
source owcmdlin.tcl
source owdisplay.tcl
source owmenu.tcl
source ownet.tcl
source owhandler.tcl

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
SetupMenu

SetupServer
