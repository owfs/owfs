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

struct RefStruct {
    DNSServiceRef sref ;
} ;

/* Look for new services with Bonjour -- will block so done in a separate thread */
static void * Process( void * v ) {
    struct RefStruct * rs = v ;
    pthread_detach( pthread_self() ) ;
    while ( DNSServiceProcessResult(rs->sref) == kDNSServiceErr_NoError ) {
        printf("DNSServiceProcessResult ref %ld\n",(long int)rs->sref) ;
        continue ;
    }
    DNSServiceRefDeallocate(rs->sref) ;
    free(rs) ;
    return NULL ;
}

/* Resolved service -- add to connection_in */
static void ResolveBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) {
    ASCII name[121] ;
    struct connection_in * in ;
    (void) tl ;
    (void) t ;
    (void) c ;
    LEVEL_DETAIL("ResolveBack ref=%d flags=%d index=%d, error=%d name=%s host=%s port=%d\n",s,f,i,e,name,host,ntohs(port)) ;
    /* remove trailing .local. */
    if ( snprintf(name,120,"%s:%d",SAFESTRING(host),ntohs(port)) < 0 ) {
        ERROR_CONNECT("Trouble with zeroconf resolve return %s\n",n) ;
        return ;
    }
    CONNINLOCK ;
        for ( in = indevice ; in ; in=in->next ) if ( strcasecmp(in->name,name)==0 ) break ;
        if ( in==NULL && OW_ArgNet(name)==0 )
            if ( Zero_detect(indevice)==0 ) BadAdapter_detect(indevice) ;
    CONNINUNLOCK ;
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) {
    struct RefStruct * rs ;
    (void) context ;
    LEVEL_DETAIL("BrowseBack ref=%d flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",s,f,i,e,name,type,domain) ;
        
    if ( (rs = malloc(sizeof(struct RefStruct))) == NULL ) return ;

    if ( e!=kDNSServiceErr_NoError ) return ;
    //printf("BrowseBack noerror\n");
    if ( (f & kDNSServiceFlagsAdd) == 0 ) return ;
    //printf("BrowseBack add\n");
    if ( DNSServiceResolve( &(rs->sref), 0,0,name,type,domain,ResolveBack,NULL) == kDNSServiceErr_NoError ) {
        pthread_t thread ;
        int err = pthread_create( &thread, 0, Process, (void *) rs ) ;
        if ( err ) {
            ERROR_CONNECT("Zeroconf/Bounjour resolve thread error %d).\n",err) ;
        }
    }
}

void OW_Browse( void ) {
    struct RefStruct * rs ;
    DNSServiceErrorType dnserr ;
    
    if ( (rs = malloc(sizeof(struct RefStruct))) == NULL ) return ;

    dnserr = DNSServiceBrowse( &Global.browse, 0,0,"_owserver._tcp",NULL,BrowseBack,NULL) ;
    rs->sref = Global.browse ;
    if ( dnserr == kDNSServiceErr_NoError ) {
        pthread_t thread ;
        int err = pthread_create( &thread, 0, Process, (void *) rs ) ;
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
