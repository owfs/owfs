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

    ---------------------------------------------------------------------------
    Implementation:
    2006 dirblob
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#define DIRBLOB_ELEMENT_LENGTH  8
#define DIRBLOB_ALLOCATION_INCREMENT 10

/*
    A "dirblob" is a structure holding a list of 1-wire serial numbers
    (8 bytes each) with some housekeeping information

    It is used for directory caches, and some "all at once" adapters types

    Most interestingly, it allocates memory dynamically.
*/

void DirblobClear(struct dirblob *db)
{
	if (db->snlist != NULL) {
		owfree(db->snlist);
		db->snlist = NULL;
	}
	db->allocated = db->devices;
	db->devices = 0;
}

void DirblobInit(struct dirblob *db)
{
	db->devices = 0;
	db->allocated = 0;
	db->snlist = 0;
	db->troubled = 0;
}

int DirblobPure(const struct dirblob *db)
{
	return !db->troubled;
}

void DirblobPoison(struct dirblob *db)
{
	db->troubled = 1;
}

int DirblobElements(const struct dirblob *db)
{
	return db->devices;
}

int DirblobAdd(const BYTE * sn, struct dirblob *db)
{
	if ( db->troubled ) {
		return -EINVAL ;
	}
	// make more room? -- blocks of 10 devices (80byte)
	if ((db->devices >= db->allocated) || (db->snlist == NULL)) {
		int newalloc = db->allocated + DIRBLOB_ALLOCATION_INCREMENT;
		BYTE *try_bigger_block = owrealloc(db->snlist, DIRBLOB_ELEMENT_LENGTH * newalloc);
		if (try_bigger_block != NULL) {
			db->allocated = newalloc;
			db->snlist = try_bigger_block;
		} else {				// allocation failed -- keep old
			db->troubled = 1;
			return -ENOMEM;
		}
	}
	// add the device and increment the counter
	memcpy(&(db->snlist[DIRBLOB_ELEMENT_LENGTH * db->devices]), sn, DIRBLOB_ELEMENT_LENGTH);
	++db->devices;
	return 0;
}

int DirblobGet(int device_index, BYTE * sn, const struct dirblob *db)
{
	if (device_index >= db->devices) {
		return -ENODEV;
	}
	memcpy(sn, &(db->snlist[DIRBLOB_ELEMENT_LENGTH * device_index]), DIRBLOB_ELEMENT_LENGTH);
	return 0;
}

/* Search for a serial number
   return position (>=0) on match
   return -1 on no match or error
 */
int DirblobSearch(BYTE * sn, const struct dirblob *db)
{
	int device_index;
	if (db == NULL || db->devices < 1) {
		return -1;
	}
	for (device_index = 0; device_index < db->devices; ++device_index) {
		if (memcmp(sn, &(db->snlist[DIRBLOB_ELEMENT_LENGTH * device_index]), DIRBLOB_ELEMENT_LENGTH) == 0) {
			return device_index;
		}
	}
	return -1;
}

// used in cache, fixes up a dirblob when retrieved from the cache
int DirblobRecreate( BYTE * snlist, int size, struct dirblob *db)
{
	DirblobInit( db ) ;

	if ( size == 0 ) {
		return 0 ;
	}

	db->snlist = (BYTE *) owmalloc(size) ;

	if ( db->snlist == NULL ) {
		db->troubled = 1 ;
		return -ENOMEM ;
	}

	memcpy(db->snlist, snlist, size);
	db->allocated = db->devices = size / 8;
	return 0 ;
}

