#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

# Uses a technique from Bryan Oakley
# Intelligent text widget autoscroll
# http://www.tclscripting.com/articles/feb06/article2.html
proc AddLog { msg tag } {
    global serve
    set t $serve(text)
    $t configure -state normal

    # get the state of the widget before we add the new message
    set yview [$t yview]
    set y1 [lindex $yview 1]

    $t insert end $msg $tag

    # autoscroll only if the user hasn't manually scrolled
    if {$y1 == 1} {
        $t see end
    }

    $t configure -state disabled
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
