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
    global status
    set serve(socket) [socket -server ServerAccept $serve(port)]
    $server insert end "Listen $serve(port) as $serve(socket)\n"
}

proc ServerAccept { sock addr port } {
    global status
    $server insert end "Accept $sock from $addr port $port\n"
    fconfigure $sock -buffering none -translation binary
    fileevent $sock readable [list ServerProcess $sock]
}

