#!/usr/bin/wish8.4

set chip("type")      DS18B20

proc SetAddress {adrstr} {
    global chip
    set chip("address")   [string toupper $adrstr]
    set chip("family")    [string range $chip("address")  0  1]
    set chip("id")        [string range $chip("address")  2 13]
    set chip("crc8")      [string range $chip("address") 14 15]
    set chip("r_id")      [string range $chip("id") 10 11][string range $chip("id") 8 9][string range $chip("id") 6 7][string range $chip("id") 4 5][string range $chip("id") 2 3][string range $chip("id") 0 1]
    set chip("r_address") $chip("crc8")$chip("r_id")$chip("family")
    set chip("present")   1
}

SetAddress 1032140080FF00E7

proc AlarmCheck {varName index op} {
    global chip
    set alarm false
    if { $chip("hightemp") < $chip("temperature") } {
        set alarm true
    } elseif { $chip("lowtemp") > $chip("temperature") } {
        set alarm true
    }
    if { $chip("alarm") != $alarm } {
        set chip("alarm") $alarm
        if { $alarm } {
            .alarm config -bg red
        } else {
            .alarm config -bg green
        }
    }
}

set chip("hightemp") 60
set chip("temperature") 16
set chip("lowtemp") 0
 
set chip("alarm") false
 
frame .g -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66
pack .g -side top -fill x
foreach g { type family address id crc8 r_address r_id present } {
    label .g.l$g -text $g
    label .g.v$g -text $chip("$g") -bg white
    grid .g.l$g .g.v$g
    grid .g.l$g -sticky e
    grid .g.v$g -sticky w
}
labelframe .alarm -text ALARM -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg green
pack .alarm -side top -fill x
label .alarm.state -textvariable chip("alarm") -bg white
pack .alarm.state
    
set color("hightemp")    #CC3300
set color("temperature") #666666
set color("lowtemp")     #6666FF

foreach f {hightemp temperature lowtemp} {
    labelframe .$f -text $f -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66
    pack .$f -side top -fill x
    scale .$f.scale -variable chip("$f") -orient horizontal -from -40 -to 125 -fg white -bg $color("$f") -state disabled
    pack .$f.scale -side right -fill x -expand true
    spinbox .$f.spin -textvariable chip("$f") -width 5 -from -40 -to 125 -state disabled
    pack .$f.spin -side right
    trace variable chip("$f") w AlarmCheck
}
.temperature.scale config -state normal
.temperature.spin config -state normal

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}
