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

#include "owfs_config.h"
#include "ow.h"

int OW_read_paged( BYTE * p, size_t size, size_t offset, const struct parsedname * pn,
    size_t pagelen, int (*readfunc)(BYTE *,const size_t,const size_t,const struct parsedname * const) ) {

    size_t pageoff = offset % pagelen ;
    if ( size==0 ) return 0 ;
    /* first page -- may be partial and start within page */
    if ( size+pageoff>pagelen ) {
        int ret ;
        size_t thispage = pagelen - pageoff ;
        if ( thispage > size ) thispage = size ;
        ret = readfunc(p,thispage,offset,pn) ;
        if (ret) return ret ;
        p += thispage ;
        size -= thispage ;
        offset += thispage ;
    }
    /* successive pages, will start at page start */
    while (size>0) {
        int ret ;
        size_t thispage = pagelen ;
        if ( thispage > size ) thispage = size ;
        ret = readfunc(p,thispage,offset,pn) ;
        if (ret) return ret ;
        p += thispage ;
        size -= thispage ;
        offset += thispage ;
    }
    return 0 ;
}

int OW_write_paged( const BYTE * p, size_t size, size_t offset, const struct parsedname * pn,
    size_t pagelen, int (*writefunc)(const BYTE *,const size_t,const size_t,const struct parsedname * const) ) {


    size_t pageoff = offset % pagelen ;
    if ( size==0 ) return 0 ;
    /* first page -- may be partial and start within page */
    if ( size+pageoff>pagelen ) {
        int ret ;
        size_t thispage = pagelen - pageoff ;
        if ( thispage > size ) thispage = size ;
        ret = writefunc(p,thispage,offset,pn) ;
        if (ret) return ret ;
        p += thispage ;
        size -= thispage ;
        offset += thispage ;
    }
    /* successive pages, will start at page start */
    while (size>0) {
        int ret ;
        size_t thispage = pagelen ;
        if ( thispage > size ) thispage = size ;
        ret = writefunc(p,thispage,offset,pn) ;
        if (ret) return ret ;
        p += thispage ;
        size -= thispage ;
        offset += thispage ;
    }
    return 0 ;
}
