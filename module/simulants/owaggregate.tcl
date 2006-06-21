#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
# $Id$
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Simulant! Generic and utility functions ########
###########################################################

# aggregate list
# ALL or BYTE
# letter number
# elements
# separator ("" binary, else ",")
#
# if exist addr.filetype.aggregate

proc ReadALL { file } {
    global chip

    # see if it's an aggregate variable
    if { ![info exist chip($file.aggregate)] } {return}

    # get aggregate data
    foreach { all letter elements sep } $chip($file.aggregate) {break}

    set A {}
    
    for {set i 0} {$i < $elements} {incr i} {
        switch $letter {
            letters { lappend A $chip($file.[string index "ABCDEFGHIJKLMNOPQRSTUVWXYZ" $i]) }
            numbers { lappend A $chip($file.$i) }
        }
    }
    set chip($file.ALL) [join $A $sep]
}
        
