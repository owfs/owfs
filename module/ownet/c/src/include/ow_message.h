/*
$Id$
    
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
*/

/* definitions and stuctures for the owserver messages */
/* This file doesn't stand alone, it must be included in ow.h */

#ifndef OW_MESSAGE_H			/* tedious wrapper */
#define OW_MESSAGE_H

/* Unique token for owserver loop checks */
union antiloop {
	struct {
		pid_t pid;
		clock_t clock;
	} simple;
	BYTE uuid[16];
};

/* Server (Socket-based) interface */
enum msg_classification {
	msg_error,
	msg_nop,
	msg_read,
	msg_write,
	msg_dir,
	msg_size,					// No longer used, leave here to compatibility
	msg_presence,
	msg_dirall,
	msg_get,
	msg_dirallslash,
	msg_getslash,
};
/* message to owserver */
struct server_msg {
	int32_t version;
	int32_t payload;
	int32_t type;
	int32_t control_flags;
	int32_t size;
	int32_t offset;
};

/* message to client */
struct client_msg {
	int32_t version;
	int32_t payload;
	int32_t ret;
	int32_t control_flags;
	int32_t size;
	int32_t offset;
};

struct serverpackage {
	const ASCII *path;
	BYTE *data;
	size_t datasize;
	BYTE *tokenstring;
	size_t tokens;
};

#define Servermessage       (((int32_t)1)<<16)
#define isServermessage( version )    (((version)&Servermessage)!=0)
#define Servertokens(version)    ((version)&0xFFFF)

#endif							/* OW_MESSAGE_H */
