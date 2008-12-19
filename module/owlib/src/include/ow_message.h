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
};
/* message to owserver */
struct server_msg {
	int32_t version;
	int32_t payload;
	int32_t type;
	int32_t sg;
	int32_t size;
	int32_t offset;
};

/* message to client */
struct client_msg {
	int32_t version;
	int32_t payload;
	int32_t ret;
	int32_t sg;
	int32_t size;
	int32_t offset;
};

union address {
	struct sockaddr sock;
	char text[120];
};

/* message to client */
struct side_msg {
	int32_t version;
	union address host;
	union address peer;
};

struct serverpackage {
	ASCII *path;
	BYTE *data;
	size_t datasize;
	BYTE *tokenstring;
	size_t tokens;
};

// Current version of the protocol
#define OWSERVER_PROTOCOL_VERSION   0

// lower 16 bits is token size (only FROM owserver of course)
#define Servertokens(tokens)		((tokens)&0xFFFF)
#define MakeServertokens(tokens)	(tokens)

// bit 17 is FROM server flag
#define ServermessageMASK		(((int32_t)1)<<16)
#define MakeServermessage		ServermessageMASK
#define isServermessage(version)	(((version) & ServermessageMASK) == ServermessageMASK)

//owserver protocol
#define ServerprotocolMASK		((0xFF)<<17)
#define Serverprotocol(version) (((version) & ServerprotocolMASK) >> 17 )
#define MakeServerprotocol(protocol) ((protocol) << 17)

// bit 32 set is Sidetap flag
#define ServersidetapMASK		(((int32_t)1)<<31)
#define MakeServersidetap		ServersidetapMASK
#define isServersidetap(version)	(((version) & ServersidetapMASK) == ServersidetapMASK)

#endif							/* OW_MESSAGE_H */
