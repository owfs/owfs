#!/usr/bin/wish8.4

# A Notebook widget for Tcl/Tk
# $Revision$
#
# Copyright (C) 1996,1997,1998 D. Richard Hipp
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA  02111-1307, USA.
#
# Author contact information:
#   drh@acm.org
#   http://www.hwaci.com/drh/


#
# Create a new notebook widget
#
proc Notebook:create {w args} {
  global Notebook
  set Notebook($w,width) 400
  set Notebook($w,height) 300
  set Notebook($w,pages) {}
  set Notebook($w,top) 0
  set Notebook($w,pad) 5
  set Notebook($w,fg,on) black
  set Notebook($w,fg,off) grey50
  canvas $w -bd 0 -highlightthickness 0 -takefocus 0
  set Notebook($w,bg) [$w cget -bg]
  bind $w <1> "Notebook:click $w %x %y"
  bind $w <Configure> "Notebook:scheduleExpand $w"
  eval Notebook:config $w $args
}

#
# Change configuration options for the notebook widget
#
proc Notebook:config {w args} {
  global Notebook
  foreach {tag value} $args {
    switch -- $tag {
      -width {
        set Notebook($w,width) $value
      }
      -height {
        set Notebook($w,height) $value
      }
      -pages {
        set Notebook($w,pages) $value
      }
      -pad {
        set Notebook($w,pad) $value
      }
      -bg {
        set Notebook($w,bg) $value
      }
      -fg {
        set Notebook($w,fg,on) $value
      }
      -disabledforeground {
        set Notebook($w,fg,off) $value
      }
    }
  }

  #
  # After getting new configuration values, reconstruct the widget
  #
  $w delete all
  set Notebook($w,x1) $Notebook($w,pad)
  set Notebook($w,x2) [expr $Notebook($w,x1)+2]
  set Notebook($w,x3) [expr $Notebook($w,x2)+$Notebook($w,width)]
  set Notebook($w,x4) [expr $Notebook($w,x3)+2]
  set Notebook($w,y1) [expr $Notebook($w,pad)+2]
  set Notebook($w,y2) [expr $Notebook($w,y1)+2]
  set Notebook($w,y5) [expr $Notebook($w,y1)+30]
  set Notebook($w,y6) [expr $Notebook($w,y5)+2]
  set Notebook($w,y3) [expr $Notebook($w,y6)+$Notebook($w,height)]
  set Notebook($w,y4) [expr $Notebook($w,y3)+2]
  set x $Notebook($w,x1)
  set cnt 0
  set y7 [expr $Notebook($w,y1)+10]
  foreach p $Notebook($w,pages) {
    set Notebook($w,p$cnt,x5) $x
    set id [$w create text 0 0 -text $p -anchor nw -tags "p$cnt t$cnt"]
    set bbox [$w bbox $id]
    set width [lindex $bbox 2]
    $w move $id [expr $x+10] $y7
    $w create line \
       $x $Notebook($w,y5)\
       $x $Notebook($w,y2) \
       [expr $x+2] $Notebook($w,y1) \
       [expr $x+$width+16] $Notebook($w,y1) \
       -width 2 -fill white -tags p$cnt
    $w create line \
       [expr $x+$width+16] $Notebook($w,y1) \
       [expr $x+$width+18] $Notebook($w,y2) \
       [expr $x+$width+18] $Notebook($w,y5) \
       -width 2 -fill black -tags p$cnt
    set x [expr $x+$width+20]
    set Notebook($w,p$cnt,x6) [expr $x-2]
    if {![winfo exists $w.f$cnt]} {
      frame $w.f$cnt -bd 0
    }
    $w.f$cnt config -bg $Notebook($w,bg)
    place $w.f$cnt -x $Notebook($w,x2) -y $Notebook($w,y6) \
      -width $Notebook($w,width) -height $Notebook($w,height)
    incr cnt
  }
  $w create line \
     $Notebook($w,x1) [expr $Notebook($w,y5)-2] \
     $Notebook($w,x1) $Notebook($w,y3) \
     -width 2 -fill white
  $w create line \
     $Notebook($w,x1) $Notebook($w,y3) \
     $Notebook($w,x2) $Notebook($w,y4) \
     $Notebook($w,x3) $Notebook($w,y4) \
     $Notebook($w,x4) $Notebook($w,y3) \
     $Notebook($w,x4) $Notebook($w,y6) \
     $Notebook($w,x3) $Notebook($w,y5) \
     -width 2 -fill black
  $w config -width [expr $Notebook($w,x4)+$Notebook($w,pad)] \
            -height [expr $Notebook($w,y4)+$Notebook($w,pad)] \
            -bg $Notebook($w,bg)
  set top $Notebook($w,top)
  set Notebook($w,top) -1
  Notebook:raise.page $w $top
}

