#!/usr/bin/wish8.4

proc Snlist { family } {
    set sn [list $family]
    foreach i { 1 2 3 4 5 6 } {
        lappend sn [expr int(rand()*256) ]
    }
    return $sn
} 

proc Crc8 { snlist } {
    set crc 0
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
    foreach c $snlist {
        set crc [ lindex $crc8table [expr $crc ^ $c] ]
    }
    return $crc
}    

proc SetAddress {cp family} {
    upvar $cp chip
    set sn [Snlist $family]
    lappend sn [Crc8 $sn]
    foreach x $sn {
        append chip("address") [format "%02X" $x]
    }
    set chip("family")    [string range $chip("address")  0  1]
    set chip("id")        [string range $chip("address")  2 13]
    set chip("crc8")      [string range $chip("address") 14 15]
    set chip("r_id")      [string range $chip("id") 10 11][string range $chip("id") 8 9][string range $chip("id") 6 7][string range $chip("id") 4 5][string range $chip("id") 2 3][string range $chip("id") 0 1]
    set chip("r_address") $chip("crc8")$chip("r_id")$chip("family")
    set chip("present")   1
}

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

set fmain [frame .x]
pack $fmain -fill both


proc Standard { cp family fram } {
    upvar $cp chip
    set fstand [frame $fram.s -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66]
    pack $fstand -side top -fill x
    foreach g { type family address id crc8 r_address r_id present } {
        label $fstand.l$g -text $g
        label $fstand.v$g -text $chip("$g") -bg white
        grid $fstand.l$g $fstand.v$g
        grid $fstand.l$g -sticky e
        grid $fstand.v$g -sticky w
    }
}

proc Alarm { cp fram } {
    upvar $cp chip
    [labelframe $fram.alarm -text ALARM -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg green]
    pack $fram.alarm -side top -fill x
    label $fram.alarm.state -textvariable alarmvar -bg white
    pack $fram.alarm.state
}
    
proc Temperatures { cp fram } {
    upvar $cp chip
    set color("hightemp")    #CC3300
    set color("temperature") #666666
    set color("lowtemp")     #6666FF

    foreach f {hightemp temperature lowtemp} {
        labelframe $fram.$f -text $f -labelanchor n -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #CCCC66
        pack $fram.$f -side top -fill x
        scale $fram.$f.scale -variable chip("$f") -orient horizontal -from -40 -to 125 -fg white -bg $color("$f") -state disabled
        pack $fram.$f.scale -side right -fill x -expand true
        spinbox $fram.$f.spin -textvariable chip("$f") -width 5 -from -40 -to 125 -state disabled
        pack $fram.$f.spin -side right
        trace variable chip("$f") w AlarmCheck
    }
    $fram.temperature.scale config -state normal
    $fram.temperature.spin config -state normal
}

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}

proc Setup10 { fmain } {
    set family 0x10
    set chip("type")      DS18B20
    set chip("hightemp") 60
    set chip("temperature") 16
    set chip("lowtemp") 0
    set chip("alarm") false

    SetAddress  chip $family
    Standard    chip $family $fmain    
    Alarm       chip $fmain
    Temeratures chip $fmain
}

Setup10 $fmain
