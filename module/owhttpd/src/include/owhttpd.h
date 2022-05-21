/*
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By Paul H Alfille
    {c} 2003 GPL
    paul.alfille@gmail.com
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

struct OutputControl {
	FILE * out ;
	int not_first ;
	char * base_url ;
	char * host ;
} ;

/* in owhttpd_present */
enum content_type { ct_text, ct_html, ct_icon, ct_json, };
void HTTPstart( struct OutputControl * oc, const char *status, const enum content_type ct);
void HTTPtitle( struct OutputControl * oc, const char *title);
void HTTPheader( struct OutputControl * oc, const char *head);
void HTTPfoot( struct OutputControl * oc);

/* in owhttpd_write.c */
void PostData(struct one_wire_query *owq);
void ChangeData(struct one_wire_query *owq);

/* in owhttpd_read.c */
void ShowDevice( struct OutputControl * oc, struct parsedname *const pn);

/* in owhttpd_dir.c */
struct JsonCBstruct {
	FILE * out ;
	int not_first ;
} ;

void ShowDir( struct OutputControl * oc, struct parsedname * pn);
int Backup(const char *path);
void JSON_dir_init(  struct OutputControl * oc ) ;
void JSON_dir_entry(  struct OutputControl * oc, const char * format, const char * data ) ;
void JSON_dir_finish(  struct OutputControl * oc ) ;

/* in ow_favicon.c */
void Favicon( struct OutputControl * oc);

/* in owhttpd_escape */
void httpunescape(BYTE * httpstr) ;
char * httpescape( const char * original_string ) ;

#endif							/* OWHTTPD_H */
