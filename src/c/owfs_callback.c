/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions
    COM - serial port functions
    DS2480 -- DS9097 serial connector

    Written 2003 Paul H Alfille
*/

#include <sys/stat.h>
#include <string.h>

#include "owlib_config.h"
#include "ow.h"
#include "owfs.h"

time_t scan_time ;

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */
int FS_getattr(const char *path, struct stat *stbuf) {
    struct parsedname pn ;
    /* Bad path */
printf("GA\n");
	memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = scan_time ;
    if ( FS_ParsedName( path , &pn ) ) {
//printf("GA bad\n");
        FS_ParsedName_destroy(&pn) ;
        return -ENOENT;
    } else if ( pn.dev==NULL ) { /* root directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
//printf("GA root\n");
    } else if ( pn.ft==NULL || pn.ft->format==ft_directory ) { /* other directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
//printf("GA other dir\n");
    } else { /* known 1-wire filetype */
        stbuf->st_mode = S_IFREG ;
        if ( pn.ft->read.v ) stbuf->st_mode |= 0444 ;
        if ( pn.ft->write.v ) stbuf->st_mode |= 0222 ;
        stbuf->st_nlink = 1;
		stbuf->st_size = FileLength( &pn ) ;
//printf("GA file\n");
    }
    FS_ParsedName_destroy(&pn) ;
    return 0 ;
}

int FS_utime(const char *path, struct utimbuf *buf) {
    /* Unused */
    (void) path ;
    (void) buf ;

    return 0;
}

int FS_truncate(const char *path, const off_t size) {

    /* Unused */
    (void) path ;
    (void) size ;

    return 0 ;
}

struct dirback {
    fuse_dirh_t h ;
    fuse_dirfil_t filler ;
} ;

/* Callback function to FS_dir */
void directory( void * data, const struct parsedname * const pn ) {
    char extname[PATH_MAX+1] ; /* probably excessive */
    if ( pn->ft ) {
	    if ( pn->ft->ag == NULL ) {
            snprintf( extname , PATH_MAX, "%s",pn->ft->name) ;
        } else if ( pn->extension == -1 ) {
            snprintf( extname , PATH_MAX, "%s.ALL",pn->ft->name) ;
        } else if ( pn->ft->ag->letters == ag_letters ) {
            snprintf( extname , PATH_MAX, "%s.%c",pn->ft->name,pn->extension+'A') ;
        } else {
            snprintf( extname , PATH_MAX, "%s.%-d",pn->ft->name,pn->extension) ;
        }
    } else if ( pn->dev->type == dev_1wire ) {
        snprintf( extname , PATH_MAX, "%02X.%02X%02X%02X%02X%02X%02X",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6]) ;
    } else {
        snprintf( extname , PATH_MAX, "%s",pn->dev->code) ;
    }
    (((struct dirback *)data)->filler)( ((struct dirback *)data)->h, extname, DT_DIR ) ;
}

int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) {
    struct parsedname pn ;
printf("GETDIR\n");

    /* dirback structure passed via void pointer to 'directory' */
    struct dirback db;
    db.h = h ;
    db.filler = filler ;

    if ( FS_ParsedName(path,&pn) || pn.ft ) { /* bad path */ /* or filetype specified */
        FS_ParsedName_destroy(&pn) ;
        return -ENOENT;
	}

    /* 'uncached' directory added if root and not already uncached */
    if ( cacheavailable && pn.type!=pn_uncached && pn.dev==NULL && pn.pathlength==0 ) {
        filler(h,"uncached",DT_DIR) ;
    }

    /* Call directory spanning function */
    FS_dir( directory, &db, &pn ) ;

    /* Clean up */
    FS_ParsedName_destroy(&pn) ;
    return 0 ;
}

int FS_open(const char *path, int flags) {
    struct parsedname pn ;
    int ret =0;

    /* Unused */
    (void) flags ;
printf("OPEN\n");
/*
    ret = FS_ParsedName( path , &pn ) ;
    FS_ParsedName_destroy(&pn) ;
*/
    return ret ;
}

int FS_statfs(struct fuse_statfs *fst) {
    (void) fst ;
    return 0 ;
}

