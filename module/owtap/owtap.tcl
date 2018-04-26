#!/bin/sh
# the next line restarts using wish \
if [ -z `which wish` ] ; then exec tclsh "$0" -- "$@" ; else exec wish "$0" -- "$@" ; fi

package require Tk

# Global: IPAddress() loose tap server
# -- port number of this program (tap) and real owserver (server). loose is stray garbage

set SocketVars {string version type payload size sg offset tokenlength totallength paylength typetext ping state sock versiontext flagtext persist return id }

set MessageList {ERROR NOP READ WRITE DIR SIZE PRESENCE DIRALL GET DIRALLSLASH GETSLASH}
set MessageListPlus $MessageList
lappend MessageListPlus PING BadHeader Unknown Total

# see http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html
set HttpList [list "GET " POST HEAD CONN "PUT " DELE TRAC]

set stats(clientlist) {}
set setup_flags(detail_list) {}

# Global: setup_flags => For object-oriented initialization

# Global: serve => information on current transaction
#         serve($sock.string) -- message to this point
#         serve($sock. version size payload sg offset type tokenlength ) -- message parsed parts
#         serve($sock.version size
#         serve(sockets) -- list of active sockets for timeout

# Global: stats => statistical counts and flags for stats windows

# Global: circ_buffer => data for the last n transactions displayed in the listboxes

#Main procedure. We actually start it at the end, to allow Proc-s to be defined first.
proc Main { argv } {
    ArgumentProcess

    CircBufferSetup 50
    DisplaySetup
    StatsSetup

    SetupTap
}

# Command line processing
# looks at command line arguments
# two options "p" and "s" for (this) tap's _p_ort, and the target ow_s_erver port
# Can handle command lines like
# -p 3000 -s 3001
# -p3000 -s 3001
# etc
proc ArgumentProcess { } {
    global IPAddress
    set mode "loose"
    # "Clear" ports
    # INADDR_ANY
    # tk_messageBox -message "$::argv" -type ok
    set IPAddress(tap.ip) "0.0.0.0"
    set IPAddress(tap.port) "0"
    set IPAddress(server.ip) "0.0.0.0"
    set IPAddress(server.port) "4304"
    foreach a $::argv {
        if { [regexp -- {^-p(.*)$} $a whole address] } {
			set mode "tap"
		} elseif { [regexp -- {^-s(.*)$} $a whole address] } {
			set mode "server"
		}  else {
			set address $a
		}
		IPandPort $mode $address
    }
    MainTitle $IPAddress(tap.ip):$IPAddress(tap.port) $IPAddress(server.ip):$IPAddress(server.port)
}

proc IPandPort { mode argument_string } {
    global IPAddress
    if { [regexp -- {^(.*?):(.*)$} $argument_string wholestring firstpart secondpart] } {
		if { $firstpart != "" } { set IPAddress($mode.ip) $firstpart }
		if { $secondpart != "" } { set IPAddress($mode.port) $secondpart }
    } else {
		if { $argument_string != "" } { set IPAddress($mode.port) $argument_string }
    }
}

# Accept from client (our "server" portion)
proc SetupTap { } {
    global IPAddress
    StatusMessage "Attempting to open surrogate server on $IPAddress(tap.ip):$IPAddress(tap.port)"
    if {[catch {socket -server TapAccept -myaddr $IPAddress(tap.ip) $IPAddress(tap.port)} result] } {
        ErrorMessage $result
    }
    StatusMessage "Success. Tap server address=[PrettySock $result]"
    MainTitle [PrettySock $result] $IPAddress(server.ip):$IPAddress(server.port)
}

