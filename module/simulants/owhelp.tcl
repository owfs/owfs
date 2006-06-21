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

proc Help { } {
    tk_messageBox -type ok -title {OWSIM command line} -message {
> wish owsim.tcl <port> <fc> <fc> ...
 port - decimal tcp port (must be above 1024)
 fc   - hex (2 digit) 1-wire family code

example:

> wish owsim.tcl 3000 10 22 01

starts owsim on port 3000 with devices:
  10 = DS18S20 (temperature probe)
  22 = DS2822 (low cost temperature probe)
  01 = DS2401 (ID only)
  
Connect to this owsim with any of:
  -- filesystem --
> owfs -s 3000 <mount point>
  -- web server http://localhost:3001 --
> owhttpd -s 3000 -p 3001 
  -- ftp server --
> owftpd -s 3000
  -- owserver multiplexor on port 3002 --
> owserver -s 3000 -p 3002
    }
}

     
