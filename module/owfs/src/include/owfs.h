/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By PAul H Alfille
    {c} 2003 GPL
    palfille@earthlink.net
*/

/* OWFS - specific header */

#ifndef OWFS_H
#define OWFS_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif

#include <fuse.h>
#include <stdlib.h>

//#include "ow_cache.h"

int FS_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) ;

int FS_getattr(const char *path, struct stat *stbuf) ;
int FS_utime(const char *path, struct utimbuf *buf) ;
int FS_truncate(const char *path, const off_t size) ;
int FS_open(const char *path, int flags) ;
/* Change in statfs definition for newer FUSE versions */
#if defined(FUSE_MAJOR_VERSION) && FUSE_MAJOR_VERSION > 1
    #define FS_statfs   NULL
#else /* FUSE_MAJOR_VERSION */
    int FS_statfs(struct fuse_statfs *fst) ;
#endif /* FUSE_MAJOR_VERSION */

extern struct fuse_operations owfs_oper;

extern time_t scan_time ; /* time of last directory scan */

#endif
