/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* ow_interate.c */
/* routines to split reads and writes if longer than page */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

int OW_read_paged( BYTE * p, size_t size, off_t offset, const struct parsedname * pn,
    size_t pagelen, int (*readfunc)(BYTE *,const size_t,const off_t,const struct parsedname * const) ) {
    /* successive pages, will start at page start */
    while (size>0) {
        size_t thispage = pagelen - (offset % pagelen) ;
        if ( thispage > size ) thispage = size ;
        if (readfunc(p,thispage,offset,pn)) return 1 ;
        p += thispage ;
        size -= thispage ;
        offset += thispage ;
    }
    return 0 ;
}

int OW_write_paged( const BYTE * p, size_t size, off_t offset, const struct parsedname * pn,
    size_t pagelen, int (*writefunc)(const BYTE *,const size_t,const off_t,const struct parsedname * const) ) {
    /* successive pages, will start at page start */
    while (size>0) {
        size_t thispage = pagelen - (offset % pagelen) ;
        if ( thispage > size ) thispage = size ;
        if (writefunc(p,thispage,offset,pn)) return 1 ;
        p += thispage ;
        size -= thispage ;
        offset += thispage ;
    }
    return 0 ;
}
