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

// #include <libgen.h>  /* for dirname() */

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
static void HTTPstart(FILE * out, const char * status, const int text ) ;
static void HTTPtitle(FILE * out, const char * title ) ;
static void HTTPheader(FILE * out, const char * head ) ;
static void HTTPfoot(FILE * out ) ;

    /* string format functions */
static void hex_convert( char * str ) ;
static void hex_only( char * str ) ;
static int httpunescape(unsigned char *httpstr) ;
static int Backup( const char * path ) ;

    /* Error page functions */
static void Bad400( FILE * out ) ;
static void Bad404( FILE * out ) ;

    /* Directory display functions */
static void ShowDir( FILE * out, const struct parsedname * const pn ) ;

/* URL parsing function */
static void URLparse( struct urlparse * up ) ;

    /* Data edit function */
static void ChangeData( struct urlparse * up , const struct parsedname * pn ) ;

    /* Device display functions */
static void ShowDevice( FILE * out, const struct parsedname * const pn ) ;
static void Show( FILE * out, const char * const path, const char * const dev, const struct parsedname * const pn ) ;
static void ShowText( FILE * out, const char * const basename, const char * const fullpath, const struct parsedname * const pn ) ;

/* --------------- Functions ---------------- */

/* Main handler for a web page */
int handle_socket(FILE * out) {
    char linecopy[PATH_MAX+1];
    char * str;
    struct urlparse up ;
    struct parsedname pn ;
    struct stateinfo si ;
    pn.si = &si ;

    str = fgets(up.line, PATH_MAX, out);
    //printf("PreParse line=%s\n",up.line);
    URLparse( &up ) ; /* Braek up URL */
    //printf("WLcmd: %s\nfile: %s\nrequest: %s\nvalue: %s\nversion: %s \n",up.cmd,up.file,up.request,up.value,up.version) ;

    /* read lines until blank */
    if (up.version) {
        do {
            str = fgets(linecopy, PATH_MAX, out);
        } while (str != NULL && strcmp(linecopy, "\r\n") && strcmp(linecopy, "\n"));
    }

//printf("WLcmd: %s\nfile: %s\nrequest: %s\nvalue: %s\nversion: %s \n",up.cmd,up.file,up.request,up.value,up.version) ;
    /*
     * This is necessary for some stupid *
     * * operating system such as SunOS
     */
    fflush(out);

    /* Good command line? */
    if ( up.cmd==NULL || strcmp(up.cmd, "GET")!=0 ) {
        Bad400( out ) ;

    /* Can't understand the file name = URL */
    } else if ( up.file == NULL ) {
        Bad404( out ) ;
    } else {
        //printf("PreParse up.file=%s\n", up.file);
        if ( FS_ParsedName( up.file , &pn ) ) {
        /* Can't understand the file name = URL */
            Bad404( out ) ;
        /* Root directory -- show the bus */
        } else if ( pn.dev == NULL ) { /* directory! */
	    ShowDir( out,& pn ) ;
        /* Single device, show it's properties */
        } else {
//printf("PreChange\n");
            ChangeData( &up, &pn ) ;
//printf("PreShow\n");
            ShowDevice( out, &pn ) ;
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
        /* trim off trailing '/' */
        --str ;
        if ( *str=='/' && str>up->file ) *str = '\0' ;
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

static void HTTPstart(FILE * out, const char * status, const int text ) {
    char d[44] ;
    time_t t = time(NULL) ;
    size_t l = strftime(d,sizeof(d),"%a, %d %b %Y %T GMT",gmtime(&t)) ;

    fprintf(out, "HTTP/1.0 %s\r\n", status);
    fprintf(out, "Date: %*s\n", (int)l, d );
    fprintf(out, "Server: %s\r\n", SVERSION);
    fprintf(out, "Last-Modified: %*s\r\n", (int)l, d );
    /*
     * fprintf( out, "MIME-version: 1.0\r\n" );
     */
    if(text)
      fprintf(out, "Content-Type: text/plain\r\n");
    else
      fprintf(out, "Content-Type: text/html\r\n");
    fprintf(out, "\r\n");
}

static void HTTPtitle(FILE * out, const char * title ) {
    fprintf(out, "<HTML><HEAD><TITLE>1-Wire Web: %s</TITLE></HEAD>\n", title);
}

static void HTTPheader(FILE * out, const char * head ) {
    fprintf(out, "<BODY "BODYCOLOR"><TABLE "TOPTABLE"><TR><TD>OWFS on %s</TD><TD><A HREF='/'>Bus listing</A></TD><TD><A HREF='http://owfs.sourceforge.net'>OWFS homepage</A></TD><TD><A HREF='http://www.maxim-ic.com'>Dallas/Maxim</A></TD><TD>by <A HREF='mailto://palfille@earthlink.net'>Paul H Alfille</A></TD></TR></TABLE>\n", indevice&&indevice->name?indevice[0].name:"");
    fprintf(out, "<H1>%s</H1><HR>\n", head);
}

static void HTTPfoot(FILE * out ) {
    fprintf(out, "</BODY></HTML>") ;
}


/* Device entry -- table line for a filetype */
static void Show( FILE * out, const char * const path, const char * const file, const struct parsedname * const pn ) {
    int len ;
    const char * basename ;
    char fullpath[PATH_MAX+1] ;
    int suglen ;
    char *buf ;
    enum ft_format format = pn->ft->format ;
    int canwrite = !readonly && pn->ft->write.v ;

    suglen = FullFileLength(pn) ;
    if( ! (buf = malloc(suglen+1)) ) {
      return;
    }

    //printf("Show path=%s, file=%s, suglen=%d pn_struct?%d, ft_directory?%d, ft_subdir?%d\n",path,file,suglen,pn->type == pn_structure,format==ft_directory,format==ft_subdir);
    /* Special processing for structure -- ascii text, not native format */
    if ( pn->type==pn_structure && format!=ft_directory && format!=ft_subdir ) {
        format = ft_ascii ;
        canwrite = 0 ;
    }

    buf[suglen] = '\0' ;

    //printf("path=%s, file=%s\n",path,file) ;
    /* Parse out subdir */
    basename = strrchr(file,'/') ;
    if ( basename ) {
        ++basename ; /* after slash */
    } else {
        basename = file ;
    }

    /* full file name (with path and subdir) */
    strcpy(fullpath,path) ;
    if ( fullpath[strlen(fullpath)-1] != '/' ) strcat( fullpath, "/" ) ;
    strcat(fullpath,basename ) ;

//    if ( snprintf(fullpath,PATH_MAX,path[strlen(path)-1]=='/'?"%s%s":"%s/%s",path,basename)<0 ) return ;
//printf("path=%s, file=%s, fullpath=%s\n",path,file, fullpath) ;

    /* Jump to special text-mode routine */
    if(pn->state & pn_text) {
        ShowText( out, basename, fullpath, pn ) ;
	free(buf);
	return;
   }
    fprintf( out, "<TR><TD><B>%s</B></TD><TD>", basename ) ;

    /* buffer for field value */
    if ( pn->ft->ag && format!=ft_binary && pn->extension==-1 ) {
        if ( pn->ft->read.v ) { /* At least readable */
            if ( (len=FS_read(fullpath,buf,suglen,0))>=0 ) {
                buf[len] = '\0' ;
                if ( canwrite ) { /* read-write */
                    fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",basename,buf ) ;
                } else { /* read only */
                    fprintf( out, "%s", buf ) ;
                }
            } else {
                fprintf(out,"Error: %s",strerror(-len)) ;
            }
        } else if ( canwrite ) { /* rare write-only */
            fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",basename );
        }
    } else {
        switch( format ) {
        case ft_directory:
        case ft_subdir:
            fprintf( out, "<A HREF='%s'>%s</A>",fullpath,file);
            break ;
        case ft_yesno:
            if ( pn->ft->read.v ) { /* at least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>=0 ) {
                    buf[len]='\0' ;
                    if ( canwrite ) { /* read-write */
                        fprintf( out, "<FORM METHOD=\"GET\"><INPUT TYPE='CHECKBOX' NAME='%s' %s><INPUT TYPE='SUBMIT' VALUE='CHANGE' NAME='%s'></FORM></FORM>", basename, (buf[0]=='0')?"":"CHECKED", basename ) ;
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
                } else {
                    fprintf(out,"Error: %s",strerror(-len)) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='ON'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='OFF'></FORM>",basename,basename ) ;
            }
            break ;
        case ft_binary:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>=0 ) {
                    if ( canwrite ) { /* read-write */
                        int i = 0 ;
                        fprintf( out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'>",basename,len>>5 ) ;
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
                } else {
                    fprintf(out,"Error: %s",strerror(-len)) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",basename,(pn->ft->suglen)>>5 ) ;
            }
            break ;
        default:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>=0 ) {
                    buf[len] = '\0' ;
                    if ( canwrite ) { /* read-write */
                        fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",basename,buf ) ;
                    } else { /* read only */
                        fprintf( out, "%s", buf ) ;
                    }
                } else {
                    fprintf(out,"Error: %s",strerror(-len)) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",basename ) ;
            }
        }
    }
    fprintf( out, "</TD></TR>\r\n" ) ;
    free(buf);
}

/* Device entry -- table line for a filetype  -- text mode*/
static void ShowText( FILE * out, const char * const basename, const char * const fullpath, const struct parsedname * const pn ) {
    int len ;
    int suglen = FullFileLength(pn) ;
    char buf[suglen+1] ;
    enum ft_format format = pn->ft->format ;
    int canwrite = !readonly && pn->ft->write.v ;

    /* Special processing for structure -- ascii text, not native format */
    if ( pn->type == pn_structure && ( format==ft_directory || format==ft_subdir ) ) {
        format = ft_ascii ;
        canwrite = 0 ;
    }

    buf[suglen] = '\0' ;


    fprintf( out, "%s ", basename ) ;

    /* buffer for field value */
    if ( pn->ft->ag && format!=ft_binary && pn->extension==-1 ) {
        if ( pn->ft->read.v ) { /* At least readable */
            if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                buf[len] = '\0' ;
                fprintf( out, "%s",buf ) ;
            }
        } else if ( canwrite ) { /* rare write-only */
	      fprintf( out, "(writeonly)" ) ;
        }
    } else {
        switch( format ) {
        case ft_directory:
        case ft_subdir:
            break ;
        case ft_yesno:
            if ( pn->ft->read.v ) { /* at least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                    fprintf( out, "%c", buf[0] ) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "(writeonly)" ) ;
            }
            break ;
        case ft_binary:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                    int i ;
                    for( i=0 ; i<len ; ++i ) {
                        fprintf( out, "%.2hhX", buf[i] ) ;
                    }
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "(writeonly)" ) ;
            }
            break ;
        default:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,suglen,0))>0 ) {
                    buf[len] = '\0' ;
                    fprintf( out, "%s",buf ) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "(writeonly)") ;
            }
        }
    }
    fprintf( out, "\r\n" ) ;
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

static void ChangeData( struct urlparse * up, const struct parsedname * pn ) {
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
printf("Change data on %s to %s\n",linecopy,up->value) ;
        if ( FS_ParsedName(linecopy,&pn2)==0 && pn2.ft && pn2.ft->write.v ) {
            switch ( pn2.ft->format ) {
            case ft_binary:
                httpunescape(up->value) ;
                hex_only(up->value) ;
                if ( strlen(up->value) == pn2.ft->suglen<<1 ) {
                    hex_convert(up->value) ;
                    FS_write( linecopy, up->value, pn2.ft->suglen, 0 ) ;
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
    }
}

static void Bad400( FILE * out ) {
    HTTPstart( out , "400 Bad Request", 0 ) ;
    HTTPtitle( out , "Error 400 -- Bad request" ) ;
    HTTPheader( out , "Unrecognized Request" ) ;
    fprintf( out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested web page is not recognized.</P>" ) ;
    fprintf( out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>" ) ;
    HTTPfoot( out ) ;
}

static void Bad404( FILE * out ) {
    HTTPstart( out , "404 File not found", 0 ) ;
    HTTPtitle( out , "Error 400 -- Item doesn't exist" ) ;
    HTTPheader( out , "Non-existent Device" ) ;
    fprintf( out, "<P>The 1-wire web server is carefully constrained for security and stability. Your requested device is not recognized.</P>" ) ;
    fprintf( out, "<P>Navigate from the <A HREF=\"/\">Main page</A> for best results.</P>" ) ;
    HTTPfoot( out ) ;
}

/* Find he next higher level by search for last slash that doesn't end the string */
/* return length of "higher level" */
/* Assumes good path:
    null delimitted
    starts with /
    non-null
*/
static int Backup( const char * path ) {
    int i = strlen(path)-1 ;
    if (i==0) return 1 ;
    if (path[i]=='/') --i ;
    while( path[i]!='/') --i ;
    return i+1 ;
}

/* Now show the device */
static void ShowDevice( FILE * out, const struct parsedname * const pn ) {
    struct parsedname pncopy ;
    char * slash;
    int b ;
    char * path2;
    /* Embedded function */
    void directory( const struct parsedname * const pn2 ) {
        char *file;
	if( !(file = malloc(OW_FULLNAME_MAX+1)) ) { /* buffer for name */
	  //printf("ShowDevice error malloc %d bytes\n",OW_FULLNAME_MAX+1) ;
	  return;
	}
        if ( FS_FileName(file,OW_FULLNAME_MAX,pn2) ) return ;
        Show( out, path2, file, pn2 ) ;
	free(file);
    }

    //printf("ShowDevice = %s\n",pn->path) ;
    if( !(path2 = strdup(pn->path)) ) {
      return;
    }
    memcpy(&pncopy, pn, sizeof(struct parsedname));
    pncopy.badcopy = 1;

    HTTPstart( out , "200 OK", (pn->state & pn_text) ) ;
    if(!(pn->state & pn_text)) {
      b = Backup(pn->path) ;
      HTTPtitle( out , &path2[1] ) ;
      HTTPheader( out , &path2[1] ) ;
      if ( IsLocalCacheEnabled(pn) && !(pn->state & pn_uncached) && pn->type==pn_real) fprintf( out , "<BR><small><A href='/uncached%s'>uncached version</A></small>",pn->path) ;
      fprintf( out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
      fprintf( out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>directory</TD></TR>",b, pn->path ) ;
    }


    if ( pn->ft ) { /* single item */
        slash = strrchr(path2,'/') ;
        /* Nested function */
        if ( slash ) slash[0] = '\0' ; /* pare off device name */
        directory(&pncopy) ;
    } else { /* whole device */
        FS_dir( directory, &pncopy ) ;
    }
    if(!(pn->state & pn_text)) {
      fprintf( out, "</TABLE>" ) ;
      HTTPfoot( out ) ;
    }
    free(path2) ;
}

/* Misnamed. Actually all directory */
static void ShowDir( FILE * out, const struct parsedname * const pn ) {
    struct parsedname pncopy;
    int b;
    /* Embedded function */
    /* Callback function to FS_dir */
    void directory( const struct parsedname * const pn2 ) {
        /* uncached tag */
        /* device name */
	char *buffer ;
        char * loc ;
        char * nam ;
        char * typ = NULL ;
	if( !(buffer = malloc(OW_FULLNAME_MAX+1) ) ) { /* buffer for name */
	  //printf("ShowDir: error malloc %d bytes\n", OW_FULLNAME_MAX+1);
	  return ;
	}
	loc = nam = buffer ;
        *buffer = '\000';
        if ( pn2->dev==NULL ) {
            if ( pn2->type != pn_real ) {
                strcpy(buffer, FS_dirname_type(pn2->type));
            } else if ( pn2->state & (pn_uncached | pn_alarm) ) {
                strcpy(buffer, FS_dirname_state(pn2->state & (pn_uncached | pn_alarm)));
            }
	    typ = strdup("directory") ;
        } else if ( pn2->dev == DeviceSimultaneous ) {
            loc = nam = pn2->dev->name ;
            typ = strdup("1-wire chip") ;
        } else if ( pn2->type == pn_real ) {
            FS_devicename(loc,OW_FULLNAME_MAX,pn2) ;
            nam = pn2->dev->name ;
            typ = strdup("1-wire chip") ;
        } else {
            loc = pn2->dev->code ;
            nam = loc ;
	    typ = strdup("directory") ;
        }
//printf("path=%s loc=%s name=%s typ=%s pn->dev=%p pn->ft=%p pn->subdir=%p pathlength=%d\n",pn->path,loc,nam,typ,pn->dev,pn->ft,pn->subdir,pn->pathlength ) ;
        if (pncopy.state & pn_text) {
            fprintf( out, "%s %s \"%s\"\r\n", loc, nam, typ ) ;
        } else {
            fprintf( out, "<TR><TD><A HREF='%s/%s'><CODE><B><BIG>%s</BIG></B></CODE></A></TD><TD>%s</TD><TD>%s</TD></TR>",
                     (strcmp(pncopy.path,"/")?pncopy.path:""), loc, loc, nam, typ ) ;
        }
	free(typ);
	free(buffer);
    }

    memcpy(&pncopy, pn, sizeof(struct parsedname));
    pncopy.badcopy = 1;

    HTTPstart( out , "200 OK", (pn->state & pn_text) ) ;

    //printf("ShowDir=%s\n", pn->path) ;
    if(pn->state & pn_text) {
      FS_dir( directory, &pncopy ) ;
      return;
    }
    HTTPtitle( out , "Directory") ;

    if ( pn->type != pn_real ) {
        HTTPheader( out , FS_dirname_type(pn->type) ) ;
    } else if (pn->state & (pn_uncached | pn_alarm) ) {
        HTTPheader( out , FS_dirname_state(pn->state & (pn_uncached | pn_alarm)) ) ;
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
