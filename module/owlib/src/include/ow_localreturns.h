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


*/

#ifndef OW_LOCALRETURNS_H			/* tedious wrapper */
#define OW_LOCALRETURNS_H

typedef int SIZE_OR_ERROR ;
typedef int ZERO_OR_ERROR ;
typedef enum { gbGOOD, gbBAD, gbOTHER, } GOOD_OR_BAD  ; // OTHER breaks the encapsulation a bit.

#define GOOD(x)	((x)==gbGOOD)
#define BAD(x)	((x)!=gbGOOD)
#define GB_to_Z_OR_E(x)	GOOD(x)?0:-EINVAL

#define RETURN_BAD_IF_BAD(x)	if ( BAD(x) ) { return gbBAD; }
#define RETURN_GOOD_IF_GOOD(x)	if (GOOD(x) ) { return gbGOOD; }
#define RETURN_ERROR_IF_BAD(x)	if ( BAD(x) ) { return -EINVAL; }

/* Use this every place we call a void * function (unless the return value is really used). */
#define VOID_RETURN NULL

#endif							/* OW_LOCALRETURNS_H */
