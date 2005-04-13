/*
 * $Id$
 */

#ifndef OFTPD_H
#define OFTPD_H

/* default port FTP server listens on (use 0 to listen on default port) */
#define DEFAULT_PORTNAME "0.0.0.0:21"

/* default maximum number of clients */
#define MAX_CLIENTS 250

/* bounds on command-line specified number of clients */
#define MIN_NUM_CLIENTS 1
#define MAX_NUM_CLIENTS 300

/* timeout (in seconds) before dropping inactive clients */
#define INACTIVITY_TIMEOUT (15 * 60)

/* README file name (sent automatically as a response to users) */
#define README_FILE_NAME "README"

#endif /* OFTPD_H */

