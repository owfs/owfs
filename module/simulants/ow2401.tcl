#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Simulant! DS2401 functions  ####################
###########################################################

proc Setup01 { addr type } {
    global chip
    set chip($addr.type) $type
    set chip($addr.display) Display01
    
}

proc Display01 { addr fmain } {
    DisplayStandard     $addr $fmain
}



###########################################################
########## Simulant! Do it  ###############################
###########################################################
#    set chip($addr.page.aggregate) [list ALL letters 3 ""]
 #   foreach x { A B C } {
  #      set chip($addr.page.$x) $x$x
#    }
 #   puts [ReadALL $addr page]
  #  WriteALL $addr page
#    foreach x { A B C } {
 #       puts $chip($addr.page.$x)
  #  }