#Main loop. Called whenever the server (listen) port accepts a connection.
proc TapAccept { sock addr port } {
    global serve
    global stats

    # Start the State machine
    set serve($sock.state) "Open client"
    while {1} {
#puts $serve($sock.state)
        switch $serve($sock.state) {
        "Open client" {
                StatusMessage "Reading client request from $addr port $port" 0
                set persist 0
                fconfigure $sock -buffering full -translation binary -encoding binary -blocking 0
                set serve($sock.state) "Persistent loop"
            }
        "Persistent loop" {
                TapSetup $sock
                set serve($sock.sock) $sock
                set current [CircBufferAllocate]
                set serve($sock.state) "Read client"
            }
        "Read client" {
                # wait a long time (3hr) for very first packet
                ResetSockTimer $sock 10000000
                fileevent $sock readable [list TapProcess $sock]
#   ShowMessage $sock
                vwait serve($sock.state)
            }
        "Process client packet" {
                fileevent $sock readable {}
                ClearSockTimer $sock
                StatusMessage "Success reading client request" 0
                set message_type $serve($sock.typetext)
                CircBufferEntryRequest $current "$addr:$port $message_type $serve($sock.payload) bytes" $serve($sock.string)
                AddClient $addr:$port
                #stats
                RequestStatsIncr $sock 0
                # now owserver
                if {$persist>0} {
                    set serve($sock.state) "Send to server"
                } else {
                    set serve($sock.state) "Open server"
                }
            }
        "Client early end" {
                StatusMessage "Client dropped connection"
                CircBufferEntryRequest $current "Client done" $serve($sock.string)
                CircBufferEntryResponse $current "<none>"
                RequestStatsIncr $sock 1
                set serve($sock.state) "Done with client"
            }
        "Web client" {
                StatusMessage "Error: owtap is not a web server"
                CircBufferEntryRequest $current "Not a web server"
                CircBufferEntryResponse $current "<none>"
                RequestStatsIncr $sock 1
                WebResponse $sock
                set serve($sock.state) "Done with client"
            }
        "Open server" {
                global IPAddress
                StatusMessage "Attempting to open connection to OWSERVER" 0
                if {[catch {socket $IPAddress(server.ip) $IPAddress(server.port)} relay] } {
                    set serve($sock.state) "Unopened server"
                } else {
                    set serve($sock.state) "Send to server"
                }
            }
        "Unopened server" {
                StatusMessage "OWSERVER error: $relay at $IPAddress(server.ip):$IPAddress(server.port)"
                set serve($relay.string) {}
                CircBufferEntryResponse $current "owserver not responding"
                set serve($sock.state) "Done with client"
            }
        "Send to server" {
                StatusMessage "Sending client request to OWSERVER" 0
                fconfigure $relay -translation binary -buffering full -encoding binary -blocking 0
                set serve($relay.sock) $sock
                puts -nonewline $relay  $serve($sock.string)
                flush $relay
                set serve($sock.state) "Read from server"
            }
        "Read from server" {
                StatusMessage "Reading OWSERVER response" 0
                TapSetup $relay
                ResetSockTimer $relay
                fileevent $relay readable [list RelayProcess $relay]
                vwait serve($sock.state)
            }
        "Server early end" {
                StatusMessage "FAILURE reading OWSERVER response" 0
                ResponseStatsIncr $relay 1
                CircBufferEntryResponse $current "network read error" $serve($relay.string)
                set serve($sock.state) "Done with server"
            }
        "Process server packet" {
                StatusMessage "Success reading OWSERVER response" 0
                ResponseAdd $relay
                fileevent $relay readable {}
                CircBufferEntryResponse $current $serve($relay.return) $serve($relay.string)
                #stats
                ResponseStatsIncr $relay 0
                set serve($sock.state) "Send to client"
            }
        "Send to client" {
                ClearSockTimer $relay
                StatusMessage "Sending OWSERVER response to client" 0
                puts -nonewline $sock  $serve($relay.string)
                flush $sock
# filter out the multi-response types and continue listening
                if { $serve($relay.ping) == 1 } {
                    set serve($sock.state) "Ping received"
                } elseif { ( $message_type=="DIR" ) && ($serve($relay.paylength)>0)} {
                    set serve($sock.state) "Dir element received"
                } else {
                    set serve($sock.state) "Persistent test"
                }
            }
        "Ping received" {
                set serve($sock.state) "Read from server"
            }
        "Dir element received" {
                set serve($sock.state) "Read from server"
            }
        "Persistent test" {
                if { $serve($relay.persist)==1 } {
                    StatPersistCounter $persist
                    incr persist
                    ClearTap $sock
                    ClearTap $relay
                    set serve($sock.state) "Persistent loop"
                } else {
                    set serve($sock.state) "Done with server"
                }
            }
        "Server timeout" {
                CircBufferEntryResponse $current "owserver read timeout"
                set serve($sock.state) "Done with server"
            }
        "Done with server"  {
                CloseSock $relay
                set serve($sock.state) "Done with client"
            }
        "Client timeout" {
                CircBufferEntryRequest $current "client read timeout"
                CircBufferEntryResponse $current "<none>"
                set serve($sock.state) "Done with client"
            }
        "Done with client" {
                CloseSock $sock
                set serve($sock.state) "Done with all"
            }
        "Done with all" {
                StatusMessage "Ready" 0
                return
            }
        default {
                StatusMessage "Internal error -- bad state: $serve($sock.state)" 1
                return
            }
        }
    }
}

# initialize statistics
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
    foreach x { request_yes request_no grant refuse -1 0 denominator max request_rate grant_rate } {
        set stats(persistence_length.$x) 0
    }
    set stats(persistence_length.15plus) 0
}

# increment stats for  request
proc RequestStatsIncr { sock is_error} {
    global stats
    global serve
    set message_type $serve($sock.typetext)
    set length [string length $serve($sock.string)]

    incr stats($message_type.tries)
    incr stats($message_type.errors) $is_error
    set stats($message_type.rate) [expr {100 * $stats($message_type.errors) / $stats($message_type.tries)} ]
    incr stats($message_type.request_bytes) $length

    incr stats(Total.tries)
    incr stats(Total.errors) $is_error
    set stats(Total.rate) [expr {100 * $stats(Total.errors) / $stats(Total.tries)} ]
    incr stats(Total.request_bytes) $length

    # persistence stats
    if { [info exist serve($sock.persist)] } {
        if { $serve($sock.persist) == 0 } {
            incr stats(persistence_length.request_no)
        } else {
            incr stats(persistence_length.request_yes)
            incr stats(persistence_length.refuse)
            set stats(persistence_length.grant_rate) [expr {100 * $stats(persistence_length.grant) / $stats(persistence_length.request_yes)}]
        }
        set stats(persistence_length.request_rate) [expr { 100 * $stats(persistence_length.request_yes) / $stats(Total.tries) }]
    }
}

