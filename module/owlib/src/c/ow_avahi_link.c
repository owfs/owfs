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

#if OW_ZERO

#include "ow.h"

#include "ow_dl.h"

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
	
GOOD_OR_BAD OW_Load_avahi_library(void)
{
#if OW_CYGWIN
	LEVEL_DEBUG("No avahi in Cygwin");
	return gbBAD ;
#elif OW_DARWIN
	LEVEL_DEBUG("At least currently, it's easier to use the native Bonjour on Mac OSX");
	return gbBAD ;

#elif defined(HAVE_DLOPEN)
#if OW_DEBUG
#endif /* OW_DEBUG */
	if (
		(avahi_client=DL_open("libavahi-client.so")) == NULL
		&&
		(avahi_client=DL_open("/usr/lib/libavahi-client.so")) == NULL
		&&
		(avahi_client=DL_open("/opt/owfs/lib/libavahi-client.so")) == NULL
	) {
		LEVEL_CONNECT("No Avahi support. Library libavahi-client couldn't be loaded") ;
		return gbBAD ;
	} else {
		LEVEL_DEBUG("Avahi support: libavahi-client loaded successfully");
	}
	DNSfunction_link(avahi_client,avahi_client_errno) ;
	DNSfunction_link(avahi_client,avahi_client_free) ;
	DNSfunction_link(avahi_client,avahi_client_new) ;
	DNSfunction_link(avahi_client,avahi_client_get_domain_name) ;
	DNSfunction_link(avahi_client,avahi_entry_group_add_service) ;
	DNSfunction_link(avahi_client,avahi_entry_group_commit) ;
	DNSfunction_link(avahi_client,avahi_entry_group_is_empty) ;
	DNSfunction_link(avahi_client,avahi_entry_group_new) ;
	DNSfunction_link(avahi_client,avahi_entry_group_reset) ;

	DNSfunction_link(avahi_client,avahi_service_resolver_free) ;
	DNSfunction_link(avahi_client,avahi_service_resolver_new) ;
	DNSfunction_link(avahi_client,avahi_service_browser_free) ;
	DNSfunction_link(avahi_client,avahi_service_browser_new) ;

	if (
		(avahi_common=DL_open("libavahi-common.so")) == NULL
		&&
		(avahi_common=DL_open("/usr/lib/libavahi-common.so")) == NULL
		&&
		(avahi_common=DL_open("/opt/owfs/lib/libavahi-common.so")) == NULL
		) {
		LEVEL_CONNECT("No Avahi support. Library libavahi-common couldn't be loaded.") ;
		return gbBAD ;
	} else {
		LEVEL_DEBUG("Avahi support: libavahi-common loaded successfully.");
	}
	DNSfunction_link(avahi_common,avahi_simple_poll_free) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_get) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_loop) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_new) ;
	DNSfunction_link(avahi_common,avahi_simple_poll_quit) ;
	DNSfunction_link(avahi_common,avahi_strerror) ;
	return gbGOOD ;
#endif
	LEVEL_CONNECT("dlopen not supported on this platform") ;			
	return gbBAD;
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