#
# This routine is called whenever the mouse-button is pressed over
# the notebook.  It determines if any page should be raised and raises
# that page.
#
proc Notebook:click {w x y} {
  global Notebook
  if {$y<$Notebook($w,y1) || $y>$Notebook($w,y6)} return
  set N [llength $Notebook($w,pages)]
  for {set i 0} {$i<$N} {incr i} {
    if {$x>=$Notebook($w,p$i,x5) && $x<=$Notebook($w,p$i,x6)} {
      Notebook:raise.page $w $i
      break
    }
  }
}

#
# For internal use only.  This procedure raised the n-th page of
# the notebook
#
proc Notebook:raise.page {w n} {
  global Notebook
  if {$n<0 || $n>=[llength $Notebook($w,pages)]} return
  set top $Notebook($w,top)
  if {$top>=0 && $top<[llength $Notebook($w,pages)]} {
    $w move p$top 0 2
  }
  $w move p$n 0 -2
  $w delete topline
  if {$n>0} {
    $w create line \
       $Notebook($w,x1) $Notebook($w,y6) \
       $Notebook($w,x2) $Notebook($w,y5) \
       $Notebook($w,p$n,x5) $Notebook($w,y5) \
       $Notebook($w,p$n,x5) [expr $Notebook($w,y5)-2] \
       -width 2 -fill white -tags topline
  }
  $w create line \
    $Notebook($w,p$n,x6) [expr $Notebook($w,y5)-2] \
    $Notebook($w,p$n,x6) $Notebook($w,y5) \
    -width 2 -fill white -tags topline
  $w create line \
    $Notebook($w,p$n,x6) $Notebook($w,y5) \
    $Notebook($w,x3) $Notebook($w,y5) \
    -width 2 -fill white -tags topline
  set Notebook($w,top) $n
  raise $w.f$n
}

#
# Change the page-specific configuration options for the notebook
#
proc Notebook:pageconfig {w name args} {
  global Notebook
  set i [lsearch $Notebook($w,pages) $name]
  if {$i<0} return
  foreach {tag value} $args {
    switch -- $tag {
      -state {
        if {"$value"=="disabled"} {
          $w itemconfig t$i -fg $Notebook($w,fg,off)
        } else {
          $w itemconfig t$i -fg $Notebook($w,fg,on)
        }
      }
      -onexit {
        set Notebook($w,p$i,onexit) $value
      }
    }
  }
}

