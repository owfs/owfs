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

#include "owfs_config.h"
#include "ow.h"
#include "owfs.h"

/* There was a major change in the function prototypes at FUSE 2.2, we'll make a flag */
#undef FUSE22PLUS
#undef FUSE1X

#ifdef FUSE_MAJOR_VERSION
    #if FUSE_MAJOR_VERSION > 2
        #define FUSE22PLUS
    #elif FUSE_MAJOR_VERSION < 2
        #define FUSE1X
    #elif FUSE_MINOR_VERSION > 1
        #define FUSE22PLUS
    #endif /* FUSE > 2.1 */
#else /* no FUSE_MAJOR_VERSION */
    #define FUSE1X
#endif /* FUSE_MAJOR_VERSION */

#ifdef FUSE22PLUS 
    #define FUSEFLAG struct fuse_file_info *
#else /* FUSE < 2.2 */
    #define FUSEFLAG int
#endif /* FUSE_MAJOR_VERSION */

static int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) ;
static int FS_utime(const char *path, struct utimbuf *buf) ;
static int FS_truncate(const char *path, const off_t size) ;
static int FS_chmod(const char *path, mode_t mode) ;
static int FS_chown(const char *path, uid_t uid, gid_t gid ) ;
static int FS_open(const char *path, FUSEFLAG flags) ;
static int FS_release(const char *path, FUSEFLAG flags) ;
#ifdef FUSE22PLUS
    static int CB_read( const char * path, char * buffer, size_t size, off_t offset, struct fuse_file_info * flags ) ;
    static int CB_write( const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * flags ) ;
#else /* fuse < 2.2 */
    #define CB_read FS_read
    #define CB_write FS_write
#endif

    /* Change in statfs definition for newer FUSE versions */
#ifdef FUSE1X
    static int FS_statfs(struct fuse_statfs *fst) ;
#else /* FUSE_MAJOR_VERSION */
    #define FS_statfs   NULL
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
    read:     CB_read,
    write:    CB_write,
    statfs:   FS_statfs,
    release:  FS_release,
};

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */
/* Needed for "SETATTR" */
static int FS_utime(const char *path, struct utimbuf *buf) {
    LEVEL_CALL("UTIME path=%s\n", NULLSTRING(path));
    (void) buf ; return 0;
}

/* Needed for "SETATTR" */
static int FS_chmod(const char *path, mode_t mode) {
    LEVEL_CALL("CHMODE path=%s\n", NULLSTRING(path));
    (void) mode ; return 0;
}

/* Needed for "SETATTR" */
static int FS_chown(const char *path, uid_t uid, gid_t gid ) {
    LEVEL_CALL("CHOWN path=%s\n", NULLSTRING(path));
    (void) uid ; (void) gid ; return 0;
}

/* In theory, should handle file opening, but OWFS doesn't care. Device opened/closed with every read/write */
static int FS_open(const char *path, FUSEFLAG flags) {
    LEVEL_CALL("OPEN path=%s\n", NULLSTRING(path));
    (void) flags ; return 0 ;
}

/* In theory, should handle file closing, but OWFS doesn't care. Device opened/closed with every read/write */
static int FS_release(const char *path, FUSEFLAG flags) {
    LEVEL_CALL("RELEASE path=%s\n", NULLSTRING(path));
    (void) flags ; return 0 ;
}

/* dummy truncation (empty) function */
static int FS_truncate(const char *path, const off_t size) {
    LEVEL_CALL("TRUNCATE path=%s\n", NULLSTRING(path));
    (void) size ; return 0 ;
}

#ifdef FUSE22PLUS
    #define FILLER(handle,name) filler(handle,name,DT_DIR,(ino_t)0)
#else /* FUSE22PLUS */
    #define FILLER(handle,name) filler(handle,name,DT_DIR)
#endif /* FUSE22PLUS */
static int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) {
    struct parsedname pn ;
    struct stateinfo si ;
    int ret ;
    /* Embedded function */
    /* Callback function to FS_dir */
    /* Prints this directory element (not the whole path) */
    void directory( const struct parsedname * const pn2 ) {
        char extname[OW_FULLNAME_MAX+1] ;
        FS_DirName( extname, OW_FULLNAME_MAX+1, pn2 ) ;
        FILLER(h,extname) ;
    }

    LEVEL_CALL("GETDIR path=%s\n", NULLSTRING(path));

    pn.si = &si ;
    if ( (ret=FS_ParsedName( path, &pn )) ) return ret ;
    //printf("FS_getdir parse=%d\n",ret);
    // first root always return Bus-list and settings/system/statistics
    pn.si->sg |= (1<<BUSRET_BIT) ;

    if ( pn.ft ) {
        ret = -ENOENT;
    } else { /* Good pn */
        /* Call directory spanning function */
        FS_dir( directory, &pn ) ;
        FILLER(h,".") ;
        FILLER(h,"..") ;
    }

    /* Clean up */
    FS_ParsedName_destroy(&pn) ;
    return ret ;
}

#ifdef FUSE22PLUS
static int CB_read( const char * path, char * buffer, size_t size, off_t offset, struct fuse_file_info * flags ) {
    (void) flags ;
    return FS_read( path, buffer, size, offset ) ;
}
static int CB_write( const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * flags ) {
    (void) flags ;
    return FS_write( path, buffer, size, offset ) ;
}
#endif /* FUSE22PLUS */
            
/* Change in statfs definition for newer FUSE versions */
#ifdef FUSE1X
int FS_statfs(struct fuse_statfs *fst) {
    LEVEL_CALL("STATFS\n");
    memset( fst, 0, sizeof(struct fuse_statfs) ) ;
    return 0 ;
}
#endif /* FUSE1X */

