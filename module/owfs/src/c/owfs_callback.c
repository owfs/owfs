/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interface
    LI -- LINK commands
    FS -- filesystem commands
    UT -- utility functions
    COM - serial port functions
    DS2480 -- DS9097U serial connector

    Written 2003 Paul H Alfille
*/

#include <sys/stat.h>
#include <string.h>

#include "owfs_config.h"
#include "ow.h"
#include "owfs.h"

time_t scan_time ;

struct fuse_operations owfs_oper = {
    getattr:    FS_getattr,
    readlink:    NULL,
    getdir:     FS_getdir,
    mknod:    NULL,
    mkdir:    NULL,
    symlink:    NULL,
    unlink:    NULL,
    rmdir:    NULL,
    rename:     NULL,
    link:    NULL,
    chmod:    NULL,
    chown:    NULL,
    truncate:    FS_truncate,
    utime:    FS_utime,
    open:    FS_open,
    read:    FS_read,
    write:    FS_write,
    statfs:    FS_statfs,
    release:    NULL
};

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */
int FS_getattr(const char *path, struct stat *stbuf) {
    struct parsedname pn ;
    struct stateinfo si ;
    pn.si = &si ;
    /* Bad path */
//printf("GA\n");
    memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = scan_time ;
    if ( FS_ParsedName( path , &pn ) ) {
//printf("GA bad\n");
        FS_ParsedName_destroy(&pn) ;
        return -ENOENT;
    } else if ( pn.dev==NULL ) { /* root directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
//printf("GA root\n");
    } else if ( pn.ft==NULL || pn.ft->format==ft_directory || pn.ft->format==ft_subdir ) { /* other directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 3;
        stbuf->st_size = 1 ; /* Arbitrary non-zero for "find" and "tree" */
//printf("GA other dir\n");
    } else { /* known 1-wire filetype */
        stbuf->st_mode = S_IFREG ;
        if ( pn.ft->read.v ) stbuf->st_mode |= 0444 ;
        if ( !readonly && pn.ft->write.v ) stbuf->st_mode |= 0222 ;
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
//printf("directory callback\n");
    if ( pn->ft ) {
        char * pname = strchr(pn->ft->name,'/') ;
        if ( pname ) {
            ++ pname ;
        } else {
            pname = pn->ft->name ;
        }

        if ( pn->ft->ag == NULL ) {
            snprintf( extname , PATH_MAX, "%s",pname) ;
        } else if ( pn->extension == -1 ) {
            snprintf( extname , PATH_MAX, "%s.ALL",pname) ;
        } else if ( pn->ft->ag->letters == ag_letters ) {
            snprintf( extname , PATH_MAX, "%s.%c",pname,pn->extension+'A') ;
        } else {
            snprintf( extname , PATH_MAX, "%s.%-d",pname,pn->extension) ;
        }
    } else if ( pn->subdir ) { /* in-device subdirectory */
        snprintf( extname , PATH_MAX, "%s",pn->subdir->name) ;
    } else if ( pn->dev->type == dev_1wire ) {
        FS_devicename( extname, PATH_MAX, pn->sn ) ;
    } else {
        snprintf( extname , PATH_MAX, "%s",pn->dev->code) ;
    }
    (((struct dirback *)data)->filler)( ((struct dirback *)data)->h, extname, DT_DIR ) ;
}

int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) {
    struct parsedname pn ;
    struct stateinfo si ;
    /* dirback structure passed via void pointer to 'directory' */
    struct dirback db;
    pn.si = &si ;
//printf("GETDIR\n");

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

/* In theory, should handle file opening, but OWFS doesn't care. Device opened/closed with every read/write */
int FS_open(const char *path, int flags) {
    /* Unused */
    (void) flags ;
    (void) path ;
//printf("OPEN\n");
    return 0 ;
}

/* Change in statfs definition for newer FUSE versions */
#if !defined(FUSE_MAJOR_VERSION) || !(FUSE_MAJOR_VERSION > 1)
int FS_statfs(struct fuse_statfs *fst) {
    memset( fst, 0, sizeof(struct fuse_statfs) ) ;
    return 0 ;
}
#endif /* FUSE_MAJOR_VERSION */
