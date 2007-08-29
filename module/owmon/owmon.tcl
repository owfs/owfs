#!/bin/sh
# the next line restarts using wish \
exec wish "$0" -- "$@"

# $Id$

# Global: IPAddress() loose tap server
# -- port number of this program (tap) and real owserver (server). loose is stray garbage

set SocketVars {string version type payload size sg offset tokenlength totallength paylength ping state persist id }

set MessageType(READ)   2
set MessageType(DIR)    4
set MessageType(DIRALL) 7
set MessageType(PreferredDIR) $MessageType(DIRALL)

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
    ArgumentProcess $argv
    global MessageType

    SetupDisplay
	SetBusList
    SetStatList "/statistics"
    StatValues
}

proc Busser { path } {
	global dirlist
	global buslist
    if { [OWSERVER_DirectoryRead $path] < 0 } {
		return
	}
	set busses [lsearch -all -regexp -inline $dirlist "^${path}bus"]
puts "$dirlist -> $busses"
	foreach bus $busses {
        lappend buslist $bus
		Busser $bus
	}
}

proc SetBusList { } {
	global buslist
	set buslist {"/"}
	Busser "/"
}

proc Statpather { prepath_length path } {
    global statpath
    global statname
    global dirlist
    if { [OWSERVER_DirectoryRead $path] < 0 } {
        lappend statpath $path
        lappend statname [string range $path [expr {$prepath_length + 1 }] end]
        return
    }
    set stats [lsearch -all -regexp -inline -not $dirlist {\.ALL$} ]
    foreach stat $stats {
        Statpather $prepath_length $stat
    }
}

proc SetStatList { path } {
    global statpath
    global statname

    set statpath {}
    set statname {}
    Statpather [string length $path] $path
}

proc StatValues { } {
    global statpath
    global statvalue
    global readvalue
    global MessageType

    set statvalues {}
    foreach stat $statpath {
        set readvalue " "
        OWSERVER_Read $MessageType(READ) $stat
        lappend statvalue $readvalue
    }

global statname
foreach n $statname v $statvalue {
puts "$v $n"
}
}

proc OWSERVER_send_message { type path sock } {
    set length [string length $path]
# add an entra char for null-termination string
    incr length
    puts -nonewline $sock [binary format "IIIIIIa*a" 0 $length $type 258 1024 0 $path \0 ]
    flush $sock
}


# Command line processing
# looks at command line arguments
# options "s" for the target ow_s_erver port
# Can handle command lines like
# -s 3001
# -s3001
# etc
proc ArgumentProcess { arg } {
    global IPAddress
# INADDR_ANY
    set IPAddress(ip) "0.0.0.0"
# default port
    set IPAddress(port) "4304"
    foreach a $arg {
        if { [regexp -- {^-s(.sock*)$} $a whole address] } {
            IPandPort $address
        }  else {
            IPandPort $a
        }
    }
    MainTitle $IPAddress(ip):$IPAddress(port)
}

proc IPandPort { argument_string } {
    global IPAddress
    if { [regexp -- {^(.*?):(.*)$} $argument_string wholestring firstpart secondpart] } {
        if { $firstpart != "" } { set IPAddress(ip) $firstpart }
        if { $secondpart != "" } { set IPAddress(port) $secondpart }
    } else {
        if { $argument_string != "" } { set IPAddress(port) $argument_string }
    }
}

# open connection to owserver
# return sock, or "" if unsuccessful
proc OpenOwserver { } {
    global IPAddress
    # StatusMessage "Attempting to open connection to OWSERVER" 0
    if {[catch {socket $IPAddress(ip) $IPAddress(port)} sock] } {
        return {}
    } else {
        return sock
    }
}

proc OWSERVER_DirectoryRead { path } {
    global MessageType
    set return_code [OWSERVER_Read $MessageType(PreferredDIR) $path]
	if { $return_code == -42 && $MessageType(PreferredDIR)==$MessageType(DIRALL) } {
		set MessageType(PreferredDIR) $MessageType(DIR)
		set return_code [OWSERVER_Read $MessageType(PreferredDIR) $path]
	}
	return $return_code
}

