#!/usr/bin/wish owsim.tcl $@
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
    foreach m { File Device Help } {
        menu $serve(menu).m$m
        $serve(menu) add cascade -label $m -menu $serve(menu).m$m  -underline 0
    }
    $serve(menu).mFile add command -label "Save Log" -underline 0 -command SaveLog
    $serve(menu).mFile add command -label "Save Log As..." -underline 10 -command SaveAsLog
    $serve(menu).mFile add separator
    $serve(menu).mFile add command -label Quit -underline 0 -command exit
}

proc SaveAsLog { } {
    global serve
    set f [tk_getSaveFile -defaultextension ".txt" -initialfile owsim[clock seconds].txt ]
    if { [string equal $f ""] } {return}
    set serve(logfile) $f
    SaveLog
}

proc SaveLog { } {
    global serve
    if { ![info exist serve(logfile)] } {
        return [SaveAsLog]
    }
    if {[catch {open $serve(logfile) {WRONLY CREAT TRUNC}} f ]}  {
        tk_messageBox -icon error -title "Save Logfile" -message "Cannot open file $serve(logfile) to write data.\n$f" -type ok
        return
    }
    if [catch [puts -nonewline $f [$serve(text) get 1.0 end] ] x ] {
        tk_messageBox -icon error -title "Save Logfile" -message "Cannot write data to $serve(logfile).\n$x" -type ok
        return
    }
    close $f
}
