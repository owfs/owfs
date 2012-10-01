/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_net holds the network utility routines. Many stolen unashamedly from Steven's Book */
/* Much modification by Christian Magnusson especially for Valgrind and embedded */
/* non-threaded fixes by Jerry Scharf */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"

/* Read "n" bytes from a descriptor. */
/* Stolen from Unix Network Programming by Stevens, Fenner, Rudoff p89 */
/* return < 0 if failure */
ssize_t udp_read(FILE_DESCRIPTOR_OR_ERROR file_descriptor, void *vptr, size_t n, const struct timeval * ptv, struct sockaddr_in *from, socklen_t *fromlen)
{
	while ( 1 ) {
		int select_result;
		fd_set readset;
		struct timeval tv ;

		/* Initialize readset */
		FD_ZERO(&readset);
		FD_SET(file_descriptor, &readset);

		/* Read if it doesn't timeout first */
		timercpy( &tv, ptv ) ;
		select_result = select(file_descriptor + 1, &readset, NULL, NULL, &tv);
		if (select_result > 0) {
			ssize_t read_or_error = 0 ;
			
			/* Is there something to read? */
			if (FD_ISSET(file_descriptor, &readset) == 0) {
				return -EIO;	/* error */
			}
			
			read_or_error = recvfrom(file_descriptor, vptr, n, 0, (struct sockaddr *)from, fromlen) ;
			
			if ( read_or_error < 0 ) {
				errno = 0;	// clear errno. We never use it anyway.
				ERROR_DATA("udp read error");
				return -EIO;
			} else {
				//Debug_Bytes( "UDPread",ptr, nread ) ;
				return read_or_error ;
			}
		} else if (select_result < 0) {	/* select error */
			if (errno == EINTR) {
				/* select() was interrupted, try again */
				continue;
			}
			ERROR_DATA("udp read selection error (network)");
			return -EIO;
		} else {				/* timed out */
			LEVEL_CONNECT("udp read timeout");
			return -EAGAIN;
		}
	}
}
