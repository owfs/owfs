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

struct announce_struct {
    struct connection_out * out ;
    ASCII * name ;
    int port ;
    ASCII * type0 ;
    ASCII * type1 ;
} ;

/* Sent back from Bounjour -- arbitrarily use it to set the Ref for Deallocation */
static void RegisterBack( DNSServiceRef s, DNSServiceFlags f, DNSServiceErrorType e, const char * name, const char * type, const char * domain, void * context ) {
    DNSServiceRef * sref = context ;
    LEVEL_DETAIL("RegisterBack ref=%d flags=%d error=%d name=%s type=%s domain=%s\n",s,f,e,name,type,domain) ;
    if ( e==kDNSServiceErr_NoError ) sref[0] = s ;
}

/* Register the out port with Bonjour -- might block so done in a separate thread */
static void * Announce( void * v ) {
    struct announce_struct * as = v ;
    DNSServiceRef sref ;
    DNSServiceErrorType err = DNSServiceRegister(&sref,0,0,Global.announce_name?Global.announce_name:as->name,as->type0,NULL,NULL,as->port,0,NULL,RegisterBack,&(as->out->sref0)) ;
    LEVEL_DEBUG("DNSServiceRequest attempt: index=%d, name=%s, port=%d, type=%s / %s\n",as->out->index,as->name,ntohs(as->port),as->type0,SAFESTRING(as->type1)) ;
#if OW_MT
    pthread_detach( pthread_self() ) ;
#endif /* OW_MT */
if ( err == kDNSServiceErr_NoError ) {
        DNSServiceProcessResult(sref) ;
    } else {
        ERROR_CONNECT("Unsuccessful call to DNSServiceRegister err = %d\n",err) ;
    }
    if (as->type1 ) {
        err = DNSServiceRegister(&sref,0,0,Global.announce_name?Global.announce_name:as->name,as->type1,NULL,NULL,as->port,0,NULL,RegisterBack,&(as->out->sref1)) ;
        if ( err == kDNSServiceErr_NoError ) {
            DNSServiceProcessResult(sref) ;
        } else {
            ERROR_CONNECT("Unsuccessful call to DNSServiceRegister err = %d\n",err) ;
        }
    }
    free(as) ;
    return NULL ;
}

void OW_Announce( struct connection_out * out ) {
    struct announce_struct * as = malloc( sizeof(struct announce_struct) ) ;
    struct sockaddr sa ;
    socklen_t sl = sizeof(sa) ;
    if ( as==NULL || Global.announce_off) return ;
    as->out = out ;
    if ( getsockname( out->fd, &sa, &sl) ) {
        ERROR_CONNECT("Could not get port number of device.\n") ;
        return ;
    }
    as->port = ((struct sockaddr_in *)(& sa))->sin_port ;
    switch ( Global.opt ) {
        case opt_httpd:
            as->name = "OWFS (1-wire) Web" ;
            as->type0 = "_http._tcp" ;
            as->type1 = "_owhttpd._tcp" ;
            break ;
        case opt_server:
            as->name = "OWFS (1-wire) Server" ;
            as->type0 = "_owserver._tcp" ;
            as->type1 = NULL ;
            break ;
        case opt_ftpd:
            as->name = "OWFS (1-wire) FTP" ;
            as->type0 = "_ftp._tcp" ;
            as->type1 = NULL ;
            break ;
        default:
            return ;
    }
#if OW_MT
    // LEVEL_DEBUG("DNSServiceRegister request: index=%d, name=%s, port=%d, type=%s / %s\n",out->index,as->name,ntohs(as->port),as->type0,as->type1) ;
    {
        pthread_t thread ;
        int err = pthread_create( &thread, 0, Announce, as ) ;
        if ( err ) {
            ERROR_CONNECT("Zeroconf/Bounjour registration thread error %d).\n",err) ;
        }
    }
#else /* OW_MT */
    Announce( as ) ;
#endif /* OW_MT */
}

#endif /* OW_ZERO */
