/*
$Id$
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2004 Paul H Alfille

 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* owserver -- responds to requests over a network socket, and processes them on the 1-wire bus/
         Basic idea: control the 1-wire bus and answer queries over a network socket
         Clients can be owperl, owfs, owhttpd, etc...
         Clients can be local or remote
                 Eventually will also allow bounce servers.

         syntax:
                 owserver
                 -u (usb)
                 -d /dev/ttyS1 (serial)
                 -p tcp port
                 e.g. 3001 or 10.183.180.101:3001 or /tmp/1wire
*/

#include "owserver.h"

/* Send fully configured message back to client.
   data is optional and length depends on "payload"
 */
int ToClient(int file_descriptor, struct client_msg *machine_order_cm, char *data)
{
	struct client_msg s_cm;
	struct client_msg *network_order_cm = &s_cm;
	
	// note data should be (const char *) but iovec complains about const arguments
	int nio = 1;
	struct iovec io[] = {
		{network_order_cm, sizeof(struct client_msg),},
		{data, machine_order_cm->payload,},
	};

	LEVEL_DEBUG("payload=%d size=%d, ret=%d, sg=0x%X offset=%d \n", machine_order_cm->payload, machine_order_cm->size, machine_order_cm->ret,
		     machine_order_cm->sg, machine_order_cm->offset);

	/* If payload==0, no extra data
		If payload <0, flag to show a delay message, again no data
	*/
	if(machine_order_cm->payload < 0) {
		io[1].iov_len = 0;
		LEVEL_DEBUG("Send delay message\n");
	} else if ( machine_order_cm->payload == 0 ) {
		io[1].iov_len = 0;
	} else if ( data == NULL ) {
		LEVEL_DEBUG("Bad data pointer -- NULL\n") ;
		io[1].iov_len = 0;
	} else {
		++nio;
	}

	network_order_cm->version = htonl(machine_order_cm->version);
	network_order_cm->payload = htonl(machine_order_cm->payload);
	network_order_cm->ret = htonl(machine_order_cm->ret);
	network_order_cm->sg = htonl(machine_order_cm->sg);
	network_order_cm->size = htonl(machine_order_cm->size);
	network_order_cm->offset = htonl(machine_order_cm->offset);

	if(machine_order_cm->payload >= 0) {
		Debug_Writev(io, nio);  // debug output of network package after it's created.
	}
	
	return writev(file_descriptor, io, nio) != (ssize_t) (io[0].iov_len + io[1].iov_len);
}
