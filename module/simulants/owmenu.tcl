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