# increment stats for  request
proc ResponseStatsIncr { sock is_error} {
    global stats
    global serve
    set message_type $serve($serve($sock.sock).typetext)
    set length [string length $serve($sock.string)]

    incr stats($message_type.errors) $is_error
    set stats($message_type.rate) [expr {100 * $stats($message_type.errors) / $stats($message_type.tries)} ]
    incr stats($message_type.response_bytes) $length

    incr stats(Total.errors) $is_error
    set stats(Total.rate) [expr {100 * $stats(Total.errors) / $stats(Total.tries)} ]
    incr stats(Total.response_bytes) $length
}

# Counter of Persistence lengths
proc StatPersistCounter { persist } {
    global stats
    # max persistence length
    if { $persist > $stats(persistence_length.max) } {
        set stats(persistence_length.max) $persist
        set stats(persistence_length.$persist) 0
    }
    # increment this bin (and decrement prior)
    if { $persist==0 } {
        incr stats(persistence_length.denominator)
    }
    set old_persist [expr {$persist-1}]
    incr stats(persistence_length.$old_persist) -1
    if { $persist == 16 } {
        incr stats(persistence_length.15plus)
    }
    incr stats(persistence_length.$persist)
    incr stats(persistence_length.grant)
    incr stats(persistence_length.refuse) -1
    set stats(persistence_length.grant_rate) [expr {100 * $stats(persistence_length.grant) / $stats(persistence_length.request_yes)}]

    set length_sum 0
    for {set x 0} {$x <= $stats(persistence_length.max)} {incr x} {
        incr length_sum [expr {$stats(persistence_length.$x) * $x}]
    }
    set stats(persistence_length.mean) [expr {($length_sum)/$stats(persistence_length.denominator)}]
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
    if { [info exist serve($sock.num] } {
        for {set i $serve($sock.num)} {$i >= 0} {incr i -1} {
            unset serve($sock.$i)
        }
        unset serve($sock.num)
    }
}

proc ResponseAdd { sock } {
    global serve
    if { [info exist serve($sock.num)] } {
        incr serve($sock.num)
    } else {
        set serve($sock.num) 0
    }
    set serve($sock.$serve($sock.num)) $serve($sock.string)
}

# close client request socket
proc SockTimeout { sock } {
    global serve
    switch $serve($serve($sock.sock).state) {
        "Read client" {
            set serve($serve($sock.sock).state) "Client timeout"
        }
        "Read from server" {
            set serve($serve($sock.sock).state) "Server timeout"
        }
        default {
            ErrorMessage "Strange timeout for $sock state=$serve($serve($sock.sock).state)"
            set serve($serve($sock.sock).state) "Server timeout"
        }
    }
    StatusMessage "Network read timeout [PrettySock $sock]" 1
}

# close client request socket
proc CloseSock { sock } {
    global serve
    ClearSockTimer $sock
    close $sock
    ClearTap $sock
}

proc ClearSockTimer { sock } {
    global serve
    if { [info exist serve($sock.id)] } {
        after cancel $serve($sock.id)
        unset serve($sock.id)
    }
}

proc ResetSockTimer { sock { msec 2000 } } {
    global serve
    ClearSockTimer $sock
    set serve($sock.id) [after $msec [list SockTimeout $sock ]]
}

# Wrapper for processing -- either change a vwait var, or just return waiting for more network traffic
proc TapProcess { sock } {
    global serve
    set read_value [ReadProcess $sock]
    switch $read_value {
        "Web client"  { set serve($sock.state) "Web client" }
        "Client early end"  { set serve($sock.state) "Client early end" }
        "Packet reloop"  { return }
        "Process packet"  { set serve($sock.state) "Process client packet" }
    }
    TypeParser serve $sock
}

# Process a oncomming owserver packet, adjusting size from header information
proc ReadProcess { sock } {
    global serve
    # test eof
    if { [eof $sock] } {
        return "Client early end"
    }
    # read what's waiting
    set new_string [read $sock]
    if { $new_string == {} } {
        return "Packet reloop"
    }
    append serve($sock.string) $new_string
    ResetSockTimer $sock
    set len [string length $serve($sock.string)]
    if { $len < 24 } {
        #do nothing -- reloop
        return "Packet reloop"
    } elseif { $serve($sock.totallength) == 0 } {
        # Look for web browser
        global HttpList
        if { [lsearch -exact $HttpList [string range $serve($sock.string) 0 3]] >= 0 } {
            return "Web client"
        }
        # at least header is in
        HeaderParser serve $sock $serve($sock.string)
    }
    #already in payload (and token) portion
    if { $len < $serve($sock.totallength) } {
        #do nothing -- reloop
        return "Packet reloop"
    }
    # Fully parsed
    set new_length [string length $serve($sock.string)]
    return "Process packet"
}

# Wrapper for processing -- either change a vwait var, or just return waiting for more network traffic
proc RelayProcess { relay } {
    global serve
    set read_value [ReadProcess $relay]
#puts "Current length [string length $serve($relay.string)] return val=$read_value"
    switch $read_value {
        "Web client"  -
        "Client early end"  { set serve($serve($relay.sock).state) "Server early end"}
        "Packet reloop"  { return }
        "Process packet"  { set serve($serve($relay.sock).state) "Process server packet" }
    }
    ErrorParser serve $relay
#puts $serve($serve($relay.sock).typetext)
#    ShowMessage $relay
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
        TransactionDetail [current_from_index $index]
    }
}

