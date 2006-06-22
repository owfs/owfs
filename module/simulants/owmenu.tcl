#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
# $Id$
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

proc SetupMenu { } {
    global serve
    set serve(menu) [menu .menu]
    . config -menu $serve(menu)
#    toplevel $serve(menu) -menu
    menu $serve(menu).file -tearoff 0
    $serve(menu) add cascade -label File -menu $serve(menu).file  -underline 0
        $serve(menu).file add command -label "Save Log" -underline 0 -command SaveLog
        $serve(menu).file add command -label "Save Log As..." -underline 9 -command SaveAsLog
        $serve(menu).file add separator
        $serve(menu).file add command -label Quit -underline 0 -command exit

    menu $serve(menu).device -tearoff 1
    $serve(menu) add cascade -label Devices -menu $serve(menu).device  -underline 0
        $serve(menu).device add command -label "Save Log" -underline 0 -command SaveLog
        $serve(menu).device add command -label "Save Log As..." -underline 9 -command SaveAsLog
        $serve(menu).device add separator
        $serve(menu).device add command -label Quit -underline 0 -command exit

    menu $serve(menu).help -tearoff 0
    $serve(menu) add cascade -label Help -menu $serve(menu).help  -underline 0
        $serve(menu).help add command -label "About OWSIM" -underline 0 -command About
        $serve(menu).help add command -label "Help with command line" -underline 0 -command Help
}
