#!/bin/sh
# the next line restarts using tclsh \
exec wish owsim.tcl "$@"
# $Id$
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
    wm title . "OWSERVER simulator -- port [lindex [fconfigure $serve(socket) -sockname] 2]"
}

proc ServerAccept { sock addr port } {
    global serve
    $serve(text) insert end " Accept $sock from $addr port $port\n" accept
    $serve(text) see end
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
    $serve(text) see end
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
        #puts "Got $serve($sock.version) len=$len"
        #foreach x {version payload type sg size offset} {
        #    puts "\t$x = $serve($sock.$x)"
        #}
    }
    #already in payload portion
    if { $len < [expr $serve($sock.payload) + 24 ] } {
        #do nothing -- reloop
        return
    }
    Handler $sock
    flush $sock
    CloseSock $sock
}

