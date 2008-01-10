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
#endif

#endif

_DNSServiceRefSockFD DNSServiceRefSockFD;
_DNSServiceProcessResult DNSServiceProcessResult;
_DNSServiceRefDeallocate DNSServiceRefDeallocate;
_DNSServiceResolve DNSServiceResolve;
_DNSServiceBrowse DNSServiceBrowse;
_DNSServiceRegister DNSServiceRegister;
_DNSServiceReconfirmRecord DNSServiceReconfirmRecord;
_DNSServiceCreateConnection DNSServiceCreateConnection;
_DNSServiceEnumerateDomains DNSServiceEnumerateDomains;

int OW_Load_dnssd_library(void)
{
	int i = 0;
#if OW_CYGWIN
	char libdirs[3][80] = {
		//{ "/opt/owfs/lib/libdns_sd.dll" },
		{"libdns_sd.dll"},
		{""}
	};

	while (*libdirs[i]) {
		/* Cygwin has dlopen and it seems to be ok to use it actually. */
		if (!(libdnssd = DL_open(libdirs[i], 0))) {
			/* Couldn't open that lib, but continue anyway */
#if 0
			char *derr;
			derr = DL_error();
			fprintf(stderr, "dlopen [%s] failed [%s]\n", libdirs[i], derr);
#endif
			i++;
			continue;
		} else {
			//fprintf(stderr, "DL_open [%s] success\n", libdirs[i]);
			break;
		}
	}
#if 0
	/* This file compiled with Microsoft Visual C doesn't work actually... */
	if (!libdnssd) {
		char file[255];
		strcpy(file, "dnssd.dll");
		if (!(libdnssd = DL_open(file, 0))) {
			/* Couldn't open that lib, but continue anyway */
		}
	}
#endif

#elif OW_DARWIN

	// MacOSX have dnssd functions in libSystem
	char libdirs[2][80] = {
		{"libSystem.dylib"},
		{""}
	};

	while (*libdirs[i]) {
		if (!(libdnssd = DL_open(libdirs[i], RTLD_LAZY))) {
			/* Couldn't open that lib, but continue anyway */
#if 0
			char *derr;
			derr = DL_error();
			fprintf(stderr, "DL_open [%s] failed [%s]\n", libdirs[i],
					derr);
#endif
			i++;
			continue;
		} else {
			//fprintf(stderr, "DL_open [%s] success\n", libdirs[i]);
			break;
		}
	}

#elif defined(HAVE_DLOPEN)

	char libdirs[3][80] = {
		{"/opt/owfs/lib/libdns_sd.so"},
		{"libdns_sd.so"},
		{""}
	};

	while (*libdirs[i]) {
		if (!(libdnssd = DL_open(libdirs[i], RTLD_LAZY))) {
			/* Couldn't open that lib, but continue anyway */
#if 0
			char *derr;
			derr = DL_error();
			fprintf(stderr, "DL_open [%s] failed [%s]\n", libdirs[i],
					derr);
#endif
			i++;
			continue;
		} else {
			//fprintf(stderr, "DL_open [%s] success\n", libdirs[i]);
			break;
		}
	}
#endif

	if (libdnssd == NULL) {
		//fprintf(stderr, "Zeroconf/Bonjour is disabled since dnssd library isn't found\n");
		return -1;
	}

	DNSServiceRefSockFD =
		(_DNSServiceRefSockFD) DL_sym(libdnssd, "DNSServiceRefSockFD");
	DNSServiceProcessResult =
		(_DNSServiceProcessResult) DL_sym(libdnssd,
										  "DNSServiceProcessResult");
	DNSServiceRefDeallocate =
		(_DNSServiceRefDeallocate) DL_sym(libdnssd,
										  "DNSServiceRefDeallocate");
	DNSServiceResolve =
		(_DNSServiceResolve) DL_sym(libdnssd, "DNSServiceResolve");
	DNSServiceBrowse =
		(_DNSServiceBrowse) DL_sym(libdnssd, "DNSServiceBrowse");
	DNSServiceRegister =
		(_DNSServiceRegister) DL_sym(libdnssd, "DNSServiceRegister");
	DNSServiceReconfirmRecord =
		(_DNSServiceReconfirmRecord) DL_sym(libdnssd,
											"DNSServiceReconfirmRecord");
	DNSServiceCreateConnection =
		(_DNSServiceCreateConnection) DL_sym(libdnssd,
											 "DNSServiceCreateConnection");
	DNSServiceEnumerateDomains =
		(_DNSServiceEnumerateDomains) DL_sym(libdnssd,
											 "DNSServiceEnumerateDomains");

	return 0;
}

int OW_Free_dnssd_library(void)
{
	int rc = -1;
	if (libdnssd) {
		rc = DL_close(libdnssd);
		libdnssd = NULL;
	}
	return rc;
}

#endif							/* OW_ZERO */
