/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

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
    Implementation:
    2006 dirblob
*/

#ifndef OW_CHARBLOB_H			/* tedious wrapper */
#define OW_CHARBLOB_H

#define NO_CHARBLOB	NULL

struct charblob {
	int troubled;
	size_t allocated;
	size_t used;
	ASCII *blob;
};

void CharblobClear(struct charblob *cb);
void CharblobInit(struct charblob *cb);
int CharblobPure(struct charblob *cb);
int CharblobAdd(const ASCII * a, size_t s, struct charblob *cb);
int CharblobAddChar(const ASCII a, struct charblob *cb);
ASCII * CharblobData(struct charblob * cb);
size_t CharblobLength( struct charblob * cb );

#endif							/* OW_CHARBLOB_H */
