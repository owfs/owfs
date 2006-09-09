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
#include "ow.h"
#include "ow_connection.h"

/* Routines for handling a linked list of connections in and out */
/* typical connection in would be gthe serial port or USB */

/* Globals */
struct connection_out * outdevice = NULL ;
int outdevices = 0 ;
struct connection_in * indevice = NULL ;
int indevices = 0 ;

struct connection_in *find_connection_in(int iindex) {
  struct connection_in *c = indevice;
  while(c) {
    if(c->index == iindex) break;
    c = c->next;
  }
  return c;
}

enum bus_mode get_busmode(struct connection_in *in) {
    if(!in) return bus_unknown;
    return in->busmode;
}

int is_servermode(struct connection_in *in) {
    return (in->busmode==bus_server) || (in->busmode==bus_zero) ;
}

/* Make a new indevice, and place it in the chain */
/* Based on a shallow copy of "in" if not NULL */
struct connection_in * NewIn( const struct connection_in * in ) {
    size_t len = sizeof(struct connection_in) ;
    struct connection_in * now = (struct connection_in *)malloc( len ) ;
    if (now) {
        if ( in ) {
            memcpy( now, in, len ) ;
        } else {
            memset( now, 0, len ) ;
        }
        now->next = indevice ; /* put in linked list at start */
        indevice = now ;
        now->index = indevices++ ;
        Asystem.elements = indevices ;
#if OW_MT
        pthread_mutex_init(&(now->bus_mutex), pmattr);
        pthread_mutex_init(&(now->dev_mutex), pmattr);
        now->dev_db = NULL ;
#endif /* OW_MT */
        /* Support DS1994/DS2404 which require longer delays, and is automatically
        * turned on in *_next_both().
        * If it's turned off, it will result into a faster reset-sequence.
        */
        now->ds2404_compliance = 0 ;
        /* Flag first pass as need to clear all branches if DS2409 present */
        now->branch.sn[0] = 1 ; // will never match a real hub
        /* Arbitrary guess at root directory size for allocating cache blob */
        now->last_root_devs = 10 ;
    } else {
        LEVEL_DEFAULT("Cannot allocate memory for adapter structure,\n") ;
    }
    return now ;
}

struct connection_out * NewOut( void ) {
    size_t len = sizeof(struct connection_out) ;
    struct connection_out * now = (struct connection_out *)malloc( len ) ;
    if (now) {
        memset( now, 0, len ) ;
        now->next = outdevice ;
        outdevice = now ;
        now->index = outdevices ++ ;
#if OW_MT
        pthread_mutex_init(&(now->accept_mutex), pmattr);
#endif /* OW_MT */
#if OW_ZERO
        // Zero sref's -- done with struct memset
        //now->sref0 = 0 ;
        //now->sref1 = 0 ;
#endif /* OW_MT */
    } else {
        LEVEL_DEFAULT("Cannot allocate memory for server structure,\n") ;
    }
    return now ;
}

void FreeIn( void ) {
    struct connection_in * next = indevice ;
    struct connection_in * now  ;

    indevice = NULL ;
    indevices = 0 ;
    while ( next ) {
        now = next ;
        next = now->next ;
#if OW_MT
        pthread_mutex_destroy(&(now->bus_mutex)) ;
        pthread_mutex_destroy(&(now->dev_mutex));
#endif /* OW_MT */
        switch (get_busmode(now)) {
        case bus_zero:
        case bus_server:
            if ( now->connin.server.host ) {
                free(now->connin.server.host) ;
                now->connin.server.host = NULL ;
            }
            if ( now->connin.server.service ) {
                free(now->connin.server.service) ;
                now->connin.server.service = NULL ;
            }
            if ( now->connin.server.ai ) {
                freeaddrinfo(now->connin.server.ai) ;
                now->connin.server.ai = NULL ;
            }
            break ;
        case bus_link:
        case bus_serial:
            COM_close(now) ;
            break ;
        case bus_usb:
#if OW_USB
            DS9490_close(now) ;
            now->name = NULL ; // rather than a static data string;
#endif
            break ;
        case bus_elink:
        case bus_ha7:
            BUS_close(now) ;
            break ;
        case bus_fake:
            if ( now->connin.fake.device ) free(now->connin.fake.device) ;
            now->connin.fake.device = NULL ;
            break ;
        case bus_i2c:
#if OW_MT
            if ( now->connin.i2c.index==0 ) {
                pthread_mutex_destroy(&(now->connin.i2c.i2c_mutex)) ;
            }
#endif /* OW_MT */
        default:
            break ;
        }
        if ( now->name) {
            free(now->name ) ;
            now->name = NULL ;
        }
        free(now) ;
    }
}

void FreeOut( void ) {
    struct connection_out * next = outdevice ;
    struct connection_out * now  ;

    outdevice = NULL ;
    outdevices = 0 ;
    while ( next ) {
        now = next ;
        next = now->next ;
        if ( now->name) {
            free(now->name ) ;
            now->name = NULL ;
        }
        if ( now->host ) {
            free(now->host) ;
            now->host = NULL ;
        } else if ( now->service ) {
            free(now->service) ;
            now->service = NULL ;
        }
        if ( now->ai ) {
            freeaddrinfo(now->ai) ;
            now->ai = NULL ;
        }
#if OW_MT
        pthread_mutex_destroy(&(now->accept_mutex)) ;
#endif /* OW_MT */
#if OW_ZERO
        if ( now->sref0 ) DNSServiceRefDeallocate(now->sref0) ;
        if ( now->sref1 ) DNSServiceRefDeallocate(now->sref1) ;
        if ( now->type ) free(now->type) ;
        if ( now->domain ) free(now->domain) ;
#endif /* OW_ZERO */
        free(now) ;
    }
}
