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
    A "charblob" is a structure holding a list of files
    
    Most interesting, it allocates memory dynamically.
*/

void CharblobClear(struct charblob *cb)
{
	if (cb->blob) {
		free(cb->blob);
	}
	CharblobInit(cb);
}

void CharblobInit(struct charblob *cb)
{
	cb->used = 0;
	cb->allocated = 0;
	cb->blob = 0;
	cb->troubled = 0;
}

int CharblobPure(struct charblob *cb)
{
	return !cb->troubled;
}

int CharblobAdd(ASCII * a, size_t s, struct charblob *cb)
{
	size_t incr = 1024;
	if (incr < s)
		incr = s;
	// make more room? -- blocks of 1k
	if (cb->used)
		CharblobAddChar(',', cb);	// add a comma
	if (cb->used + s > cb->allocated) {
		int newalloc = cb->allocated + incr;
		ASCII *temp = realloc(cb->blob, newalloc);
		if (temp) {
			memset(&temp[cb->allocated], 0, incr);	// set the new memory to blank
			cb->allocated = newalloc;
			cb->blob = temp;
		} else {				// allocation failed -- keep old
			cb->troubled = 1;
			return -ENOMEM;
		}
	}
	memcpy(&cb->blob[cb->used], a, s);
	cb->used += s;
	return 0;
}

int CharblobAddChar(ASCII a, struct charblob *cb)
{
	// make more room? -- blocks of 1k
	if (cb->used + 1 > cb->allocated) {
		int newalloc = cb->allocated + 1024;
		ASCII *temp = realloc(cb->blob, newalloc);
		if (temp) {
			memset(&temp[cb->allocated], 0, 1024);	// set the new memory to blank
			cb->allocated = newalloc;
			cb->blob = temp;
		} else {				// allocation failed -- keep old
			cb->troubled = 1;
			return -ENOMEM;
		}
	}
	cb->blob[cb->used] = a;
	++cb->used;
	return 0;
}
