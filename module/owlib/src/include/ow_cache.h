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

#define Make_SlaveSpecificTag(tag,change)  static char ip_name_##tag[] = #tag ; static struct internal_prop ip_##tag = { ip_name_##tag , change }
#define Make_SlaveSpecificTag_exportable(tag,change)  char ip_name_##tag[] = #tag ; struct internal_prop ip_##tag = { ip_name_##tag , change }
#define SlaveSpecificTag(tag)     (& ip_##tag)

extern struct internal_prop ip_S_T ;
extern struct internal_prop ip_S_V ;

/* Cache  and Storage functions */
void Cache_Open(void);
void Cache_Close(void);
void Cache_Clear(void);

GOOD_OR_BAD OWQ_Cache_Add(const struct one_wire_query *owq);
GOOD_OR_BAD Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn);
GOOD_OR_BAD Cache_Add_Device(const int bus_nr, const BYTE *sn);
GOOD_OR_BAD Cache_Add_SlaveSpecific(const void *data, const size_t datasize, const struct internal_prop *ip, const struct parsedname *pn);
GOOD_OR_BAD Cache_Add_Alias(const ASCII *name, const BYTE * sn) ;
GOOD_OR_BAD Cache_Add_Simul(const struct internal_prop *ip, const struct parsedname *pn);
void Cache_Add_Alias_Bus(const ASCII * alias_name, INDEX_OR_ERROR bus);

GOOD_OR_BAD OWQ_Cache_Get(struct one_wire_query *owq);
GOOD_OR_BAD Cache_Get(void *data, size_t * dsize, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_Dir(struct dirblob *db, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_Device(void *bus_nr, const struct parsedname *pn);
GOOD_OR_BAD Cache_Get_SlaveSpecific(void *data, size_t dsize, const struct internal_prop *ip, const struct parsedname *pn);
ASCII * Cache_Get_Alias(const BYTE * sn) ;
GOOD_OR_BAD Cache_Get_Simul_Time(const struct internal_prop *ip, time_t * dwell_time, const struct parsedname * pn);
INDEX_OR_ERROR Cache_Get_Alias_Bus(const ASCII * alias_name) ;
GOOD_OR_BAD Cache_Get_Alias_SN(const ASCII * alias_name, BYTE * sn );

void OWQ_Cache_Del(struct one_wire_query *owq);
void OWQ_Cache_Del_ALL(struct one_wire_query *owq);
void OWQ_Cache_Del_BYTE(struct one_wire_query *owq);
void OWQ_Cache_Del_parts(struct one_wire_query *owq);

void Cache_Del_Dir(const struct parsedname *pn);
void Cache_Del_Device(const struct parsedname *pn);
void Cache_Del_Internal(const struct internal_prop *ip, const struct parsedname *pn);
void Cache_Del_Simul(const struct internal_prop *ip, const struct parsedname *pn) ;
void Cache_Del_Mixed_Aggregate(const struct parsedname *pn);
void Cache_Del_Mixed_Individual(const struct parsedname *pn);
void Cache_Del_Alias_Bus(const ASCII * alias_name);
void Cache_Del_Alias(const BYTE * sn);

void Aliaslist( struct memblob * mb  ) ;

#endif							/* OWCACHE_H */
