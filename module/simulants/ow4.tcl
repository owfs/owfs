###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Server Process tcp handler #####################
###########################################################

set serve(ENOENT) -2
set serve(ENODEV) -19
set serve(ENOTDIR) -20
set serve(EISDIR) -21
set serve(EINVAL) -22
set serve(ERANGE) -34
set serve(EIO) -5
set serve(EMSGSIZE) -90

proc SetupServer { } {
    global serve
    set serve(socket) [socket -server ServerAccept $serve(port)]
    $serve(text) insert end "Listen as requested: portname=$serve(port)\n" listen
    $serve(text) insert end [eval {format "As implemented: address=%s hostname=%s portname=%s\n"} [fconfigure $serve(socket) -sockname]] listen
    $serve(frame) configure -text "OWSERVER simulator -- port [lindex [fconfigure $serve(socket) -sockname] 2]"
}

proc ServerAccept { sock addr port } {
    global serve
    $serve(text) insert end " Accept $sock from $addr port $port\n" accept
    fconfigure $sock -buffering full -encoding binary -blocking 0
    set serve($sock.string) ""
    set serve($sock.payload) 0
    fileevent $sock readable [list ServerProcess $sock]
}

proc CloseSock { sock } {
    global serve
    close $sock
    foreach x {version payload type sg size offset} {
        if { [info exist serve($sock.$x)] } {
            unset serve($sock.$x)
        }
    }
    $serve(text) insert end " Accept $sock closed\n" accept
}

proc ServerProcess { sock } {
    global serve
    # test eof
    if { [eof $sock] } {
        CloseSock $sock
        return
    }
    # read what's waiting
    append serve($sock.string) [read $sock]
    set len [string length $serve($sock.string)]
    if { $len < 24 } {
        #do nothing -- reloop
        return
    } elseif { $serve($sock.payload) == 0 } {
        # at least header is in
        #/* message to owserver */
        #struct server_msg {
        #    int32_t version ;
        #    int32_t payload ;
        #    int32_t type ;
        #    int32_t sg ;
        #    int32_t size ;
        #    int32_t offset ;
        #} ;
        binary scan $serve($sock.string) {IIIIII} serve($sock.version) serve($sock.payload) serve($sock.type) serve($sock.sg) serve($sock.size) serve($sock.offset)
        puts "Got $serve($sock.version) len=$len"
        foreach x {version payload type sg size offset} {
            puts "\t$x = $serve($sock.$x)"
        }
    }
    #already in payload portion
    if { $len < [expr $serve($sock.payload) + 24 ] } {
        #do nothing -- reloop
        return
    }
    #enum msg_classification {
    #    msg_error,
    #    msg_nop,
    #    msg_read,
    #    msg_write,
    #    msg_dir,
    #    msg_size, // No longer used, leave here to compatibility
    #    msg_presence,
    #} ;
    switch $serve($sock.type) {
        0   { 
                $serve(text) insert end "  Error\n" read 
                set resp [ServerNone $sock]
            }
        1   { 
                $serve(text) insert end "  NOP\n" read
                set resp [ServerNone $sock]
            }
        2   { 
                $serve(text) insert end "  Read [string range $serve($sock.string) 24 end-1]\n" read
                set resp [ServerRead $sock]
            }
        3   { 
                $serve(text) insert end "  Write [string range $serve($sock.string) 24 [expr $len - $serve($sock.size) - 1] ]\n" read
                set resp [ServerWrite $sock]
            }
        4   { 
                $serve(text) insert end "  Dir [string range $serve($sock.string) 24 end-1]\n" read
                set resp [ServerDir $sock]
            }
        5   { 
                $serve(text) insert end "  Size\n" read
                set resp [ServerNone $sock]
            }
        6   { 
                $serve(text) insert end "  Present [string range $serve($sock.string) 24 end-1]\n" read
                set resp [ServerPresent $sock]
            }
        default   { 
                $serve(text) insert end "  Unrecognized\n" read
                set resp [ServerNone $sock]
            }
    }
    foreach {ret size offset val} $resp {break} 
    $serve(text) insert end "   Respond ret=$ret val=$val\n" write
    puts -nonewline $sock  [binary format IIIIIIa* $serve($sock.version) [string length $val] $ret $serve($sock.sg) $size $offset $val]
    flush $sock
    CloseSock $sock
}

#/* message to client */
#struct client_msg {
#    int32_t version ;
#    int32_t payload ;
#    int32_t ret ;
#    int32_t sg ;
#    int32_t size ;
#    int32_t offset ;
#} ;

proc ServerNone { sock } {
    return [list 0 0 0 ""]
}

proc ServerRead { sock } {
    global serve
    global chip
    foreach {ret typ alarm dev fil ext} [ParsePath [string range $serve($sock.string) 24 end] $sock] {break}
    # parse
    if { $ret != 0 } { return [list $ret 0 0 ""] }
    # is file?
    if { ![string equal $typ f] } { return [list $serve(EISDIR) 0 0 ""] }
    set addr $chip($dev)
    # make sure variable defined
    if { ![info exist chip($addr.$fil)] } { return [list $serve(ENOENT) 0 0 ""] }
    set v [string range $chip($addr.$fil) $serve($sock.offset) end]
    set vl [string length $v]
    # check return length
    if { $vl > $serve($sock.size) } { return [list $serve(EMSGSIZE) 0 0 ""] }
    # return value
    return [list $vl $vl $serve($sock.offset) $v]
}