#
# This procedure raises a notebook page given its name.  But first
# we check the "onexit" procedure for the current page (if any) and
# if it returns false, we don't allow the raise to proceed.
#
proc Notebook:raise {w name} {
  global Notebook
  set i [lsearch $Notebook($w,pages) $name]
  if {$i<0} return
  if {[info exists Notebook($w,p$i,onexit)]} {
    set onexit $Notebook($w,p$i,onexit)
    if {"$onexit"!="" && [eval uplevel #0 $onexit]!=0} {
      Notebook:raise.page $w $i
    }
  } else {
    Notebook:raise.page $w $i
  }
}

#
# Return the frame associated with a given page of the notebook.
#
proc Notebook:frame {w name} {
  global Notebook
  set i [lsearch $Notebook($w,pages) $name]
  if {$i>=0} {
    return $w.f$i
  } else {
    return {}
  }
}

#
# Try to resize the notebook to the next time we become idle.
#
proc Notebook:scheduleExpand w {
  global Notebook
  if {[info exists Notebook($w,expand)]} return
  set Notebook($w,expand) 1
  after idle "Notebook:expand $w"
}

#
# Resize the notebook to fit inside its containing widget.
#
proc Notebook:expand w {
  global Notebook
  set wi [expr [winfo width $w]-($Notebook($w,pad)*2+4)]
  set hi [expr [winfo height $w]-($Notebook($w,pad)*2+36)]
  Notebook:config $w -width $wi -height $hi
  catch {unset Notebook($w,expand)}
}



###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

proc Snlist { family } {
    set sn [list $family]
    foreach i { 1 2 3 4 5 6 } {
        lappend sn [expr int(rand()*256) ]
    }
    return $sn
} 

proc Crc8 { snlist } {
    global crc8table
    set crc 0
    foreach c $snlist {
        set crc [ lindex $crc8table [expr $crc ^ $c] ]
    }
    return $crc
}    

proc SetAddress {family} {
    global chip
    set sn [Snlist $family]
    lappend sn [Crc8 $sn]
    foreach x $sn {
        append addr [format "%02X" $x]
    }
    set chip($addr.address) $addr
    set chip($addr.family)    [string range $addr  0  1]
    set chip($addr.id)        [string range $addr  2 13]
    set chip($addr.crc8)      [string range $addr 14 15]
    set chip($addr.r_id)      [string range $addr) 12 13][string range $addr 10 11][string range $addr 8 9][string range $addr 6 7][string range $addr 4 5][string range $addr 2 3]
    set chip($addr.r_address) $chip($addr.crc8)$chip($addr.r_id)$chip($addr.family)
    set chip($addr.present)   1
    return $addr
}

