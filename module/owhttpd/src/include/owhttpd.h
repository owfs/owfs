/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By PAul H Alfille
    {c} 2003 GPL
    palfille@earthlink.net
*/

/* OWHTTPD - specific header */

#ifndef OWHTTPD_H
#define OWHTTPD_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h> // signal
#include <stdlib.h> // exit
#include <unistd.h> // chdir
#include <stdio.h> // perror
#include <fcntl.h> // open
#include <pwd.h> // getpwuid
#include <grp.h> // initgroups
#include <syslog.h>

struct listen_sock {
    int             socket;
    struct sockaddr_in sin;
};

struct active_sock {
    int             socket;
    FILE           *io;
    struct in_addr  peer_addr;
};


/*
 * Main routine for actually handling a request
 * deals with a conncection
 */
int handle_socket(struct active_sock a_sock);


#endif
