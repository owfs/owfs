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

#if OW_ZERO

#include "ow_connection.h"

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
    LEVEL_DETAIL("RegisterBack ref=%d flags=%d error=%d name=%s type=%s domain=%s\n",s,f,e,SAFESTRING(name),SAFESTRING(type),SAFESTRING(domain)) ;
    if ( e==kDNSServiceErr_NoError ) sref[0] = s ;
    LEVEL_DEBUG("RegisterBack: done\n");
}

/* Register the out port with Bonjour -- might block so done in a separate thread */
static void * Announce( void * v ) {
    struct announce_struct *as = (struct announce_struct *)v ;
    DNSServiceRef sref ;
    DNSServiceErrorType err ;
    
    if(libdnssd == NULL) return NULL;
#if OW_MT
    pthread_detach( pthread_self() ) ;
#endif /* OW_MT */

    LEVEL_DEBUG("Announce: 1\n");

    err = DNSServiceRegister(&sref,0,0,Global.announce_name?Global.announce_name:as->name,as->type0,NULL,NULL,as->port,0,NULL,RegisterBack,&(as->out->sref0)) ;
    if(err)
      LEVEL_DEBUG("Announce: err=%d\n", err);

    //LEVEL_DEBUG("DNSServiceRequest attempt: index=%d, name=%s, port=%d, type=%s / %s\n",as->out->index,SAFESTRING(as->name),ntohs(as->port),SAFESTRING(as->type0),SAFESTRING(as->type1)) ;
    LEVEL_DEBUG("Announce: 2\n");
    if ( err == kDNSServiceErr_NoError ) {
        DNSServiceProcessResult(sref) ;
    } else {
        ERROR_CONNECT("Unsuccessful call to DNSServiceRegister err = %d\n",err) ;
    }
    LEVEL_DEBUG("Announce: 3\n");
    if (as->type1 ) {
        err = DNSServiceRegister(&sref,0,0,Global.announce_name?Global.announce_name:as->name,as->type1,NULL,NULL,as->port,0,NULL,RegisterBack,&(as->out->sref1)) ;
        if ( err == kDNSServiceErr_NoError ) {
            DNSServiceProcessResult(sref) ;
        } else {
            ERROR_CONNECT("Unsuccessful call to DNSServiceRegister err = %d\n",err) ;
        }
    }
    free(as) ;
    LEVEL_DEBUG("Announce: end\n");
#if OW_MT
    pthread_exit(NULL);
#endif
    return NULL ;
}

void OW_Announce( struct connection_out * out ) {
    struct announce_struct * as = malloc( sizeof(struct announce_struct) ) ;
    struct sockaddr sa ;
#if OW_MT
    pthread_t thread ;
    int err ;
#endif
    socklen_t sl = sizeof(sa) ;

    if(libdnssd == NULL) return;

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
    LEVEL_DEBUG("OW_Announce: 1\n");
    if(libdnssd) {
#if OW_MT
      //LEVEL_DEBUG("DNSServiceRegister request: index=%d, name=%s, port=%d, type=%s / %s\n",out->index,SAFESTRING(as->name),ntohs(as->port),SAFESTRING(as->type0),SAFESTRING(as->type1)) ;
      err = pthread_create( &thread, NULL, Announce, (void *)as ) ;
      if ( err ) {
	ERROR_CONNECT("Zeroconf/Bounjour registration thread error %d).\n",err) ;
      }
#else /* OW_MT */
      Announce( as ) ;
#endif /* OW_MT */
    }
    LEVEL_DEBUG("OW_Announce: end\n");
}

#else  /* OW_ZERO */

void OW_Announce( struct connection_out * out ) {
  (void) out;
  return;
}

#endif /* OW_ZERO */
