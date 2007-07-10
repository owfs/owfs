#!/bin/sh
# the next line restarts using wish \
exec wish "$0" -- "$@"

# $Id$

# Global: Ports() loose tap server
# -- port number of this program (tap) and real owserver (server). loose is stray garbage

set SocketVars [list string version type payload size sg offset tokenlength totallength state ]

# Global: serve ENOENT EINVAL,... error codes
#         serve(port) -- same as Ports(tap)
#         serve(socket) -- listing socket
#         serve($sock.string) -- message to this point
#         serve($sock. version size payload sg offset type tokenlength ) -- message parsed parts
#         serve($sock.version size 

proc Main { argv } {
    global Ports

    ArgumentProcess $argv
    wm title . "OWSERVER protocol tap -- port $Ports(tap) to $Ports(server)"
    
    CircBufferSetup 50
    
    DisplaySetup

    if { $Ports(server) == 0 } {
        ErrorMessage "No owserver port"
    }

    if { $Ports(tap) } {
        SetupTap
    } else {
        ErrorMessage "No tap port"
    }
}

# Command line processing
# looks at command line arguments
# two options "p" and "s" for (this) tap's _p_ort, and the target ow_s_erver port
# Can handle command lines like
# -p 3000 -s 3001
# -p3000 -s 3001
# etc
proc ArgumentProcess { arg } {
    global Ports
    set mode "loose"
    # "Clear" ports
    set Ports(tap) 0
    set Ports(server) 0
    foreach a $arg {
        if { [regexp {p} $a] } { set mode "tap" }
        if { [regexp {s} $a] } { set mode "server" }
        set pp 0
        regexp {[0-9]{1,}} $a pp
        if { $pp } { set Ports($mode) $pp }
    }
}

# Accept from client (our "server" portion)
proc SetupTap { } {
    global serve
    global Ports
    StatusMessage "Attempting to open surrogate server on $Ports(tap)"
    if {[catch {socket -server TapAccept $Ports(tap)} result] } {
        ErrorMessage $result
    }
    set serve(socket) $result 
    StatusMessage [eval {format "Success. Tap server address=%s (%s:%s) "} [fconfigure $serve(socket) -sockname]]
}

proc TapAccept { sock addr port } {
    global serve
    set serve($sock.state) "Open client"
    while {1} {
#    puts "----------- $serve($sock.state) ---------------"
        switch $serve($sock.state) {
        "Open client" {
                StatusMessage "Reading client request from $addr port $port" 0
                set current [CircBufferAllocate]
                fconfigure $sock -buffering full -encoding binary -blocking 0
                ClearTap $sock
                TapSetup $sock
                set serve($sock.state) "Read client"
            }
        "Read client" { 
                fileevent $sock readable [list TapProcess $sock]
                vwait serve($sock.state)
            }
        "Log client input" { 
                fileevent $sock readable {}
                StatusMessage "Success reading client request" 0
                CircBufferEntryRequest $current [format "%s %d bytes" [MessageType $serve($sock.type)] $serve($sock.payload)] 
#                puts "XXXXXXXXXXXXX CLIENT MESSAGE XXXXXXXXXXXXXXXXXXXXXXXXX"
#                ShowMessage $sock
                set serve($sock.state) "Open server"
            }
        "Client early end" {
                StatusMessage "FAILURE reading client request"
                CircBufferEntryRequest $current "network read error"
                CloseTap $sock
                return
            }
        "Open server" {
                global Ports
                StatusMessage "Attempting to open connection to OWSERVER port $Ports(server)" 0
                if {[catch {socket localhost $Ports(server)} result] } {
                    StatusMessage "OWSERVER error: $result"
                    CloseTap $sock
                    return
                }
                set relay $result
                set serve($sock.state) "Send to server"
            }
        "Send to server" { 
                StatusMessage "Sending client request to OWSERVER" 0
                fconfigure $relay -buffering full -encoding binary -blocking 0
                ClearTap $relay
                TapSetup $relay
                puts -nonewline $relay  $serve($sock.string)
                flush $relay
                set serve($sock.state) "Read from server"
            }
        "Read from server" {
                StatusMessage "Reading OWSERVER response" 0
                fileevent $relay readable [list RelayProcess $relay $sock]
                vwait serve($sock.state)
            }
        "Server early end" {
                StatusMessage "FAILURE reading OWSERVER response" 0
                CircBufferEntryResponse $current "network read error"
                ShowMessage $relay
                CloseTap $relay
                CloseTap $sock
                return
            }
        "Log server input" {
                StatusMessage "Success reading OWSERVER response" 0
                fileevent $relay readable {}
                CircBufferEntryResponse $current [format "Return value %d" $serve($relay.type)]
#                puts "XXXXXXXXXXXXX SERVER MESSAGE XXXXXXXXXXXXXXXXXXXXXXXXX"
#                ShowMessage $relay
                set serve($sock.state) "Send to client"
            }
        "Send to client" { 
                StatusMessage "Sending OWSERVER response to client" 0
                puts -nonewline $sock  $serve($relay.string)
                CloseTap $relay
                set serve($sock.state) "Done"
            }
        "Done"  {
                StatusMessage "Ready" 0
                CloseTap $sock
                return
            }                
        }
    }
}

