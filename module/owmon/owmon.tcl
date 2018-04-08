#!/bin/sh
# the next line restarts using wish \
if [ -z `which wish` ] ; then exec tclsh "$0" -- "$@" ; else exec wish "$0" -- "$@" ; fi

package require Tk

# directories to monitor
set PossiblePanels {system statistics structure settings interface}


set SocketVars {string version type payload size sg offset tokenlength totallength paylength ping state id }

# owserver message types
set MessageTypes(READ)   2
set MessageTypes(DIR)    4
set MessageTypes(DIRALL) 7
set MessageTypes(PreferredDIR) $MessageTypes(DIRALL)

set refresh_counter 0

# Global: window_exists => For object-oriented initialization

# Global: serve => information on current transaction
#         serve(string) -- message to this point
#         serve(version size payload sg offset type tokenlength ) -- message parsed parts

# Global: stats => statistical counts and flags for stats windows

#Main procedure. We actually start it at the end, to allow Proc-s to be defined first.
proc Main { } {
    ArgumentProcess
    global IsPanelVisible
    global PossiblePanels
    global DisplayLock
    global PanelDataField
	global SelectedBus
    global RefreshFrequency   

    # wait till ready
    set DisplayLock 1

    # Set panels to display at start
    foreach panel $PossiblePanels {
        #default
        set IsPanelVisible($panel) 0
        # refresh rate
        set RefreshFrequency($panel) 0
    }
    
    set IsPanelVisible(system) 1
    set IsPanelVisible(statistics) 1
    set IsPanelVisible(interface) 1

    # Show screen
    SetupDisplay
    
    # bus list (and default)
    set SelectedBus ""
	SetBusList
    $PanelDataField(bus) selection set 0 0

    # Show data for first time
    set DisplayLock 0
    foreach panel $PossiblePanels {
    	DirListValues $panel
	}
    
    # Start clock
    RefreshCounter
}

# Create the list of bus-es by doing recursive directory searches and selecting only bus.x entries
# the list is put in DiscoveredBusList
# No trailing "/"
proc SetBusList { } {
	global DiscoveredBusList
    global PanelDataField

    # disable until data is complete
    $PanelDataField(bus) configure -state disabled
	
    set DiscoveredBusList "<root>"
	BusListRecurser ""

# enable now
    $PanelDataField(bus) configure -state normal
}

# path is searched and added. No trailing "/"
proc BusListRecurser { path } {
	global DiscoveredBusList
    
    set value_from_owserver [OWSERVER_DirectoryRead "$path/"]
    if { [lindex $value_from_owserver 0] < 0 } {
        return
    }
    set busses [ lsearch -all -regexp -inline $value_from_owserver {/bus\.\d+$} ]
    foreach bus $busses {
        lappend DiscoveredBusList $bus
        BusListRecurser $bus
    }
}

proc SetDirList { bus type } {
    global data_array

    set path $bus/$type
    set data_array($bus.$type.path) {}
    set data_array($bus.$type.name) {}
    DirListRecurser  $bus $path $type
    set data_array($bus.$type.path) [lrange $data_array($bus.$type.path) 1 end]
    set data_array($bus.$type.name) [lrange $data_array($bus.$type.name) 1 end]
}

proc DirListRecurser { bus path type } {
    global data_array
    lappend data_array($bus.$type.name) [regsub -all .*?/ [string range $path [string length $bus/$type] end] \t]
    
    set value_from_owserver [OWSERVER_DirectoryRead $path]
    if { [lindex $value_from_owserver 0] < 0 } {
        #puts "F $bus $path"
        lappend data_array($bus.$type.path) $path
    } else {
        #puts "D $bus $path"
        lappend data_array($bus.$type.path) NULL
        set dirlist [lsearch -all -regexp -inline -not [lrange $value_from_owserver 1 end] {\.ALL$|\.\d{3,}$} ]
        foreach dir $dirlist {
            DirListRecurser $bus $dir $type
        }
    }
}

