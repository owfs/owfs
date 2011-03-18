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

#ifndef OWCACHE_H				/* tedious wrapper */
#define OWCACHE_H

/* Internal properties -- used by some devices */
/* in passing to store state information        */
struct internal_prop {
	char *name;
	enum fc_change change;
};

enum simul_type { simul_temp, simul_volt, simul_end };

#if OW_CACHE

#define Make_SlaveSpecificTag(tag,change)  static char ip_name_##tag[] = #tag ; static struct internal_prop ip_##tag = { ip_name_##tag , change }
#define SlaveSpecificTag(tag)     (& ip_##tag)

extern struct internal_prop ipSimul[] ;

/* Cache  and Storage functions */
void Cache_Open(void);
void Cache_Close(void);
void Cache_Clear(void);

GOOD_OR_BAD OWQ_Cache_Add(const struct one_wire_query *owq);
GOOD_OR_BAD Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn);
GOOD_OR_BAD Cache_Add_Device(const int bus_nr, const BYTE *sn);
GOOD_OR_BAD Cache_Add_SlaveSpecific(const void *data, const size_t datasize, const struct internal_prop *ip, const struct parsedname *pn);
GOOD_OR_BAD Cache_Add_Alias(const ASCII *name, const BYTE * sn) ;
GOOD_OR_BAD Cache_Add_Simul(const enum simul_type type, const struct parsedname *pn);
void Cache_Add_Alias_Bus(const BYTE * alias_name, int datasize, INDEX_OR_ERROR bus);

GOOD_OR_BAD OWQ_Cache_Get(struct one_wire_query *owq);
GOOD_OR_BAD Cache_Get(void *data, size_t * dsize, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_Dir(struct dirblob *db, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_Device(void *bus_nr, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_SlaveSpecific(void *data, size_t dsize, const struct internal_prop *ip, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_Alias(ASCII * name, size_t length, const BYTE * sn) ;
GOOD_OR_BAD Cache_Get_SerialNumber(const ASCII * name, BYTE * sn) ;
GOOD_OR_BAD Cache_Get_Simul_Time(enum simul_type type, time_t * dwell_time, const struct parsedname * pn);
INDEX_OR_ERROR Cache_Get_Alias_Bus(const BYTE * alias_name, int datasize) ;
GOOD_OR_BAD Cache_Get_Alias_SN(const BYTE * alias_name, int datasize, BYTE * sn );

void OWQ_Cache_Del(struct one_wire_query *owq);
void OWQ_Cache_Del_ALL(struct one_wire_query *owq);
void OWQ_Cache_Del_BYTE(struct one_wire_query *owq);
void OWQ_Cache_Del_parts(struct one_wire_query *owq);

void Cache_Del_Dir(const struct parsedname *pn);
void Cache_Del_Device(const struct parsedname *pn);
void Cache_Del_Internal(const struct internal_prop *ip, const struct parsedname *pn);
void Cache_Del_Simul(enum simul_type type, const struct parsedname *pn) ;
void Cache_Del_Mixed_Aggregate(const struct parsedname *pn);
void Cache_Del_Mixed_Individual(const struct parsedname *pn);
void Cache_Del_Alias_Bus(const BYTE * alias_name, int datasize);

void Aliaslist( struct memblob * mb  ) ;

#else							/* OW_CACHE */

#define Make_SlaveSpecificTag(tag, change)
#define SlaveSpecificTag(tag)     NULL

#define Cache_Open( void )
#define Cache_Close( void )
#define Cache_Clear( void )

#define Cache_Add_Dir(db,pn )               (gbBAD)
#define Cache_Add_Device(bus_nr,sn )        (gbBAD)
#define Cache_Add_SlaveSpecific(data,datasize,ip,pn )    (gbBAD)
#define OWQ_Cache_Add( owq )                (gbBAD)
#define Cache_Add_Alias(name, sn)           (gbBAD)
#define Cache_Add_Simul(type,pn)            (gbBAD)
#define Cache_Add_Alias_Bus(name,size,bus)	(1)

#define Cache_Get(data,dsize,pn )           (gbBAD)
#define Cache_Get_Dir(db,pn )               (gbBAD)
#define OWQ_Cache_Get( owq )                (gbBAD)

#define Cache_Get_Device(bus_nr,pn )        (gbBAD)
#define Cache_Get_SlaveSpecific(data,dsize,ip,pn )       (gbBAD)
#define Cache_Get_Alias(name, length, sn)   (gbBAD)
#define Cache_Get_SerialNumber(name, sn)    (gbBAD)
#define Cache_Get_Simul_Time(type,time,pn)  (1)
#define Cache_Get_Alias_Bus(name,size) 		(INDEX_BAD)
#define Cache_Get_Alias_SN(name,size,sn)	(gbBAD)

#define Cache_Del(pn )                      (1)
#define Cache_Del_Dir(pn )                  (1)
#define Cache_Del_Device(pn )               (1)
#define Cache_Del_Internal(ip,pn )          (1)

#define OWQ_Cache_Del( owq )                (1)
#define OWQ_Cache_Del_ALL( owq)             (1)
#define OWQ_Cache_Del_BYTE( owq)            (1)
#define OWQ_Cache_Del_parts( owq)           (1)

#define Cache_Del_Simul(type,pn)            (1)
#define Cache_Del_Mixed_Aggregate(pn)       (1)
#define Cache_Del_Mixed_Individual(pn)      (1)
#define Cache_Del_Alias_Bus(name,size)		(1)

#define Aliaslist(mb)     

#endif							/* OW_CACHE */

#endif							/* OWCACHE_H */
