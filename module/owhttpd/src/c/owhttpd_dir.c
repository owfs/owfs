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

/* Find he next higher level by search for last slash that doesn't end the string */
/* return length of "higher level" */
/* Assumes good path:
    null delimitted
    starts with /
    non-null
*/
int Backup( const char * path ) {
    int i = strlen(path)-1 ;
    if (i==0) return 1 ;
    if (path[i]=='/') --i ;
    while( path[i]!='/') --i ;
    return i+1 ;
}

#define name_directory    "directory"
#define name_onewire_chip "1-wire chip"

/* Misnamed. Actually all directory */
void ShowDir( FILE * out, const struct parsedname * const pn ) {
    struct parsedname pncopy;
    int b;
    /* Embedded function */
    /* Callback function to FS_dir */
    void directory( const struct parsedname * const pn2 ) {
        /* uncached tag */
        /* device name */
        /* Have to allocate all buffers to make it work for Coldfire */
        char *buffer ;
        char * loc ;
        char * nam ;
        const char * typ = NULL ;
        if( !(buffer = malloc(OW_FULLNAME_MAX+1) ) ) { /* buffer for name */
//printf("ShowDir: error malloc %d bytes\n", OW_FULLNAME_MAX+1);
            return ;
        }
        loc = nam = buffer ;
        *buffer = '\000';
        if ( pn2->dev==NULL ) {
            if ( pn2->type != pn_real ) {
                FS_dirname_type(buffer,OW_FULLNAME_MAX,pn2) ;
            } else if ( pn2->state ) {
                FS_dirname_state(buffer,OW_FULLNAME_MAX,pn2) ;
            }
            typ = name_directory ;
        } else if ( pn2->dev == DeviceSimultaneous ) {
            loc = nam = pn2->dev->name ;
            typ = name_onewire_chip ;
        } else if ( pn2->type == pn_real ) {
            FS_devicename(loc,OW_FULLNAME_MAX,pn2->sn,pn2) ;
            nam = pn2->dev->name ;
            typ = name_onewire_chip ;
        } else {
            strcpy( loc, pn2->dev->code ) ;
            nam = loc ;
            typ = name_directory ;
        }
        //printf("path=%s loc=%s name=%s typ=%s pn->dev=%p pn->ft=%p pn->subdir=%p pathlength=%d\n",pn->path,loc,nam,typ,pn->dev,pn->ft,pn->subdir,pn->pathlength ) ;
        if (pn->state & pn_text) {
            fprintf( out, "%s %s \"%s\"\r\n", loc, nam, typ ) ;
        } else {
            fprintf( out, "<TR><TD><A HREF='%s/%s'><CODE><B><BIG>%s</BIG></B></CODE></A></TD><TD>%s</TD><TD>%s</TD></TR>", (strcmp(pncopy.path,"/")?pncopy.path:""), loc, loc, nam, typ ) ;
            /* pncopy is the parent dir */
        }
        free(buffer);
    }

    memcpy(&pncopy, pn, sizeof(struct parsedname));

    HTTPstart( out , "200 OK", (pn->state & pn_text) ) ;

    //printf("ShowDir=%s\n", pn->path) ;

    if(pn->state & pn_text) {
      FS_dir( directory, &pncopy ) ;
      return;
    }

    HTTPtitle( out , "Directory") ;

    if ( pn->type != pn_real ) {
        /* return whole path since tree structure could be much deeper now */
        /* first / is stripped off */
        HTTPheader( out , &pn->path[1]) ;
    } else if (pn->state) {
        /* return whole path since tree structure could be much deeper now */
        /* first / is stripped off */
        HTTPheader( out , &pn->path[1]) ;
    } else {
        HTTPheader( out , "directory" ) ;
    }


    fprintf( out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;

    {   // Find higher level by path manipulation
        b = Backup(pn->path) ;
        if (b!=1) {
            fprintf( out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>higher level</TD><TD>directory</TD></TR>",b, pn->path ) ;
        } else {
            fprintf( out, "<TR><TD><A HREF='/'><CODE><B><BIG>top</BIG></B></CODE></A></TD><TD>highest level</TD><TD>directory</TD></TR>" ) ;
        }
    }

    FS_dir( directory, &pncopy ) ;
    fprintf( out, "</TABLE>" ) ;
    HTTPfoot( out ) ;
}
