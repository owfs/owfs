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

#define SVERSION "owhttpd"

#define BODYCOLOR  "BGCOLOR='#BBBBBB'"
#define TOPTABLE "WIDTH='100%%' BGCOLOR='#DDDDDD' BORDER='1'"
#define DEVTABLE "BGCOLOR='#DDDDDD' BORDER='1'"
#define VALTABLE "BGCOLOR='#DDDDDD' BORDER='1'"

#include "owfs_config.h"
#include "ow.h"
#include "owhttpd.h"


/* ------------ Protoypes ---------------- */

struct urlparse {
    char line[PATH_MAX+1];
    char * cmd ;
    char * file ;
    char * version ;
    char * request ;
    char * value ;
} ;

    /* Utility HTML page display functions */
static void HTTPstart(FILE * out, const char * status ) ;
static void HTTPtitle(FILE * out, const char * title ) ;
static void HTTPheader(FILE * out, const char * head ) ;
static void HTTPfoot(FILE * out ) ;

    /* string format functions */
static void hex_convert( char * str ) ;
static void hex_only( char * str ) ;
static int httpunescape(unsigned char *httpstr) ;

    /* Error page functions */
static void Bad400( struct active_sock * a_sock ) ;
static void Bad404( struct active_sock * a_sock ) ;

    /* Directory display functions */
static void RootDir( struct active_sock * a_sock, struct parsedname * pn ) ;

/* URL parsing function */
static void URLparse( struct urlparse * up ) ;

    /* Data edit function */
static void ChangeData( struct urlparse * up , struct parsedname * pn ) ;

    /* Device display functions */
static void ShowDevice( struct active_sock * a_sock, const char * file, struct parsedname * pn ) ;
static void Show( FILE * out, const char * const path, const char * const dev, const struct parsedname * pn ) ;


/* --------------- Functions ---------------- */

/* Main handler for a web page */
int handle_socket(struct active_sock * const a_sock) {
    char linecopy[PATH_MAX+1];
    char * str;
    struct urlparse up ;
    struct parsedname pn ;
    struct stateinfo si ;
    pn.si = &si ;

    { /* magic socket stuff */
        struct sockaddr_in peer;
        socklen_t          socklen = sizeof(peer);
        if ( getpeername(a_sock->socket, (struct sockaddr *) &peer, &socklen) < 0) {
            syslog(LOG_WARNING,"couldn't get peer name, dropping connection\n");
            return 1;
        }
        a_sock->peer_addr = peer.sin_addr;
    }
    str = fgets(up.line, PATH_MAX, a_sock->io);
//printf("PreParse line=%s\n",up.line);
    URLparse( &up ) ; /* Braek up URL */
//printf("WLcmd: %s\nfile: %s\nrequest: %s\nvalue: %s\nversion: %s \n",up.cmd,up.file,up.request,up.value,up.version) ;

    /* read lines until blank */
    if (up.version) {
        do {
            str = fgets(linecopy, PATH_MAX, a_sock->io);
        } while (str != NULL && strcmp(linecopy, "\r\n") && strcmp(linecopy, "\n"));
    }

//printf("WLcmd: %s\nfile: %s\nrequest: %s\nvalue: %s\nversion: %s \n",up.cmd,up.file,up.request,up.value,up.version) ;
    /*
     * This is necessary for some stupid *
     * * operating system such as SunOS
     */
    fflush(a_sock->io);

    /* Good command line? */
    if ( up.cmd==NULL || strcmp(up.cmd, "GET")!=0 ) {
        Bad400( a_sock ) ;

    /* Can't understand the file name = URL */
    } else if ( up.file == NULL ) {
        Bad404( a_sock ) ;
    } else {
//printf("PreParse\n");
        if ( FS_ParsedName( up.file , &pn ) ) {
        /* Can't understand the file name = URL */
            Bad404( a_sock ) ;
        /* Root directory -- show the bus */
        } else if ( pn.dev == NULL ) { /* directory! */
            RootDir( a_sock, & pn ) ;
        /* Single device, show it's properties */
        } else { /* a single device */
//printf("PreChange\n");
            ChangeData( &up, &pn ) ;
//printf("PreShow\n");
            ShowDevice( a_sock, up.file, &pn ) ;
        }
//printf("PreDestroy\n");
        FS_ParsedName_destroy( &pn ) ;
    }
//printf("Done\n");
    return 0 ;
}

