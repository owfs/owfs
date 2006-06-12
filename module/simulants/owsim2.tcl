#!/usr/bin/wish

option add *highlightThickness 0
tk_setPalette gray60

source rnotebook.tcl

###########################################################
###########################################################
########## Enough of notebook, lets do Simulant! ##########
###########################################################
###########################################################

source ow.tcl

if {[catch {wm iconbitmap . @"/home/owfs/owfs.ico"}] } {
    puts $errorInfo
}

wm geometry . 500x400

set dlist [list 0x10 0x10 0x10 0x01 0x20]
foreach d $dlist {
    SetAddress $d
}

set pan [frame .p -bg cyan]
pack $pan -fill both -expand 1

set pane [panedwindow $pan.pane]
pack $pane -side top -expand yes -fill both

set panel [frame $pane.left -bg cyan]
pack $panel -fill both
set paner [frame $pane.right -bg cyan]
pack $paner -fill both

#positionWindow $w

scrollbar $panel.scroll -command "$pane.left.listbox yview"
set chosen [listbox $panel.listbox -yscroll "$pane.left.scroll set"  -listvariable devname -setgrid 1 -height 12 -bg cyan]
pack $panel.scroll -side right -fill y
pack $panel.listbox -side left -expand 1 -fill both

bind $chosen <<ListboxSelect>> {Rside  %W}

proc Rside {src} {
    global paner
    global chip
    global selected_device
    set d $chip([$src get [$src curselection]])
    if { [string compare $d $selected_device] != 0 } {
        eval destroy [winfo children $paner]
        $chip($d.process) $d $paner
        set selected_device $d
    }
}

$pane add $panel $paner
set selected_device $chip([lindex $devname 0])
$chip($selected_device.process) $selected_device $paner