# create visual aspects of program
proc DisplaySetup { } {
    global circ_buffer
    # Top pane, tranaction logs
    frame .log -bg yellow

    scrollbar .log.transaction_scroll -command [ list ScrollTogether ]
    label .log.request_title -text "Client request" -bg yellow -relief ridge
    label .log.response_title -text "Owserver response" -bg yellow -relief ridge
    listbox .log.request_list -width 40 -height 10 -selectmode single -yscroll [list Left_ScrollByKey ] -bg lightyellow
    listbox .log.response_list -width 40 -height 10 -selectmode single -yscroll [list Right_ScrollByKey] -bg lightyellow

    foreach lb {request_list response_list} {
        bind .log.$lb <ButtonRelease-1> {+ SelectionMade %W %y }
        bind .log.$lb <space> {+ SelectionMade %W }
    }

    grid .log.request_title -row 0 -column 0 -sticky news
    grid .log.response_title -row 0 -column 1 -sticky news
    grid .log.request_list -row 1 -column 0 -sticky news
    grid .log.response_list -row 1 -column 1 -sticky news
    grid .log.transaction_scroll -row 1 -column 2 -sticky news

    pack .log -side top -fill x -expand true

    #bottom pane, status
    label .status -anchor w -width 80 -relief sunken -height 1 -textvariable current_status -bg white
    pack .status -side bottom -fill x
    bind .status <ButtonRelease-1> [list .main_menu.view invoke "Status messages"]

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
        .main_menu.file add command -label "Restart" -underline 0 -command Restart
        .main_menu.file add separator
        .main_menu.file add command -label "Quit" -underline 0 -command exit

    # statistics menu
    menu .main_menu.view -tearoff 0
    .main_menu add cascade -label View -menu .main_menu.view  -underline 0
        .main_menu.view add checkbutton -label "Statistics by Message type" -underline 14 -indicatoron 1 -command {StatByType}
        .main_menu.view add checkbutton -label "Persistence rates" -underline 12 -indicatoron 1 -command {RatePersist}
        .main_menu.view add checkbutton -label "Persistence lengths" -underline 12 -indicatoron 1 -command {LengthPersist}
        .main_menu.view add separator
        .main_menu.view add checkbutton -label "Detail window list" -underline 0 -indicatoron 1 -command {DetailList}
        .main_menu.view add separator
        .main_menu.view add checkbutton -label "Clients" -underline 0 -indicatoron 1 -command {StatByClient}
        .main_menu.view add separator
        .main_menu.view add checkbutton -label "Status messages" -underline 0 -indicatoron 1 -command {StatusWindow}

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
    if { $priority > 0 } {
        global status_messages
        lappend status_messages $msg
        if { [llength $status_messages] > 50 } {
            set status_messages [lreplace $status_messages 0 0]
        }
    }
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
    if { $total >= $size } {
        # delete top listbox entry (oldest)
        .log.request_list delete 0
        .log.response_list delete 0
        # clear old entry
        if { [info exist circ_buffer($cb_index.num)] } {
            set num $circ_buffer($cb_index.num)
            for {set x 0} { $x < $num } {incr x} {
                unset circ_buffer($cb_index.response.$x)
            }
        }
    }
    set circ_buffer($cb_index.num) 0
    set circ_buffer($cb_index.request) ""
    .log.request_list insert end "$total: pending"
    .log.response_list insert end "$total: pending"
    return $total
}

# place a new request packet
proc CircBufferEntryRequest { current request {transaction_string "" } } {
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
    set cb_index [ expr { $current % $size } ]
    set circ_buffer($cb_index.request) $transaction_string
}

# place a new response packet
proc CircBufferEntryResponse { current response {transaction_string "" } } {
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
    .log.response_list insert $index "$current: $response"
    .log.response_list delete [expr $index + 1 ]

    #Now store packet
    set cb_index [ expr { $current % $size } ]
    set circ_buffer($cb_index.response.$circ_buffer($cb_index.num)) $transaction_string
    incr circ_buffer($cb_index.num)
}

# get the slot in the circ_buffer from the listbox index
proc cb_from_index { index } {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    if { $total < $size } { return $index }
    return [expr { ($total + $index) % $size }]
}

