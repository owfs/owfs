#!/bin/sh owsim.tcl
# $Id$
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## About OWSIM ####################################
###########################################################

proc About { } {
    tk_messageBox -type ok -title {About OWSIM} -message {
Program: owsim
    
Synopsis: owserver simulator
    
Description: owsim mimics the real 1-wire server: owserver
  The communication is logged and the simulated 1-wire devices are shown on screen.
  1-wire parameters like temperature can be set on screen rather than measured.
    
    
Author: Paul H Alfille <palfille@earthlink.net>
    
Copyright: June 2006 GPL 2.0 license (though a more liberal license may be possible).
    
Website: http://www.owfs.org
Download: http://owfs/sourceforge.net
    }
}

     
