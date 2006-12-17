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
    2006 dirblob
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/*
    A "dirblob" is a structure holding a list of 1-wire serial numbers
    (8 bytes each) with some housekeeping information
    
    It is used for directory caches, and some "all at once" adapters types

    Most interesting, it allocates memory dynamically.
*/

void DirblobClear(struct dirblob *db)
{
	if (db->snlist) {
		free(db->snlist);
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

int DirblobPure(struct dirblob *db)
{
	return !db->troubled;
}

int DirblobAdd(BYTE * sn, struct dirblob *db)
{
	// make more room? -- blocks of 10 devices (80byte)
	if ((db->devices >= db->allocated) || (db->snlist == NULL)) {
		int newalloc = db->allocated + 10;
		BYTE *temp = realloc(db->snlist, 8 * newalloc);
		if (temp) {
			db->allocated = newalloc;
			db->snlist = temp;
		} else {				// allocation failed -- keep old
			db->troubled = 1;
			return -ENOMEM;
		}
	} else {
	}
	// add the device and increment the counter
	memcpy(&(db->snlist[8 * db->devices]), sn, 8);
	++db->devices;
	return 0;
}

int DirblobGet(int dev, BYTE * sn, struct dirblob *db)
{
	if (dev >= db->devices)
		return -ENODEV;
	memcpy(sn, &(db->snlist[8 * dev]), 8);
	return 0;
}
