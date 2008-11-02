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
//#include <dlfcn.h>

#include "ow.h"
#include "ow_avahi.h"
#include "ow_dl.h"

#if OW_ZERO

DLHANDLE avahi_client = NULL;
DLHANDLE avahi_common = NULL;

/* Special functions that need to be linked in dynamically
-- libavahi-client.so
avahi_client_errno
avahi_client_free
avahi_client_new
avahi_entry_group_add_service
avahi_entry_group_commit
avahi_entry_group_is_empty
avahi_entry_group_new
avahi_entry_group_reset
--libavahi-common.so
avahi_simple_poll_free
avahi_simple_poll_get
avahi_simple_poll_loop
avahi_simple_poll_new
avahi_simple_poll_quit
avahi_strerror
*/

#if OW_CYGWIN

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#endif

#define DNSfunction_link( lib , name )  name = DL_sym( lib, #name );\
	if ( name == NULL ) {\
		LEVEL_CONNECT("Avahi is disabled since "#name" isn't found\n");\
		return -1;\
	}
	
int OW_Load_avahi_library(void)
{
#if OW_CYGWIN
	// No avahi in Cygwin
	return -1 ;
#elif OW_DARWIN
	// At least currently, it's easier to use the native Bonjour on Mac OSX
	return -1 ;

#elif defined(HAVE_DLOPEN)
	if (
		(avahi_client=DL_open("libavahi-client.so", RTLD_LAZY)) == NULL
		&&
		(avahi_client=DL_open("/usr/lib/libavahi-client.so", RTLD_LAZY)) == NULL
		&&
		(avahi_client=DL_open("/opt/owfs/lib/libavahi-client.so", RTLD_LAZY)) == NULL
	) {
		LEVEL_CONNECT("No Avahi support. Library libavahi-client couldn't be loaded.\n") ;
		return -1 ;
	}
	DNSfunction_link(avahi_client,avahi_client_errno) ;
	DNSfunction_link(avahi_client,avahi_client_free) ;
	DNSfunction_link(avahi_client,avahi_client_new) ;
	DNSfunction_link(avahi_client,avahi_entry_group_add_service) ;
	DNSfunction_link(avahi_client,avahi_entry_group_commit) ;
	DNSfunction_link(avahi_client,avahi_entry_group_is_empty) ;
	DNSfunction_link(avahi_client,avahi_entry_group_new) ;
	DNSfunction_link(avahi_client,avahi_entry_group_reset) ;

	if (
		(avahi_common=DL_open("libavahi-common.so", RTLD_LAZY)) == NULL
		&&
		(avahi_common=DL_open("/usr/lib/libavahi-common.so", RTLD_LAZY)) == NULL
		&&
		(avahi_common=DL_open("/opt/owfs/lib/libavahi-common.so", RTLD_LAZY)) == NULL
		) {
		LEVEL_CONNECT("No Avahi support. Library libavahi-commonn couldn't be loaded.\n") ;
		return -1 ;
	}
	DNSfunction_link(avahi_common,avahi_simple_poll_free) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_get) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_loop) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_new) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_quit) ;
	DNSfunction_link(avahi_common,avahi_strerror) ;
	return 0 ;
#endif
				
	return -1;
}

void OW_Free_avahi_library(void)
{
	if (avahi_client) {
		DL_close(avahi_client);
		avahi_client = NULL;
	}
	if (avahi_common) {
		DL_close(avahi_common);
		avahi_common = NULL;
	}
}
				
#endif							/* OW_ZERO */
