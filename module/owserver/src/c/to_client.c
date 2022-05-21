/*
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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
int ToClient(int file_descriptor, struct client_msg *machine_order_cm, const char *data)
{
	struct client_msg s_cm;
	struct client_msg *network_order_cm = &s_cm;
	
	int nio = 1; // at least header

#if ( __GNUC__ > 4 ) || (__GNUC__ == 4 && __GNUC_MINOR__ > 4 )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
	// note data should be (const char *) but iovec complains about const arguments
	struct iovec io[] = {
		{network_order_cm, sizeof(struct client_msg),},
		{ (char *) data, machine_order_cm->payload,},
	};
#pragma GCC diagnostic pop
#else
	// note data should be (const char *) but iovec complains about const arguments
	struct iovec io[] = {
		{network_order_cm, sizeof(struct client_msg),},
		{ (char *) data, machine_order_cm->payload,},
	};
#endif

	// Prep header
	network_order_cm->version       = htonl( machine_order_cm->version       );
	network_order_cm->payload       = htonl( machine_order_cm->payload       );
	network_order_cm->ret           = htonl( machine_order_cm->ret           );
	network_order_cm->control_flags = htonl( machine_order_cm->control_flags );
	network_order_cm->size          = htonl( machine_order_cm->size          );
	network_order_cm->offset        = htonl( machine_order_cm->offset        );

	LEVEL_DEBUG("payload=%d size=%d, ret=%d, sg=0x%X offset=%d ", machine_order_cm->payload, machine_order_cm->size, machine_order_cm->ret,
		     machine_order_cm->control_flags, machine_order_cm->offset);

	/* If payload==0, no extra data
		If payload <0, flag to show a delay message, again no data
	*/

	if(machine_order_cm->payload < 0) {
		LEVEL_DEBUG("Send delay message (ping)");
	} else if ( machine_order_cm->payload == 0 ) {
		LEVEL_DEBUG("No data") ;
	} else if ( data == NULL ) {
		LEVEL_DEBUG("Bad data pointer -- NULL") ;
	} else {
		nio = 2; // add data segment
		TrafficOutFD("to server data",io[1].iov_base,io[1].iov_len,file_descriptor);
	}

	return writev(file_descriptor, io, nio) != (ssize_t) (io[0].iov_len + io[1].iov_len);
}
