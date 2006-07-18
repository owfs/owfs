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

/* ow_net holds the network utility routines. Many stolen unashamedly from Steven's Book */
/* Much modification by Christian Magnusson especially for Valgrind and embedded */
/* non-threaded fixes by Jerry Scharf */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* Broadcast to discover HA7 servers */
/* Wait 10 seconds for responses */
    struct addrinfo * ai ;
    struct addrinfo hint ;

    memset(&hint,0,sizeof(struct addrinfo) ) ;
    hint.ai_flags = ;
    hint.ai_family = AF_INET ;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = ;
    if ( getaddrinfo( "224.1.2.3", "4567", &hint, &ai ) ) {