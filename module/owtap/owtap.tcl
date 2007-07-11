#!/bin/sh
# the next line restarts using wish \
exec wish "$0" -- "$@"

# $Id$

# Global: Ports() loose tap server
# -- port number of this program (tap) and real owserver (server). loose is stray garbage

set SocketVars {string version type payload size sg offset tokenlength totallength state}

set MessageList {ERROR NOP READ WRITE DIR SIZE PRESENCE DIRALL GET Unknown}
set MessageListPlus [lappend $MessageList Total]

# Global: serve ENOENT EINVAL,... error codes
#         serve(port) -- same as Ports(tap)
#         serve(socket) -- listing socket
#         serve($sock.string) -- message to this point
#         serve($sock. version size payload sg offset type tokenlength ) -- message parsed parts
#         serve($sock.version size 

#Main procedure. We actually start it at the end, to allow Proc-s to be defined first.
proc Main { argv } {
    global Ports

    ArgumentProcess $argv
    wm title . "OWSERVER protocol tap -- port $Ports(tap) to $Ports(server)"
    
    CircBufferSetup 50
    
    DisplaySetup

    if { $Ports(server) == 0 } {
        CommandLine
        ErrorMessage "No owserver port"
    }

    if { $Ports(tap) } {
        SetupTap
    } else {
        CommandLine
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

#Main loop. Called whenever the server (listen) port accepts a connection.
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

# Initialize array for client request
proc TapSetup { sock } {
    global serve
    set serve($sock.string) ""
    set serve($sock.totallength) 0
}

# Clear out client request array after a connection (frees memory)
proc ClearTap { sock } {
    global serve
    global SocketVars
    foreach x $SocketVars {
        if { [info exist serve($sock.$x)] } {
            unset serve($sock.$x)
        }
    }
}

# close client request socket
proc CloseTap { sock } {
    global serve
    close $sock
    ClearTap $sock
}

# Wrapper for processing -- either change a vwait var, or just return waiting for more network traffic
proc TapProcess { sock } {
    global serve
    set read_value [ReadProcess $sock]
    switch $read_value {
        2  { set serve($sock.state) "Client early end" }
        0  { return }
        1  { set serve($sock.state) "Log client input" }
    }
}   

# Process a oncomming owserver packet, adjusting size from header information
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
        HeaderParse $sock
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

# Wrapper for processing -- either change a vwait var, or just return waiting for more network traffic
proc RelayProcess { relay sock } {
    global serve
    set read_value [ReadProcess $relay]
    switch $read_value {
        2  { set serve($sock.state) "Server early end"}
        0  { return }
        1  { set serve($sock.state) "Log server input" }
    }
}   

# Parse Header information (24 bytes)
proc HeaderParse { sock } {
    global serve
    binary scan $serve($sock.string) {IIIIII} serve($sock.version) serve($sock.payload) serve($sock.type) serve($sock.sg) serve($sock.size) serve($sock.offset)
    set serve($sock.tokenlength) [expr ( $serve($sock.version) & 0xFFFF) * 16 ]
    set serve($sock.totallength) [expr $serve($sock.tokenlength) + $serve($sock.payload) + 24 ]
}

# Debugging routine -- show all the packet info
proc ShowMessage { sock } {
    global serve
    global SocketVars
    foreach x $SocketVars {
        if { [info exist serve($sock.$x)] } {
            puts "\t$sock.$x = $serve($sock.$x)"
        }
    }
}

# Limit num to type's range
proc MessageNunber { num } {
    global MessageList
    set len [expr [llength $MessageList] - 1]
puts "$num $len"
    if { $num > $len } { return $len }
    return $num
}

# Message type (owserver protocol type)
#  used for client only, same field
#  is used for "return" back from owserver
proc MessageType { num } {
    global MessageList
    return [lindex $MessageList $num]
}

# callback from scrollbar, moves each listbox field
proc ScrollTogether { args } {
    eval {.log.request_list yview} $args 
    eval {.log.response_list yview} $args 
}

# scroll other listbox and scrollbar
proc Left_ScrollByKey { args } {
    eval { .log.transaction_scroll set } $args
    .log.response_list yview moveto [lindex [.log.request_list yview] 0 ]
    .log.response_list activate [.log.request_list index active ]
}

# scroll other listbox and scrollbar
proc Right_ScrollByKey { args } {
    eval { .log.transaction_scroll set } $args
    .log.request_list yview moveto [lindex [.log.response_list yview] 0 ]
    .log.request_list activate [.log.response_list index active ]
}

# Selection from listbox
proc SelectionMade { widget } {
    set x [ $widget index active ]
    # do something with the selected line
}    

# create visual aspects of program
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

    SetupMenu
}

# Menu construction
proc SetupMenu { } {
#    toplevel . -menu .main_menu
     menu .main_menu -tearoff 0
    . config -menu .main_menu
     menu .main_menu.file -tearoff 0
     .main_menu add cascade -label File -menu .main_menu.file  -underline 0
         .main_menu.file add command -label "Log to File..." -underline 0 -command SaveLog -state disabled
         .main_menu.file add command -label "Stop logging" -underline 0 -command SaveAsLog -state disabled
         .main_menu.file add separator
         .main_menu.file add command -label Quit -underline 0 -command exit
# 
#     menu .main_menu.device -tearoff 1
#     .main_menu add cascade -label Devices -menu .main_menu.device  -underline 0
#         .main_menu.device add command -label "Save Log" -underline 0 -command SaveLog
#         .main_menu.device add command -label "Save Log As..." -underline 9 -command SaveAsLog
#         .main_menu.device add separator
#         .main_menu.device add command -label Quit -underline 0 -command exit

    menu .main_menu.help -tearoff 0
    .main_menu add cascade -label Help -menu .main_menu.help  -underline 0
        .main_menu.help add command -label "About OWTAP" -underline 0 -command About
        .main_menu.help add command -label "Command Line" -underline 0 -command CommandLine
        .main_menu.help add command -label "OWSERVER  Protocol" -underline 0 -command Protocol
}

# error routine -- popup and exit
proc ErrorMessage { msg } {
    StatusMessage "Fatal error -- $msg"    
    tk_messageBox -title "Fatal error" -message $msg -type ok -icon error
    exit 1
}

# status -- set status message
#   possibly store past messages
#   Use priority to test if should be stored
proc StatusMessage { msg { priority 1 } } {
    global current_status
    set current_status $msg
}

# Circular buffer for past connections
#   size is number of elements 
proc CircBufferSetup { size } {
    global circ_buffer
    set circ_buffer(size) $size
    set circ_buffer(total) 0
    set circ_buffer(current) [expr $size - 1 ]
}

# Save a spot for the coming connection
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

# place a new request packet
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
     
# place a new response packet
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

# initiqalize statistics
proc StatsSetup { } {
	global stats
	global MessageListPlus
	foreach x $MessageListPlus {
		set stats($x.tries) 0
		set stats($x.errors) 0
		set stats($x.rate) 0
		set stats($x.request_bytes) 0
		set stats($x.response_bytes) 0
	}
}

# Popup giving attribution
proc About { } {
    tk_messageBox -type ok -title {About owtap} -message {
Program: owtap    
Synopsis: owserver protocol inspector
    
Description: owtap is interposed 
  between owserver and client.
  
  The communication is logged and 
  shown on screen.
  
  All messages are transparently 
  forwarded  between client and server.
    
    
Author: Paul H Alfille <paul.alfille@gmail.com>
    
Copyright: July 2007 GPL 2.0 license
    
Website: http://www.owfs.org
    }
}
     
# Popup giving commandline
proc CommandLine { } {
    tk_messageBox -type ok -title {owtap command line} -message {
syntax: owtap.tcl -s serverport -p tapport

  server port is the address of owserver
  tapport is the port assigned this program

  Usage (owdir example)

  If owserver was invoked as:
    owserver -p 3000 -u
  a client (say owdir) would normally call:
    owdir -s 3000 /

  To use owtap, invoke it with
    owtap.tcl -s 3000 -p 4000
  and now call owdir with:
    owdir -s 4000 /
    }
}

# Popup on protocol
proc Protocol { } {
    tk_messageBox -type ok -title {owserver protocol} -message {
The owserver protocol is a tcp/ip
protocol for communication between
owserver and clients.

It is recognized as a "well
known port" by the IANA (4304)
and has an associated mDNS
service (_owserver._tcp).

Datails can be found at:
http://www.owfs.org/index.php?page=owserver-protocol    
    }
}

#Finally, all the Proc-s have been defined, so run everything.
Main $argv