/* URL handler for a web page */
static void URLparse( struct urlparse * up ) {
    char            * str;
    int             first = 1;

    up->cmd = up->version = up->file = up->request = up->value = NULL ;

    /* Separate out the three parameters, all in same line */
    for ( str=up->line ; *str ; str++ ) {
        switch (*str) {
        case ' ':
        case '\n':
        case '\r':
            *str = '\0' ;
            first = 1 ;
            break ;
        default:
            if ( !first || up->version ) {
                break ;
            } else if (up->file ) {
                up->version = str ;
            } else if ( up->cmd ) {
                up->file = str ;
            } else {
                up->cmd = str ;
            }
            first = 0 ;
        }
    }

    /* Separate out the filename and FORM data */
    if ( up->file ) {
        for ( str=up->file ; *str ; str++ ) {
            if ( *str == '?' ) {
                *str='\0' ;
                up->request = str+1 ;
                break ;
            }
        }
    }

    /* Separate out the FORM field and value */
    if ( up->request ) {
        for ( str=up->request ; *str ; str++ ) {
            if ( *str == '=' ) {
                *str='\0' ;
                up->value = str+1 ;
                break ;
            }
        }
    }
}

static void HTTPstart(FILE * out, const char * status ) {
    char d[44] ;
    time_t t = time(NULL) ;
    size_t l = strftime(d,sizeof(d),"%a, %d %b %Y %T GMT",gmtime(&t)) ;

    fprintf(out, "HTTP/1.0 %s\r\n", status);
    fprintf(out, "Date: %*s\n", l, d );
    fprintf(out, "Server: %s\r\n", SVERSION);
    fprintf(out, "Last-Modified: %*s\r\n", l, d );
    /*
     * fprintf( out, "MIME-version: 1.0\r\n" );
     */
    fprintf(out, "Content-Type: text/html\r\n");
    fprintf(out, "\r\n");
}

static void HTTPtitle(FILE * out, const char * title ) {
    fprintf(out, "<HTML><HEAD><TITLE>1-Wire Web: %s</TITLE></HEAD>\n", title);
}

static void HTTPheader(FILE * out, const char * head ) {
    fprintf(out, "<BODY "BODYCOLOR"><TABLE "TOPTABLE"><TR><TD>OWFS on %s</TD><TD><A HREF='/'>Bus listing</A></TD><TD><A HREF='http://owfs.sourceforge.net'>OWFS homepage</A></TD><TD><A HREF='http://www.maxim-ic.com'>Dallas/Maxim</A></TD><TD>by <A HREF='mailto://palfille@earthlink.net'>Paul H Alfille</A></TD></TR></TABLE>\n", devport);
    fprintf(out, "<H1>%s</H1><HR>\n", head);
}

static void HTTPfoot(FILE * out ) {
    fprintf(out, "</BODY></HTML>") ;
}

/* Callback function to FS_dir */
void directory( void * data, const struct parsedname * const pn ) {
    char extpath[PATH_MAX+1] = "/" ; /* probably excessive */
    char extname[33] ; /* probably excessive */
    char *devtype[] = { "1-Wire", "Interface", "Status", "Statistics", } ;
    int i ;

    /* uncached tag */
    if ( pn->type == pn_uncached ) strcat(extpath,"uncached/") ;

    /* Branch path */
    for ( i=0 ; i < pn->pathlength ; ++i ) {
            FS_devicename( &extpath[strlen(extpath)],32,pn->bp[i].sn) ;
            strcat( extpath,pn->bp[i].branch ? "/aux/":"/main/" ) ;
    }

    /* device name */
    if ( pn->dev->type == dev_1wire ) {
        FS_devicename(extname,32,pn->sn) ;
    } else {
        snprintf( extname , 32, "%s",pn->dev->code) ;
    }

    if ( pn->ft ) {
        Show( (FILE *) data, extpath, extname, pn ) ;
    } else { /* Root or branch directory */
        fprintf( (FILE *) data,
            "<TR><TD><A HREF='%s%s'><CODE><B><BIG>%s</BIG></B></CODE></A></TD><TD>%s</TD><TD>%s</TD></TR>",
            extpath, extname, extname, pn->dev->name, devtype[pn->dev->type] ) ;
    }
}