# get the total list from listbox index
proc current_from_index { index } {
    global circ_buffer
    set size $circ_buffer(size)
    set total $circ_buffer(total)
    if { $total < $size } { return $index }
    return [expr { $total + $index - $size  + 1 }]
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

# Show a list of detail windows
proc DetailList { } {
    set window_name .detaillist
    set menu_name .main_menu.view
    set menu_index "Detail window list"

    if { [ WindowAlreadyExists $window_name $menu_name $menu_index ] } {
        return
    }

    global setup_flags

    listbox $window_name.lb -listvariable setup_flags(detail_list) -width 30 -yscrollcommand [list $window_name.sb set] -selectmode extended -bg lightyellow
    scrollbar $window_name.sb -command [list $window_name.lb yview]
    set f [frame $window_name.f]
    set all [button $f.all -text All -command [list $window_name.lb selection set 0 end]]
    set none [button $f.none -text None -command [list $window_name.lb selection clear 0 end]]
    set delete [button $f.close -text "Close selected" -command [list DetailClear $window_name.lb]]
    pack $all -side left
    pack $none -side left
    pack $delete -side right
    pack $f -side bottom -fill x
    pack $window_name.sb -side right -fill y
    pack $window_name.lb -side left -fill both -expand true
}

proc DetailClear { list_box } {
    foreach i [$list_box curselection] {
        lappend windows [$list_box get $i]
    }
    foreach w $windows {
        DetailDelete $w
    }
}

proc DetailDelete { window_name } {
    global setup_flags
    set i [lsearch -exact $setup_flags(detail_list) $window_name]
    if { $i >= 0 } {
        set setup_flags(detail_list) [lreplace $setup_flags(detail_list) $i $i]
    }
    destroy $window_name
}

# Client list
proc AddClient { client } {
    global ClientList
    if { [info exist ClientList($client)] } {
        incr ClientList($client)
    } else {
        set ClientList($client) 1
    }

    global setup_flags

    if { [ info exist setup_flags(.clientlist) ] } {
        BuildClientList
    }
}

proc BuildClientList { } {
    global stats
    global ClientList
    set l {}
    foreach {k v} [array get ClientList] {
        lappend l [format {%20.20s %8d} $k $v]
    }
    set stats(clientlist) $l
}

# Show a table of Past status messages
proc StatByClient { } {
    set window_name .clientlist
    set menu_name .main_menu.view
    set menu_index "Clients"

    if { [ WindowAlreadyExists $window_name $menu_name $menu_index ] } {
        return
    }

    global stats

    BuildClientList
    listbox $window_name.lb -listvariable stats(clientlist) -width 30 -yscrollcommand [list $window_name.sb set] -bg lightyellow
    scrollbar $window_name.sb -command [list $window_name.lb yview]
    pack $window_name.sb -side right -fill y
    pack $window_name.lb -side left -fill both -expand true
}

# Show a table of Past status messages
proc LengthPersist { } {
    set window_name .lengthpersistwindow
    set menu_name .main_menu.view
    set menu_index "Persistence lengths"

    if { [ WindowAlreadyExists $window_name $menu_name $menu_index ] } {
        return
    }

    global stats

    label $window_name.at -text "Length" -bg blue -fg white
    grid  $window_name.at -row 0 -column 0 -sticky news
    label $window_name.bt -text "Count" -bg blue -fg white
    grid  $window_name.bt -row 0 -column 1 -sticky news

    label $window_name.ax -textvariable stats(persistence_length.max) -bg lightyellow
    grid  $window_name.ax -row 1 -column 0 -sticky news
    label $window_name.bx -text "Max" -bg lightyellow
    grid  $window_name.bx -row 1 -column 1 -sticky news
    label $window_name.am -textvariable stats(persistence_length.mean) -bg lightyellow
    grid  $window_name.am -row 2 -column 0 -sticky news
    label $window_name.bm -text "Mean" -bg lightyellow
    grid  $window_name.bm -row 2 -column 1 -sticky news

    set row 3
    set bg white
    for {set x 0} {$x < 16} {incr x} {
        label $window_name.a${x} -text $x -bg $bg
        grid  $window_name.a${x} -row $row -column 0 -sticky news
        label $window_name.b${x} -textvariable stats(persistence_length.$x) -bg $bg
        grid  $window_name.b${x} -row $row -column 1 -sticky news
        if {$bg=="white"} { set bg lightyellow} else {set bg white}
        incr row
    }
    label $window_name.ap -text "> 15" -bg $bg
    grid  $window_name.ap -row $row -column 0 -sticky news
    label $window_name.bp -textvariable stats(persistence_length.15plus) -bg $bg
    grid  $window_name.bp -row $row -column 1 -sticky news
}

# Show a table of Past status messages
proc RatePersist { } {
    set window_name .ratepersistwindow
    set menu_name .main_menu.view
    set menu_index "Persistence rates"

    if { [ WindowAlreadyExists $window_name $menu_name $menu_index ] } {
        return
    }

    global stats

    label $window_name.a0 -text "Persistence" -bg blue -fg white
    grid  $window_name.a0 -row 0 -column 0 -sticky news
    label $window_name.a1 -text "Requests" -bg lightblue
    grid  $window_name.a1 -row 1 -column 0 -sticky news
    label $window_name.a2 -text "Granted" -bg lightblue
    grid  $window_name.a2 -row 2 -column 0 -sticky news

    label $window_name.b0 -text "number" -bg yellow
    grid  $window_name.b0 -row 0 -column 1 -sticky news
    label $window_name.b1 -textvariable stats(persistence_length.request_yes) -bg white
    grid  $window_name.b1 -row 1 -column 1 -sticky news
    label $window_name.b2 -textvariable stats(persistence_length.grant) -bg white
    grid  $window_name.b2 -row 2 -column 1 -sticky news

    label $window_name.c0 -text "total" -bg orange
    grid  $window_name.c0 -row 0 -column 2 -sticky news
    label $window_name.c1 -textvariable stats(Total.tries) -bg lightyellow
    grid  $window_name.c1 -row 1 -column 2 -sticky news
    label $window_name.c2 -textvariable stats(persistence_length.request_yes) -bg lightyellow
    grid  $window_name.c2 -row 2 -column 2 -sticky news

    label $window_name.d0 -text "rate %" -bg yellow
    grid  $window_name.d0 -row 0 -column 3 -sticky news
    label $window_name.d1 -textvariable stats(persistence_length.request_rate) -bg white
    grid  $window_name.d1 -row 1 -column 3 -sticky news
    label $window_name.d2 -textvariable stats(persistence_length.grant_rate) -bg white
    grid  $window_name.d2 -row 2 -column 3 -sticky news
}

# Show a table of Past status messages
proc StatusWindow { } {
    set window_name .statuswindow
    set menu_name .main_menu.view
    set menu_index "Status messages"

    if { [ WindowAlreadyExists $window_name $menu_name $menu_index ] } {
        return
    }

    global status_messages

    # create status window
    scrollbar $window_name.xsb -orient horizontal -command [list $window_name.lb xview]
    pack $window_name.xsb -side bottom -fill x -expand 1
    scrollbar $window_name.ysb -orient vertical -command [list $window_name.lb yview]
    pack $window_name.ysb -fill y -expand 1 -side right
    listbox $window_name.lb -listvar status_messages -bg white -yscrollcommand [list $window_name.ysb set] -xscrollcommand [list $window_name.xsb set] -width 80
    pack $window_name.lb -fill both -expand 1 -side left
}

#proc window handler for statistics and status windows
#return 1 if old, 0 if new
proc WindowAlreadyExists { window_name menu_name menu_index } {
    global setup_flags

    if { [ info exist setup_flags($window_name) ] } {
        if { $setup_flags($window_name) } {
            # hide window
            wm withdraw $window_name
            set setup_flags($window_name) 0
        } else {
            # show window
            wm deiconify $window_name
            set setup_flags($window_name) 1
        }
        return 1
    }

    # create window
    toplevel $window_name
    wm title $window_name $menu_index
    # delete handler
    wm protocol $window_name WM_DELETE_WINDOW [list $menu_name invoke $menu_index]
    # now set flag
    set setup_flags($window_name) 1
    return 0
}

# Show a table of packets and bytes by type (DIR, READ,...)
# Separate window that is pretty self contained.
# Data values use -textvariable so auto-update
# linked by "globals" for variables and types
# linked my menu position and index (checkbox)
# catches delete and hides instead (via menu action)
proc StatByType { } {
    set window_name .statbytype
    set menu_name .main_menu.view
    set menu_index "Statistics by Message type"

    if { [ WindowAlreadyExists $window_name $menu_name $menu_index ] } {
        return
    }

    global stats
    global MessageListPlus

    # create stats window
    set column_number 0
    label $window_name.l${column_number}0 -text "Type" -bg blue -fg white
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
}

#make a window that has to be dismissed by hand
proc TransactionDetail { index } {
    # make a unique window name
    global setup_flags
    set window_name .transaction_$index

    # Does the window exist?
    if { [winfo exists $window_name] == 1 } {
        raise $window_name
        return
    }

    # Make the window
    toplevel $window_name -bg white
    wm title $window_name "Transaction $index"
    set cb_index [cb_from_index $index]

    RequestDetail $window_name $cb_index
    ResponseDetail $window_name $cb_index

    # Add to a list
    set l [concat $setup_flags(detail_list) $window_name]
    set setup_flags(detail_list) [lsort -unique $l]

    # delete handler
    wm protocol $window_name WM_DELETE_WINDOW [list DetailDelete $window_name ]
}

# Parse for return HeaderParser
proc ErrorParser { array_name prefix } {
	upvar 1 $array_name a_name
	set a_name($prefix.return) [DetailReturn a_name $prefix]
}

# Parse for TYPE after HeaderParser
proc TypeParser { array_name prefix } {
	upvar 1 $array_name a_name
    global MessageList
	if { $a_name($prefix.totallength) < 24 } {
		set a_name($prefix.typetext) BadHeader
		return
	}
	set type [lindex $MessageList $a_name($prefix.type)]
	if { $type == {} } {
		set a_name($prefix.typetext) Unknown
		return
	}
    set a_name($prefix.typetext) $type
}

#Parse header information and place in array
# works for request or response (though type is "ret" in response)
proc HeaderParser { array_name prefix string_value } {
	upvar 1 $array_name a_name
	set length [string length $string_value]
    foreach x {version payload type flags size offset typetext} {
		set a_name($prefix.$x) ""
	}
    foreach x {paylength tokenlength totallength ping} {
		set a_name($prefix.$x) 0
	}
    binary scan $string_value {IIIIII} a_name($prefix.version) a_name($prefix.payload) a_name($prefix.type) a_name($prefix.flags) a_name($prefix.size) a_name($prefix.offset)
	if { $length < 24 } {
		set a_name($prefix.totallength) $length
		set a_name($prefix.typetext) BadHeader
		return
	}
	if { $a_name($prefix.payload) == -1 } {
		set a_name($prefix.paylength) 0
		set a_name($prefix.ping) 1
	} else {
		set a_name($prefix.paylength) $a_name($prefix.payload)
	}
    set version $a_name($prefix.version)
    set flags $a_name($prefix.flags)
    set tok [expr { $version & 0xFFFF}]
    set ver [expr { $version >> 16}]
    set a_name($prefix.persist) [expr {($flags&0x04)?1:0}]
    set a_name($prefix.versiontext) "T$tok V$ver"
    set a_name($prefix.flagtext) [DetailFlags $flags]
    set a_name($prefix.tokenlength) [expr {$tok * 16} ]
    set a_name($prefix.totallength) [expr {$a_name($prefix.tokenlength)+$a_name($prefix.paylength)+24}]
}

# Request portion
proc RequestDetail { window_name cb_index } {
    global circ_buffer

    DetailRow $window_name yellow orange version payload type flags size offset

    # get request data
    HeaderParser q x $circ_buffer($cb_index.request)
    # request headers
    DetailRow $window_name white lightyellow $q(x.version) $q(x.payload) $q(x.type) $q(x.flags) $q(x.size) $q(x.offset)
# request headers
    if { [string length $circ_buffer($cb_index.request)] >= 24 } {
        TypeParser q x
        DetailRow $window_name white lightyellow $q(x.versiontext) $q(x.paylength) $q(x.typetext) $q(x.flagtext) $q(x.size) $q(x.offset)
        if { $q(x.paylength) > 0 } {
            switch $q(x.typetext) {
                "WRITE" { DetailPayloadPlus $window_name lightyellow white $circ_buffer($cb_index.request) $q(x.paylength) $q(x.size) }
                default { DetailPayload $window_name lightyellow $circ_buffer($cb_index.request) $q(x.paylength) }
            }
        }
        wm title $window_name "[wm title $window_name]: $q(x.typetext)"
    }
}

# Response portion
proc ResponseDetail { window_name cb_index } {
    global circ_buffer

    DetailRow $window_name #a6dcff #a6b1ff version payload return flags size offset

    # get response data
    set num $circ_buffer($cb_index.num)
    for {set i 0} {$i < $num} {incr i} {
        HeaderParser r x $circ_buffer($cb_index.response.$i)
        set offset $r(x.offset)
        DetailRowPlus $window_name white #EEEEFF $i $r(x.version) $r(x.payload) $r(x.type) $r(x.flags) $r(x.size) $offset
        if { [string length $circ_buffer($cb_index.response.$i)] >= 24 } {
            ErrorParser r x
            switch [$window_name.x22 cget -text] {
                "DIR"   -
                "DIRALL" {set offset [DetailOffset $offset]}
                "DIRALLSLASH" {set offset [DetailOffset $offset]}
            }
            DetailRow $window_name white #EEEEFF $r(x.versiontext) [expr {$r(x.ping)?"PING":$r(x.paylength)}] $r(x.return) $r(x.flagtext) $r(x.size) $offset
            DetailPayload $window_name #EEEEFF $circ_buffer($cb_index.response.$i) $r(x.paylength)
        }
    }
}

proc DetailPayload { window_name color full_string payload } {
    DetailText $window_name $color [string range $full_string 24 [expr {$payload + 24}] ]
}

proc DetailPayloadPlus { window_name color1 color2 full_string payload size } {
    set endpay [expr {24+$payload-$size-1}]
    DetailText $window_name $color1 [string range $full_string 24 $endpay ]
    incr endpay
    DetailText $window_name $color2 [string range $full_string $endpay [expr {$payload + 24}] ]
}

proc DetailText { window_name color text_string } {
    set row [lindex [grid size $window_name] 1]
    set columns [lindex [grid size $window_name] 0]
    label $window_name.t${row} -text $text_string -bg $color -relief ridge -wraplength 640 -justify left -anchor w
    grid  $window_name.t${row} -row $row -columnspan $columns -sticky news
}

# Standard detail row, one cell per value
proc DetailRow { window_name color1 color2 v0 v1 v2 v3 v4 v5 } {
    set row [lindex [grid size $window_name] 1]
    set w0 [label $window_name.x${row}0 -text $v0 -bg $color1]
    DetailRowRest $window_name $color1 $color2 $row $w0 $v1 $v2 $v3 $v4 $v5
}

# Augmented detail row, one cell per value, plus num in blue
proc DetailRowPlus { window_name color1 color2 num v0 v1 v2 v3 v4 v5 } {
    set row [lindex [grid size $window_name] 1]
    set w0 [frame $window_name.x${row}0 -bd 0 -relief flat]
    set n0 [label $w0.n -text $num -bg #a6b1ff -fg white]
    set m0 [label $w0.m -text $v0 -bg $color1]
    pack $n0 -side left -anchor w -fill none
    pack $m0 -side right -fill x -expand 1
    DetailRowRest $window_name $color1 $color2 $row $w0 $v1 $v2 $v3 $v4 $v5
}

# first element could be augmented by packet number
# adds row and widget for first cell instead of value
proc DetailRowRest { window_name color1 color2 row w0 v1 v2 v3 v4 v5 } {
    label $window_name.x${row}1 -text $v1 -bg $color2
    label $window_name.x${row}2 -text $v2 -bg $color1
    label $window_name.x${row}3 -text $v3 -bg $color2
    label $window_name.x${row}4 -text $v4 -bg $color1
    label $window_name.x${row}5 -text $v5 -bg $color2
    grid  $w0 $window_name.x${row}1 $window_name.x${row}2 $window_name.x${row}3 $window_name.x${row}4 $window_name.x${row}5 -row $row -sticky news
}

proc DetailReturn { array_name prefix } {
    upvar 1 $array_name a_name
    if { [info exists a_name($prefix.type)] } {
        set ret $a_name($prefix.type)
    } else {
        return "ESHORT"
    }
    if { $ret >= 0 } { return "OK" }
    switch -- $ret {
        -1      { return "EPERM"}
        -2      { return "ENOENT"}
        -5      { return "EIO"}
        -12     { return "ENOMEM"}
        -14     { return "EFAULT"}
        -19     { return "ENODEV"}
        -20     { return "ENOTDIR"}
        -21     { return "EISDIR"}
        -22     { return "EINVAL"}
        -34     { return "ERANGE"}
        -42     { return "ENOMSG"}
        default { return "ERROR"}
    }
}

proc DetailOffset { offset } {
    return [expr {$offset&0x0001?"resume ":""}][expr {$offset&0x0002?"alarm ":""}][expr {$offset&0x0004?"ovdr ":""}][expr {$offset&0x8000?"temp ":""}][expr {$offset&0x4000?"volt ":""}][expr {$offset&0x2000?"chain ":""}]
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
    return $T$F[expr {$flags&0x10?" safe":""}][expr {$flags&0x08?" alias":""}][expr {$flags&0x04?" persist":""}][expr {$flags&0x02?" bus":""}][expr {$flags&0x01?" cache":""}]
}

proc WebResponse { sock } {
    set R [lindex {$Revision$} 1]
    fconfigure $sock -buffering full -translation crlf -blocking 0
    puts $sock "HTTP/1.0 400 Bad Request"
    puts $sock "Date: [clock format [clock seconds] -gmt 1]"
    puts $sock "Server: OWTAP-$R"
    puts $sock "Last-Modified: [clock format [clock seconds] -gmt 1]"
    puts $sock "Content-Type: text/html"
    puts $sock ""
    puts $sock "<HTML><HEAD><TITLE>OWTAP-$R owserver protocol inspector</TITLE></HEAD>"
    puts $sock "<BODY><P>Attempt to access OWTAP directly. You probably meant to access OWHTTPD, the 1-wire web server.</P>"
    puts $sock "<P>For more information see <A HREF=http://www.owfs.org>OWFS website</A> or the <A HREF=http://sourceforge.net/projects/owfs/>Sourceforge site.</A?</P?</BODY></HTML>"
    flush $sock
}

# socket name in readable format
proc PrettySock { sock } {
    set socklist [fconfigure $sock -sockname]
    return [lindex $socklist 1]:[lindex $socklist 2]
}
proc PrettyPeer { sock } {
    set socklist [fconfigure $sock -peername]
    return [lindex $socklist 1]:[lindex $socklist 2]
}

proc MainTitle { tap server } {
    wm title . "OWTAP ($tap) tap of owserver ($server)"
}

proc Restart { } {
#   foreach ch [chan names] { close $ch }
    foreach channel [file channels] {
        if { [regexp -- {^std(out|in|err)$} $channel ] == 0 } {
            # change to non-blocking and close
            if { [ catch {
                fconfigure $channel -blocking 0 ;
                close $channel;
} reason ] == 1 } {
            StatusMessage "Error closing channel $channel $reason" 1
            }
        }
    }
#    exec [info nameofexecutable] $::argv0 "--" {*}$::argv &
    if { [info nameofexecutable] eq $::argv0 } {
        eval exec [list [info nameofexecutable]] "--" $::argv &
    } else {
        eval exec [list [info nameofexecutable]] [list $::argv0] "--" $::argv "&"
    }
    exit
}

#Finally, all the Proc-s have been defined, so run everything.
Main $argv