proc ServerWrite { sock } {
    global serve
    global chip
    foreach {ret typ alarm dev fil ext} [ParsePath [string range $serve($sock.string) 24 end-[expr $serve($sock.size) + 1]] $sock] {break}
    # parse
    if { $ret != 0 } { return [list $ret 0 0 ""] }
    # is file?
    if { ![string equal $typ f] } { return [list $serve(EISDIR) 0 0 ""] }
    set addr $chip($dev)
    # make sure variable defined
    if { ![info exist chip($addr.$fil)] } { return [list $serve(ENOENT) 0 0 ""] }
    set v  [string range $serve($sock.string) end-[expr $serve($sock.size) - 1 ] end ]
    if { [catch {set chip($addr.$fil) $v}] } {
        set v [string trim $v \00 ]
        set chip($addr.$fil) $v
    }
    return [list  [string length $v] [string length $v] 0 ""]
}

# . error code
#   type r/d/f/s (root dev file subdir)
#   alarm 0/1
#   device name
#   filename
#   extension
proc ServerDir { sock } {
    global serve
    foreach {ret typ alarm dev fil ext} [ParsePath [string range $serve($sock.string) 24 end] $sock] {break}
    if { $ret != 0 } { return [list $ret 0 0 ""] }
    switch $typ {
        r       { return [RootDir $alarm $sock] }
        s       { return [SubDir $dev $fil $sock] }
        d       { return [DevDir $dev $sock] }
        f       -
        default { return [list $serve(ENOTDIR) 0 0 ""] }
    }
}

proc RootDir { alarm sock } {
    global serve
    global devname
    global chip
    set flags 0
    foreach d $devname {
        set addr $chip($d)
        # device "present"?
        if { [info exist chip($addr.present)] } {
            if { !$chip($addr.present) } {continue}
        }
        # in alarm state?
        if { $alarm } {
            if { ![info exist chip($addr.alarm)] } {continue}
            if { !$chip($addr.alarm) } {continue}
        } elseif { [info exist chip($chip($addr.family).flags)] } {
            # process flags
            set flags [expr $flags | $chip($chip($addr.family).flags)]
        }
        set e $d\00
        $serve(text) insert end "   Respond $d\n" write
        puts -nonewline $sock  [binary format {IIIIIIa*} $serve($sock.version) [string length $e] 0 $serve($sock.sg) [string length $e] 0 $e]
    }
    return [list 0 0 $flags ""]
}

proc DevDir { dev sock } {
    global serve
    global chip
    foreach t [list "" $chip($chip($dev).family)] {
        foreach x [list $t.read $t.write] {
            if { [info exist chip($x)] } {
                foreach y $chip($x) {
                    lappend d [lindex [split $y "/"] 0]
                }
            }
        }
    }
    foreach x [lsort -dictionary -unique $d] {
        set e $dev/$x\00
        $serve(text) insert end "   Respond $dev/$x\n" write
        puts -nonewline $sock  [binary format {IIIIIIa*} $serve($sock.version) [string length $e] 0 $serve($sock.sg) [string length $e] 0 $e]
    }
    return [list 0 0 0 ""]
}

proc ServerPresent { sock } {
    global serve
    foreach {ret typ alarm dev fil ext} [ParsePath [string range $serve($sock.string) 24 end] $sock] {break}
    # return value
    return [list $ret 0 0 ""]
}

#returns list
# . error code
#   type r/d/f/s (root dev file subdir)
#   alarm 0/1
#   device name
#   filename
#   extension
proc ParsePath { path sock } {
    global serve
    global chip
    set path [string trimright $path \00]
    # remove uncached
    regsub -all -nocase {\</uncached\>} $path "/" path
    # remove "bus.0"
    regsub -nocase {\</bus.0\>} $path "/" path
    # flag and remove alarm
    set alarm [regexp -nocase {\</alarm\>} $path]
    regsub -all -nocase {\</alarm\>} $path "/" path
    # remove leading "/" and check for root directory
    set path [string range $path 1 end]
    if { [string length $path] == 0 } {return [list 0 r $alarm "" "" 0]}
    # tease out device name
    set pathlist [split $path "/"]
    # check if exists
    if { ![info exist chip([lindex $pathlist 0])] } {return [list $serve(ENOENT) d $alarm [lindex $pathlist 0] "" 0]}
    # check if present
    set dev $chip([lindex $pathlist 0])
    if { !$chip($dev.present) } {return [list $serve(ENOENT) d $alarm [lindex $pathlist 0] "" 0]}
    # just a device?
    if { [llength $pathlist] == 1 } {return [list 0 d $alarm $dev "" 0]}
    # tease out file and extension (extension=0 of none)
    foreach {file ext} [split [join [lrange $pathlist 1 end] "/"].0 "."] {break}
    # make sure file exists in read or write lists
    foreach x [list "" $chip($dev.family)] {
        foreach y [list $x.read $x.write] {
            if { [info exist chip($y)] } {
                if { [lsearch $chip($y) $file] > -1 } {
                    return [list 0 f $alarm $dev $file $ext]
                }
            }
        }
    }
    # found vald file
    return [list $serve(ENOENT) f $alarm $dev $file $ext]
}
