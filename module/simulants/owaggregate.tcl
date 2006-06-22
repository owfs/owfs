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

proc ReadALL { addr file } {
    global chip
    set devfile $addr.$file
    # see if it's an aggregate variable
    if { ![info exist chip($devfile.aggregate)] } {return}

    # get aggregate data
    foreach { all letter elements sep } $chip($devfile.aggregate) {break}

    set A {}
    
    for {set i 0} {$i < $elements} {incr i} {
        switch $letter {
            letters { lappend A $chip($devfile.[string index "ABCDEFGHIJKLMNOPQRSTUVWXYZ" $i]) }
            numbers { lappend A $chip($devfile.$i) }
        }
    }
    set chip($devfile.ALL) [join $A $sep]
}
        
proc WriteALL { addr file } {
    global chip
    set devfile $addr.$file
    # see if it's an aggregate variable
    if { ![info exist chip($devfile.aggregate)] } {return}

    # get aggregate data
    foreach { all letter elements sep } $chip($devfile.aggregate) {break}

    if { $sep eq "" } {
        set l [expr [string length $chip($devfile.ALL)] / $elements]
        set s 0
        set e [expr $s - 1 ]
        for {set i 0} {$i < $elements} {incr i} {
            switch $letter {
                letters { set chip($devfile.[string index "ABCDEFGHIJKLMNOPQRSTUVWXYZ" $i]) [string range $chip($devfile.ALL) $s $e] }
                numbers { set chip($file.$i) [string range $chip($devfile.ALL) $s $e] }
            }
            incr s $l
            incr e $l
        }
    } else {
        set A [split $chip($devfile.ALL) #sep]
        for {set i 0} {$i < $elements} {incr i} {
            switch $letter {
                letters { set chip($devfile.[string index "ABCDEFGHIJKLMNOPQRSTUVWXYZ" $i]) [lindex $A $i] }
                numbers { set chip($file.$i) [lindex $A $i] }
            }
        }
    }
}
        
