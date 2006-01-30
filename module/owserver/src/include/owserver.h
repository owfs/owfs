/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By Paul H Alfille
    {c} 2003 GPL
    palfille@earthlink.net
*/

/* OWSERVER - specific header */

#ifndef OWSERVER_H
#define OWSERVER_H

#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

#include <netinet/in.h> // ntohl ...
#include <netdb.h> // addrinfo 
#include <pthread.h>

struct active_sock {
    int             socket;
    FILE           *io;
    struct in_addr  peer_addr;
};

struct timeval tv = { 10, 0, } ;

#define MAXBUFFERSIZE  65000

#endif /* OWSERVER_H */
