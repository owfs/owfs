# $Id$
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Simulant! Temperature-specific functions #######
###########################################################

# DS18S20
set chip(10.read) [list temperature temphigh templow trim trimvalid trimblanket power die]
set chip(10.write) [list temphigh templow trimblanket]

# DE18B20
set chip(28.read) [concat $chip(10.read) [list fasttemp]]
set chip(28.write) $chip(10.write)

# DS 1922
set chip(22.read} $chip(28.read)
set chip(22.write) $chip(28.write)

proc Setup10 { addr fmain } {
    global chip

    set chip($addr.temphigh) 60
    set chip($addr.temperature) 16
    set chip($addr.templow) 0
    set chip($addr.alarm) false

    set chip($addr.die) B2
    set chip($addr.power) 0
    set chip($addr.trim) B2
    set chip($addr.trimblanket) B2B2
    set chip($addr.trimvalid) 1

    Standard $addr $fmain

    #add power
    set fstand $chip($addr.fstand)
    foreach g {power} {
        label $fstand.l$g -text $g
        switch $g {
            power   {checkbutton $fstand.v$g -variable chip($addr.$g) -bg white}
            default {label $fstand.v$g -text $chip($addr.$g) -bg white}
        }
        grid $fstand.l$g $fstand.v$g
        grid $fstand.l$g -sticky e
        grid $fstand.v$g -sticky w
    }

    Alarm10 $addr $fmain
    Temperatures $addr $fmain
}

proc Setup28 { addr fmain } {
    global chip
    # the only difference (externally) is fasttemp, which we'll make indentical
    set chip($addr.fasttemp) 0
    Setup10 $addr $fmain
    set chip($addr.fasttemp) $chip($addr.temperature)
}

proc AlarmCheck10 {varName index op} {
    global chip
    regexp {(.*?)\.(.*?)} $index match addr temp
    set alarm false
    if { [info exist chip($addr.fasttemp)] } { set chip($addr.fasttemp) $chip($addr.temperature) }
    if { $chip($addr.temphigh) < $chip($addr.temperature) } {
        set alarm true
    } elseif { $chip($addr.templow) > $chip($addr.temperature) } {
        set alarm true
    }
    if { $chip($addr.alarm) != $alarm } {
        set chip($addr.alarm) $alarm
        if { $alarm } {
            $chip($addr.alarmcolor) config -bg red
        } else {
            $chip($addr.alarmcolor) config -bg green
        }
    }
}

proc Alarm10 { addr fram } {
    global chip
    set chip($addr.alarmcolor) [labelframe $fram.alarm -text "ALARM" -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg green]
    pack $fram.alarm -side top -fill x
    label $fram.alarm.state -textvariable chip($addr.alarm) -bg white
    pack $fram.alarm.state
}
    
proc Temperatures { addr fram } {
    global chip
    global color

    set vars [list temphigh temperature templow]
    if { [info exist chip($addr.fasttemp)] } { lappend vars fasttemp }

    foreach f $vars {
        switch $f {
            temphigh    -
            templow     { set stat "disabled" }
            default     { set stat "normal" }
        }

        labelframe $fram.$f -text $f -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66
        pack $fram.$f -side top -fill x
        scale $fram.$f.scale -variable chip($addr.$f) -orient horizontal -from -40 -to 125 -fg white -bg $color("$f") -state $stat
        pack $fram.$f.scale -side left -fill x -expand true
        spinbox $fram.$f.spin -textvariable chip($addr.$f) -width 3 -from -40 -to 125 -state $stat
        pack $fram.$f.spin -side right
        trace variable chip($addr.$f) w AlarmCheck10
    }
    if { [info exist chip($addr.fasttemp)] } {
        $fram.fasttemp.scale configure -variable chip($addr.temperature)
        $fram.fasttemp.spin configure -textvariable chip($addr.temperature)
    }
}

###########################################################
########## Simulant! Do it  ###############################
###########################################################
