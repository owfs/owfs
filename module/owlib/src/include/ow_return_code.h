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

#include "ow_debug.h"

// Not stand-alone but part of ow_h

extern char * return_code_strings[] ;

// Fragile: return_code_out_of_bounds is set to the last entry in the error (return_code) list
// it's defined in ow_return_code.c
#define return_code_out_of_bounds 210

extern int return_code_calls[] ;

#define N_RETURN_CODES (return_code_out_of_bounds+1)

#define RETURN_CODE_INIT(pn)  do { (pn)->return_code = 0 ; ++return_code_calls[0]; } while (0) ;

#define RETURN_IF_ERROR(pn)		if ( (pn)->return_code != 0 ) { return ; }

#ifndef __FILE__
  #define __FILE__ ""
#endif

#ifndef __LINE__
  #define __LINE__ ""
#endif

#ifndef __func__
  #define __func__ ""
#endif

/* Set return code into the proper place in parsedname strcture */
#define RETURN_CODE_SET(rc,pn)    return_code_set_scalar( rc, pn, __FILE__, __LINE__, __func__ )

/* Set return code into an integer */
#define RETURN_CODE_SET_SCALAR(rc,i)    return_code_set_scalar( rc, &(i), __FILE__, __LINE__, __func__ )

/* Unconditional return with return code */
#define RETURN_CODE_RETURN(rc)	do { int i ; RETURN_CODE_SET_SCALAR(rc,i) ; return -i; } while(0) ;

/* Conditional return with return code */
#define RETURN_CODE_ERROR_RETURN(rc)	do { int i ; RETURN_CODE_SET_SCALAR(rc,i) ; if ( i != 0 ) { return -i; } } while(0) ;

void return_code_set( int rc, struct parsedname * pn, const char * d_file, const char * d_line, const char * d_func ) ;
void return_code_set_scalar( int rc, int * pi, const char * d_file, const char * d_line, const char * d_func ) ;
void Return_code_setup(void) ;

#endif							/* OW_RETURN_CODE_H */