/* Device entry -- table line for a filetype */
static void Show( FILE * out, const char * const path, const char * const dev, const struct parsedname * pn ) {
    int len ;
    char file[33] ;
    char * subdir ;
    char fullpath[PATH_MAX+1] ;
    int suglen = FileLength(pn) + 1 ;
    char buf[suglen] ;
    int canwrite = !readonly && pn->ft->write.v ;

    /* Construct filename with extension */
    if ( pn->ft->ag == NULL ) {
        snprintf( file , PATH_MAX, "%s",pn->ft->name) ;
    } else if ( pn->extension == -1 ) {
        snprintf( file , PATH_MAX, "%s.ALL",pn->ft->name) ;
    } else if ( pn->ft->ag->letters == ag_letters ) {
        snprintf( file , PATH_MAX, "%s.%c",pn->ft->name,pn->extension+'A') ;
    } else {
        snprintf( file , PATH_MAX, "%s.%-d",pn->ft->name,pn->extension) ;
    }

    /* Parse out subdir */
    subdir = strrchr(file,'/') ;
    if ( subdir ) {
        ++subdir ; /* after slash */
    } else {
        subdir = file ;
    }
    fprintf( out, "<TR><TD><B>%s</B></TD><TD>", subdir ) ;

    /* full file name (with path and subdir) */
    strcpy(fullpath,path) ;
    strcat(fullpath,dev) ;
    strcat(fullpath,"/") ;
    strcat(fullpath,file) ;
    /* buffer for field value */
    if ( pn->ft->ag && pn->ft->format!=ft_binary && pn->extension==-1 ) {
        if ( pn->ft->read.v ) { /* At least readable */
            if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                buf[len] = '\0' ;
                if ( canwrite ) { /* read-write */
                    fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",file,buf ) ;
                } else { /* read only */
                    fprintf( out, "%s", buf ) ;
                }
            }
        } else if ( canwrite ) { /* rare write-only */
            fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",file ) ;
        }
    } else {
        switch( pn->ft->format ) {
        case ft_directory:
        case ft_subdir:
            fprintf( out, "<A HREF='%s'>%s</A>",fullpath,file);
            break ;
        case ft_yesno:
            if ( pn->ft->read.v ) { /* at least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                    buf[len]='\0' ;
                    if ( canwrite ) { /* read-write */
                        fprintf( out, "<FORM METHOD=\"GET\"><INPUT TYPE='CHECKBOX' NAME='%s' %s><INPUT TYPE='SUBMIT' VALUE='CHANGE' NAME='%s'></FORM></FORM>", file, (buf[0]=='0')?"":"CHECKED", file ) ;
                    } else { /* read-only */
                        switch( buf[0] ) {
                        case '0':
                            fprintf( out, "NO" ) ;
                            break;
                        case '1':
                            fprintf( out, "YES" ) ;
                            break;
                        }
                    }
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='ON'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='OFF'></FORM>",file,file ) ;
            }
            break ;
        case ft_binary:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                    if ( canwrite ) { /* read-write */
                        int i = 0 ;
                        fprintf( out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'>",file,len>>5 ) ;
                        while (i<len) {
                            fprintf( out, "%.2hhX", buf[i] ) ;
                            if ( ((++i)<len) && (i&0x1F)==0 ) fprintf( out, "\r\n" ) ;
                        }
                        fprintf( out, "</TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>" ) ;
                    } else { /* read only */
                        int i = 0 ;
                        fprintf( out, "<PRE>" ) ;
                        while (i<len) {
                            fprintf( out, "%.2hhX", buf[i] ) ;
                            if ( ((++i)<len) && (i&0x1F)==0 ) fprintf( out, "\r\n" ) ;
                        }
                        fprintf( out, "</PRE>" ) ;
                    }
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",file,(pn->ft->suglen)>>5 ) ;
            }
            break ;
        default:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                    buf[len] = '\0' ;
                    if ( canwrite ) { /* read-write */
                        fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",file,buf ) ;
                    } else { /* read only */
                        fprintf( out, "%s", buf ) ;
                    }
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",file ) ;
            }
        }
    }
    fprintf( out, "</TD></TR>\r\n" ) ;
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
static void hex_convert( char * str ) {
    char * uc = str ;
    unsigned char * hx = str ;
    for( ; *uc ; uc+=2 ) *hx++ = string2num( uc ) ;
}

/* reads an as ascii hex string, strips out non-hex, converts in place */
static void hex_only( char * str ) {
    char * uc = str ;
    char * hx = str ;
    while( *uc ) {
        if ( isxdigit(*uc) ) *hx++ = *uc ;
        uc ++ ;
    }
    *hx = '\0' ;
}

/* Change web-escaped string back to straight ascii */
static int httpunescape(unsigned char *httpstr) {
    unsigned char *in = httpstr ;          /* input string pointer */
    unsigned char *out = httpstr ;          /* output string pointer */

    while(*in)
    {
        switch (*in) {
        case '%':
            ++ in ;
            if ( in[0]=='\0' || in[1]=='\0' ) return 1 ;
            *out++ = string2num((char *) in ) ;
            ++ in ;
            break ;
        case '+':
            *out++ = ' ';
            break ;
        default:
            *out++ = *in;
            break ;
        }
        in++;
    }
    *out = '\0';
    return 0;
}

