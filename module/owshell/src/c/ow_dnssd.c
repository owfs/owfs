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
#include "ow.h"

#include "ow_dl.h"
#include "ow_dnssd.h"

DLHANDLE libdnssd = NULL;

_DNSServiceRefSockFD DNSServiceRefSockFD;
_DNSServiceProcessResult DNSServiceProcessResult;
_DNSServiceRefDeallocate DNSServiceRefDeallocate;
_DNSServiceResolve DNSServiceResolve;
_DNSServiceBrowse DNSServiceBrowse;
_DNSServiceRegister DNSServiceRegister;
_DNSServiceReconfirmRecord DNSServiceReconfirmRecord;
_DNSServiceCreateConnection DNSServiceCreateConnection;
_DNSServiceEnumerateDomains DNSServiceEnumerateDomains;

int OW_Load_dnssd_library(void) {
  char file[255];
#if OW_CYGWIN
  strcpy(file, "dnssd.dll");
  if (!(libdnssd = DL_open(file, RTLD_LAZY))) {
    /* Couldn't open that lib, but continue anyway */
#if 0
    char *derr;
    derr = DL_error();
    fprintf(stderr, "DL_open [%s] failed [%s]\n", file, derr);
#endif
  }
#elif defined(HAVE_DLOPEN)
  int i;
  char libdirs[4][80] = {
    { "/opt/owfs/lib/libdns_sd.so" },
    { "libdns_sd.so" },
    { "" }
  };

  i = 0;
  while(*libdirs[i]) {
    strcpy(file, libdirs[i]);

    if (!(libdnssd = DL_open(file, RTLD_LAZY))) {
      /* Couldn't open that lib, but continue anyway */
#if 0
      char *derr;
      derr = DL_error();
      fprintf(stderr, "DL_open [%s] failed [%s]\n", file, derr);
#endif
      i++;
      continue;
    } else {
#if 0
      fprintf(stderr, "DL_open [%s] success\n", file);
#endif
      break;
    }
  }
#endif

  if(libdnssd == NULL) {
    //fprintf(stderr, "Zeroconf/Bonjour is disabled since dnssd library is not found\n");
    return -1;
  }

  DNSServiceRefSockFD = (_DNSServiceRefSockFD)DL_sym(libdnssd, "DNSServiceRefSockFD");
  //fprintf(stderr, "DNSServiceRefSockFD=%p\n", (void *)DNSServiceRefSockFD);
  DNSServiceProcessResult = (_DNSServiceProcessResult)DL_sym(libdnssd, "DNSServiceProcessResult");
  DNSServiceRefDeallocate = (_DNSServiceRefDeallocate)DL_sym(libdnssd, "DNSServiceRefDeallocate");
  DNSServiceResolve = (_DNSServiceResolve)DL_sym(libdnssd, "DNSServiceResolve");
  DNSServiceBrowse = (_DNSServiceBrowse)DL_sym(libdnssd, "DNSServiceBrowse");
  DNSServiceRegister  = (_DNSServiceRegister)DL_sym(libdnssd, "DNSServiceRegister");
  DNSServiceReconfirmRecord = (_DNSServiceReconfirmRecord)DL_sym(libdnssd, "DNSServiceReconfirmRecord");
  DNSServiceCreateConnection = (_DNSServiceCreateConnection)DL_sym(libdnssd, "DNSServiceCreateConnection");
  DNSServiceEnumerateDomains = (_DNSServiceEnumerateDomains)DL_sym(libdnssd, "DNSServiceEnumerateDomains");
  return 0;
}

int OW_Free_dnssd_library(void) {
  int rc = -1;
  if(libdnssd)  rc = DL_close(libdnssd);
  libdnssd = NULL;
  return rc;
}
