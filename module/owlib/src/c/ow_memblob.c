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
    2006 memblob
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#define MEMBLOB_ALLOCATION_INCREMENT 1000

/*
    A "memblob" is a structure holding a list of 1-wire serial numbers
    (8 bytes each) with some housekeeping information
    
    It is used for directory caches, and some "all at once" adapters types

    Most interesting, it allocates memory dynamically.
*/

void MemblobClear(struct memblob *mb)
{
	if (mb->memory_storage != NULL) {
		free(mb->memory_storage);
		mb->memory_storage = NULL;
	}
	db->used = 0;
	mb->devices = 0;
}

void MemblobInit(struct memblob *mb)
{
	mb->used = 0;
	mb->allocated = 0;
	mb->memory_storage = NULL;
}

int MemblobAdd(BYTE * data, size_t length, struct memblob *mb)
{
	// make more room? -- blocks of 10 devices (80byte)
	if ((mb->used + length > mb->allocated) || (mb->memory_storage == NULL)) {
		int newalloc = mb->allocated + MEMBLOB_ALLOCATION_INCREMENT;
		BYTE *try_bigger_block = realloc(mb->memory_storage, newalloc);
		if (try_bigger_block!=NULL) {
			db->allocated = newalloc;
			db->memory_storage = try_bigger_block;
		} else {				// allocation failed -- keep old
			return -ENOMEM;
		}
	}
	// add the device and increment the counter
	memcpy(&(mb->memory_storage[mb->used]), data, length);
	mb->used += length ;
	return 0;
}

BYTE * MemblobGet(const struct memblob *mb)
{
    return mb->memory_storage ;
}