#Main loop. Called whenever the server (listen) port accepts a connection.
proc OWSERVER_Read { message_type path } {
    global serve
    global dirlist
    global MessageType

# Start the State machine
    set serve(state) "Open server"
    set dirlist {}
    set error_status 0
    while {1} {
    # puts $serve(state)
        switch $serve(state) {
        "Open server" {
            global IPAddress
            StatusMessage "Attempting to open connection to OWSERVER" 0
            if {[catch {socket $IPAddress(ip) $IPAddress(port)} sock] } {
                set serve(state) "Unopened server"
            } else {
                set serve(state) "Send to server"
            }
        }
        "Unopened server" {
            StatusMessage "OWSERVER error: $sock at $IPAddress(ip):$IPAddress(port)"
            set serve(state) "Server error"
        }
        "Send to server" {
            StatusMessage "Sending client request to OWSERVER" 0
            set serve(sock) $sock
            fconfigure $sock -translation binary -buffering full -encoding binary -blocking 0
            OWSERVER_send_message $message_type $path $sock
            set serve(state) "Read from server"
        }
        "Read from server" {
            StatusMessage "Reading OWSERVER response" 0
            TapSetup
            ResetSockTimer
            fileevent $sock readable [list OWSERVER_Process]
            vwait serve(state)
        }
        "Server early end" {
            StatusMessage "FAILURE reading OWSERVER response" 0
            set serve(state) "Server error"
        }
        "Process server packet" {
            StatusMessage "Success reading OWSERVER response" 0
            fileevent $sock readable {}
            set serve(state) "Packet complete"
        }
        "Packet complete" {
            ClearSockTimer
            # filter out the multi-response types and continue listening
            if { $serve(ping) == 1 } {
                set serve(state) "Ping received"
            } elseif { $serve(type) < 0 } {
                # error -- return it (may help find non-DIRALL servers)
                set error_status $serve(type)
                set serve(state) "Done with server"
            } elseif { $message_type==$MessageType(DIRALL) } {
                set serve(state) "Dirall received"
            } elseif { $message_type==$MessageType(READ) } {
                set serve(state) "Read received"
            } else {
                set serve(state) "Dir element received"
            }
        }
        "Ping received" {
            set serve(state) "Read from server"
        }
        "Dir element received" {
            if ( $serve(paylength)==0 ) {
                # last (null) element
                set serve(state) "Done with server"
            } else {
                # add element to list
                lappend dirlist [Payload]
                set serve(state) "Read from server"
            }
        }
        "Dirall received" {
            set dirlist [split [Payload] ,]
            set serve(state) "Done with server"
        }
        "Read received" {
            global readvalue
            set readvalue [Payload]
            set serve(state) "Done with server"
        }
        "Server timeout" {
            StatusMessage "owserver read timeout" 1
            set serve(state) "Server error"
        }
        "Server error" {
            set error_return 1
            StatusMessage "Error code $error_return" 1
            set serve(state) "Done with server"
        }
        "Done with server"  {
            CloseSock
            set serve(state) "Done with all"
        }
        "Done with all" {
            StatusMessage "Ready" 0
            return $error_status
        }
        default {
            StatusMessage "Internal error -- bad state: $serve($sock.state)" 1
            return $error_status
        }
        }
    }
}

# Initialize array for client request
proc TapSetup { } {
    global serve
    set serve(string) ""
    set serve(totallength) 0
}

# Clear out client request array after a connection (frees memory)
proc ClearTap { } {
    global serve
    global SocketVars
    foreach x $SocketVars {
        if { [info exist serve($x)] } {
            unset serve($x)
}
}
}

# close client request socket
proc SockTimeout {} {
    global serve
    switch $serve(state) {
        "Read from server" {
            set serve(state) "Server timeout"
}
        default {
            ErrorMessage "Strange timeout for state=$serve(state)"
            set serve(state) "Server timeout"
}
}
    StatusMessage "Network read timeout" 1
}

# close client request socket
proc CloseSock {} {
    global serve
    ClearSockTimer
    if { [info exists serve(sock)] } {
        close $serve(sock)
        unset serve(sock)
    }
    ClearTap
}

proc ClearSockTimer { } {
    global serve
    if { [info exist serve(id)] } {
        after cancel $serve(id)
        unset serve(id)
}
}

proc ResetSockTimer { { msec 2000 } } {
    global serve
    ClearSockTimer
    set serve(id) [after $msec [list SockTimeout]]
}

# Process a oncomming owserver packet, adjusting size from header information
proc ReadProcess {} {
    global serve
    set sock $serve(sock)
# test eof
    if { [eof $sock] } {
        return 2
}
# read what's waiting
    set new_string [read $sock]
    if { $new_string == {} } {
        return 0
}
    append serve(string) $new_string
    ResetSockTimer
    set len [string length $serve(string)]
    if { $len < 24 } {
#do nothing -- reloop
        return 0
} elseif { $serve(totallength) == 0 } {
# at least header is in
        HeaderParser $serve(string)
}
#already in payload (and token) portion
    if { $len < $serve(totallength) } {
#do nothing -- reloop
        return 0
}
# Fully parsed
    set new_length [string length $serve(string)]
    return 1
}

# Wrapper for processing -- either change a vwait var, or just return waiting for more network traffic
proc OWSERVER_Process {  } {
    global serve
    set read_value [ReadProcess]
#puts "Current length [string length $serve($relay.string)] return val=$read_value"
    switch $read_value {
        3  - 
        2  { set serve(state) "Server early end"}
        0  { return }
        1  { set serve(state) "Process server packet" }
}
}

# Debugging routine -- show all the packet info
proc ShowMessage {} {
    global serve
    global SocketVars
    foreach x $SocketVars {
        if { [info exist serve($x)] } {
            puts "\t$x = $serve($x)"
}
}
}

# Selection from listbox
proc SelectionMade { widget y } {
    set index [ $widget nearest $y ]
    if { $index >= 0 } {
        ShowBus [$widget get $index]
}
}


