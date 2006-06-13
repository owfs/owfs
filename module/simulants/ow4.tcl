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
    $serve(text) insert end "Accept $sock from $addr port $port\n" accept
    puts "Accept $sock from $addr port $port\n"
    fconfigure $sock -buffering full -encoding binary -blocking 0
    set serve($sock.string) ""
    set serve($sock.payload) 0
    fileevent $sock readable [list ServerProcess $sock]
}

#/* Server (Socket-based) interface */
#enum msg_classification {
#    msg_error,
#    msg_nop,
#    msg_read,
#    msg_write,
#    msg_dir,
#    msg_size, // No longer used, leave here to compatibility
#    msg_presence,
#} ;
#/* message to owserver */
#struct server_msg {
#    int32_t version ;
#    int32_t payload ;
#    int32_t type ;
#    int32_t sg ;
#    int32_t size ;
#    int32_t offset ;
#} ;

#/* message to client */
#struct client_msg {
#    int32_t version ;
#    int32_t payload ;
#    int32_t ret ;
#    int32_t sg ;
#    int32_t size ;
#    int32_t offset ;
#} ;


proc CloseSock { sock } {
    global serve
    close $sock
    unset serve($sock.string)
    unset serve($sock.version)
    unset serve($sock.payload)
    unset serve($sock.type)
    unset serve($sock.sg)
    unset serve($sock.size)
    unset serve($sock.offset)
    $serve(text) insert end "Accept $sock closed\n" accept
}

proc ServerProcess { sock } {
    global serve
    # read 6 "word" header
    append serve($sock.string) [read $sock]
    set len [string length $serve($sock.string)]
    if { $len < 24 } {
        #do nothing -- reloop
        return
    } elseif { $serve($sock.payload) == 0 } {
        # at least header is in
        binary scan $serve($sock.string) {I6} serve($sock.version) serve($sock.payload) serve($sock.type) serve($sock.sg) serve($sock.size) serve($sock.offset)
        puts "Got $serve($sock.version) $serve($sock.payload) $serve($sock.type) $serve($sock.sg) $serve($sock.size) $serve($sock.offset)"
    }
    #already in payload portion
    if { $len < [expr $serve($sock.payload) + 24 ] } {
        #do nothing -- reloop
        return
    }
}