# Get all the data values for a given bus/type
proc DirListValues { type } {
    global IsPanelVisible
    global DisplayLock
	global SelectedBus
    global PanelDataField
    global data_array
    global MessageTypes

    # See if this is a displayed panel
    if { $IsPanelVisible($type) == 0 } {
        return
    }

    # prevent stacked refresh
    if { $DisplayLock } {
        return
    }
    
    set DisplayLock 1
    
    if { ![info exists data_array($SelectedBus.$type.path)] } {
        $PanelDataField($type) delete 1.0 end
        $PanelDataField($type) insert end "First pass through $SelectedBus/$type\n  ... getting parameter list...\n"
        #puts "bus=$bus type=$type"
        SetDirList $SelectedBus $type
    }

    $PanelDataField($type) delete 1.0 end
    foreach path $data_array($SelectedBus.$type.path) name $data_array($SelectedBus.$type.name) {
        if { $path != "NULL" } {
            set value_from_owserver [OWSERVER_Read $MessageTypes(READ) $path]
            set change_tag $type.black
            if {[lindex $value_from_owserver 0] < 0 } {
                set value_from_owserver ""
            } else {
                set value_from_owserver [lindex $value_from_owserver 1]
            }
            if { ![info exists data_array($SelectedBus.$type.$path)] } {
                set data_array($SelectedBus.$type.$path) $value_from_owserver
            } elseif { $data_array($SelectedBus.$type.$path) != $value_from_owserver } {
                set data_array($SelectedBus.$type.$path) $value_from_owserver
                set change_tag $type.red
            } elseif { $PanelDataField($type.diff) } {
                continue
            }
            $PanelDataField($type) insert end "[format {%12.12s} $value_from_owserver]   $name\n" $change_tag
        } else {
                set value_from_owserver ""
                $PanelDataField($type) insert end "               " $type.spacing "$name\n" $type.underline
        }
    }

    set DisplayLock 0
}

proc OWSERVER_send_message { type path sock } {
    set length [string length $path]
# add an entra char for null-termination string
    incr length
# 262= f.i, busret and persist flags
	if { [catch {puts -nonewline $sock [binary format "IIIIIIa*a" 0 $length $type 262 1024 0 $path \0 ] }] } {
		return 1
	} 
    flush $sock
	return 0
}