static void ChangeData( struct urlparse * up, struct parsedname * pn ) {
    char *property ;
    char linecopy[PATH_MAX+1];
    strcpy( linecopy , up->file ) ;
    strcat( linecopy , "/" ) ;
    property = linecopy+strlen(linecopy) ;
    /* Do command processing and make changes to 1-wire devices */
    if ( pn->dev!=&NoDevice && up->request && up->value && !readonly ) {
        strcpy( property , up->request ) ;
        if ( FS_ParsedName(linecopy,pn)==0 && pn->ft && pn->ft->write.v ) {
            switch ( pn->ft->format ) {
            case ft_binary:
                httpunescape(up->value) ;
                hex_only(up->value) ;
                if ( strlen(up->value) == pn->ft->suglen<<1 ) {
                    hex_convert(up->value) ;
                    FS_write( linecopy, up->value, pn->ft->suglen, 0 ) ;
                }
                break;
            case ft_yesno:
                if ( pn->ft->ag==NULL || pn->extension>=0 ) { // Single check box handled differently
                    FS_write( linecopy, strncasecmp(up->value,"on",2)?"0":"1", 2, 0 ) ;
                    break;
                }
                // fall through
            default:
                httpunescape(up->value) || FS_write(linecopy,up->value,strlen(up->value)+1,0) ;
                break;
            }
        }
    }
}

static void Bad400( struct active_sock * a_sock ) {
    HTTPstart( a_sock->io , "400 Bad Request" ) ;
    HTTPtitle( a_sock->io , "Error 400 -- Bad request") ;
    HTTPheader( a_sock->io , "Unrecognized Request") ;
    fprintf( a_sock->io, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested web page is not recognized.</P>" ) ;
    fprintf( a_sock->io, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>" ) ;
    HTTPfoot( a_sock->io ) ;
}

static void Bad404( struct active_sock * a_sock ) {
    HTTPstart( a_sock->io , "404 File not found" ) ;
    HTTPtitle( a_sock->io , "Error 400 -- Item doesn't exist") ;
    HTTPheader( a_sock->io , "Non-existent Device") ;
    fprintf( a_sock->io, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested device is not recognized.</P>" ) ;
    fprintf( a_sock->io, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>" ) ;
    HTTPfoot( a_sock->io ) ;
}

static void ShowDevice( struct active_sock * a_sock, const char * file, struct parsedname * pn ) {
    /* Now show the device */
    HTTPstart( a_sock->io , "200 OK" ) ;
    HTTPtitle( a_sock->io , &file[1]) ;
    HTTPheader( a_sock->io , &file[1]) ;
    if ( cacheavailable && pn->type!=pn_uncached ) fprintf( a_sock->io , "<BR><small><A href='/uncached%s'>uncached version</A></small>",file) ;
    fprintf( a_sock->io, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
    FS_dir( directory, a_sock->io, pn ) ;
    fprintf( a_sock->io, "</TABLE>" ) ;
    HTTPfoot( a_sock->io ) ;
}

/* Misnamed. Actually all directory */
static void RootDir( struct active_sock * a_sock, struct parsedname * pn ) {
    HTTPstart( a_sock->io , "200 OK" ) ;

    if ( pn->pathlength ) { /* Branch */
        int i ;
        HTTPtitle( a_sock->io , "Branch Directory") ;
        HTTPheader( a_sock->io , "Device Listing") ;
        fprintf( a_sock->io, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
        /* Branch path */
        fprintf( a_sock->io,"<TR><TD><A HREF='/") ;
        for ( i=0 ; i < pn->pathlength ; ++i ) {
            char extname[33] ;
            FS_devicename(extname,32,pn->bp[i].sn) ;
            fprintf( a_sock->io,"%s/",extname) ;
            if ( i+1 == pn->pathlength ) break ;
            fprintf( a_sock->io,"%s/",pn->bp[i].branch ? "aux":"main" ) ;
        }
        fprintf( a_sock->io,"'><CODE><B><BIG>UP</BIG></B></CODE></A></TD><TD>Branch</TD><TD>Directory</TD></TR>") ;
    } else {
        switch (pn->type) {
        case pn_uncached:
            HTTPtitle( a_sock->io , "Uncached Directory") ;
            HTTPheader( a_sock->io , "Device Listing (uncached)") ;
            fprintf( a_sock->io, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
            break ;
        case pn_alarm:
            HTTPtitle( a_sock->io , "Alarm Directory") ;
            HTTPheader( a_sock->io , "Alarming Device Listing") ;
            fprintf( a_sock->io, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
            break ;
        default:
            HTTPtitle( a_sock->io , "Directory") ;
            HTTPheader( a_sock->io , "Device Listing") ;
            fprintf( a_sock->io, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
            if ( cacheavailable && pn->pathlength==0 )
                fprintf( a_sock->io,
                    "<TR><TD><A HREF='/uncached'><CODE><B><BIG>uncached</BIG></B></CODE></A></TD><TD>Immediate</TD><TD>Directory</TD></TR>") ;
        }
    }
    FS_dir( directory, a_sock->io, pn ) ;
    fprintf( a_sock->io, "</TABLE>" ) ;
    HTTPfoot( a_sock->io ) ;
}
