/*
 * $Id$
 */

#include "owftpd.h"
#include <limits.h>
#include <stdarg.h>
#include <fnmatch.h>

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

enum fil_sta {
    filsta_init, // need to test for initial /
    filsta_init2,// need to test for virginity (no wildness)
    filsta_back, // .. still allowed
    filsta_next, // figure out this level
    filsta_last, // last level
    filsta_tame, // no wildcard at all
} ;

enum file_list {
    flist_list ,
    flist_nlst ,
} ;

static void fdprintf(int fd, const char *fmt, ...);
static void List_show( enum file_list fl, int out, const struct parsedname * pn ) ;
static int FileLexParse( ASCII * CurBuffer, ASCII * rest, enum fil_sta filsta, enum file_list fl, int out ) ;
static int WildLexParse( ASCII * CurBuffer, ASCII * match, ASCII * rest, enum file_list fl, int out ) ;
static const char * skip_ls_options(const char *filespec) ;

static void List_show( enum file_list fl, int out, const struct parsedname * pn ) {
    struct stat stbuf ;
    time_t now;
    struct tm tm_now;
    double age;
    char date_buf[13];
    ASCII * fil = strrchr( pn->path, '/' ) ;

    switch( fl ) {
        case flist_list:
            FS_fstat_postparse(&stbuf,pn) ;
            fdprintf(out, stbuf.st_mode&S_IFDIR ? "d" : "-" ) ;
            fdprintf(out, pn->ft->read.v ? "r" : "-" ) ;
            fdprintf(out, pn->ft->write.v ? "w-" : "--" ) ;
            fdprintf(out, pn->ft->read.v ? "r" : "-" ) ;
            fdprintf(out, pn->ft->write.v ? "w-" : "--" ) ;
            fdprintf(out, pn->ft->read.v ? "r" : "-" ) ;
            fdprintf(out, pn->ft->write.v ? "w-" : "--" ) ;
            /* output link & ownership information */
            fdprintf(out, " %3d %-8d %-8d %8lu ",
                     stbuf.st_nlink,
                     stbuf.st_uid,
                     stbuf.st_gid,
                     (unsigned long)stbuf.st_size);
            /* output date */
            localtime_r(&stbuf.st_mtime, &tm_now);
            age = difftime(&tm_now, stbuf.st_mtime);
            if ((age > 60 * 60 * 24 * 30 * 6) || (age < -(60 * 60 * 24 * 30 * 6))) {
                strftime(date_buf, sizeof(date_buf), "%b %e  %Y", &tm_now);
            } else {
                strftime(date_buf, sizeof(date_buf), "%b %e %H:%M", &tm_now);
            }
            fdprintf(out, "%s ", date_buf);
            /* Fall Through */
        case flist_nlst:
            /* output filename */
            fdprintf(out, "%s\r\n", &fil[1]);
    }
}

static int FileLexParse( ASCII * CurBuffer, ASCII * rest, enum fil_sta filsta, enum file_list fl, int out ) {
    struct parsedname pn ;
    while ( 1 ) {
        LEVEL_DEBUG("FileLexParse Path=%s, rest=%s\n",SAFESTRING(CurBuffer),SAFESTRING(rest));
        switch( filsta ) {
            case filsta_init:
                printf("filsta_init\n");
                rest = skip_ls_options(rest) ;
                if ( CurBuffer[strlen(CurBuffer)-1]!='/' ) strcat( CurBuffer, "/" ) ;
                if ( rest==NULL || rest[0]=='\0' ) {
                    filsta = filsta_tame ;
                } else if ( rest[0] == '/' ) {
                    strcpy( CurBuffer, "/" ) ;
                    rest = &rest[1] ;
                    filsta = filsta_init2 ;
                } else {
                    filsta = filsta_init2 ;
                }
                break ;
            case filsta_init2:
                printf("filsta_init2\n");
                if ( index( rest, '*') || index( rest, '[') || index( rest, '?' ) ) {
                    filsta = filsta_back ;
                } else {
                    filsta = filsta_tame ;
                }
                break ;
            case filsta_back:
                printf("filsta_back\n");
                if ( rest[0]=='.' && rest[1]=='.' ) {
                    // Move back
                    ASCII * back = strrchr( CurBuffer, '/' ) ;
                    if ( back && (back[1]=='\0') ) {
                        back[0] = '\0' ;
                        back = strrchr( CurBuffer, '/' ) ;
                    }
                    if ( back ) {
                        back[1] = '\0' ;
                    } else {
                        strcpy( CurBuffer, "/" ) ;
                    }
                    // look for next file part
                    if ( rest[2]=='\0' || rest[2]== '/' ) {
                        filsta = filsta_next ;
                        rest = &rest[3] ;
                    } else {
                        return -ENOENT ;
                    }
                } else {
                    filsta = filsta_next ; // off the double dot trail
                }
                break ;
            case filsta_next:
                printf("filsta_next\n");
                if ( rest==NULL ) {
                    filsta = filsta_last ;
                } else {
                    ASCII * oldrest = strsep( &rest, "/" ) ;
                    if ( index( oldrest, '*') || index( oldrest, '[') || index( oldrest, '?' ) ) {
                        return WildLexParse( CurBuffer, oldrest, rest, fl, out ) ;
                    } else {
                        strcat( CurBuffer, oldrest ) ;
                        if ( rest ) strcat( CurBuffer, "/" ) ;
                        filsta = filsta_next ;
                    }
                }
                break ;
            case filsta_tame:
                printf("filsta_tame\n");
                strcpy( CurBuffer, rest ) ;
                if ( CurBuffer[strlen(CurBuffer)-1]=='/' )
                    return WildLexParse( CurBuffer, "*", NULL, fl, out ) ;
                if ( FS_ParsedName( CurBuffer, &pn ) ) {
                    if ( IsDir(&pn) ) {
                        strcat( CurBuffer, "/" ) ;
                        WildLexParse( CurBuffer, "*", NULL, fl, out ) ;
                    } else {
                        List_show( fl, out, &pn ) ;
                    }
                    FS_ParsedName_destroy( &pn ) ;
                }
                return 0 ;
            case filsta_last:
                printf("filsta_last\n");
                if ( FS_ParsedNamePlus( CurBuffer, rest, &pn ) ) {
                    List_show( fl, out, &pn ) ;
                    FS_ParsedName_destroy( &pn ) ;
                }
                return 0 ;
        }
    }
}

