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

#ifndef OW_DIRBLOB_H			/* tedious wrapper */
#define OW_DIRBLOB_H

struct dirblob {
	int troubled;
	int allocated;
	int devices;
	BYTE *snlist;
};

void DirblobClear(struct dirblob *db);
void DirblobInit(struct dirblob *db);
int DirblobPure(const struct dirblob *db);
void DirblobPoison(struct dirblob *db);
int DirblobElements(const struct dirblob *db);
int DirblobAdd(BYTE * sn, struct dirblob *db);
int DirblobGet(int dev, BYTE * sn, const struct dirblob *db);
int DirblobSearch(BYTE * sn, const struct dirblob *db);
int DirblobRecreate( BYTE * snlist, int size, struct dirblob *db);

#endif							/* OW_DIRBLOB_H */
