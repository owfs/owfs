/*
$Id$
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2003 Paul H Alfille

 * based on chttpd/1.0
 * main program. Somewhat adapted from dhttpd
 * Copyright (c) 0x7d0 <noop@nwonknu.org>
 * Some parts
 * Copyright (c) 0x7cd David A. Bartold
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "owfs_config.h"
#include "ow.h" // for libow
#include "owhttpd.h" // httpd-specific

/*
 * Default port to listen too. If you aren't root, you'll need it to
 * be > 1024 unless you plan on using a config file
 */
#define DEFAULTPORT	80

static struct listen_sock l_sock;
static void ow_exit( int e ) ;
static int accept_sock(struct listen_sock sock) ;
static void shutdown_sock(struct listen_sock sock) ;
static int get_active_sock(struct active_sock *a_sock, const int socketd) ;
static int get_listen_sock(struct listen_sock *sock) ;

unsigned int    debug;

void handle_sigchild(int unused) {
    (void) unused ;
    signal(SIGCHLD, handle_sigchild);
}

void handle_sigterm(int unused) {
    (void) unused ;
    shutdown_sock(l_sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    char c ;
    int s ;

//    pid_t           pid;
    LibSetup() ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
			"Usage: %s ttyDevice -p tcpPort [options] \n"
			"   or: %s [options] -d ttyDevice -p tcpPort \n"
			"    -p port   -- tcp port for web serving process (e.g. 3001)\n" ,
            argv[0],argv[0] ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc-1 ) {
        ComSetup(argv[optind]) ;
        ++optind ;
    }

    if ( devfd==-1 && devusb==0 ) {
        fprintf(stderr, "No device port specified (-d or -u)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if ( portnum==-1 ) {
        fprintf(stderr, "No TCP port specified (-p)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if (get_listen_sock(&l_sock) < 0) {
	fprintf(stderr, "socket problems: %s failed to start\n", argv[0]);
	ow_exit(1);
    }

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

    signal(SIGCHLD, handle_sigchild);
    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, handle_sigterm);

    for (;;) {
        s = accept_sock(l_sock);
//printf("LOOP\n");
            if (s != -1) {
                struct active_sock ch_sock;
                if ( !get_active_sock(&ch_sock, s) ) handle_socket(ch_sock);
                fflush(ch_sock.io);
                fclose(ch_sock.io);
                shutdown(s, 2);
                close(s);
            } else {
                /* failed to accept */
                syslog(LOG_WARNING,"Failed to accept a socket\n");
            }
    }
}

static void ow_exit( int e ) {
    LibClose() ;
    closelog() ;
    exit( e ) ;
}

/* -------------------------------------------------------------------
   ---------------- socket routines ----------------------------------
   ------------------------------------------------------------------- */
/*
 * get_listen_sock
 *   Start up the socket listening routines and return a filedescriptor
 *   for that socket
 */
static int get_listen_sock(struct listen_sock *sock) {
    int             status;
    int             one;

    status = 0;
    one = 1;
    memset(&(sock->sin), 0, sizeof(sock->sin));
    sock->sin.sin_family = AF_INET;
    sock->sin.sin_addr.s_addr = htonl(0);
    sock->sin.sin_port = htons(portnum);

    if ( ((sock->socket = socket(AF_INET, SOCK_STREAM, 0))==-1) ||
	setsockopt(sock->socket, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) ||
	bind(sock->socket, (struct sockaddr *) &(sock->sin), sizeof((sock->sin))) ||
        listen(sock->socket, 5)
       ) {
	shutdown(sock->socket, 2);
	sock->socket = -1;
    }
    return (sock->socket);
}

/*
 * accept_sock:
 * accepts a new socket and returns the file descriptor
 */
static int accept_sock(struct listen_sock sock) {
    int             n_sock;
    socklen_t       size = sizeof((sock.sin));
    n_sock = accept((sock.socket),(struct sockaddr*)&(sock.sin), &size);
    return n_sock;

}

static void shutdown_sock(struct listen_sock sock) {
    if (sock.socket != -1)
	shutdown(sock.socket, 2);
}

/*
 * get_active_sock:
 * sets up FILE* for http socket.
 * returns 1 for failure 0 for success
 * Note, changed return to match OWFS style
 */
static int get_active_sock(struct active_sock *a_sock, const int socketd) {
    a_sock->socket = socketd;
    a_sock->io = fdopen(a_sock->socket, "w+");
    if (!(a_sock->io)) {
	close(a_sock->socket);
	return (-1);
    }
    return 0;
}


