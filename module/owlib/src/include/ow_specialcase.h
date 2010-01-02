/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions

    LICENSE (As of version 2.5p4 2-Oct-2006)
    owlib: GPL v2
    owfs, owhttpd, owftpd, owserver: GPL v2
    owshell(owdir owread owwrite owpresent): GPL v2
    owcapi (libowcapi): GPL v2
    owperl: GPL v2
    owtcl: LGPL v2
    owphp: GPL v2
    owpython: GPL v2
    owsim.tcl: GPL v2
    where GPL v2 is the "Gnu General License version 2"
    and "LGPL v2" is the "Lesser Gnu General License version 2"


    Written 2003 Paul H Alfille
*/

#ifndef OW_SPECIALCASE_H			/* tedious wrapper */
#define OW_SPECIALCASE_H

#include "ow_connection.h" // for bus_types
#include "ow_filetype.h" // for READ and WRITE functions

/* Predeclare one_wire_query */
struct one_wire_query;

struct specialcase_key {
	enum adapter_type adapter ;
	unsigned char family_code ;
	struct filetype * filetype ;
} ;

struct specialcase {
	struct specialcase_key key ;
	int (*read) (struct one_wire_query *);	// read callback function
	int (*write) (struct one_wire_query *);	// write callback function
} ;

extern void * SpecialCase ; // tree of special case devices

/* ---- Prototypes --- */
void SpecialCase_add( struct connection_in * in, unsigned char family_code, const char * property, int (*read_func) (struct one_wire_query *), int (*write_func) (struct one_wire_query *)) ;
int SpecialCase_read( struct one_wire_query * owq ) ;
int SpecialCase_write( struct one_wire_query * owq ) ;
void SpecialCase_close( void ) ;

#endif							/* OW_SPECIALCASE_H */
