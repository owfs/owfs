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

#include "owfs_config.h"
#include "ow.h"

/* Routines for handling a linked list of connections in and out */
/* typical connection in would be gthe serial port or USB */

/* Globals */
struct connection_out * outdevice = NULL ;
int outdevices = 0 ;

struct connection_in * indevice = NULL ;
int indevices = 0 ;

struct connection_in *find_connection_in(int index) {
  struct connection_in *c = indevice;
  while(c) {
    if(c->index == index) break;
    c = c->next;
  }
  return c;
}

struct connection_in * NewIn( void ) {
    int len = sizeof(struct connection_in) ;
    struct connection_in * last = NULL ;
    struct connection_in * now = indevice ;
    while ( now ) {
        last = now ;
        now = now->next ;
    }
    now = malloc( len ) ;
    if (now) {
        bzero(now,len) ;
        if ( indevice ) {
            last->next = now ;
            now->index = last->index+1 ;
        } else {
            indevice = now ;
            now->index = 1 ;
        }
        ++ indevices ;
	now->speed = B9600;
#ifdef OW_MT
	pthread_mutex_init(&(now->bus_mutex), pmattr);
#endif /* OW_MT */
    }
    return now ;
}

struct connection_out * NewOut( void ) {
    int len = sizeof(struct connection_out) ;
    struct connection_out * last = NULL ;
    struct connection_out * now = outdevice ;
    while ( now ) {
        last = now ;
        now = now->next ;
    }
    now = malloc( len ) ;
    if (now) {
        bzero(now,len) ;
        if ( outdevice ) {
            last->next = now ;
        } else {
            outdevice = now ;
        }
        ++ outdevices ;
#ifdef OW_MT
	pthread_mutex_init(&(now->accept_mutex), pmattr);
#endif /* OW_MT */
    }
    return now ;
}

void FreeIn( void ) {
    struct connection_in * next = indevice ;
    struct connection_in * now  ;

    indevice = NULL ;
    indevices = 0 ;
#ifdef OW_MT
    pthread_mutex_destroy(&(now->bus_mutex)) ;
#endif /* OW_MT */
    while ( next ) {
        now = next ;
        next = now->next ;
        switch (now->busmode) {
        case bus_remote:
            if ( now->host ) {
                free(now->host) ;
                now->host = NULL ;
            }
            if ( now->service ) {
                free(now->service) ;
                now->service = NULL ;
            }
            if ( now->ai ) {
                freeaddrinfo(now->ai) ;
                now->ai = NULL ;
            }
            break ;
        case bus_serial:
            COM_close(now) ;
            break ;
#ifdef OW_USB
        case bus_usb:
            DS9490_close(now) ;
            break ;
#endif /* OW_USB */
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
        }
        if ( now->service ) {
            free(now->service) ;
            now->service = NULL ;
        }
        if ( now->ai ) {
            freeaddrinfo(now->ai) ;
            now->ai = NULL ;
        }
#ifdef OW_MT
        pthread_mutex_destroy(&(now->accept_mutex)) ;
#endif /* OW_MT */
        free(now) ;
    }
}
