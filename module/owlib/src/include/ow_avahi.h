/*
	OWFS -- One-Wire filesystem
	OWHTTPD -- One-Wire Web Server
	Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

// Some of this code taken from the Avahi project (legally)
// The following header is in those header files:
/***
This file is part of avahi.

avahi is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

avahi is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with avahi; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
USA.
***/
// Note that although these headers are LGPL v2.1+, the rest of OWFS is GPL v2

#ifndef OW_AVAHI_H
#define OW_AVAHI_H

#if OW_AVAHI

#include <avahi-client/client.h>
#include <avahi-common/thread-watch.h>

typedef enum { EnumDummy, } Enum ;



/**********************************************/
/* Prototypes */
struct connection_out ;
GOOD_OR_BAD OW_Load_avahi_library(void) ;
void OW_Free_avahi_library(void) ;
void *OW_Avahi_Announce( void * v ) ;
void *OW_Avahi_Browse(void * v) ;

#endif /* OW_AVAHI */

#endif 	/* OW_AVAHI_H */