proc SetupDisplay {} {
    global buslist
    global display_text
    panedwindow .main -orient horizontal
    
    # Bus list is a listbox
    set f_bus [frame .main.bus]
    listbox $f_bus.lb -listvariable buslist -width 20 -yscrollcommand [list $f_bus.sb set] -selectmode browse -bg lightyellow
    scrollbar $f_bus.sb -command [list $f_bus.lb yview]
    pack $f_bus.sb -side right -fill y
    pack $f_bus.lb -side left -fill both -expand true
    bind $f_bus.lb <ButtonRelease-1> {+ SelectionMade %W %y }
    bind $f_bus.lb <space> {+ SelectionMade %W }
    
    # statistics information is a textbox
    set f_stats [frame .main.stats]
    set display_text(stats) [
        text $f_stats.text \
            -yscrollcommand [list $f_stats.sby set] \
            -xscrollcommand [list $f_stats.sbx set] \
            -tabs {12 right 15 20 25 30 35} \
            -bg white
        ]
    scrollbar $f_stats.sby -command [list $f_stats.text yview]
    scrollbar $f_stats.sbx -command [list $f_stats.text xview] -orient horizontal
    pack $f_stats.sby -side right -fill y
    pack $f_stats.sbx -side bottom -fill x
    pack $f_stats.text -side left -fill both -expand true
    
    
    # system information is a textbox
    set f_system [frame .main.system]
    set display_text(system) [
        text $f_system.text \
            -yscrollcommand [list $f_system.sby set] \
            -xscrollcommand [list $f_system.sbx set] \
            -bg lightblue
        ]
    scrollbar $f_system.sby -command [list $f_system.text yview]
    scrollbar $f_system.sbx -command [list $f_system.text xview] -orient horizontal
    pack $f_system.sby -side right -fill y
    pack $f_system.sbx -side bottom -fill x
    pack $f_system.text -side left -fill both -expand true

    .main add .main.bus .main.system .main.stats
    
    label .status -anchor w -width 80 -relief sunken -height 1 -textvariable current_status -bg white
    pack .status -side bottom -fill x
    bind .status <ButtonRelease-1> [list .main_menu.view invoke "Status messages"]
    pack .main -side top -fill both -expand true
    
    menu .main_menu -tearoff 0
    . config -menu .main_menu
    menu .main_menu.view -tearoff 0
    .main_menu add cascade -label View -menu .main_menu.view  -underline 0
        .main_menu.view add checkbutton -label "Status messages" -underline 0 -indicatoron 1 -command {StatusWindow}

# help menu
    menu .main_menu.help -tearoff 0
    .main_menu add cascade -label Help -menu .main_menu.help  -underline 0
        .main_menu.help add command -label "About OWMON" -underline 0 -command About
        .main_menu.help add command -label "Command Line" -underline 0 -command CommandLine
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

# Popup giving attribution
proc About { } {
    tk_messageBox -type ok -title {About owmon} -message {
Program: owmon
Synopsis: owserver statistics inspector

Description: owmon displays
  the internal statistics kept
  by owserver.

  The information can help diagnosing
  and predicting hardware problems.

Author: Paul H Alfille <paul.alfille@gmail.com>

Copyright: Aug 2007 GPL 2.0 license

Website: http://www.owfs.org
}
}

# Popup giving commandline
proc CommandLine { } {
    tk_messageBox -type ok -title {owmon command line} -message {
syntax: owmon.tcl -s serverport 

  server port is the address of owserver

  owserver address can be in forms
  host:port
  :port
  port

  default is localhost:4304
}
}

# Popup giving version
proc Version { } {
    regsub -all {[$:a-zA-Z]} {$Revision$} {} Version
    regsub -all {[$:a-zA-Z]} {$Date$} {} Date
    tk_messageBox -type ok -title {owmon version} -message "
Version $Version
Date    $Date
    "
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

#Parse header information and place in array
# works for request or response (though type is "ret" in response)
proc HeaderParser { string_value } {
    global serve
    set length [string length $string_value]
    foreach x {version payload type flags size offset} {
        set serve($x) ""
}
    foreach x {paylength tokenlength totallength ping} {
        set serve($x) 0
}
    binary scan $string_value {IIIIII} serve(version) serve(payload) serve(type) serve(flags) serve(size) serve(offset)
    if { $length < 24 } {
        set serve(totallength) $length
        set serve(typetext) BadHeader
        return
}
    if { $serve(payload) == -1 } {
        set serve(paylength) 0
        set serve(ping) 1
} else {
        set serve(paylength) $serve(payload)
}
    set version $serve(version)
    set flags $serve(flags)
    set tok [expr { $version & 0xFFFF}]
    set ver [expr { $version >> 16}]
    set serve(persist) [expr {($flags&0x04)?1:0}]
    set serve(tokenlength) [expr {$tok * 16} ]
    set serve(totallength) [expr {$serve(tokenlength)+$serve(paylength)+24}]
}

proc Payload {} {
    global serve
    # remove trailing null
    string range $serve(string) 24 [expr {$serve(paylength) + 24 - 1}]
}

proc MainTitle { server_address } {
    wm title . "OWSERVER ($server_address) Monitor"
}

#Finally, all the Proc-s have been defined, so run everything.
Main $argv
