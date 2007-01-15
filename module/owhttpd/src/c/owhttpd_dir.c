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

// #include <libgen.h>  /* for dirname() */

/* --------------- Functions ---------------- */
static void ShowDirText( FILE * out, const struct parsedname * const pn ) ;

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

struct showdirstruct {
    char * prepath ;
    FILE * out ;
} ;
    /* Callback function to FS_dir */
static void ShowDirCallback( void * v, const struct parsedname * const pn2 ) {
    /* uncached tag */
    /* device name */
    /* Have to allocate all buffers to make it work for Coldfire */
    struct showdirstruct * sds = v ;
    char buffer[OW_FULLNAME_MAX+1] ;
    char * loc = buffer ;
    char * nam = buffer ;
    const char * typ = NULL ;
    buffer[0] = '\0';
    if ( pn2->dev==NULL ) {
        if ( NotRealDir(pn2) ) {
            FS_dirname_type(buffer,OW_FULLNAME_MAX,pn2) ;
        } else if ( pn2->state ) {
            FS_dirname_state(buffer,OW_FULLNAME_MAX,pn2) ;
        }
        typ = name_directory ;
    } else if ( pn2->dev == DeviceSimultaneous ) {
        loc = nam = pn2->dev->name ;
        typ = name_onewire_chip ;
    } else if ( IsRealDir(pn2) ) {
        FS_devicename(loc,OW_FULLNAME_MAX,pn2->sn,pn2) ;
        nam = pn2->dev->name ;
        typ = name_onewire_chip ;
    } else {
        strcpy( loc, pn2->dev->code ) ;
        nam = loc ;
        typ = name_directory ;
    }
        //printf("path=%s loc=%s name=%s typ=%s pn->dev=%p pn->ft=%p pn->subdir=%p pathlength=%d\n",pn->path,loc,nam,typ,pn->dev,pn->ft,pn->subdir,pn->pathlength ) ;
    fprintf( sds->out, "<TR><TD><A HREF='%s/%s'><CODE><B><BIG>%s</BIG></B></CODE></A></TD><TD>%s</TD><TD>%s</TD></TR>", sds->prepath, loc, loc, nam, typ ) ;
}
    /* Misnamed. Actually all directory */
void ShowDir( FILE * out, const struct parsedname * const pn ) {
    int b = Backup(pn->path) ;
    struct showdirstruct sds = { strcmp(pn->path,"/")?pn->path:"", out, } ;
    if ( pn->state & pn_text ) {
        ShowDirText( out, pn ) ;
        return ;
    }

    HTTPstart( out , "200 OK", ct_html ) ;

    //printf("ShowDir=%s\n", pn->path) ;

//    if(pn->state & pn_text) {

    HTTPtitle( out , "Directory") ;

    if ( NotRealDir(pn) ) {
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

    if (b!=1) {
        fprintf( out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>higher level</TD><TD>directory</TD></TR>",b, pn->path ) ;
    } else {
        fprintf( out, "<TR><TD><A HREF='/'><CODE><B><BIG>top</BIG></B></CODE></A></TD><TD>highest level</TD><TD>directory</TD></TR>" ) ;
    }

    FS_dir2( ShowDirCallback, &sds, pn ) ;
    fprintf( out, "</TABLE>" ) ;
    HTTPfoot( out ) ;
}
static void ShowDirTextCallback( void * v, const struct parsedname * const pn2 ) {
    /* uncached tag */
    /* device name */
    /* Have to allocate all buffers to make it work for Coldfire */
    struct showdirstruct * sds = v ;
    char buffer[OW_FULLNAME_MAX+1] ;
    char * loc = buffer ;
    char * nam = buffer ;
    const char * typ = NULL ;
    buffer[0] = '\0';
    if ( pn2->dev==NULL ) {
        if ( NotRealDir(pn2) ) {
            FS_dirname_type(buffer,OW_FULLNAME_MAX,pn2) ;
        } else if ( pn2->state ) {
            FS_dirname_state(buffer,OW_FULLNAME_MAX,pn2) ;
        }
        typ = name_directory ;
    } else if ( pn2->dev == DeviceSimultaneous ) {
        loc = nam = pn2->dev->name ;
        typ = name_onewire_chip ;
    } else if ( IsRealDir(pn2) ) {
        FS_devicename(loc,OW_FULLNAME_MAX,pn2->sn,pn2) ;
        nam = pn2->dev->name ;
        typ = name_onewire_chip ;
    } else {
        strcpy( loc, pn2->dev->code ) ;
        nam = loc ;
        typ = name_directory ;
    }
        //printf("path=%s loc=%s name=%s typ=%s pn->dev=%p pn->ft=%p pn->subdir=%p pathlength=%d\n",pn->path,loc,nam,typ,pn->dev,pn->ft,pn->subdir,pn->pathlength ) ;
    //printf("path=%s loc=%s name=%s typ=%s pn->dev=%p pn->ft=%p pn->subdir=%p pathlength=%d\n",pn->path,loc,nam,typ,pn->dev,pn->ft,pn->subdir,pn->pathlength ) ;
    fprintf( sds->out, "%s %s \"%s\"\r\n", loc, nam, typ ) ;
    free(buffer);
}
static void ShowDirText( FILE * out, const struct parsedname * const pn ) {
    struct showdirstruct sds = { NULL, out, } ;

    HTTPstart( out , "200 OK", ct_text ) ;

    FS_dir2( ShowDirTextCallback, &sds, pn ) ;
    return;
}