proc TapSetup { sock } {
    global serve
    set serve($sock.string) ""
    set serve($sock.totallength) 0
}

proc ClearTap { sock } {
    global serve
    global SocketVars
    foreach x $SocketVars {
        if { [info exist serve($sock.$x)] } {
            unset serve($sock.$x)
        }
    }
}

proc CloseTap { sock } {
    global serve
    close $sock
    ClearTap $sock
}

proc TapProcess { sock } {
    global serve
    set read_value [ReadProcess $sock]
    switch $read_value {
        2  {
            set serve($sock.state) "Client early end"
            }
        0   {
            return
            }
        1   {
            set serve($sock.state) "Log client input"
            }
    }
}   

proc ReadProcess { sock } {
    global serve
    # test eof
    if { [eof $sock] } {
        return 2
    }
    set current_length [string length $serve($sock.string)]
    # read what's waiting
    append serve($sock.string) [read $sock]
    set len [string length $serve($sock.string)]
    if { $len < 24 } {
        #do nothing -- reloop
        return 0
    } elseif { $serve($sock.totallength) == 0 } {
        # at least header is in
        ClientHeaderParse $sock
    }
    #already in payload (and token) portion
    if { $len < $serve($sock.totallength) } {
        #do nothing -- reloop
        return 0
    }
    # Fully parsed
    set new_length [string length $serve($sock.string)]
    return 1
}

proc RelayProcess { relay sock } {
    global serve
    set read_value [ReadProcess $relay]
    switch $read_value {
        2  {
            set serve($sock.state) "Server early end"
            }
        0   {
            return
            }
        1   {
            set serve($sock.state) "Log server input"
            }
    }
}   

proc ClientHeaderParse { sock } {
    global serve
    binary scan $serve($sock.string) {IIIIII} serve($sock.version) serve($sock.payload) serve($sock.type) serve($sock.sg) serve($sock.size) serve($sock.offset)
    set serve($sock.tokenlength) [expr ( $serve($sock.version) & 0xFFFF) * 16 ]
    set serve($sock.totallength) [expr $serve($sock.tokenlength) + $serve($sock.payload) + 24 ]
}

proc ShowMessage { sock } {
    global serve
    global SocketVars
    foreach x $SocketVars {
        if { [info exist serve($sock.$x)] } {
            puts "\t$sock.$x = $serve($sock.$x)"
        }
    }
}

proc MessageType { num } {
    switch $num {
        0 {return "ERROR"}
        1 {return "NOP"}
        2 {return "READ"}
        3 {return "WRITE"}
        4 {return "DIR"}
        5 {return "SIZE"}
        6 {return "PRESENCE"}
        7 {return "DIRALL"}
        8 {return "GET"}
        default {return "Unknown"} 
    }
}

