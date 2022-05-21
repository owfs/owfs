/*
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

#ifndef OW_VISIBILITY_H			/* tedious wrapper */
#define OW_VISIBILITY_H


#define NO_READ_FUNCTION NULL
#define NO_WRITE_FUNCTION NULL

enum e_visibility {
	visible_never,
	visible_not_now,
	visible_now,
	visible_always,
} ;

enum e_visibility AlwaysVisible( const struct parsedname * pn ) ;
#define VISIBLE AlwaysVisible

enum e_visibility NeverVisible( const struct parsedname * pn ) ; 
#define INVISIBLE NeverVisible

#define VISIBILE_PRESENT INVISIBLE

GOOD_OR_BAD GetVisibilityCache( int * visibility_parameter, const struct parsedname * pn ) ;
void SetVisibilityCache( int visibility_parameter, const struct parsedname * pn ) ;

enum e_visibility FS_visible( const struct parsedname * pn ) ;

#endif							/* OW_VISIBILITY_H */
