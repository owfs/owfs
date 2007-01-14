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

static void WildLexCD( struct cd_parse_s * cps, ASCII * match ) ;

void FileLexCD( struct cd_parse_s * cps ) {
    struct parsedname pn ;
    while ( 1 ) {
        switch( cps->pse ) {
            case parse_status_init:
                LEVEL_DEBUG("FTP parse_status_init Path<%s> File <%s>\n",cps->buffer,cps->rest);
                /* cps->buffer is absolute */
                /* trailing / only at root */
                cps->ret = 0 ;
                cps->solutions = 0 ;
                cps->dir = NULL ;
                if ( cps->rest==NULL || cps->rest[0]=='\0' ) {
                    cps->pse = parse_status_tame ;
                } else{
                    if ( cps->rest[0]=='/' ) { // root (absolute) specification
                        cps->buffer[1] = '\0' ;
                        ++cps->rest ;
                    }
                    cps->pse = parse_status_init2 ;
                }
                break ;
            case parse_status_init2:
                LEVEL_DEBUG("FTP parse_status_init2 Path<%s> File <%s>\n",cps->buffer,cps->rest);
                /* cps->buffer is absolute */
                /* trailing / only at root */
                if ( (cps->rest[0]=='.'&&cps->rest[1]=='.') || strpbrk( cps->rest, "*[?" ) ) {
                    cps->pse = parse_status_back ;
                } else {
                    cps->pse = parse_status_tame ;
                }
                break ;
            case parse_status_back:
                LEVEL_DEBUG("FTP parse_status_back Path<%s> File <%s>\n",cps->buffer,cps->rest);
                /* cps->buffer is absolute */
                /* trailing / only at root */
                if ( cps->rest[0]=='.' && cps->rest[1]=='.' ) {
                    // Move back
                    ASCII * back = strrchr( cps->buffer, '/' ) ;
                    back[1] = '\0' ;
                    // look for next file part
                    if ( cps->rest[2]=='\0' ) {
                        cps->pse = parse_status_last ;
                        cps->rest = NULL ;
                    } else if ( cps->rest[2]== '/' ) {
                        cps->pse = parse_status_next ;
                        cps->rest = &cps->rest[3] ;
                    } else {
                        cps->ret = -ENOENT ;
                        return ;
                    }
                } else {
                    cps->pse = parse_status_next ; // off the double dot trail
                }
                break ;
            case parse_status_next:
                LEVEL_DEBUG("FTP parse_status_next Path<%s> File <%s>\n",cps->buffer,cps->rest);
                /* cps->buffer is absolute */
                /* trailing / only at root */
                if ( cps->rest==NULL || cps->rest[0]=='\0' ) {
                    cps->pse = parse_status_last ;
                } else {
                    ASCII * oldrest = strsep( &cps->rest, "/" ) ;
                    if ( strpbrk( oldrest, "*[?" ) ) {
                        WildLexCD( cps, oldrest ) ;
                        return ;
                    } else {
                        if ( oldrest && ( strlen(cps->buffer) + strlen(oldrest) + 4 > PATH_MAX )) {
                            cps->ret = -ENAMETOOLONG ;
                            return ;
                        }
                        if ( cps->buffer[1] ) strcat( cps->buffer, "/" ) ;
                        strcat( cps->buffer, oldrest ) ;
                        cps->pse = parse_status_next ;
                    }
                }
                break ;
            case parse_status_tame:
                LEVEL_DEBUG("FTP parse_status_tame Path<%s> File <%s>\n",cps->buffer,cps->rest);
                /* cps->buffer is absolute */
                /* trailing / only at root */
                if ( cps->rest && ( strlen(cps->buffer) + strlen(cps->rest) + 4 > PATH_MAX )) {
                    cps->ret = -ENAMETOOLONG ;
                    return ;
                }
                if ( cps->buffer[1] )strcat( cps->buffer, "/" ) ;
                strcat( cps->buffer, cps->rest ) ;
                if ( FS_ParsedName( cps->buffer, &pn )==0 ) {
                    if ( IsDir(&pn) ) {
                        ++cps->solutions ;
                        if (cps->solutions == 1 ) cps->dir = strdup(pn.path) ;
                    } else {
                        cps->ret = -ENOTDIR ;
                    }
                    FS_ParsedName_destroy( &pn ) ;
                } else {
                    cps->ret = -ENOENT ;
                }
                return ;
            case parse_status_last:
                LEVEL_DEBUG("FTP parse_status_last Path<%s> File <%s>\n",cps->buffer,cps->rest);
                /* cps->buffer is absolute */
                /* trailing / only at root */
                if ( cps->rest && ( strlen(cps->buffer) + strlen(cps->rest) + 4 > PATH_MAX ) ) {
                    cps->ret = -ENAMETOOLONG ;
                    return ;
                }
                if ( FS_ParsedNamePlus( cps->buffer, cps->rest, &pn )==0 ) {
                    if ( IsDir(&pn) ) {
                        ++cps->solutions ;
                        if (cps->solutions == 1 ) cps->dir = strdup(pn.path) ;
                    } else {
                        cps->ret = -ENOTDIR ;
                    }
                    FS_ParsedName_destroy( &pn ) ;
                }
                return ;
        }
    }
}