proc AlarmCheck {varName index op} {
    global chip
    puts $varName
    puts $index
    regexp {(.*?)\.(.*?)} $index match addr temp
    puts $addr
    set alarm false
    if { $chip($addr.hightemp) < $chip($addr.temperature) } {
        set alarm true
    } elseif { $chip($addr.lowtemp) > $chip($addr.temperature) } {
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

proc Standard { addr fram } {
    global chip
    set fstand [frame $fram.s -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66]
    pack $fstand -side top -fill x
    foreach g { type family address id crc8 r_address r_id present } {
        label $fstand.l$g -text $g
        label $fstand.v$g -text $chip($addr.$g) -bg white
        grid $fstand.l$g $fstand.v$g
        grid $fstand.l$g -sticky e
        grid $fstand.v$g -sticky w
    }
}

proc Alarm { addr fram } {
    global chip
    set chip($addr.alarmcolor) [labelframe $fram.alarm -text "ALARM" -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg green]
    pack $fram.alarm -side top -fill x
    label $fram.alarm.state -textvariable chip($addr.alarm) -bg white
    pack $fram.alarm.state
}
    
proc Temperatures { addr fram } {
    global chip
    global color

    foreach f {hightemp temperature lowtemp} {
        labelframe $fram.$f -text $f -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66
        pack $fram.$f -side top -fill x
        scale $fram.$f.scale -variable chip($addr.$f) -orient horizontal -from -40 -to 125 -fg white -bg $color("$f") -state disabled
        pack $fram.$f.scale -side right -fill x -expand true
        spinbox $fram.$f.spin -textvariable chip($addr.$f) -width 5 -from -40 -to 125 -state disabled
        pack $fram.$f.spin -side right
        trace variable chip($addr.$f) w AlarmCheck
    }
    $fram.temperature.scale config -state normal
    $fram.temperature.spin config -state normal
}

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}

proc setupGlobals { } {
    global color
    set color("hightemp")    #CC3300
    set color("temperature") #666666
    set color("lowtemp")     #6666FF

    global crc8table
    set crc8table [list \
        0 94 188 226 97 63 221 131 194 156 126 32 163 253 31 65 \
        157 195 33 127 252 162 64 30 95 1 227 189 62 96 130 220 \
        35 125 159 193 66 28 254 160 225 191 93 3 128 222 60 98 \
        190 224 2 92 223 129 99 61 124 34 192 158 29 67 161 255 \
        70 24 250 164 39 121 155 197 132 218 56 102 229 187 89 7 \
        219 133 103 57 186 228 6 88 25 71 165 251 120 38 196 154 \
        101 59 217 135 4 90 184 230 167 249 27 69 198 152 122 36 \
        248 166 68 26 153 199 37 123 58 100 134 216 91 5 231 185 \
        140 210 48 110 237 179 81 15 78 16 242 172 47 113 147 205 \
        17 79 173 243 112 46 204 146 211 141 111 49 178 236 14 80 \
        175 241 19 77 206 144 114 44 109 51 209 143 12 82 176 238 \
        50 108 142 208 83 13 239 177 240 174 76 18 145 207 45 115 \
        202 148 118 40 171 245 23 73 8 86 180 234 105 55 213 139 \
        87 9 235 181 54 104 138 212 149 203 41 119 244 170 72 22 \
        233 183 85 11 136 214 52 106 43 117 151 201 74 20 246 168 \
        116 42 200 150 21 75 169 247 182 232 10 84 215 137 107 53 \
    ]
}

proc Setup10 { fmain } {
    global chip
    set family 0x10

    set addr [SetAddress $family]
    set chip($addr.type)      DS18B20
    set chip($addr.hightemp) 60
    set chip($addr.temperature) 16
    set chip($addr.lowtemp) 0
    set chip($addr.alarm) false

    Standard     $addr $fmain
    Alarm        $addr $fmain
    Temperatures $addr $fmain
}

setupGlobals
set devices [list One Two Three Four Five]
foreach 

Notebook:create .n -pages $devices -pad 20
pack .n -fill both -expand 1
set w [Notebook:frame .n One]
label $w.l -text "Hello.\nThis is page one"
pack $w.l -side top -padx 10 -pady 50
set w [Notebook:frame .n Two]
text $w.t -font fixed -yscrollcommand "$w.sb set" -width 40
$w.t insert end "This is a text widget.  Type in it, if you want\n"
pack $w.t -side left -fill both -expand 1
scrollbar $w.sb -orient vertical -command "$w.t yview"
pack $w.sb -side left -fill y
set w [Notebook:frame .n Three]
set p3 red
frame $w.f
pack $w.f -padx 30 -pady 30
foreach c {red orange yellow green blue violet} {
  radiobutton $w.f.$c -fg $c -text $c -variable p3 -value $c -anchor w
  pack $w.f.$c -side top -fill x
}
set w [Notebook:frame .n Four]
frame $w.f
pack $w.f -padx 30 -pady 30
button $w.f.b -text {Goto} -command [format {
  set i [%s cursel]
  if {[string length $i]>0} {
    Notebook:raise .n [%s get $i]
  }
} $w.f.lb $w.f.lb]
pack $w.f.b -side bottom -expand 1 -pady 5
listbox $w.f.lb -yscrollcommand "$w.f.sb set"
scrollbar $w.f.sb -orient vertical -command "$w.f.lb yview"
pack $w.f.lb -side left -expand 1 -fill both
pack $w.f.sb -side left -fill y
$w.f.lb insert end One Two Three Four Five
set w [Notebook:frame .n Five]
button $w.b -text Exit -command exit
pack $w.b -side top -expand 1
#Setup10 $fmain

#set fmain [frame .x]
#pack $fmain -fill both
