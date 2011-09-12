/*
$Id$
    OW -- One-Wire filesystem

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
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/

#ifndef OW_RETURN_CODE_H				/* tedious wrapper */
#define OW_RETURN_CODE_H

// Not stand-alone but part of ow_h

enum e_return_code {
	e_rc_success = 0 ,
	e_rc_inval = 9 ,
	e_rc_last,
} ;

#define N_RETURN_CODES (e_rc_last)

extern char * return_code_strings[N_RETURN_CODES] ;

extern int return_code_calls[N_RETURN_CODES] ;

#define RETURN_CODE_OUT_OF_RANGE (rc)  ((rc)<e_rc_success || (rc)>e_rc_last)

#define RETURN_CODE_INIT(pn)  do { (pn)->return_code = e_rc_success ; } while (0) ;

#define RETURN_CODE_SET(rc,pn)  do { int rcs_r = rc ; \
			if ( RETURN_CODE_OUT_OF_RANGE(rc) ) { LEVEL_DEBUG("Bad return_code %d",rc); rcs_r = e_rc_last ; } \
			LEVEL_DEBUG("Set return_code %d <%s> for %s",rcs_r,return_code_strings[rcs_r],(pn)->path); (pn)->return_code = rcs_r ; ++ return_code_calls[rcs_r]; \
			} while (0) ;

#endif							/* OW_RETURN_CODE_H */
