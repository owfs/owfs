# -*- Tcl -*-
# File: ow.tcl
#
# Created: Tue Jan 11 20:34:11 2005
#
# $Id$
#

namespace eval ::OW {
    proc ::OW { args } { return [eval ::OW::init $args] }
    proc ::ow { args } { return [eval ::OW::ow $args] }
}

proc ::OW::init {args} {
    array set oldopts {
        -format -f
        -celsius -C
        -fahenheit -F
        -kelvin -K
        -rankine -R
        -readonly -r
        -cache -t
    }
    set sargs ""
    foreach arg $args {
        if {[info exists oldopts($arg)]} {
            append sargs $oldopts($arg) { }
        } else {
            append sargs $arg { }
        }
    }
    set sargs [string trim $sargs]
    return [::OW::_init $sargs]
}

proc ::OW::ow {opt args} {
    switch -exact $opt {
        open {
            return [eval ::OW::init $args]
        }
        close {
            return [eval ::OW::finish $args]
        }
        version {
            return [eval ::OW::version $args]
        }
        error {
            return [eval ::OW::set_error $args]
        }
        opened {
            return [eval ::OW::isconnect $args]
        }
        get {
            return [eval ::OW::get $args]
        }
        put {
            return [eval ::OW::put $args]
        }
        isdir {
            return [eval ::OW::isdir $args]
        }
        isdirectory {
            return [eval ::OW::isdir $args]
        }
        set {
            return [eval ::OW::_set $args]
        }
        default {
            error "bad option \"$opt\": must be open, close, version, error, opened, get, put, set, isdir or isdirectory"
        }
    }
}

proc ::OW::_join {args} {
    foreach a $args {
        append res {/} [string trim $a {/}]
    }
    regsub -all {//} $res {/} res
    return $res
}

proc ::OW::_set {path} {
    set path "/[string trim $path {/}]"
    if {[exists $path]} {
	if {![exists ::OW::id_set]} {set ::OW::id_set 0}
	incr ::OW::id_set
	set nid "owcmd$::OW::id_set"
#        regsub -all {\W} $path {_} nid
        set evalstr [format {proc ::OW::%s {args} {return [eval ::OW::device %s $args]}} $nid $path]
        eval $evalstr
        return "::OW::$nid"
    } else {
        error "node \"$path\" not exists"
    }
}

proc ::OW::device {base opt args} {
    set path $base
    switch -exact $opt {
        get {
            set lst {}
            if {[set x [lsearch -exact $args {-list}]] >= 0} {
                set lst {-list}
                set args [lreplace $args $x $x]
            }
            set path [_join $base [lindex $args 0]]
            return [eval ::OW::get $path $lst]
        }
        put {
            if {[llength $args] > 1} {
                set path [_join $base [lindex $args 0]]
                set args [lreplace $args 0 0]
            }
            return [eval ::OW::put $path $args]
        }
        isdir {
            return [eval ::OW::isdir [_join $base [lindex $args 0]]]
        }
        isdirectory {
            return [eval ::OW::isdir [_join $base [lindex $args 0]]]
        }
        set {
            return [eval ::OW::_set [_join $base [lindex $args 0]]]
        }
        path {
            return $path
        }
        default {
            error "bad option \"$opt\": must be get, put, set, path, isdir or isdirectory"
        }
    }
}

proc ::OW::set_error {opt args} {
    switch -exact $opt {
        level {
            if {$args == ""} {
                error "wrong # args: should be \"ow error level ?val?\""
            }
            eval ::OW::set_error_level $args
        }
        print {
            if {$args == ""} {
                error "wrong # args: should be \"ow error print ?val?\""
            }
            eval ::OW::set_error_print $args
        }
        default {
            error "bad option \"$opt\": must be level or print"
        }
    }
}
