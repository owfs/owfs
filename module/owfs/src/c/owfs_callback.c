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

static int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) ;
static int FS_utime(const char *path, struct utimbuf *buf) ;
static int FS_truncate(const char *path, const off_t size) ;
static int FS_open(const char *path, int flags) ;
static int FS_release(const char *path, int flags) ;
static int FS_chmod(const char *path, mode_t mode) ;
static int FS_chown(const char *path, uid_t uid, gid_t gid ) ;
/* Change in statfs definition for newer FUSE versions */
#if defined(FUSE_MAJOR_VERSION) && FUSE_MAJOR_VERSION > 1
    #define FS_statfs   NULL
#else /* FUSE_MAJOR_VERSION */
    static int FS_statfs(struct fuse_statfs *fst) ;
#endif /* FUSE_MAJOR_VERSION */

struct fuse_operations owfs_oper = {
    getattr:  FS_fstat,
    readlink:  NULL,
    getdir:   FS_getdir,
    mknod:     NULL,
    mkdir:     NULL,
    symlink:   NULL,
    unlink:    NULL,
    rmdir:     NULL,
    rename:    NULL,
    link:      NULL,
    chmod:    FS_chmod,
    chown:    FS_chown,
    truncate: FS_truncate,
    utime:    FS_utime,
    open:     FS_open,
    read:     FS_read,
    write:    FS_write,
    statfs:   FS_statfs,
    release:  FS_release,
};

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */
/* Needed for "SETATTR" */
static int FS_utime(const char *path, struct utimbuf *buf) {
    (void) path ; (void) buf ; return 0;
}

/* Needed for "SETATTR" */
static int FS_chmod(const char *path, mode_t mode) {
    (void) path ; (void) mode ; return 0;
}

/* Needed for "SETATTR" */
static int FS_chown(const char *path, uid_t uid, gid_t gid ) {
    (void) path ; (void) uid ; (void) gid ; return 0;
}

/* In theory, should handle file opening, but OWFS doesn't care. Device opened/closed with every read/write */
static int FS_open(const char *path, int flags) {
    (void) flags ; (void) path ; return 0 ;
}

/* In theory, should handle file closing, but OWFS doesn't care. Device opened/closed with every read/write */
static int FS_release(const char *path, int flags) {
    (void) flags ; (void) path ; return 0 ;
}

/* dummy truncation (empty) function */
static int FS_truncate(const char *path, const off_t size) {
    (void) path ; (void) size ; return 0 ;
}

static int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) {
    struct parsedname pn ;
    struct stateinfo si ;
    int ret ;
    /* Callback function to FS_dir */
    /* Prints this directory element (not the whole path) */
    void directory( const struct parsedname * const pn2 ) {
        char extname[OW_FULLNAME_MAX+1] ; /* buffer for name */
        FS_DirName( extname, OW_FULLNAME_MAX+1, pn2 ) ;
//printf("DIR %s\n",extname);
#ifdef FUSE_MAJOR_VERSION

    #if ((FUSE_MAJOR_VERSION == 2) && (FUSE_MINOR_VERSION >= 2)) || \
     (FUSE_MAJOR_VERSION >= 3)
        /* Newer fuse versions (2.2 and later) have an inode argument */
        filler( h, extname, DT_DIR, 0 ) ;
    #else
        /* Probably fuse-version 1.0 to 2.1 */
        filler( h, extname, DT_DIR ) ;
    #endif

#else /* FUSE_MAJOR_VERSION */
        /* Probably really old fuse-version */
        filler( h, extname, DT_DIR ) ;
#endif /* FUSE_MAJOR_VERSION */
    }
    
    pn.si = &si ;
//printf("GETDIR\n");

    if ( FS_ParsedName(path,&pn) || pn.ft ) { /* bad path */ /* or filetype specified */
        ret = -ENOENT;
    } else { /* Good pn */
        /* Call directory spanning function */
        FS_dir( directory, &pn ) ;
        ret = 0 ;
    }

    /* Clean up */
    FS_ParsedName_destroy(&pn) ;
    return ret ;
}

/* Change in statfs definition for newer FUSE versions */
#if !defined(FUSE_MAJOR_VERSION) || !(FUSE_MAJOR_VERSION > 1)
int FS_statfs(struct fuse_statfs *fst) {
    memset( fst, 0, sizeof(struct fuse_statfs) ) ;
    return 0 ;
}
#endif /* FUSE_MAJOR_VERSION */

