/*
 * $Id$
 */

#ifndef OFTPD_H
#define OFTPD_H

/* address to listen on (use NULL to listen on all addresses) */
#define FTP_ADDRESS NULL

/* default port FTP server listens on (use 0 to listen on default port) */
#define FTP_PORT 0

/* ranges possible for command-line specified port numbers */
#define MIN_PORT 0
#define MAX_PORT 65535

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

