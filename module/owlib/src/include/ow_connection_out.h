/*
$Id$
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
*/

#ifndef OW_CONNECTION_OUT_H			/* tedious wrapper */
#define OW_CONNECTION_OUT_H

/* Outbound connections (to web address or ownet client) */
/* Called in ow_connection.h */

/* Network connection structure */
struct connection_out {
	struct connection_out *next;
	void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor);
	char *name;
	char *host;
	char *service;
	int index;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct {
		char *type;					// for zeroconf
		char *domain;				// for zeroconf
		char *name;					// zeroconf name
	} zero ;
#if OW_MT
	pthread_t tid;
#endif							/* OW_MT */
#if OW_ZERO
	DNSServiceRef sref0;
	DNSServiceRef sref1;
#endif
};

extern struct outbound_control {
	int active ; // how many "bus" entries are currently in linked list
	int next_index ; // increasing sequence number
	struct connection_out * head ; // head of a linked list of "bus" entries
} Outbound_Control ; // Single global struct -- see ow_connect.c

#endif							/* OW_CONNECTION_OUT_H */
