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

static void ResolveBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) ;
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) ;
static void HandleCall( DNSServiceRef sref ) ;

static void HandleCall( DNSServiceRef sref ) {
    int fd = DNSServiceRefSockFD(sref) ;
    DNSServiceErrorType err = kDNSServiceErr_Unknown ;
    if ( fd >= 0 ) {
        while (1) {
            fd_set readfd ;
            struct timeval tv = {10,0} ;
            FD_ZERO(&readfd) ;
            FD_SET(fd,&readfd) ;
            if ( select(fd+1,&readfd, NULL, NULL, &tv ) > 0 ) {
                if ( FD_ISSET(fd,&readfd) ) {
                    err = DNSServiceProcessResult(sref) ;
                }
            } else if ( errno == EINTR ) {
                continue ;
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
        fprintf(stderr,"Service Discovery Process result error  %d\n",err ) ;
        exit(1) ;
    }
}


static void ResolveBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) {
    ASCII name[121] ;
    struct connection_in * in ;
    (void) tl ;
    (void) t ;
    (void) c ;
    printf("ResolveBack ref=%ld flags=%d index=%d, error=%d name=%s host=%s port=%d\n",(long int)s,f,i,e,name,host,ntohs(port)) ;
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
    printf("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,name,type,domain) ;
    
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
        fprintf(stderr,"Browse callback error = %d\n",e) ;
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
        fprintf(stderr,"DNSServiceBrowse error = %d\n",dnserr) ;
        exit(1) ;
    }
}

#endif /* OW_ZERO */
