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
########## Overall main window setup ######################
###########################################################

proc SetupDisplay { fram } {
    global serve
    set serve(paned) [panedwindow $fram.paned -orient vertical]

    set serve(panet) [frame $serve(paned).top -padx 5 -pady 5 -bg #996633]
    StatusFrame $serve(panet)

    set serve(paneb) [frame $serve(paned).bottom -padx 5 -pady 5 -bg #996633]
    SetupPanels $serve(paneb)

    $serve(paned) add $serve(panet) $serve(paneb)
    pack $serve(paned) -fill both -expand true
}

###########################################################
########## Status bar / messages panel ####################
###########################################################

proc StatusFrame { fram } {
    global serve

    set serve(text) [text $fram.text -relief ridge -yscrollcommand "$fram.scroll set" -state normal -wrap char -takefocus 0 -setgrid 1 -height 7 -bg #330000]
    scrollbar $fram.scroll -command "$fram.text yview"
    pack $fram.scroll -side right -fill y
    pack $fram.text -side left -fill both -expand true
    # tags
    $serve(text) tag configure listen -foreground black -background yellow
    $serve(text) tag configure accept -foreground blue -background yellow
    $serve(text) tag configure read -foreground white
    $serve(text) tag configure write -foreground yellow
}

###########################################################
########## Status bar / messages panel ####################
###########################################################

proc SetupPanels { pane } {
    global chip
    global devname
    global serve

    set serve(panel) [labelframe $pane.left -labelanchor nw -text Devices -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #996633 -fg white]
    pack $serve(panel) -side left -expand false -anchor nw
    set serve(paner) [labelframe $pane.right -labelanchor nw -relief ridge -borderwidth 3 -padx 5 -pady 5 -bg #996633 -fg white]
    pack $serve(paner) -fill x

    foreach dev $devname {
        set addr $chip($dev)
        radiobutton $serve(panel).$addr -variable serve(selected) -text $dev -value $addr -anchor nw
        pack $serve(panel).$addr -side top -fill x
    }

    trace add variable serve(selected) {write} Raise

    # Now set to first
    set serve(lastselected) ""
    set serve(selected) $chip([lindex $devname 0])
}

proc Raise {ar in op} {
    global chip
    global serve
    if { $serve(selected) eq $serve(lastselected) } {return}
    eval destroy [winfo children $serve(paner)]
    set addr $serve(selected)
    $serve(paner) configure -text $chip($addr.name)
    $chip($addr.display) $addr $serve(paner)
    set serve(lastselected) $serve(selected)
}
