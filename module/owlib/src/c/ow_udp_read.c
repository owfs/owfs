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
ssize_t udp_read(int file_descriptor, void *vptr, size_t n, const struct timeval * ptv, struct sockaddr_in *from, socklen_t *fromlen)
{
	size_t nleft;
	char *ptr;

	ptr = vptr;
	nleft = n;

	while (nleft > 0) {
		int select_result;
		fd_set readset;
		struct timeval tv = { ptv->tv_sec, ptv->tv_usec, };

		/* Initialize readset */
		FD_ZERO(&readset);
		FD_SET(file_descriptor, &readset);

		/* Read if it doesn't timeout first */
		select_result = select(file_descriptor + 1, &readset, NULL, NULL, &tv);
		if (select_result > 0) {
			ssize_t read_or_error;
			/* Is there something to read? */
			if (FD_ISSET(file_descriptor, &readset) == 0) {
				return -EIO;	/* error */
			}
			read_or_error = recvfrom(file_descriptor, ptr, nleft, 0, (struct sockaddr *)from, fromlen) ;
			if ( read_or_error < 0 ) {
				if (errno == EINTR) {
					errno = 0;	// clear errno. We never use it anyway.
				} else {
					ERROR_DATA("udp read error\n");
					return -EIO;
				}
			} else if (read_or_error == 0) {
				break;			/* EOF */
			} else {
				//Debug_Bytes( "UDPread",ptr, nread ) ;
				nleft -= read_or_error;
				ptr += read_or_error;
			}
		} else if (select_result < 0) {	/* select error */
			if (errno == EINTR) {
				/* select() was interrupted, try again */
				continue;
			}
			ERROR_DATA("udp read selection error (network)\n");
			return -EIO;
		} else {				/* timed out */
			LEVEL_CONNECT("udp read timeout after %d bytes\n", n - nleft);
			return -EAGAIN;
		}
	}
	return (n - nleft);			/* return >= 0 */
}
