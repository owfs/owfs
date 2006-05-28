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
static void Show( FILE * out, const char * const path, const char * const file ) ;
static void ShowText( FILE * out, const char * path, const char * file ) ;

/* --------------- Functions ---------------- */

/* Device entry -- table line for a filetype */
static void Show( FILE * out, const char * path, const char * file ) {
    struct parsedname pn ;

    //printf("Show: path=%s, file=%s\n",path,file) ;
    if ( FS_ParsedNamePlus( path, file, &pn ) ) {
        fprintf( out, "<TR><TD><B>%s</B></TD><TD>", file ) ;
        fprintf( out, "<B>Unparsable name</B></TD></TR>" ) ;
        return ;
    }
    
    /* Left column */
    fprintf( out, "<TR><TD><B>%s</B></TD><TD>", file ) ;
    
    if ( pn.ft==NULL || pn.ft->format==ft_directory || pn.ft->format==ft_subdir ) { /* Directory jump */
        fprintf( out, "<A HREF='%s'>%s</A>",pn.path,file);
    } else {
        int canwrite = !readonly && ( pn.ft->write.v != NULL ) ;
        int canread = ( pn.ft->read.v != NULL ) ;
        int suglen = 0 ;
        enum ft_format format = pn.ft->format ;
        int len ;
        char *buf = NULL ;

        if ( pn.type==pn_structure ) {
            format = ft_ascii ;
            canread = 1 ;
            canwrite = 0 ;
        }
        
        if ( (suglen=FullFileLength(&pn)) < 0 ) {
            //printf("Show: can't find file-size of %s ???\n", pn->path);
            suglen = 0 ;
        }
        
        /* buffer for field value */
        if( (buf = malloc((size_t)suglen+1)) ) {
            buf[suglen] = '\0' ;
        
            if ( canread ) { /* At least readable */
                if ( (len=FS_read_postparse(buf, (size_t)suglen, 0, &pn))>=0 ) {
                    buf[len] = '\0' ;
                    //printf("SHOW read of %s len = %d, value=%s\n",pn.path,len,SAFESTRING(buf)) ;
                }
            }
        
            if ( format == ft_binary ) { /* bianry uses HEX mode */
                if ( canread ) { /* At least readable */
                    if ( len>=0 ) {
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
                    } else {
                    //printf("Error read %s %d\n", fullpath, suglen);
                        fprintf(out,"Error: %s",strerror(-len)) ;
                    }
                } else if ( canwrite ) { /* rare write-only */
                    fprintf( out, "<CODE><FORM METHOD='GET'><TEXTAREA NAME='%s' COLS='64' ROWS='%-d'></TEXTAREA><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM></CODE>",file,(pn.ft->suglen)>>5 ) ;
                }
            } else if ( pn.extension>=0 && (format==ft_yesno||format==ft_bitfield) ) {
                if ( canread ) { /* at least readable */
                    if ( len>=0 ) {
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
                    } else {
                        fprintf(out,"Error: %s",strerror(-len)) ;
                    }
                } else if ( canwrite ) { /* rare write-only */
                    fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='ON'><INPUT TYPE='SUBMIT' NAME='%s' VALUE='OFF'></FORM>",file,file ) ;
                }
            } else{
                if ( canread ) { /* At least readable */
                    if ( len>=0 ) {
                        if ( canwrite ) { /* read-write */
                            fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s' VALUE='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",file,buf ) ;
                        } else { /* read only */
                            fprintf( out, "%s", buf ) ;
                        }
                    } else {
                        fprintf(out,"Error: %s",strerror(-len)) ;
                    }
                } else if ( canwrite ) { /* rare write-only */
                    fprintf( out, "<FORM METHOD='GET'><INPUT TYPE='TEXT' NAME='%s'><INPUT TYPE='SUBMIT' VALUE='CHANGE'></FORM>",file );
                }
            }
            free(buf);
        }
    }
    fprintf( out, "</TD></TR>\r\n" ) ;
    FS_ParsedName_destroy( &pn ) ;
}

