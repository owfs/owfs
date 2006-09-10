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

struct BrowseStruct {
    char * name ;
    char * type ;
    char * domain ;
} ;

static struct connection_in * FindIn( struct BrowseStruct * bs ) ;
static struct BrowseStruct * BSCreate( const char * name, const char * type, const char * domain ) ;
static void BSKill( struct BrowseStruct * bs ) ;
static void * Process( void * v ) ;
static void ResolveBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char *n, const char *host, uint16_t port, uint16_t tl, const char *t, void *c ) ;
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) ;

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
    struct BrowseStruct * bs = c ;
    struct connection_in * in ;
    (void) tl ;
    (void) t ;
    printf("ResolveBack ref=%ld flags=%d index=%d, error=%d name=%s host=%s port=%d\n",(long int)s,f,i,e,name,host,ntohs(port)) ;
    printf("ResolveBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,bs->name,bs->type,bs->domain) ;
    LEVEL_DETAIL("ResolveBack ref=%d flags=%d index=%d, error=%d name=%s host=%s port=%d\n",(long int)s,f,i,e,name,host,ntohs(port)) ;
    /* remove trailing .local. */
    if ( snprintf(name,120,"%s:%d",SAFESTRING(host),ntohs(port)) < 0 ) {
        ERROR_CONNECT("Trouble with zeroconf resolve return %s\n",n) ;
    } else {
        if ( (in=FindIn(bs))==NULL ) { // new or old connection_in slot?
            CONNINLOCK ;
                if ( (in=NewIn(NULL)) ) {
                    BUSLOCKIN(in) ;
                    in->name = strdup(name) ;
                    in->busmode = bus_server ;
                }
            CONNINUNLOCK ;
        }
        if (in) {
            if ( Zero_detect(in) ) BadAdapter_detect(in) ;
            BUSUNLOCKIN(in) ;
        }
    }
    BSKill(bs) ;
}

static struct BrowseStruct * BSCreate( const char * name, const char * type, const char * domain ) {
    struct BrowseStruct * bs = malloc(sizeof(struct BrowseStruct)) ;
    if ( bs ) {
        bs->name   = name   ? strdup( name   ) : NULL ;
        bs->type   = type   ? strdup( type   ) : NULL ;
        bs->domain = domain ? strdup( domain ) : NULL ;
    }
    return bs ;
}

static void BSKill( struct BrowseStruct * bs ) {
    if ( bs ) {
        if (bs->name) free(bs->name) ;
        if (bs->type) free(bs->type) ;
        if (bs->domain) free(bs->domain) ;
        free(bs) ;
    }
}

static struct connection_in * FindIn( struct BrowseStruct * bs ) {
    struct connection_in * now ;
    CONNINLOCK ;
        for ( now = indevice ; now ; now = now->next ) {
            if ( now->busmode!=bus_zero
                || strcasecmp(now->name,bs->name)
                 || strcasecmp(now->type,bs->type)
                 || strcasecmp(now->domain,bs->domain)
            ) continue ;
            BUSLOCKIN(now) ;
            break ;
        }
    CONNINUNLOCK ;
    return now ;
}

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void BrowseBack( DNSServiceRef s, DNSServiceFlags f, uint32_t i, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) {
    struct RefStruct * rs = malloc(sizeof(struct RefStruct)) ;
    struct BrowseStruct * bs = BSCreate(name,type,domain) ;
    struct connection_in * in ;
    (void) context ;
    printf("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,name,type,domain) ;
    LEVEL_DETAIL("BrowseBack ref=%ld flags=%d index=%d, error=%d name=%s type=%s domain=%s\n",(long int)s,f,i,e,name,type,domain) ;
        
    
    if ( e!=kDNSServiceErr_NoError || rs==NULL || bs==NULL ) goto unallocate ;

    //printf("BrowseBack noerror\n");
    if ( (in=FindIn(bs)) ) {
        FreeClientAddr(in) ;
        BUSUNLOCKIN(in) ;
    }
    if ( (f & kDNSServiceFlagsAdd) == 0 ) goto unallocate ; // Delete
    
    //Add
    //printf("BrowseBack add\n");
    if ( DNSServiceResolve( &(rs->sref), 0,0,name,type,domain,ResolveBack,bs) == kDNSServiceErr_NoError ) {
        pthread_t thread ;
        int err ;
        printf("Resolve %ld %s|%s|%s\n",(long int)rs->sref,name,type,domain) ;
        err = pthread_create( &thread, 0, Process, (void *) rs ) ;
        if ( err ) {
            ERROR_CONNECT("Zeroconf/Bounjour resolve thread error %d).\n",err) ;
            goto unallocate ;
        }
    }
    return ;

    unallocate: // free memory here unless callback and thread created
    BSKill(bs) ;
    if (rs) free(rs) ;
    return ;
}

void OW_Browse( void ) {
    struct RefStruct * rs ;
    DNSServiceErrorType dnserr ;
    
    if ( (rs = malloc(sizeof(struct RefStruct))) == NULL ) return ;

    dnserr = DNSServiceBrowse( &Global.browse, 0,0,"_owserver._tcp",NULL,BrowseBack,NULL) ;
    rs->sref = Global.browse ;
    if ( dnserr == kDNSServiceErr_NoError ) {
        pthread_t thread ;
        int err ;
        printf("Browse %ld %s|%s|%s\n",(long int)Global.browse,"","_owserver._tcp","") ;
        err = pthread_create( &thread, 0, Process, (void *) rs ) ;
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
