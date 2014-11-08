/*
OWFS -- One-Wire filesystem
OWHTTPD -- One-Wire Web Server
Written 2003 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPL
See the header file: ow.h for full attribution
1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"

#if OW_ZERO && ! OW_DARWIN

#include "ow.h"

#include "ow_dl.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define DNSfunction_link( lib , name )  do { name = DL_sym( lib, #name );\
	if ( name == NULL ) {\
		LEVEL_CONNECT("Avahi is DISABLED since "#name" isn't found");\
		return gbBAD;\
	} else { \
		LEVEL_DEBUG("Avahi library function found: "#name);\
	} } while (0)
	

#endif							/* OW_ZERO && ! OW_DARWIN */
