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

#include "owshell.h"

#if OW_ZERO

#if OW_CYGWIN

struct function_table_t {
  void *func;
  char name[80];
};

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

#endif /* OW_CYGWIN */


static void ResolveBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) ;
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) ;
static void HandleCall( DNSServiceRef sref ) ;

static void HandleCall( DNSServiceRef sref ) {
    int fd;
    DNSServiceErrorType err = kDNSServiceErr_Unknown ;
    fd = DNSServiceRefSockFD(NULL) ;
    fprintf(stderr, "HandleCall: fd=%d (just a test of function-call, should result -1)\n", fd);
    fd = DNSServiceRefSockFD(sref) ;
    fprintf(stderr, "HandleCall: fd=%d\n", fd);
    if ( fd >= 0 ) {
        while (1) {
            fd_set readfd ;
	    int rc;
            struct timeval tv = {10,0} ;
            FD_ZERO(&readfd) ;
            FD_SET(fd,&readfd) ;
            rc = select(fd+1,&readfd, NULL, NULL, &tv );
            if ( rc == -1 ) {
	        if ( errno == EINTR ) {
		  continue ;
		}
                perror("Service Discovery select returned error\n") ;
	    } else if ( rc > 0 ) {
                if ( FD_ISSET(fd,&readfd) ) {
                    err = DNSServiceProcessResult(sref) ;
                }
            } else {
                perror("Service Discovery timed out\n") ;
            }
            break ;
        }
    } else {
        fprintf(stderr,"No Service Discovery socket\n") ;
    }
    DNSServiceRefDeallocate(sref) ;
    if ( err != kDNSServiceErr_NoError ) {
        fprintf(stderr,"Service Discovery Process result error  0x%X\n",(int)err ) ;
        exit(1) ;
    }
}


static void ResolveBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) {
    ASCII name[121] ;
    struct connection_in * in ;
    (void) tl ;
    (void) t ;
    (void) c ;
    (void) n ;
    fprintf(stderr, "ResolveBack ref=%ld flags=%ld index=%ld, error=%d name=%s host=%s port=%d\n",(long int)s,(long int)f,(long int)i,(int)e,SAFESTRING(name),SAFESTRING(host),ntohs(port)) ;
    if ( snprintf(name,120,"%s:%d",SAFESTRING(host),ntohs(port)) < 0 ) {
        perror("Trouble with zeroconf resolve return\n") ;
    } else if ( (in=NewIn()) ) {
        in->name = strdup(name) ;
        return ;
    }
    exit(1) ;
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) {
    (void) context ;
    fprintf(stderr, "BrowseBack ref=%ld flags=%ld index=%ld, error=%d name=%s type=%s domain=%s\n",(long int)s,(long int)f,(long int)i,(int)e,SAFESTRING(name),SAFESTRING(type),SAFESTRING(domain)) ;
    
    if ( e == kDNSServiceErr_NoError ) {

        if ( f & kDNSServiceFlagsAdd ) { // Add
            DNSServiceRef sr ;
                
            if ( DNSServiceResolve( &sr, 0,0,name,type,domain,ResolveBack,NULL) == kDNSServiceErr_NoError ) {
                HandleCall(sr) ;
                return ;
            } else {
                fprintf(stderr,"Service Resolve error on %s\n",SAFESTRING(name)) ;
            }
        } else {
            fprintf(stderr,"OWSERVER %s is leaving\n",name) ;
        }
    } else {
        fprintf(stderr,"Browse callback error = %d\n", (int)e) ;
    }
    exit(1) ;
}

void OW_Browse( void ) {
    DNSServiceErrorType dnserr ;
    DNSServiceRef sref ;

    dnserr = DNSServiceBrowse( &sref, 0,0,"_owserver._tcp",NULL,BrowseBack,NULL) ;
    if ( dnserr == kDNSServiceErr_NoError ) {
        HandleCall(sref) ;
    } else {
        fprintf(stderr,"DNSServiceBrowse error = %d\n", (int)dnserr) ;
        exit(1) ;
    }
}

#endif /* OW_ZERO */