#Create a scrollable listbox containing color names. When a color is
# double-clicked, the label on the bottom will change to show the
# selected color name and will also change the background color

proc ScrollTogether { args } {
    eval {.log.request_list yview} $args 
    eval {.log.response_list yview} $args 
}

proc Left_ScrollByKey { args } {
    eval { .log.transaction_scroll set } $args
    .log.response_list yview moveto [lindex [.log.request_list yview] 0 ]
    .log.response_list activate [.log.request_list index active ]
}

proc Right_ScrollByKey { args } {
    eval { .log.transaction_scroll set } $args
    .log.request_list yview moveto [lindex [.log.response_list yview] 0 ]
    .log.request_list activate [.log.response_list index active ]
}

proc SelectionMade { widget } {
    set x [ $widget index active ]
    # do something with the selected line
}    


proc DisplaySetup { } {
    global circ_buffer
    # Top pane, tranaction logs
    frame .log -background red

    scrollbar .log.transaction_scroll -command [ list ScrollTogether ]
    label .log.request_title -text "Client request"
    label .log.respnse_title -text "Owserver response"
    listbox .log.request_list -width 40 -height 10 -selectmode single -yscroll [list Left_ScrollByKey ]
    listbox .log.response_list -width 40 -height 10 -selectmode single -yscroll [list Right_ScrollByKey]

    bind Listbox <ButtonRelease-1> {+ SelectionMade %W }
    bind Listbox <space> {+ SelectionMade %W }

    grid .log.request_title -row 0 -column 0 -sticky news
    grid .log.respnse_title -row 0 -column 1 -sticky news
    grid .log.request_list -row 1 -column 0 -sticky news
    grid .log.response_list -row 1 -column 1 -sticky news
    grid .log.transaction_scroll -row 1 -column 2 -sticky news

    pack .log -side top -fill x -expand true
    
    #bottom pane, status
    label .status -anchor w -width 80 -relief sunken -height 1 -textvariable current_status
    pack .status -side bottom -fill x

}

proc ErrorMessage { msg } {
    StatusMessage "Fatal error -- $msg"    
    tk_messageBox -title "Fatal error" -message $msg -type ok -icon error
    exit 1
}

proc StatusMessage { msg { priority 1 } } {
    global current_status
    set current_status $msg
}

proc CircBufferSetup { size } {
    global circ_buffer
    set circ_buffer(size) $size
    set circ_buffer(total) 0
    set circ_buffer(current) [expr $size - 1 ]
}

proc CircBufferAllocate { } {
    global circ_buffer
    set size $circ_buffer(size)
    incr circ_buffer(total)
    incr circ_buffer(current)
    set current $circ_buffer(current)
    if { $current == $size } {
        set circ_buffer(current) 0
        set current 0
    }
    set circ_buffer($current.request) " "
    set circ_buffer($current.response) " "
    set circ_buffer($current.total) $circ_buffer(total)
    if { $circ_buffer(total) > $size } {
        .log.request_list delete 0
        .log.response_list delete 0
    }
    .log.request_list insert end "x"
    .log.response_list insert end "x"
    return $current
}

proc CircBufferEntryRequest { current request } {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    set old_total $circ_buffer($current.total)
    # Still filling for the first time?
    if { $total < $size } {
        set index [expr $old_total - 1]
    } else {
        set index [ expr $size - $total + $old_total - 1 ]
    }
    if { $index > [expr $size - 1 ] } {
        set index 0
    }
    .log.request_list insert $index $request
    .log.request_list delete [expr $index + 1 ]
}    
     
proc CircBufferEntryResponse { current response } {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    set old_total $circ_buffer($current.total)
    # Still filling for the first time?
    if { $total < $size } {
        set index [expr $old_total - 1]
    } else {
        set index [ expr $size - $total + $old_total - 1 ]
    }
    if { $index > [expr $size - 1 ] } {
        set index 0
    }
    .log.response_list insert $index $response
    .log.response_list delete [expr $index + 1 ]
}    
     
Main $argv

