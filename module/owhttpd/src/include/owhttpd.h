/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By Paul H Alfille
    {c} 2003 GPL
    palfille@earthlink.net
*/

/* OWHTTPD - specific header */

#ifndef OWHTTPD_H
#define OWHTTPD_H

#include "owfs_config.h"
#include "ow.h" // for libow

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

#define SVERSION "owhttpd"

#define BODYCOLOR  "BGCOLOR='#BBBBBB'"
#define TOPTABLE "WIDTH='100%%' BGCOLOR='#DDDDDD' BORDER='1'"
#define DEVTABLE "BGCOLOR='#DDDDDD' BORDER='1'"
#define VALTABLE "BGCOLOR='#DDDDDD' BORDER='1'"

/*
 * Main routine for actually handling a request
 * deals with a conncection
 */
/* in owhttpd_handler.c */
int handle_socket(FILE * out);

/* in owhttpd_present */
void HTTPstart(FILE * out, const char * status, const unsigned int text ) ;
void HTTPtitle(FILE * out, const char * title ) ;
void HTTPheader(FILE * out, const char * head ) ;
void HTTPfoot(FILE * out ) ;

/* in owhttpd_write.c */
void ChangeData( char * value , const struct parsedname * pn ) ;

/* in owhttpd_read.c */
void ShowDevice( FILE * out, const struct parsedname * const pn ) ;

/* in owhttpd_dir.c */
void ShowDir( FILE * out, const struct parsedname * const pn ) ;
int Backup( const char * path ) ;

#endif /* OWHTTPD_H */
