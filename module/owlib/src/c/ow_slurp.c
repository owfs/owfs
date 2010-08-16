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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

/* slurp up any pending chars -- used at the start to clear the com buffer */
void Slurp( FILE_DESCRIPTOR_OR_ERROR file_descriptor, unsigned long usec )
{
	BYTE data[1] ;
	while (1) {
		fd_set readset;
		struct timeval tv;
		
		// very short timeout
		tv.tv_sec = 0;
		tv.tv_usec = usec;
		/* Initialize readset */
		FD_ZERO(&readset);
		FD_SET(file_descriptor, &readset);
		
		/* Read if it doesn't timeout first */
		if ( select(file_descriptor + 1, &readset, NULL, NULL, &tv) < 1 ) {
			return ;
		}
		if (FD_ISSET(file_descriptor, &readset) == 0) {
			return ;
		}
		ignore_result = read(file_descriptor, data, 1) ;
		LEVEL_DEBUG("Slurped in %.2X",data[0]);
	}
}
