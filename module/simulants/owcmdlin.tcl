#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Command line parsing ###########################
###########################################################

proc CommandLineParsing { } {
    global argv
    global serve
    
    set serve(port) 0
    foreach a $argv {
        if { [regexp {^[0-9a-fA-F]{2}$} $a] == 1 } {
            SetAddress $a
        } elseif { [regexp {[0-9]{3,}} $a] == 1 } {
            set serve(port) $a
        }
    }
    if { ![info exists dlist] || [llength $dlist] == 0 } {
        SetAddress 10
    }
}
