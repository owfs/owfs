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

/* --------------- Prototypes---------------- */
static void Show( FILE * out, const char * const path, const char * const file, const struct parsedname * const pn ) ;
static void ShowText( FILE * out, const char * const basename, const char * const fullpath, const struct parsedname * const pn, int suglen, char *buf ) ;

/* --------------- Functions ---------------- */

/* Device entry -- table line for a filetype */
static void Show( FILE * out, const char * const path, const char * const file, const struct parsedname * const pn ) {
    int len ;
    struct parsedname pn2 ;
    const char * basename ;
    char fullpath[PATH_MAX+1] ;
    int suglen = 0 ;
    char *buf = NULL ;
    enum ft_format format ;
    int canwrite = 0 ;

    //printf("Show: path=%s, file=%s\n",path,file) ;

    if(!pn->ft) {
      format = ft_subdir ;      /* it seems to be a subdir */
    } else {
      format = pn->ft->format ;
      canwrite = !readonly && pn->ft->write.v ;
    }
    //printf("Show path=%s, file=%s, suglen=%d pn_struct?%d, ft_directory?%d, ft_subdir?%d\n",path,file,suglen,pn->type == pn_structure,format==ft_directory,format==ft_subdir);

    /* Parse out subdir */
    basename = strrchr(file,'/') ;
    if ( basename ) {
        ++basename ; /* after slash */
    } else {
        basename = file ;
    }
    strcpy(fullpath, path) ;
    if ( fullpath[strlen(fullpath)-1] != '/' ) strcat( fullpath, "/" ) ;
    strcat(fullpath,basename ) ;

    if ( (FS_ParsedName(fullpath, &pn2) == 0) ) {
        if ((pn2.state & pn_bus) && FS_RemoteBus(&pn2)) {
            //printf("call FS_size(%s)\n", fullpath);
            suglen = FS_size(fullpath) ;
        } else {
            //printf("call FS_size_postparse\n");
            suglen = FS_size_postparse(pn) ;
        }
    } else {
        suglen = 0;
        //printf("FAILED parsename %s\n", fullpath);
    }
    FS_ParsedName_destroy( &pn2 ) ;

    if(suglen <= 0) {
        //printf("Show: can't find file-size of %s ???\n", pn->path);
        suglen = 0 ;
    }
    if( ! (buf = malloc((size_t)suglen+1)) ) return;
    buf[suglen] = '\0' ;

    //printf("Show path=%s, file=%s, suglen=%d pn_struct?%d, ft_directory?%d, ft_subdir?%d\n",path,file,suglen,pn->type == pn_structure,format==ft_directory,format==ft_subdir);

    /* Special processing for structure -- ascii text, not native format */
    if ( pn->type==pn_structure && format!=ft_directory && format!=ft_subdir ) {
        format = ft_ascii ;
        canwrite = 0 ;
    }

    //if ( snprintf(fullpath,PATH_MAX,path[strlen(path)-1]=='/'?"%s%s":"%s/%s",path,basename)<0 ) return ;

    //printf("pn->path=%s, pn->path_busless=%s\n",pn->path, pn->path_busless) ;
    //printf("path=%s, file=%s, fullpath=%s\n",path,file, fullpath) ;

    /* Jump to special text-mode routine */
    if(pn->state & pn_text) {
        ShowText( out, basename, fullpath, pn, suglen, buf ) ;
        free(buf);
        return;
   }
    fprintf( out, "<TR><TD><B>%s</B></TD><TD>", basename ) ;

    /* buffer for field value */
    if ( pn->ft && pn->ft->ag && format!=ft_binary && pn->extension==-1 ) {
        if ( pn->ft->read.v ) { /* At least readable */
            if ( (len=FS_read(fullpath, buf, (size_t)suglen, 0))>=0 ) {
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
                if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>=0 ) {
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
                if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>=0 ) {
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
		    //printf("Error read %s %d\n", fullpath, suglen);
                    fprintf(out,"Error: %s",strerror(-len)) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",basename,(pn->ft->suglen)>>5 ) ;
            }
            break ;
        default:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>=0 ) {
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
static void ShowText( FILE * out, const char * const basename, const char * const fullpath, const struct parsedname * const pn, int suglen, char *buf ) {
    int len ;
    enum ft_format format ;
    int canwrite = 0 ;

    //printf("ShowText: basename=%s, fullpath=%s\n",basename,fullpath) ;

    if(!pn->ft) {
        format = ft_subdir ;      /* it seems to be a subdir */
    } else {
        format = pn->ft->format ;
        canwrite = !readonly && pn->ft->write.v ;
    }

    /* Special processing for structure -- ascii text, not native format */
    if ( pn->type == pn_structure && ( format==ft_directory || format==ft_subdir ) ) {
        format = ft_ascii ;
        canwrite = 0 ;
    }

    fprintf( out, "%s ", basename ) ;

    /* buffer for field value */
    if ( pn->ft && pn->ft->ag && format!=ft_binary && pn->extension==-1 ) {
        if ( pn->ft->read.v ) { /* At least readable */
            if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>0 ) {
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
                 if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>0 ) {
                     fprintf( out, "%c", buf[0] ) ;
                 }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "(writeonly)" ) ;
            }
            break ;
        case ft_binary:
            if ( pn->ft->read.v ) { /* At least readable */
                if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>0 ) {
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
                if ( (len=FS_read(fullpath,buf,(size_t)suglen,0))>0 ) {
                    buf[len] = '\0' ;
                    fprintf( out, "%s",buf ) ;
                }
            } else if ( canwrite ) { /* rare write-only */
                fprintf( out, "(writeonly)") ;
            }
        }
    }
    fprintf( out, "\r\n" ) ;
    return;
}

