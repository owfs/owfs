/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2006 Paul H Alfille
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
#include "ow_dnssd.h"

DLHANDLE libdnssd = NULL;

#if OW_CYGWIN

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#endif /* OW_CYGWIN */

_DNSServiceRefSockFD DNSServiceRefSockFD;
_DNSServiceProcessResult DNSServiceProcessResult;
_DNSServiceRefDeallocate DNSServiceRefDeallocate;
_DNSServiceResolve DNSServiceResolve;
_DNSServiceBrowse DNSServiceBrowse;
_DNSServiceRegister DNSServiceRegister;
_DNSServiceReconfirmRecord DNSServiceReconfirmRecord;
_DNSServiceCreateConnection DNSServiceCreateConnection;
_DNSServiceEnumerateDomains DNSServiceEnumerateDomains;

#define DNSfunction_link( name )  name = (_##name) DL_sym( libdnssd, #name );\
	if ( name == NULL ) {\
		LEVEL_CONNECT("Zeroconf/Bonjour is disabled since "#name" isn't found");\
		return -1;\
	} else { \
		LEVEL_DEBUG("Linked in Bonjour function "#name) ;\
	}

int OW_Load_dnssd_library(void)
{
	int i ;
	libdnssd = NULL ; // marker for successful opening of library

	char libdirs[3][80] = {

#if OW_CYGWIN
		//{ "/opt/owfs/lib/libdns_sd.dll" },
		{"libdns_sd.dll"},

#elif OW_DARWIN
	// MacOSX have dnssd functions in libSystem
		{"libSystem.dylib"},

#elif defined(HAVE_DLOPEN)
		{"/opt/owfs/lib/libdns_sd.so"},
		{"libdns_sd.so"},
#endif

		{""} // needed at end
	};

	for ( i=0 ; *libdirs[i] ; ++i ) {
		libdnssd = DL_open( libdirs[i] ) ;

		if ( libdnssd != NULL ) {
			LEVEL_CONNECT( "DL_open [%s] success", libdirs[i] );
			break ;
		}
	}

	if (libdnssd == NULL) {
		LEVEL_CONNECT("Zeroconf/Bonjour is disabled since dnssd library isn't found");
		return -1;
	}

	DNSfunction_link(DNSServiceRefSockFD);
	DNSfunction_link(DNSServiceProcessResult);
	DNSfunction_link(DNSServiceRefDeallocate);
	DNSfunction_link(DNSServiceResolve);
	DNSfunction_link(DNSServiceBrowse);
	DNSfunction_link(DNSServiceRegister);
	DNSfunction_link(DNSServiceReconfirmRecord);
	DNSfunction_link(DNSServiceCreateConnection);
	DNSfunction_link(DNSServiceEnumerateDomains);

	return 0;
}

void OW_Free_dnssd_library(void)
{
	if (libdnssd) {
		DL_close(libdnssd);
		libdnssd = NULL;
	}
}

#endif							/* OW_ZERO */
