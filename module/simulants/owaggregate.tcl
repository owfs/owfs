#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
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

proc ReadBYTE { addr file } {
    global chip
    set devfile $addr.$file
    # see if it's an aggregate variable
    if { ![info exist chip($devfile.aggregate)] } {return}

    # get aggregate data
    foreach { all letter elements sep } $chip($devfile.aggregate) {break}

    set B 0
    
    for {set i 0} {$i < $elements} {incr i} {
        switch $letter {
            letters { set b $chip($devfile.[string index "ABCDEFGHIJKLMNOPQRSTUVWXYZ" $i]) }
            numbers { set b $chip($devfile.$i) }
        }
        if [catch [expr $b != 0 ] b] {set b 0}
        set B [ expr ( $B << 1 ) | $b ]
    }
    set chip($devfile.BYTE) $B
}
        
proc WriteBYTE { addr file } {
    global chip
    set devfile $addr.$file
    # see if it's an aggregate variable
    if { ![info exist chip($devfile.aggregate)] } {return}

    # get aggregate data
    foreach { all letter elements sep } $chip($devfile.aggregate) {break}

    if [ catch [expr $chip($devfile.BYTE) + 0 ] B] {[set B 0]}
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
        