#ifdef NO_NESTED_FUNCTIONS

#if OW_MT
pthread_mutex_t WildLexCDmutex = PTHREAD_MUTEX_INITIALIZER ;
#endif /* OW_MT */

ASCII * WildLexCDend = NULL;
ASCII * WildLexCDrest = NULL ;
struct cd_parse_s * WildLexCDcps = NULL;
ASCII * WildLexCDmatch = NULL;

void WildLexCDdirectory( const struct parsedname * const pn2 ) {
    FS_DirName( &WildLexCDend[1], OW_FULLNAME_MAX, pn2 ) ;
    //printf("Try %s vs %s\n",end,match) ;
    //if ( fnmatch( match, end, FNM_PATHNAME|FNM_CASEFOLD ) ) return ;
    if ( fnmatch( WildLexCDmatch, &WildLexCDend[1], FNM_PATHNAME ) ) return ;
    //printf("Match! %s\n",end) ;
    WildLexCDcps->pse = parse_status_next ;
    FileLexCD( WildLexCDcps ) ;
    WildLexCDcps->rest = WildLexCDrest ;
}

static void WildLexCD( struct cd_parse_s * cps, ASCII * match ) {
    int root = (cps->buffer[1]=='\0') ;
    struct parsedname pn ;

#if OW_MT
    pthread_mutex_lock(&WildLexCDmutex) ;
#endif /* OW_MT */

    WildLexCDend = &cps->buffer[strlen(cps->buffer)] ;
    WildLexCDcps = cps;
    WildLexCDmatch = match;

    LEVEL_DEBUG("FTP Wildcard patern matching: Path=%s, Pattern=%s, rest=%s\n",SAFESTRING(cps->buffer),SAFESTRING(match),SAFESTRING(cps->rest));
    /* Check potential length */
    if ( strlen(cps->buffer)+OW_FULLNAME_MAX+2 > PATH_MAX ) {
        cps->ret = -ENAMETOOLONG ;
        return ;
    }
    
    if ( cps->rest ) WildLexCDrest = strdup(cps->rest ) ;

    if ( FS_ParsedName( cps->buffer, &pn ) ) {
        cps->ret = -ENOENT ;
    } else {
        if ( pn.ft ) {
            cps->ret = -ENOTDIR ;
        } else {
            if ( root ) --WildLexCDend ;
            WildLexCDend[0] = '/' ;
            FS_dir( WildLexCDdirectory, &pn ) ;
            if ( root ) ++WildLexCDend ;
            WildLexCDend[0] = '\0' ; // restore cps->buffer
        }
        FS_ParsedName_destroy( &pn ) ;
    }
    if ( WildLexCDrest ) {
      free( WildLexCDrest ) ;
      WildLexCDrest = NULL;
    }

#if OW_MT
    pthread_mutex_unlock(&WildLexCDmutex) ;
#endif /* OW_MT */
}

#else /* NO_NESTED_FUNCTIONS */

static void WildLexCD( struct cd_parse_s * cps, ASCII * match ) {
    ASCII * end = &cps->buffer[strlen(cps->buffer)] ;
    ASCII * rest = NULL ;
    int root = (cps->buffer[1]=='\0') ;
    struct parsedname pn ;
    /* Embedded callback function */
    void directory( const struct parsedname * const pn2 ) {
        FS_DirName( &end[1], OW_FULLNAME_MAX, pn2 ) ;
        //printf("Try %s vs %s\n",end,match) ;
        //if ( fnmatch( match, end, FNM_PATHNAME|FNM_CASEFOLD ) ) return ;
        if ( fnmatch( match, &end[1], FNM_PATHNAME ) ) return ;
        //printf("Match! %s\n",end) ;
        cps->pse = parse_status_next ;
        FileLexCD( cps ) ;
        cps->rest = rest ;
    }

    LEVEL_DEBUG("FTP Wildcard patern matching: Path=%s, Pattern=%s, rest=%s\n",SAFESTRING(cps->buffer),SAFESTRING(match),SAFESTRING(cps->rest));
    /* Check potential length */
    if ( strlen(cps->buffer)+OW_FULLNAME_MAX+2 > PATH_MAX ) {
        cps->ret = -ENAMETOOLONG ;
        return ;
    }
    
    if ( cps->rest ) rest = strdup(cps->rest ) ;

    if ( FS_ParsedName( cps->buffer, &pn ) ) {
        cps->ret = -ENOENT ;
    } else {
        if ( pn.ft ) {
            cps->ret = -ENOTDIR ;
        } else {
            if ( root ) --end ;
            end[0] = '/' ;
            FS_dir( directory, &pn ) ;
            if ( root ) ++end ;
            end[0] = '\0' ; // restore cps->buffer
        }
        FS_ParsedName_destroy( &pn ) ;
    }
    if ( rest ) free( rest ) ;
}

#endif /* NO_NESTED_FUNCTIONS */
