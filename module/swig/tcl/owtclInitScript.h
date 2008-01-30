// -*- C++ -*-
// File: owtclInitScript.h
//
// Created: Mon Jan 10 22:53:33 2005
//
// $Id$
//

/*
 * The following string is the startup script executed when the ow is
 * loaded.  It looks on disk in several different directories for a script
 * "ow.tcl" that is compatible with this version of owtcl.
 */

static char owtclInitScript[] = "if {[info proc owtclInit]==\"\"} {\n\
  proc owtclInit {} {\n\
    global tcl_pkgPath errorInfo env\n\
    rename owtclInit {}\n\
    set errors {}\n\
    if {![info exists env(OWTCL_LIBRARY_FILE)]} {\n\
	set env(OWTCL_LIBRARY_FILE) ow.tcl\n\
    }\n\
    if {[info exists env(OWTCL_LIBRARY)]} {\n\
	lappend dirs $env(OWTCL_LIBRARY)\n\
    }\n\
    lappend dirs " OWTCL_PACKAGE_PATH "\n\
    if {[info exists tcl_pkgPath]} {\n\
	foreach i $tcl_pkgPath {\n\
	    lappend dirs [file join $i owtcl-" OWTCL_VERSION "] \\\n\
		[file join $i owtcl] $i\n\
	}\n\
    }\n\
    lappend dirs [pwd]\n\
    foreach i $dirs {\n\
	set try [file join $i $env(OWTCL_LIBRARY_FILE)]\n\
	if {[file exists $try]} {\n\
	    if {![catch {uplevel #0 [list source $try]} msg]} {\n\
		set env(OWTCL_LIBRARY) $i\n\
		return\n\
	    } else {\n\
		append errors \"$try: $msg\n$errorInfo\n\"\n\
	    }\n\
	}\n\
    }\n"
#ifdef NO_EMBEDDED_RUNTIME
	"    set msg \"Can't find a $env(OWTCL_LIBRARY_FILE) in the following directories: \n\"\n\
    append msg \"    $dirs\n\n$errors\n\n\"\n\
    append msg \"This probably means that owtcl wasn't installed properly.\"\n\
    return -code error $msg\n"
#else
	"    set env(OWTCL_LIBRARY) EMBEDDED_RUNTIME\n" "    uplevel #0 {"
#	include "ow.tcl.h"
	"    }\n"
#endif
	"  }\n\
}\n\
owtclInit";

/*
 * The init script can't make certain calls in a safe interpreter,
 * so we always have to use the embedded runtime for it
 */
static char owtclSafeInitScript[] = "if {[info proc owtclInit]==\"\"} {\n\
  proc owtclInit {} {\n\
    set env(OWTCL_LIBRARY) EMBEDDED_RUNTIME\n"
#ifdef NO_EMBEDDED_RUNTIME
	"    append msg \"owtcl requires embedded runtime to be compiled for\"\n\
    append msg \" use in safe interpreters\"\n\
    return -code error $msg\n"
#endif
	"    uplevel #0 {"
#	include "ow.tcl.h"
	"    }" "  }\n\
}\n\
owtclInit";
