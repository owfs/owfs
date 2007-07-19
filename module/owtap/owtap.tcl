#!/bin/sh
# the next line restarts using wish \
exec wish "$0" -- "$@"

# $Id$

# Global: Ports() loose tap server
# -- port number of this program (tap) and real owserver (server). loose is stray garbage

set SocketVars {string version type payload size sg offset tokenlength totallength state}

set MessageList {ERROR NOP READ WRITE DIR SIZE PRESENCE DIRALL GET Unknown}
set MessageListPlus $MessageList
lappend MessageListPlus BadHeader Total

# Global: setup_flags => For object-oriented initialization

# Global: serve => information on current transaction
#         serve(port) -- same as Ports(tap)
#         serve(socket) -- listing socket
#         serve($sock.string) -- message to this point
#         serve($sock. version size payload sg offset type tokenlength ) -- message parsed parts
#         serve($sock.version size

# Global: stats => statistical counts and flags for stats windows

# Global: circ_buffer => data for the last n transactions displayed in the listboxes

#Main procedure. We actually start it at the end, to allow Proc-s to be defined first.
proc Main { argv } {
    global Ports

    ArgumentProcess $argv
    wm title . "OWSERVER protocol tap -- port $Ports(tap) to $Ports(server)"

    CircBufferSetup 50
    DisplaySetup
    StatsSetup

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
    global stats
    set serve($sock.state) "Open client"
    while {1} {
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
                set message_type [MessageType $serve($sock.type)]
                CircBufferEntryRequest $current [format "%s %d bytes" $message_type $serve($sock.payload)] $sock
                #stats
                RequestStatsIncr $sock $message_type 0
                # now owserver
                set serve($sock.state) "Open server"
            }
        "Client early end" {
                StatusMessage "FAILURE reading client request"
                CircBufferEntryRequest $current "network read error" $sock
                if { [string length $serve($sock.string] < 24 } {
                    RequestStatsIncr $sock BadHeader 1
                } else {
                    RequestStatsIncr $sock $message_type 1
                }
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
                if { [string length $serve($sock.string] < 24 } {
                     ResponseStatsIncr $relay BadHeader 1
                } else {
                     ResponseStatsIncr $relay $message_type 1
                }
                CircBufferEntryResponse $current "network read error" $relay
                ShowMessage $relay
                CloseTap $relay
                CloseTap $sock
                return
            }
        "Log server input" {
                StatusMessage "Success reading OWSERVER response" 0
                fileevent $relay readable {}
                CircBufferEntryResponse $current [format "Return value %d" $serve($relay.type)] $relay
                #stats
                ResponseStatsIncr $relay $message_type 0
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

# increment stats for  request
proc RequestStatsIncr { sock message_type is_error} {
    global stats
    global serve
    set length [string length $serve($sock.string)]

    incr stats($message_type.tries)
    incr stats($message_type.errors) $is_error
    set stats($message_type.rate) [expr {100 * $stats($message_type.errors) / $stats($message_type.tries)} ]
    incr stats($message_type.request_bytes) $length

    incr stats(Total.tries)
    incr stats(Total.errors) $is_error
    set stats(Total.rate) [expr {100 * $stats(Total.errors) / $stats(Total.tries)} ]
    incr stats(Total.request_bytes) $length
}

# increment stats for  request
proc ResponseStatsIncr { sock message_type is_error} {
    global stats
    global serve
    set length [string length $serve($sock.string)]

    incr stats($message_type.errors) $is_error
    set stats($message_type.rate) [expr {100 * $stats($message_type.errors) / $stats($message_type.tries)} ]
    incr stats($message_type.response_bytes) $length

    incr stats(Total.errors) $is_error
    set stats(Total.rate) [expr {100 * $stats(Total.errors) / $stats(Total.tries)} ]
    incr stats(Total.response_bytes) $length
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
proc MessageNumber { num } {
    global MessageList
    set len [expr [llength $MessageList] - 1]
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
proc SelectionMade { widget y } {
    set index [ $widget nearest $y ]
    if { $index >= 0 } {
        TransactionDetail $index
    }
}

# create visual aspects of program
proc DisplaySetup { } {
    global circ_buffer
    # Top pane, tranaction logs
    frame .log -bg yellow

    scrollbar .log.transaction_scroll -command [ list ScrollTogether ]
    label .log.request_title -text "Client request" -bg yellow -relief ridge
    label .log.respnse_title -text "Owserver response" -bg yellow -relief ridge
    listbox .log.request_list -width 40 -height 10 -selectmode single -yscroll [list Left_ScrollByKey ] -bg lightyellow
    listbox .log.response_list -width 40 -height 10 -selectmode single -yscroll [list Right_ScrollByKey] -bg lightyellow

    bind Listbox <ButtonRelease-1> {+ SelectionMade %W %y }
    bind Listbox <space> {+ SelectionMade %W }

    grid .log.request_title -row 0 -column 0 -sticky news
    grid .log.respnse_title -row 0 -column 1 -sticky news
    grid .log.request_list -row 1 -column 0 -sticky news
    grid .log.response_list -row 1 -column 1 -sticky news
    grid .log.transaction_scroll -row 1 -column 2 -sticky news

    pack .log -side top -fill x -expand true

    #bottom pane, status
    label .status -anchor w -width 80 -relief sunken -height 1 -textvariable current_status -bg white
    pack .status -side bottom -fill x

    SetupMenu
}

# Menu construction
proc SetupMenu { } {
    global stats
#    toplevel . -menu .main_menu
     menu .main_menu -tearoff 0
    . config -menu .main_menu

    # file menu
     menu .main_menu.file -tearoff 0
     .main_menu add cascade -label File -menu .main_menu.file  -underline 0
         .main_menu.file add command -label "Log to File..." -underline 0 -command SaveLog -state disabled
         .main_menu.file add command -label "Stop logging" -underline 0 -command SaveAsLog -state disabled
         .main_menu.file add separator
         .main_menu.file add command -label Quit -underline 0 -command exit

    # statistics menu
     menu .main_menu.stats -tearoff 0
     .main_menu add cascade -label Statistics -menu .main_menu.stats  -underline 0
         .main_menu.stats add checkbutton -label "by Message type" -underline 3 -indicatoron 1 -command {StatByType}
#
#     menu .main_menu.device -tearoff 1
#     .main_menu add cascade -label Devices -menu .main_menu.device  -underline 0
#         .main_menu.device add command -label "Save Log" -underline 0 -command SaveLog
#         .main_menu.device add command -label "Save Log As..." -underline 9 -command SaveAsLog
#         .main_menu.device add separator
#         .main_menu.device add command -label Quit -underline 0 -command exit

    # help menu
    menu .main_menu.help -tearoff 0
    .main_menu add cascade -label Help -menu .main_menu.help  -underline 0
        .main_menu.help add command -label "About OWTAP" -underline 0 -command About
        .main_menu.help add command -label "Command Line" -underline 0 -command CommandLine
        .main_menu.help add command -label "OWSERVER  Protocol" -underline 0 -command Protocol
        .main_menu.help add command -label "Version" -underline 0 -command Version
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
    set circ_buffer(total) -1
}

# Save a spot for the coming connection
proc CircBufferAllocate { } {
    global circ_buffer
    set size $circ_buffer(size)
    incr circ_buffer(total)
    set total $circ_buffer(total)
    set cb_index [ expr { $total % $size } ]
    set circ_buffer($cb_index.request) ""
    set circ_buffer($cb_index.response) ""
    if { $total >= $size } {
        .log.request_list delete 0
        .log.response_list delete 0
    }
    .log.request_list insert end "x"
    .log.response_list insert end "x"
    return $total
}

# place a new request packet
proc CircBufferEntryRequest { current request sock} {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    if { [expr {$current + $size}] <= $total } {
        StatusMessage "Packet buffer history overflow. (nonfatal)" 0
        return
    }
    # Still filling for the first time?
    if { $total < $size } {
        set index $current
    } else {
        set index [ expr $size - $total + $current - 1 ]
    }
    .log.request_list insert $index $request
    .log.request_list delete [expr $index + 1 ]

    #Now store packet
    global serve
    set cb_index [ expr { $current % $size } ]
    set circ_buffer($cb_index.request) $serve($sock.string)
}

# place a new response packet
proc CircBufferEntryResponse { current response sock } {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    if { [expr {$current + $size}] <= $total } {
        StatusMessage "Packet buffer history overflow. (nonfatal)" 0
        return
    }
    # Still filling for the first time?
    if { $total < $size } {
        set index $current
    } else {
        set index [ expr $size - $total + $current - 1 ]
    }
    .log.response_list insert $index $response
    .log.response_list delete [expr $index + 1 ]

    #Now store packet
    global serve
    set cb_index [ expr { $current % $size } ]
    set circ_buffer($cb_index.response) $serve($sock.string)
}

# get the slot in the circ_buffer from the listbox index
proc cb_from_index { index } {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    if { $total < $size } { return $index }
    return [expr { ($total + $index) % $size }]
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

# Popup giving version
proc Version { } {
    regsub -all {[$:a-zA-Z]} {$Revision$} {} Version
    regsub -all {[$:a-zA-Z]} {$Date$} {} Date
    tk_messageBox -type ok -title {owtap version} -message "
Version $Version
Date    $Date
    "
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

# Show a table of packets and bytes by type (DIR, READ,...)
# Separate window that is pretty self contained.
# Data values use -textvariable so auto-update
# linked by "globals" for variables and types
# linked my menu position and index (checkbox)
# catches delete and hides instead (via menu action)
proc StatByType { } {
    set window_name .statbytype
    set menu_name .main_menu.stats
    set menu_index 0
    global stats
    global MessageListPlus
    global setup_flags

    if { [ info exist setup_flags($window_name) ] } {
        if { $setup_flags($window_name) } {
# hide stats window
            wm withdraw $window_name
            set setup_flags($window_name) 0

} else {
# show stats window
            wm deiconify $window_name
            set setup_flags($window_name) 1
}
        return
}

# create stats window
    toplevel $window_name
    set column_number 0
    label $window_name.l${column_number}0 -text "Type" -bg blue
    grid $window_name.l${column_number}0 -row 0 -column $column_number -sticky news
    label $window_name.l${column_number}1 -text "Packets" -bg lightblue
    grid $window_name.l${column_number}1 -row 1 -column $column_number -sticky news
    label $window_name.l${column_number}2 -text "Errors" -bg  lightblue
    grid $window_name.l${column_number}2 -row 2 -column $column_number -sticky news
    label $window_name.l${column_number}3 -text "Error %" -bg  lightblue
    grid $window_name.l${column_number}3 -row 3 -column $column_number -sticky news
    frame $window_name.ff4 -bg blue
    grid $window_name.ff4 -column 1 -row 0
    label $window_name.l${column_number}5 -text "bytes in" -bg  lightblue
    grid $window_name.l${column_number}5 -row 5 -column $column_number -sticky news
    label $window_name.l${column_number}6 -text "bytes out" -bg  lightblue
    grid $window_name.l${column_number}6 -row 6 -column $column_number -sticky news
    set bgcolor white
    set bgcolor2 yellow
    foreach x $MessageListPlus {
        incr column_number
        label $window_name.${column_number}0 -text $x -bg $bgcolor2
        grid  $window_name.${column_number}0 -row 0 -column $column_number -sticky news
        label $window_name.${column_number}1 -textvariable stats($x.tries) -bg $bgcolor
        grid  $window_name.${column_number}1 -row 1 -column $column_number -sticky news
        label $window_name.${column_number}2 -textvariable stats($x.errors) -bg $bgcolor
        grid  $window_name.${column_number}2 -row 2 -column $column_number -sticky news
        label $window_name.${column_number}3 -textvariable stats($x.rate) -bg $bgcolor
        grid  $window_name.${column_number}3 -row 3 -column $column_number -sticky news
        label $window_name.${column_number}5 -textvariable stats($x.request_bytes) -bg $bgcolor
        grid  $window_name.${column_number}5 -row 5 -column $column_number -sticky news
        label $window_name.${column_number}6 -textvariable stats($x.response_bytes) -bg $bgcolor
        grid  $window_name.${column_number}6 -row 6 -column $column_number -sticky news
        if { $bgcolor == "white" } { set bgcolor lightyellow } else { set bgcolor white }
        if { $bgcolor2 == "yellow" } { set bgcolor2 orange } else { set bgcolor2 yellow }
}
    frame $window_name.f4 -bg yellow
    grid $window_name.f4 -column 1 -row 4 -columnspan [llength MessageListPlus]
# delete handler
    wm protocol $window_name WM_DELETE_WINDOW [list $menu_name invoke $menu_index]
# now set flag
    set setup_flags($window_name) 1
}

#make a window that has to be dismissed by hand
proc TransactionDetail { index } {
    # make a unique window name
    global setup_flags
    if { [ info exist setup_flags(detail) ] } {
        incr setup_flags(detail)
    } else {
        set setup_flags(detail) 0
    }
    set window_name .detail$setup_flags(detail)

    # Make the window
    toplevel $window_name -bg white
    set cb_index [cb_from_index $index]

    RequestDetail $window_name $cb_index
    ResponseDetail $window_name $cb_index
}

# Request portion
proc RequestDetail { window_name cb_index } {
    global circ_buffer

    DetailRow $window_name yellow orange version payload type flags size offset

    # get request data
    set q(version) ""
    set q(payload) ""
    set q(type) ""
    set q(flags) ""
    set q(size) ""
    set q(offset) ""
    binary scan $circ_buffer($cb_index.request) {IIIIII} q(version) q(payload) q(type) q(flags) q(size) q(offset)
    # request headers
    DetailRow $window_name white lightyellow $q(version) $q(payload) $q(type) $q(flags) $q(size) $q(offset)
# request headers
    if { [string length $circ_buffer($cb_index.request)] >= 24 } {
        DetailRow $window_name white lightyellow [DetailVersion $q(version)] $q(payload) [DetailType $q(type)] [DetailFlags $q(flags)] $q(size) $q(offset)
    }
}

# Response portion
proc ResponseDetail { window_name cb_index } {
    global circ_buffer

    DetailRow $window_name #a6dcff #a6b1ff version payload return flags size offset

    set r(version) ""
    set r(payload) ""
    set r(return) ""
    set r(flags) ""
    set r(size) ""
    set r(offset) ""
    binary scan $circ_buffer($cb_index.response) {IIIIII} r(version) r(payload) r(return) r(flags) r(size) r(offset)
    DetailRow $window_name white #dbedf7 $r(version) $r(payload) $r(return) $r(flags) $r(size) $r(offset)
    if { [string length $circ_buffer($cb_index.response)] >= 24 } {
        DetailRow $window_name white #dbedf7 [DetailVersion $r(version)] $r(payload) $r(return) [DetailFlags $r(flags)] $r(size) $r(offset)
    }
}

proc DetailText { window_name color text_string } {
    set row [lindex [grid size $window_name] 1]
    set columns [lindex [grid size $window_name] 0]
    label $window_name.t${row} -text $text_string -bg $color
    grid  $window_name.t${row0} -row $row -columnspan $columns -sticky news -relief ridge
}

proc DetailRow { window_name color1 color2 v0 v1 v2 v3 v4 v5 } {
    set row [lindex [grid size $window_name] 1]
    label $window_name.x${row}0 -text $v0 -bg $color1
    label $window_name.x${row}1 -text $v1 -bg $color2
    label $window_name.x${row}2 -text $v2 -bg $color1
    label $window_name.x${row}3 -text $v3 -bg $color2
    label $window_name.x${row}4 -text $v4 -bg $color1
    label $window_name.x${row}5 -text $v5 -bg $color2
    grid  $window_name.x${row}0 $window_name.x${row}1 $window_name.x${row}2 $window_name.x${row}3 $window_name.x${row}4 $window_name.x${row}5 -row $row -sticky news
}

proc DetailVersion { version } {
    set tok [expr { $version & 0xFFFF}]
    set ver [expr { $version >> 16}]
    return "T$tok V$ver"
}

proc DetailType { typ } {
    return [MessageType [MessageNumber $typ]]
}

proc DetailFlags { flags } {
    switch [expr {($flags >>16)&0xFF}] {
        0 {set T "C"}
        1 {set T "F"}
        2 {set T "K"}
        3 {set T "R"}
        default {set T "?temp"}
    }
    switch [expr {$flags >>24}] {
        0 {set F " f.i"}
        1 {set F " fi"}
        2 {set F " f.i.c"}
        3 {set F " f.ic"}
        4 {set F " fi.c"}
        5 {set F " fic"}
        default {set F " ?format"}
    }
    return $T$F[expr {$flags&0x04?" persist":""}][expr {$flags&0x02?" bus":""}][expr {$flags&0x01?" cache":""}]
}
 
#Finally, all the Proc-s have been defined, so run everything.
Main $argv
