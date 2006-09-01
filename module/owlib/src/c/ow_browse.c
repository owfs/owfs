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
#include "ow_connection.h"

#if OW_ZERO

#if OW_MT

/* Resolved service -- add to connection_in */
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) {
    ASCII name[121] ;
    struct connection_in * in ;
    int len = host==NULL ? -1 : (int)strlen(host) ;
    (void) tl ;
    (void) t ;
    (void) c ;
    LEVEL_DETAIL("Resolve Callback ref=%d flags=%d index=%d, error=%d name=%s host=%s port=%d\n",s,f,i,e,name,host,ntohs(port)) ;
    /* remove trailing .local. */
    if ( len < 0 ) return ;
    //if ( len >= 7 && strncmp(".local.",&host[len-7],7)==0 ) len -= 7 ;
    if ( snprintf(name,120,"%.*s:%d",len,host,ntohs(port)) < 0 ) {
        ERROR_CONNECT("Trouble with zeroconf browse return %s\n",n) ;
        return ;
    }
    CONNINLOCK ;
        for ( in = indevice ; in ; in=in->next ) if ( strcasecmp(in->name,name)==0 ) break ;
        if ( in==NULL && OW_ArgNet(name)==0 && Server_detect(indevice)==0 ) indevice->busmode = bus_zero ;
    CONNINUNLOCK ;
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void CallBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) {
    DNSServiceRef sref ;
    (void) context ;
    LEVEL_DETAIL("Browse Callback ref=%d flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",s,f,i,e,name,type,domain) ;
    if ( e!=kDNSServiceErr_NoError ) return ;
    printf("Browse Callback noerror\n");
    if ( (f & kDNSServiceFlagsAdd) == 0 ) return ;
    printf("Browse Callback add\n");
    if ( DNSServiceResolve( &sref, 0,0,name,type,domain,BrowseBack,NULL) == kDNSServiceErr_NoError ) {
        printf("Browse Callback Resolve\n");
        DNSServiceProcessResult(sref) ;
        printf("Browse Callback Resolve Process\n");
        DNSServiceRefDeallocate(sref) ;
        printf("Browse Callback Resolve Deallocate\n");
    }
}

/* Look for new services with Bonjour -- will block so done in a separate thread */
static void * Browse( void * v ) {
    (void) v ;
    pthread_detach( pthread_self() ) ;
    while ( DNSServiceProcessResult(Global.browse) == kDNSServiceErr_NoError ) {
        //printf("DNSServiceProcessResult in Browse\n") ;
        continue ;
    }
    return NULL ;
}

void OW_Browse( void ) {
    DNSServiceErrorType dnserr = DNSServiceBrowse( &Global.browse, 0,0,"_owserver._tcp",NULL,CallBack,NULL) ;
    if ( dnserr == kDNSServiceErr_NoError ) {
        pthread_t thread ;
        int err = pthread_create( &thread, 0, Browse, NULL ) ;
        if ( err ) {
            ERROR_CONNECT("Zeroconf/Bounjour browsing thread error %d).\n",err) ;
        }
    } else {
        LEVEL_CONNECT("DNSServiceBrowse error = %d\n",dnserr) ;
    }
}

#else /* OW_MT */

void OW_Browse( void ) {
    LEVEL_CONNECT("Zeroconf/Bonjour requires multithreading support (a compile-time configuration setting).\n");
}

#endif /* OW_MT */

#endif /* OW_ZERO */
