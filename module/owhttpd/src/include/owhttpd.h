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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

#include <pwd.h>				// getpwuid
#include <grp.h>				// initgroups
#include <limits.h>

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
enum content_type { ct_text, ct_html, ct_icon, ct_json, };
void HTTPstart(FILE * out, const char *status, const enum content_type ct);
void HTTPtitle(FILE * out, const char *title);
void HTTPheader(FILE * out, const char *head);
void HTTPfoot(FILE * out);

/* in owhttpd_write.c */
void PostData(struct one_wire_query *owq);
void ChangeData(struct one_wire_query *owq);

/* in owhttpd_read.c */
void ShowDevice(FILE * out, struct parsedname *const pn);

/* in owhttpd_dir.c */
void ShowDir(FILE * out, struct parsedname * pn);
int Backup(const char *path);

/* in ow_favicon.c */
void Favicon(FILE * out);

/* in owhttpd_escape */
void httpunescape(BYTE * httpstr) ;
char * httpescape( const char * original_string ) ;

#endif							/* OWHTTPD_H */
