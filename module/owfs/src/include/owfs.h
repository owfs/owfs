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

extern struct fuse_operations owfs_oper;

#endif
