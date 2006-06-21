# $Id$
###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Simulant! Generic and utility functions ########
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

set standardProps { address crc8 family id locator present r_address r_id r_locator type }

# input a text hex pair (family code)
# generate a random seial number and return it
# fill fields in devlist, devname and chip
# set family-specific stuff 
proc SetAddress {famcode} {
    global chip
    global devlist
    global devname
    global standardProps
    
    # convert code to integer
    scan $famcode "%2x" family
    # Set random serial number with crc8
    set sn [Snlist $family]
    lappend sn [Crc8 $sn]
    # format the address
    foreach x $sn {
        append addr [format "%02X" $x]
    }
    # set separate parts for fields
    set chip($addr.address) $addr
    set chip($addr.family)    [string range $addr  0  1]
    set chip($addr.id)        [string range $addr  2 13]
    set chip($addr.crc8)      [string range $addr 14 15]
    set chip($addr.r_id)      [string range $addr) 12 13][string range $addr 10 11][string range $addr 8 9][string range $addr 6 7][string range $addr 4 5][string range $addr 2 3]
    set chip($addr.r_address) $chip($addr.crc8)$chip($addr.r_id)$chip($addr.family)
    #default present
    set chip($addr.present)   1
    # locator
    set chip($addr.locator) FFFFFFFFFFFFFFFF
    set chip($addr.r_locator) FFFFFFFFFFFFFFFF
    # directory-style name
    set chip($addr.name)      [ format {%s.%s} $chip($addr.family) $chip($addr.id) ]
    # backref
    #  f.i
    set chip($chip($addr.family).$chip($addr.id)) $addr
    #  fi
    set chip($chip($addr.family)$chip($addr.id)) $addr
    #  f.i.c
    set chip($chip($addr.family).$chip($addr.id).$chip($addr.crc8)) $addr
    #  fi.c
    set chip($chip($addr.family)$chip($addr.id).$chip($addr.crc8)) $addr
    #  f.ic
    set chip($chip($addr.family).$chip($addr.id)$chip($addr.crc8)) $addr
    #  fic
    set chip($chip($addr.family)$chip($addr.id)$chip($addr.crc8)) $addr
    # read and write properties
    set chip($addr.read)  $standardProps
    set chip($addr.write) {}
    # set family-specific -- type and function
    switch $famcode {
        10  { Setup10 $addr DS18S20}
        22  { Setup28 $addr DS1822 }
        28  { Setup28 $addr DS18B20}
        01  { Setup01 $addr DS2401 }
        default { Setup01 $addr "unknown" }
    }
    lappend devlist $addr
    lappend devname $chip($addr.name)
    set devname [lsort $devname]
    return $addr
}

# Globals
set color("temphigh")    #CC3300
set color("temperature") #666666
set color("fasttemp")    #777777
set color("templow")     #6666FF

# Globals
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

###########################################################
########## Simulant! Standard options (all devices ########
###########################################################

proc DisplayStandard { addr fram } {
    global chip
    global standardProps
    set fstand [frame $fram.s -relief ridge -borderwidth 3 -padx 2 -pady 2 -bg #CCCC66]
    set chip($addr.fstand) $fstand
    pack $fstand -side left -fill y
    foreach g $standardProps {
        label $fstand.l$g -text $g
        switch $g {
            present {checkbutton $fstand.v$g -variable chip($addr.$g) -bg white}
            default {label $fstand.v$g -text $chip($addr.$g) -bg white}
        }
        grid $fstand.l$g $fstand.v$g
        grid $fstand.l$g -sticky e
        grid $fstand.v$g -sticky w
    }

    set fextra [frame $fram.e -relief ridge -borderwidth 3 -padx 2 -pady 2 -bg #CCCC66]
    set chip($addr.fextra) $fextra
    pack $fextra -side left -fill both -expand true
}

