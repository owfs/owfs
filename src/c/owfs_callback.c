   /*
    FUSE: Filesystem in Userspace
    OW: Module for reading 1-wire devices
*/

/*
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


/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */
int FS_getattr(const char *path, struct stat *stbuf) {
    struct parsedname pn ;
    /* Bad path */
//printf("GA\n");
	memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = scan_time ;
    if ( FS_ParsedName( path , &pn ) ) {
//printf("GA bad\n");
        return -ENOENT;
    } else if ( pn.dev==NULL ) { /* root directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
//printf("GA root\n");
    } else if ( pn.ft==NULL ) { /* other directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
//printf("GA other dir\n");
    } else { /* known 1-wire filetype */
        stbuf->st_mode = S_IFREG ;
        if ( pn.ft->read ) stbuf->st_mode |= 0444 ;
        if ( pn.ft->write ) stbuf->st_mode |= 0222 ;
        stbuf->st_nlink = 1;
		stbuf->st_size = FileLength( &pn ) ;
//printf("GA file\n");
    }
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

int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) {
    struct parsedname pn ;
//printf("GD path=%s\n",path) ;
    if ( FS_ParsedName(path,&pn) || pn.ft ) { /* bad path */ /* or filetype specified */
//printf("GD error\n") ;
        return -ENOENT;
	}
//printf("GD Good\n") ;
    if ( pn.dev ) { /* 1-wire device */
        int i ;
//printf("GD - directory\n") ;
//printf("GD - directory dev=%p, nodev= %p, nft=%d\n",pn.dev,&NoDevice,pn.dev->nft ) ;
		for ( i=0 ; i<pn.dev->nft ; ++i ) {
		    if ( ((pn.dev)->ft)[i].ag ) {
                int ext = ((pn.dev)->ft)[i].ag->elements ;
                int j ;
                char extname[PATH_MAX+1] ; /* probably excessive */
                for ( j=0 ; j < ext ; ++j ) {
				    if ( ((pn.dev)->ft)[i].ag->letters == ag_letters ) {
                        snprintf( extname , PATH_MAX, "%s.%c",((pn.dev)->ft)[i].name,j+'A') ;
                    } else {
                        snprintf( extname , PATH_MAX, "%s.%-d",((pn.dev)->ft)[i].name,j) ;
					}
//printf("GD %s\n",extname) ;
                    filler ( h , extname , 0 ) ;
                }
                snprintf( extname , PATH_MAX, "%s.ALL",((pn.dev)->ft)[i].name) ;
                filler ( h , extname , 0 ) ;
//printf("GD %s\n",extname) ;
            } else {
                filler( h , ((pn.dev)->ft)[i].name , 0 ) ;
            }
        }
    } else { /* root directory */
        char buf[16] ;
        char nam[16] ;
//printf("GD - root directory\n") ;
        time( &scan_time ) ;
//printf("GD - root directory dev=%p, nodev= %p, nft=%d\n",pn.dev,&NoDevice,pn.dev->nft ) ;
        /* Root directory, needs .,.. and scan of devices */
        filler(h, ".", 0);
        filler(h, "..", 0);
        switch (Version2480) {
        case 3:
            filler(h,"DS9097U",0) ;
            break ;
        case 7:
            filler(h,"LINK",0) ;
            break ;
        }
        BUS_lock() ;
        if ( OW_first(buf) ) return 0 ;
        FS_parse_dir(nam,buf) ;
        filler(h,nam,0) ;
        while ( ! OW_next(buf) ) {
            FS_parse_dir(nam,buf) ;
            filler(h,nam,0) ;
        } ;
        BUS_unlock() ;
    }
    return 0;
}

int FS_open(const char *path, int flags) {
    struct parsedname pn ;

    /* Unused */
    (void) flags ;

    if ( FS_ParsedName( path , &pn ) ) return -ENOENT ;
    return 0 ;
}

int FS_statfs(struct fuse_statfs *fst) {
    (void) fst ;
    return 0 ;
}