/* Now show the device */
void ShowDevice( FILE * out, const struct parsedname * const pn ) {
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
    FS_DirName(file,OW_FULLNAME_MAX,pn2);
    //printf("ShowDevice: emb: pn2->ft=%p pn2->subdir=%p pn2->dev=%p path2=%s file=%s\n", pn2->ft, pn2->subdir, pn2->dev, path2, file);
        Show( out, path2, file, pn2 ) ;
        free(file);
    }

    //printf("ShowDevice = %s  bus_nr=%d pn->dev=%p\n",pn->path, pn->bus_nr, pn->dev) ;
    if(! (path2 = strdup(pn->path)) ) return ;
    memcpy(&pncopy, pn, sizeof(struct parsedname));

    HTTPstart( out , "200 OK", (pn->state & pn_text) ) ;
    if(!(pn->state & pn_text)) {
      b = Backup(pn->path) ;
      HTTPtitle( out , &pn->path[1] ) ;
      HTTPheader( out , &pn->path[1] ) ;
      if ( IsLocalCacheEnabled(pn) && !(pn->state & pn_uncached) && pn->type==pn_real) fprintf( out , "<BR><small><A href='/uncached%s'>uncached version</A></small>",pn->path) ;
      fprintf( out, "<TABLE BGCOLOR=\"#DDDDDD\" BORDER=1>" ) ;
      fprintf( out, "<TR><TD><A HREF='%.*s'><CODE><B><BIG>up</BIG></B></CODE></A></TD><TD>directory</TD></TR>",b, pn->path ) ;
    }


    if ( pn->ft ) { /* single item */
        //printf("single item path=%s pn->path=%s\n", path2, pn->path);
        slash = strrchr(path2,'/') ;
        /* Nested function */
        if ( slash ) slash[0] = '\0' ; /* pare off device name */
            //printf("single item path=%s\n", path2);
        directory(&pncopy) ;
    } else { /* whole device */
        //printf("whole directory path=%s pn->path=%s\n", path2, pn->path);
        //printf("pn->dev=%p pn->ft=%p pn->subdir=%p\n", pn->dev, pn->ft, pn->subdir);
        FS_dir( directory, &pncopy ) ;
    }
    if(!(pn->state & pn_text)) {
        fprintf( out, "</TABLE>" ) ;
        HTTPfoot( out ) ;
    }
    free(path2) ;
}
