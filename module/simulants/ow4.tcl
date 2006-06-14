###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

###########################################################
########## Server Process tcp handler #####################
###########################################################

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
    puts "Accept $sock from $addr port $port\n"
    fconfigure $sock -buffering full -encoding binary -blocking 0
    set serve($sock.string) ""
    set serve($sock.payload) 0
    fileevent $sock readable [list ServerProcess $sock]
}

proc CloseSock { sock } {
    global serve
    close $sock
    foreach x {version payload type sg size offset} {
        unset serve($sock.$x)
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
    puts $len
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
                $serve(text) insert end "  Read [string range $serve($sock.string) 24 end]\n" read
                set resp [ServerRead $sock]
            }
        3   { 
                $serve(text) insert end "  Write [string range $serve($sock.string) 24 [expr $len - $serve($sock.size)] ]\n" read
                set resp [ServerWrite $sock]
            }
        4   { 
                $serve(text) insert end "  Dir [string range $serve($sock.string) 24 end]\n" read
                set resp [ServerDir $sock]
            }
        5   { 
                $serve(text) insert end "  Size\n" read
                set resp [ServerNone $sock]
            }
        6   { 
                $serve(text) insert end "  Present [string range $serve($sock.string) 24 end]\n" read
                set resp [ServerPresent $sock]
            }
        default   { 
                $serve(text) insert end "  Unrecognized\n" read
                set resp [ServerNone $sock]
            }
    }
    set ret [lindex $resp 0]
    set size [lindex $resp 1]
    set val [lindex $resp 2]
    $serve(text) insert end "   Respond ret=$ret val=$val\n" write
    puts -nonewline $sock  [binary format {IIIIII} $serve($sock.version) [string length $val] $ret $serve($sock.sg) $size $serve($sock.offset)]
    flush $sock
    puts Done!
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
    return [list 0 0 ""]
}

proc ServerRead { sock } {
    return [list 0 0 ""]
}

proc ServerWrite { sock } {
    return [list 0 0 ""]
}

proc ServerDir { sock } {
    return [list 0 0 ""]
}

proc ServerPresent { sock } {
    return [list 0 0 ""]
}

proc ParseName { sock } {
    
