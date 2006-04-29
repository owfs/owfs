/*
$Id$
 * http.c for owhttpd (1-wire web server)
 * By Paul Alfille 2003, using libow
 * offshoot of the owfs ( 1wire file system )
 *
 * GPL license ( Gnu Public Lincense )
 *
 * Based on chttpd. copyright(c) 0x7d0 greg olszewski <noop@nwonknu.org>
 *
 */

#include "owhttpd.h"
#include <limits.h>

// #include <libgen.h>  /* for dirname() */

/* --------------- Functions ---------------- */


void ChangeData( struct urlparse * up, const struct parsedname * pn ) {
    char linecopy[PATH_MAX+1];
    strcpy( linecopy , up->file ) ;
    if ( pn->ft ) { /* pare off any filetype */
        char * r = strrchr(linecopy,'/') ;
        if (r) r[1] = '\0' ;
	//printf("Change data ft yes \n") ;
    } else {
        strcat( linecopy , "/" ) ;
    }
    /* Do command processing and make changes to 1-wire devices */
    if ( pn->dev!=&NoDevice && up->request && up->value && !readonly ) {
        struct parsedname pn2 ;

        memcpy( &pn2, pn, sizeof(struct parsedname)) ; /* shallow copy */

        strcat( linecopy , up->request ) ; /* add on requested file type */
        //printf("Change data on %s to %s\n",linecopy,up->value) ;
        if ( FS_ParsedName(linecopy,&pn2)==0 && pn2.ft && pn2.ft->write.v ) {
            switch ( pn2.ft->format ) {
            case ft_binary:
                httpunescape(up->value) ;
                hex_only(up->value) ;
                if ( (int)strlen(up->value) == (pn2.ft->suglen<<1) ) {
                    hex_convert(up->value) ;
                    FS_write( linecopy, up->value, (size_t) pn2.ft->suglen, 0 ) ;
                }
                break;
            case ft_yesno:
                if ( pn2.ft->ag==NULL || pn2.extension>=0 ) { // Single check box handled differently
                    FS_write( linecopy, strncasecmp(up->value,"on",2)?"0":"1", 2, 0 ) ;
                    break;
                }
                // fall through
            default:
                if(!httpunescape(up->value)) FS_write(linecopy,up->value,strlen(up->value)+1,0) ;
                break;
            }
        }
    FS_ParsedName_destroy(&pn2);
    }
}