# Command line processing
# looks at command line arguments
# options "s" for the target ow_s_erver port
# Can handle command lines like
# -s 3001
# -s3001
# etc
proc ArgumentProcess { } {
    global IPAddress
# INADDR_ANY
    set IPAddress(ip) "0.0.0.0"
# default port
    set IPAddress(port) "4304"
    foreach a $::argv {
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
        if { $firstpart != "" } {
            set IPAddress(ip) $firstpart
        }
        if { $secondpart != "" } {
            set IPAddress(port) $secondpart
        }
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
    global MessageTypes

    set return_code [OWSERVER_Read $MessageTypes(PreferredDIR) $path]
    if { $return_code == -42 && $MessageTypes(PreferredDIR) == $dMessageTypes(DIRALL) } {
		set MessageTypes(PreferredDIR) $MessageTypes(DIR)
		set return_code [OWSERVER_Read $MessageTypes(PreferredDIR) $path]
	}
	return $return_code
}

#Main loop. Called whenever the server (listen) port accepts a connection.
proc OWSERVER_Read { message_type path } {
    global serve

# Start the State machine
    set serve(state) "Open server"
    set value_from_owserver {}
    set error_status 0
    while {1} {
        #puts "State machine = $serve(state) message_type=$message_type path=$path"
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
            if { [OWSERVER_send_message $message_type $path $sock]==0 } {
	            set serve(state) "Read from server"
			} elseif { [info exist serve(persist)] && $serve(persist)==1 } {
				CloseSock
				set serve(persist) 0
	            set serve(state) "Open server"
			} else {
	            set serve(state) "Server wont listen"
			}
        }
		"Persistent server" {
			set sock $serve(sock)
		}
        "Read from server" {
            StatusMessage "Reading OWSERVER response" 0
            TapSetup
            ResetSockTimer
            fileevent $sock readable [list OWSERVER_Process]
            vwait serve(state)
        }
        "Server wont listen" {
            StatusMessage "FAILURE sending to OWSERVER" 0
            set serve(state) "Server error"
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
            global MessageTypes
            if { $serve(ping) == 1 } {
                set serve(state) "Ping received"
            } elseif { $serve(type) < 0 } {
                # error -- return it (may help find non-DIRALL servers)
                set error_status $serve(type)
                set serve(state) "Done with server"
            } elseif { $message_type == $MessageTypes(DIRALL) } {
                set serve(state) "Dirall received"
            } elseif { $message_type == $MessageTypes(READ) } {
                set serve(state) "Read received"
            } else {
                set serve(state) "Dir element received"
            }
        }
        "Ping received" {
            set serve(state) "Read from server"
        }
        "Dir element received" {
            if { $serve(paylength)==0 } {
                # last (null) element
                set serve(state) "Done with server"
            } else {
                # add element to list
                lappend value_from_owserver [PayloadWithNULL]
                set serve(state) "Read from server"
            }
        }
        "Dirall received" {
            set value_from_owserver [split [PayloadWithNULL] ,]
            set serve(state) "Done with server"
        }
        "Read received" {
            lappend value_from_owserver [PayloadNoNULL]
            set serve(state) "Done with server"
        }
        "Server timeout" {
            ClearSockTimer
            StatusMessage "owserver read timeout" 1
            set serve(state) "Server error"
        }
        "Server error" {
            set error_return 1
            StatusMessage "Error code $error_return" 1
			set serve(persist) 0
            set serve(state) "Done with server"
        }
        "Done with server"  {
			if { [info exist serve(persist)] && $serve(persist)==1 } {
	            set serve(state) "Server persist"
			} else {
	            set serve(state) "Server close"
			}
        }
		"Server persist" {
            ClearTap
			ClearSockTimer
            set serve(state) "Done with all"
		}
		"Server close" {
            CloseSock
            set serve(state) "Done with all"
		}
        "Done with all" {
            StatusMessage "Ready" 0
            set serve(state) "Return"
        }
        "Return" {
            return [linsert $value_from_owserver 0 $error_status]
        }
        default {
            StatusMessage "Internal error -- bad state: $serve($sock.state)" 1
            set serve(state) "Return"
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

# Selection from bus listbox (left panel)
proc SelectionMade { widget y } {
    global PossiblePanels
    global PanelDataField
	global SelectedBus

    global data_array
    catch { unset data_array }
    
    set index [ $widget nearest $y ]
    if { $index >= 0 } {
        set bus [$widget get $index]
        set SelectedBus [expr { ($bus eq "<root>") ? "" : $bus } ]
        foreach panel $PossiblePanels {
            DirListValues $panel
        }
    }
}

proc SetupDisplay {} {
    global IsPanelVisible
    global PossiblePanels
	global DiscoveredBusList
    global RefreshFrequency
    global PanelDataField

    panedwindow .main -orient horizontal
    
    # Bus list is a listbox
    set f_bus [frame .main.bus]
    set color [Color bus]
    set PanelDataField(bus) [
        listbox $f_bus.lb \
            -listvariable DiscoveredBusList \
            -width 20 \
            -yscrollcommand [list $f_bus.sb set] \
            -selectmode browse \
            -bg $color
        ]
    scrollbar $f_bus.sb -command [list $f_bus.lb yview] -troughcolor $color
    pack $f_bus.sb -side right -fill y
    pack $f_bus.lb -side left -fill both -expand true
    bind $PanelDataField(bus) <ButtonRelease-1> {+ SelectionMade %W %y }
    bind $PanelDataField(bus) <space> {+ SelectionMade %W }
    
    .main add .main.bus

    foreach dir $PossiblePanels {
        SetupPanel $dir
    }
    
    label .status -anchor w -relief sunken -height 1 -textvariable current_status -bg white
    pack .status -side bottom -fill x
    bind .status <ButtonRelease-1> [list .main_menu.view invoke "Status messages"]
    pack .main -side top -fill both -expand true
    
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

    menu .main_menu.view -tearoff 0
    .main_menu add cascade -label View -menu .main_menu.view  -underline 0
        foreach dir $PossiblePanels {
            .main_menu.view add checkbutton -label $dir -indicatoron 1 -command [list PanelShow $dir] -variable IsPanelVisible($dir)
        }
        .main_menu.view add separator
        .main_menu.view add checkbutton -label "Status messages" -underline 0 -indicatoron 1 -command {StatusWindow}

    foreach panel $PossiblePanels {
        menu .main_menu.$panel -tearoff 0
        .main_menu add cascade -label $panel -menu .main_menu.$panel -state [expr {$IsPanelVisible($panel)?"normal":"disabled"}]
        menu .main_menu.$panel.refresh -tearoff 0
        .main_menu.$panel add cascade -menu .main_menu.$panel.refresh -label {Refresh rate}
        foreach {label value} {"Manual" 0 "20 seconds" 1 "1 minute" 3 "5 minutes" 15} {
            .main_menu.$panel.refresh add radiobutton -label $label -value $value -variable RefreshFrequency($panel)
        }
        .main_menu.$panel add checkbutton -label "Differences only" -underline 0 -variable PanelDataField($panel.diff)
}

# help menu
    menu .main_menu.help -tearoff 0
    .main_menu add cascade -label Help -menu .main_menu.help  -underline 0
        .main_menu.help add command -label "About OWMON" -underline 0 -command About
        .main_menu.help add command -label "Command Line" -underline 0 -command CommandLine
        .main_menu.help add command -label "Version" -underline 0 -command Version
}

proc SetupPanel { panel } {
    global IsPanelVisible
    global PanelFrame
    global PanelDataField
    
    set color [Color $panel]
    if { $IsPanelVisible($panel) } {
        set f [frame .main.$panel]
        
        # top bar
        frame $f.top
        button $f.top.b -text $panel -command [list DirListValues $panel] -bg $color
        checkbutton $f.top.d -text "∆" -command [list DirListValues $panel] -variable PanelDataField($panel.diff) -bg $color
        button $f.top.x -text "X" -command [list .main_menu.view invoke $panel] -bg $color
        pack $f.top.x -side right
        pack $f.top.d -side right
        pack $f.top.b -side left -fill x -expand true

        # text and scroll bars
        set PanelDataField($panel) [
            text $f.text \
                -yscrollcommand [list $f.sby set] \
                -xscrollcommand [list $f.sbx set] \
                -tabs {12 right 15 20 25} \
                -wrap none \
                -width 40 \
                -bg $color
            ]
        scrollbar $f.sby -command [list $f.text yview] -troughcolor $color
        scrollbar $f.sbx -command [list $f.text xview] -orient horizontal -troughcolor $color
        pack $f.top -side top -fill x
        pack $f.sby -side right -fill y
        pack $f.sbx -side bottom -fill x
        pack $f.text -side left -fill both -expand true
    
        set PanelFrame($panel) $f
        .main add $f -after .main.[PriorPanel $panel]

        $PanelDataField($panel) tag configure $panel.red       -foreground red
        $PanelDataField($panel) tag configure $panel.black     -foreground black
        $PanelDataField($panel) tag configure $panel.spacing   -spacing1 5 -spacing3 3
        $PanelDataField($panel) tag configure $panel.underline -underline 1

        set PanelDataField($panel.diff) 0
    }
}

proc PanelShow { panel } {
    global IsPanelVisible
    global PanelFrame

    if { $IsPanelVisible($panel) } {
        .main_menu entryconfigure $panel -state normal

        if { [info exist PanelFrame($panel)] } {
            .main add .main.$panel -after .main.[PriorPanel $panel]
        } else {
            SetupPanel $panel
        }
        
        DirListValues $panel
    } else {
        .main_menu entryconfigure $panel -state disabled
        .main forget $PanelFrame($panel)
    }
}

proc PriorPanel { dir } {
    global IsPanelVisible
    global PossiblePanels
    
    set prior bus

    foreach prior_dir $PossiblePanels {
        if { $prior_dir==$dir } {
            break
        }
        if { $IsPanelVisible($prior_dir) } {
            set prior $prior_dir
        }
    }
    return $prior
}

proc Color { dir } {
    switch $dir {
    bus         { return #D6E2E0}
    system      { return #DBF0FF}
    statistics  { return #E2FFF1}
    alarm       { return #FFC3C3}
    structure   { return #F3DEFF}
    settings    { return #FCFFDE}
    chain       { return #DDDFC2}
    simultaneous { return #C3EEF9}
    interface   { return #FFF6C8}
    default     { return #C8C8C8}
    }
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
    global window_exists

    if { [ info exist window_exists($window_name) ] } {
        if { $window_exists($window_name) } {
# hide window
            wm withdraw $window_name
            set window_exists($window_name) 0
        } else {
# show window
            wm deiconify $window_name
            set window_exists($window_name) 1
        }
        return 1
    }

# create window
    toplevel $window_name
    wm title $window_name $menu_index
# delete handler
    wm protocol $window_name WM_DELETE_WINDOW [list $menu_name invoke $menu_index]
# now set flag
    set window_exists($window_name) 1
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

proc PayloadWithNULL {} {
    global serve
    # remove trailing null
    string range $serve(string) 24 [expr {$serve(paylength) + 24 - 2}]
}

proc PayloadNoNULL {} {
    global serve
    # remove trailing null
    string range $serve(string) 24 [expr {$serve(paylength) + 24 - 1}]
}

proc RefreshCounter { } {
    global refresh_counter
    global PossiblePanels
    global RefreshFrequency
    
    incr refresh_counter
    foreach panel $PossiblePanels {
        if { $RefreshFrequency($panel)==0 } {
            continue
        }
        if { [expr { $refresh_counter % $RefreshFrequency($panel) }]==0 } {
            DirListValues $panel
        }
    }

    # set timer again
    after 20000 RefreshCounter
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

proc MainTitle { server_address } {
    wm title . "OWMON owserver ($server_address) monitor"
}

#Finally, all the Proc-s have been defined, so run everything.
Main
