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
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

#ifndef OWCACHE_H  /* tedious wrapper */
#define OWCACHE_H

#define iCACHE_READ( I, pn )    do { size_t s=sizeof(INT)   ; if Cache_Get((void *) I, &s, pn ) ; return 0 ; } while (0)
#define yCACHE_READ( Y, pn )    do { size_t s=sizeof(INT)   ; if Cache_Get((void *) Y, &s, pn ) ; return 0 ; } while (0)
#define uCACHE_READ( U, pn )    do { size_t s=sizeof(UINT)  ; if Cache_Get((void *) U, &s, pn ) ; return 0 ; } while (0)
#define fCACHE_READ( F, pn )    do { size_t s=sizeof(_FLOAT); if Cache_Get((void *) F, &s, pn ) ; return 0 ; } while (0)
#define dCACHE_READ( D, pn )    do { size_t s=sizeof(_DATE) ; if Cache_Get((void *) D, &s, pn ) ; return 0 ; } while (0)
#define aCACHE_READ( A, size, offset, pn )    do { size_t s=(size) ; if Cache_Get( (void *) A, &s, pn ) ; return FS_output_ascii(A,size,offset,A,s ) ; } while (0)
#define bCACHE_READ( B, size, offset, pn )    do { size_t s=(size) ; if Cache_Get( (void *) B, &s, pn ) ; return size ; } while (0)

#define iCACHE_WRITE( I, pn )         Cache_Add((void *) I, sizeof(INT), pn )
#define yCACHE_WRITE( Y, pn )         Cache_Add((void *) Y, sizeof(INT), pn )
#define uCACHE_WRITE( U, pn )         Cache_Add((void *) U, sizeof(UINT), pn )
#define fCACHE_WRITE( F, pn )         Cache_Add((void *) F, sizeof(_FLOAT), pn )
#define dCACHE_WRITE( D, pn )         Cache_Add((void *) D, sizeof(_DATE), pn )
#define aCACHE_WRITE( A, size, pn )   Cache_Add( (void *) A, size, pn )
#define bCACHE_WRITE( B, size, pn )   Cache_Add( (void *) B, size, pn )

#define CACHE_ERASE(pn)     Cache_Del(pn)

#if OW_CACHE

/* Cache  and Storage functions */
void Cache_Open( void ) ;
void Cache_Close( void ) ;
void Cache_Clear( void ) ;
int Cache_Add(          const void * data, const size_t datasize, const struct parsedname * pn ) ;
int Cache_Add_Dir( const struct dirblob * db, const struct parsedname * pn ) ;
int Cache_Add_Device( const int bus_nr, const struct parsedname * pn ) ;
int Cache_Add_Internal( const void * data, const size_t datasize, const struct internal_prop * ip, const struct parsedname * pn ) ;
int Cache_Get(          void * data, size_t * dsize, const struct parsedname * pn ) ;
int Cache_Get_Dir( struct dirblob * db, const struct parsedname * pn ) ;
int Cache_Get_Device( void * bus_nr, const struct parsedname * pn ) ;
int Cache_Get_Internal( void * data, size_t * dsize, const struct internal_prop * ip, const struct parsedname * pn ) ;
int Cache_Del(          const struct parsedname * pn                                                                   ) ;
int Cache_Del_Dir( const struct parsedname * pn ) ;
int Cache_Del_Device( const struct parsedname * pn ) ;
int Cache_Del_Internal( const struct internal_prop * ip, const struct parsedname * pn ) ;

#else /* OW_CACHE */

#define Cache_Open( void )
#define Cache_Close( void )
#define Cache_Clear( void )
#define Cache_Add(data,datasize,pn )        (1)
#define Cache_Add_Dir(db,pn )               (1)
#define Cache_Add_Device(bus_nr,pn )        (1)
#define Cache_Add_Internal(data,datasize,ip,pn )    (1)
#define Cache_Get(data,dsize,pn )           (1)
#define Cache_Get_Dir(db,pn )               (1)
#define Cache_Get_Device(bus_nr,pn )        (1)
#define Cache_Get_Internal(data,dsize,ip,pn )       (1)
#define Cache_Del(pn )                      (1)
#define Cache_Del_Dir(pn )                  (1)
#define Cache_Del_Device(pn )               (1)
#define Cache_Del_Internal(ip,pn )          (1)

#endif /* OW_CACHE */

#endif /* OWCACHE_H */
