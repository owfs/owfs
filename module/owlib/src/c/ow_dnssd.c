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

#if OW_ZERO
#if OW_CYGWIN

#include "ow_dnssd.h"

#if 0
typedef int DNSSD_API (*_DNSServiceRefSockFD)(DNSServiceRef sdRef);
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceProcessResult)(DNSServiceRef sdRef);
typedef void DNSSD_API (*_DNSServiceRefDeallocate)(DNSServiceRef sdRef);
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceRegister)
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,         /* may be NULL */
    const char                          *regtype,
    const char                          *domain,       /* may be NULL */
    const char                          *host,         /* may be NULL */
    uint16_t                            port,
    uint16_t                            txtLen,
    const void                          *txtRecord,    /* may be NULL */
    DNSServiceRegisterReply             callBack,      /* may be NULL */
    void                                *context       /* may be NULL */
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceAddRecord)
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        *RecordRef,
    DNSServiceFlags                     flags,
    uint16_t                            rrtype,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceUpdateRecord)
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        RecordRef,     /* may be NULL */
    DNSServiceFlags                     flags,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceRemoveRecord)
    (
    DNSServiceRef                 sdRef,
    DNSRecordRef                  RecordRef,
    DNSServiceFlags               flags
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceBrowse)
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *regtype,
    const char                          *domain,    /* may be NULL */
    DNSServiceBrowseReply               callBack,
    void                                *context    /* may be NULL */
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceResolve)
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    DNSServiceResolveReply              callBack,
    void                                *context  /* may be NULL */
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceRegisterRecord)
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        *RecordRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl,
    DNSServiceRegisterRecordReply       callBack,
    void                                *context    /* may be NULL */
    );
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceCreateConnection)(DNSServiceRef *sdRef);
typedef DNSServiceErrorType DNSSD_API (*_DNSServiceQueryRecord)
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    DNSServiceQueryRecordReply          callBack,
    void                                *context  /* may be NULL */
    );
typedef void DNSSD_API (*_DNSServiceReconfirmRecord)
    (
    DNSServiceFlags                    flags,
    uint32_t                           interfaceIndex,
    const char                         *fullname,
    uint16_t                           rrtype,
    uint16_t                           rrclass,
    uint16_t                           rdlen,
    const void                         *rdata
    );
typedef int DNSSD_API (*_DNSServiceConstructFullName)
    (
    char                            *fullName,
    const char                      *service,      /* may be NULL */
    const char                      *regtype,
    const char                      *domain
    );
#endif

#include <Windows.h>
#include <w32api/winbase.h>
#include <w32api/windef.h>
#include <winnt.h>


_DNSServiceRefSockFD DNSServiceRefSockFD;
_DNSServiceProcessResult DNSServiceProcessResult;
_DNSServiceRefDeallocate DNSServiceRefDeallocate;
_DNSServiceResolve DNSServiceResolve;
_DNSServiceBrowse DNSServiceBrowse;
_DNSServiceRegister DNSServiceRegister;
_DNSServiceReconfirmRecord DNSServiceReconfirmRecord;
_DNSServiceCreateConnection DNSServiceCreateConnection;
_DNSServiceEnumerateDomains DNSServiceEnumerateDomains;

HMODULE libdnssd;

int OW_Load_dnssd_library(void) {
  fprintf(stderr, "Open dnssd.dll and find functions...\n");
  libdnssd = LoadLibrary("dnssd.dll");
  if(libdnssd == NULL) {
    fprintf(stderr, "Can't load dnssd.dll\n");
    exit(1);
  }

  DNSServiceRefSockFD = (_DNSServiceRefSockFD)GetProcAddress(libdnssd, "DNSServiceRefSockFD");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceRefSockFD", (void *)DNSServiceRefSockFD);

  DNSServiceProcessResult = (_DNSServiceProcessResult)GetProcAddress(libdnssd, "DNSServiceProcessResult");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceProcessResult", (void *)DNSServiceProcessResult);

  DNSServiceRefDeallocate = (_DNSServiceRefDeallocate)GetProcAddress(libdnssd, "DNSServiceRefDeallocate");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceRefDeallocate", (void *)DNSServiceRefDeallocate);

  DNSServiceResolve = (_DNSServiceResolve)GetProcAddress(libdnssd, "DNSServiceResolve");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceResolve", (void *)DNSServiceResolve);

  DNSServiceBrowse = (_DNSServiceBrowse)GetProcAddress(libdnssd, "DNSServiceBrowse");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceBrowse", (void *)DNSServiceBrowse);

  DNSServiceRegister = (_DNSServiceRegister)GetProcAddress(libdnssd, "DNSServiceRegister");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceRegister", (void *)DNSServiceRegister);

  DNSServiceReconfirmRecord = (_DNSServiceReconfirmRecord)GetProcAddress(libdnssd, "DNSServiceReconfirmRecord");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceReconfirmRecord", (void *)DNSServiceReconfirmRecord);

  DNSServiceCreateConnection = (_DNSServiceCreateConnection)GetProcAddress(libdnssd, "DNSServiceCreateConnection");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceCreateConnection", (void *)DNSServiceCreateConnection);

  DNSServiceEnumerateDomains = (_DNSServiceEnumerateDomains)GetProcAddress(libdnssd, "DNSServiceEnumerateDomains");
  fprintf(stderr, "function %s is at %p\n", "DNSServiceEnumerateDomains", (void *)DNSServiceEnumerateDomains);

  return 0;
}


int OW_Free_dnssd_library(void) {
  int rc = -1;
  if(libdnssd) rc = FreeLibrary(libdnssd);
  libdnssd = NULL;
  return rc;
}


#endif

#endif