static int WildLexParse( ASCII * CurBuffer, ASCII * match, ASCII * rest, enum file_list fl, int out ) {
    ASCII * end = &CurBuffer[strlen(CurBuffer)] ;
    int ret ;
    struct parsedname pn ;
    /* Embedded callback function */
    void directory( const struct parsedname * const pn2 ) {
        FS_DirName( end, OW_FULLNAME_MAX, pn2 ) ;
        fdprintf(out,"Try %s\r\n",end) ;
        //if ( fnmatch( match, end, FNM_PATHNAME|FNM_CASEFOLD ) ) return ;
        if ( fnmatch( match, end, FNM_PATHNAME ) ) return ;
        FileLexParse( CurBuffer, rest, filsta_next, fl, out ) ;
    }

    LEVEL_DEBUG("Wildcard patern matching. Path=%s, Pattern=%s, rest=%s\n",SAFESTRING(CurBuffer),SAFESTRING(match),SAFESTRING(rest));
    if ( FS_ParsedName( CurBuffer, &pn ) ) {
        ret = -ENOENT ;
    } else {
        if ( pn.ft ) {
        ret = -ENOTDIR ;
        } else {
            FS_dir( directory, &pn ) ;
            end = '\0' ; // restore CurBuffer
        }
        FS_ParsedName_destroy( &pn ) ;
    }
    return ret ;
}
                                                
            

/* if no localtime_r() is available, provide one */
#ifndef HAVE_LOCALTIME_R
#include <pthread.h>

struct tm *localtime_r(const time_t *timep, struct tm *timeptr) {
    static pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&time_lock);
    *timeptr = *(localtime(timep));
    pthread_mutex_unlock(&time_lock);
    return timeptr;
}
#endif /* HAVE_LOCALTIME_R */

int file_nlst(int out, const char *cur_dir, const char *filespec) {
    char pattern[PATH_MAX+1];

    strcpy( pattern, "/" ) ;
    if ( cur_dir ) strcpy( pattern, cur_dir ) ;
    LEVEL_DEBUG("NLST dir=%s, file=%s\n",SAFESTRING(pattern),SAFESTRING(filespec)) ;
    return FileLexParse( pattern, filespec, filsta_init, flist_nlst, out )==0 ;
}

int file_list(int out, const char *cur_dir, const char *filespec) {
    char pattern[PATH_MAX+1];
    
    strcpy( pattern, "/" ) ;
    if ( cur_dir ) strcpy( pattern, cur_dir ) ;
    LEVEL_DEBUG("NLST dir=%s, file=%s\n",SAFESTRING(pattern),SAFESTRING(filespec)) ;
    return FileLexParse( pattern, filespec, filsta_init, flist_list, out )==0 ;
}
/* write with care for max length and incomplete outout */
static void fdprintf(int fd, const char *fmt, ...) {
    char buf[PATH_MAX+1];
    int buflen;
    va_list ap;
    int amt_written;
    int write_ret;

    daemon_assert(fd >= 0);
    daemon_assert(fmt != NULL);

    va_start(ap, fmt);
    buflen = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (buflen <= 0) {
        return;
    }
    if (buflen >= sizeof(buf)) {
        buflen = sizeof(buf)-1;
    }

    amt_written = 0;
    while (amt_written < buflen) {
        write_ret = write(fd, buf+amt_written, buflen-amt_written);
        if (write_ret <= 0) {
            return;
        }
        amt_written += write_ret;
    }
}

/* 
  hack workaround clients like Midnight Commander that send:
      LIST -al /dirname 
*/
static const char * skip_ls_options(const char *filespec) {
    daemon_assert(filespec != NULL);

    for (;;) {
        /* end when we've passed all options */
        if (*filespec != '-') {
            break;
        }
        filespec++;

        /* if we find "--", eat it and any following whitespace then return */
        if ((filespec[0] == '-') && (filespec[1] == ' ')) {
            filespec += 2;
            while (isspace(*filespec)) {
                filespec++;
            }
            break;
        }

        /* otherwise, skip this option */
        while ((*filespec != '\0') && !isspace(*filespec)) {
            filespec++;
        }

        /* and skip any whitespace */
        while (isspace(*filespec)) {
            filespec++;
        }
    }

    daemon_assert(filespec != NULL);

    return filespec;
}

