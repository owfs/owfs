/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <sys/stat.h>
#include <string.h>

#include "owfs_config.h"
#include "ow.h"

int FS_fstat(const char *path, struct stat *stbuf) {
    struct parsedname pn ;
    struct stateinfo si ;
    int ret = 0 ;
    pn.si = &si ;
    /* Bad path */
//printf("GA\n");
    memset(stbuf, 0, sizeof(struct stat));
    if ( FS_ParsedName( path , &pn ) ) {
//printf("GA bad\n");
        ret = -ENOENT;
    } else if ( pn.dev==NULL ) { /* root directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
        stbuf->st_size = 1 ; /* Arbitrary non-zero for "find" and "tree" */
        stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = start_time ;
//printf("GA root\n");
    } else if ( pn.ft==NULL || pn.ft->format==ft_directory || pn.ft->format==ft_subdir ) { /* other directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
        stbuf->st_size = 1 ; /* Arbitrary non-zero for "find" and "tree" */
        FSTATLOCK
            stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = dir_time ;
        FSTATUNLOCK
//printf("GA other dir\n");
    } else { /* known 1-wire filetype */
        stbuf->st_mode = S_IFREG ;
        if ( pn.ft->read.v ) stbuf->st_mode |= 0444 ;
        if ( !readonly && pn.ft->write.v ) stbuf->st_mode |= 0222 ;
        stbuf->st_nlink = 1;
        stbuf->st_size = FullFileLength( &pn ) ;
        switch ( pn.ft->change ) {
        case ft_volatile:
        case ft_Avolatile:
        case ft_second:
        case ft_statistic:
            stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = time(NULL) ;
            break ;
        case ft_stable:
        case ft_Astable:
            FSTATLOCK
                stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = dir_time ;
            FSTATUNLOCK
            break ;
        default:
            stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = start_time ;
        }
//printf("GA file\n");
    }
    FS_ParsedName_destroy(&pn) ;
    return ret ;
}