/* Device entry -- table line for a filetype */
static void ShowText( FILE * out, const char * path, const char * file ) {
    struct parsedname pn ;

    //printf("Show: path=%s, file=%s\n",path,file) ;
    if ( FS_ParsedNamePlus( path, file, &pn ) ) {
        return ;
    }

    /* Left column */
    fprintf( out, "%s ", file ) ;
    
    if ( pn.ft==NULL || pn.ft->format==ft_directory || pn.ft->format==ft_subdir ) { /* Directory jump */
    } else {
        int canwrite = !readonly && ( pn.ft->write.v != NULL ) ;
        int canread = ( pn.ft->read.v != NULL ) ;
        int suglen = 0 ;
        enum ft_format format = pn.ft->format ;
        int len ;
        char *buf = NULL ;

        if ( pn.type==pn_structure ) {
            format = ft_ascii ;
            canread = 1 ;
            canwrite = 0 ;
        }
        
        if ( (suglen=FullFileLength(&pn)) < 0 ) {
            //printf("Show: can't find file-size of %s ???\n", pn->path);
            suglen = 0 ;
        }

        /* buffer for field value */
        if( (buf = malloc((size_t)suglen+1)) ) {
            buf[suglen] = '\0' ;
        
            if ( canread ) { /* At least readable */
                if ( (len=FS_read_postparse(buf, (size_t)suglen, 0, &pn))>=0 ) {
                    buf[len] = '\0' ;
                    //printf("SHOW read of %s len = %d, value=%s\n",pn.path,len,SAFESTRING(buf)) ;
                }
            }
        
            if ( format == ft_binary ) { /* bianry uses HEX mode */
                if ( canread ) { /* At least readable */
                    if ( len>=0 ) {
                        int i ;
                        for( i=0 ; i<len ; ++i ) {
                            fprintf( out, "%.2hhX", buf[i] ) ;
                        }
                    }
                } else if ( canwrite ) { /* rare write-only */
                    fprintf( out, "(writeonly)") ;
                }
            } else if ( pn.extension>=0 && (format==ft_yesno||format==ft_bitfield) ) {
                if ( canread ) { /* at least readable */
                    if ( len>=0 ) {
                        fprintf( out, "%c", buf[0] ) ;
                    }
                } else if ( canwrite ) { /* rare write-only */
                    fprintf( out, "(writeonly)") ;
                }
            } else{
                if ( canread ) { /* At least readable */
                    if ( len>=0 ) {
                        fprintf( out, "%s", buf ) ;
                    }
                } else if ( canwrite ) { /* rare write-only */
                    fprintf( out, "(writeonly)") ;
                }
            }
            free(buf);
        }
    }
    fprintf( out, "\r\n" ) ;
    FS_ParsedName_destroy( &pn ) ;
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
        pn2->state & pn_text ? ShowText( out, path2, file ) : Show( out, path2, file ) ;
        free(file);
    }

    //printf("ShowDevice = %s  bus_nr=%d pn->dev=%p\n",pn->path, pn->bus_nr, pn->dev) ;
    if(! (path2 = strdup(pn->path)) ) return ;
    memcpy(&pncopy, pn, sizeof(struct parsedname));

    HTTPstart( out , "200 OK", (pn->state & pn_text)?ct_text:ct_html ) ;
    if(!(pn->state & pn_text)) {
        b = Backup(pn->path) ;
        HTTPtitle( out , &pn->path[1] ) ;
        HTTPheader( out , &pn->path[1] ) ;
        if ( IsLocalCacheEnabled(pn) && !(pn->state & pn_uncached) && pn->type==pn_real)
            fprintf( out , "<BR><small><A href='/uncached%s'>uncached version</A></small>",pn->path) ;
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
