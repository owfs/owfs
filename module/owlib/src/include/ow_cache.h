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

#ifndef OWCACHE_H				/* tedious wrapper */
#define OWCACHE_H

/* Internal properties -- used by some devices */
/* in passing to store state information        */
struct internal_prop {
    char *name;
    enum fc_change change;
};

#if OW_CACHE

#define MakeInternalProp(tag,change)  static char ip_name_##tag[] = #tag ; static struct internal_prop ip_##tag = { ip_name_##tag , change }
#define InternalProp(tag)     (& ip_##tag)

/* Cache  and Storage functions */
void Cache_Open(void);
void Cache_Close(void);
void Cache_Clear(void);

int OWQ_Cache_Add( const struct one_wire_query * owq ) ;
int Cache_Add(const void *data, const size_t datasize,
			  const struct parsedname *pn);
int Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn);
int Cache_Add_Device(const int bus_nr, const struct parsedname *pn);
int Cache_Add_Internal(const void *data, const size_t datasize,
					   const struct internal_prop *ip,
					   const struct parsedname *pn);

int OWQ_Cache_Get( struct one_wire_query * owq ) ;
int Cache_Get(void *data, size_t * dsize, const struct parsedname *pn);
int Cache_Get_Strict(void *data, size_t dsize,
					 const struct parsedname *pn);
int Cache_Get_Dir(struct dirblob *db, const struct parsedname *pn);
int Cache_Get_Device(void *bus_nr, const struct parsedname *pn);
int Cache_Get_Internal(void *data, size_t * dsize,
					   const struct internal_prop *ip,
					   const struct parsedname *pn);
int Cache_Get_Internal_Strict(void *data, size_t dsize,
							  const struct internal_prop *ip,
							  const struct parsedname *pn);

int OWQ_Cache_Del( const struct one_wire_query * owq ) ;
int Cache_Del(const struct parsedname *pn);
int Cache_Del_Dir(const struct parsedname *pn);
int Cache_Del_Device(const struct parsedname *pn);
int Cache_Del_Internal(const struct internal_prop *ip,
					   const struct parsedname *pn);
void CookTheCache(void);

#else							/* OW_CACHE */

#define MakeInternalProp(tag)
#define InternalProp(tag)     NULL

#define Cache_Open( void )
#define Cache_Close( void )
#define Cache_Clear( void )

#define Cache_Add(data,datasize,pn )        (1)
#define Cache_Add_Dir(db,pn )               (1)
#define Cache_Add_Device(bus_nr,pn )        (1)
#define Cache_Add_Internal(data,datasize,ip,pn )    (1)
#define OWQ_Cache_Add( owq )                (1)

#define Cache_Get(data,dsize,pn )           (1)
#define Cache_Get_Dir(db,pn )               (1)
#define Cache_Get_Strict(data,dsize,pn )    (1)
#define OWQ_Cache_Get( owq )                (1)

#define Cache_Get_Device(bus_nr,pn )        (1)
#define Cache_Get_Internal(data,dsize,ip,pn )       (1)
#define Cache_Get_Internal_Strict(data,dsize,ip,pn )       (1)

#define Cache_Del(pn )                      (1)
#define Cache_Del_Dir(pn )                  (1)
#define Cache_Del_Device(pn )               (1)
#define Cache_Del_Internal(ip,pn )          (1)
#define OWQ_Cache_Del( owq )                (1)
#define CookTheCache()

#endif							/* OW_CACHE */

#endif							/* OWCACHE_H */
