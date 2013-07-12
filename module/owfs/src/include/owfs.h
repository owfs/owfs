/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By Paul H Alfille
    {c} 2003 GPL
    paul.alfille@gmail.com
*/

/* OWFS - specific header */

#ifndef OWFS_H
#define OWFS_H

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* Include FUSE -- http://fuse.sf.net */
/* Lot's of version-specific code */

//#define FUSE_USE_VERSION 26
// FUSE_USE_VERSION is set from configure script
#include <fuse.h>
#ifndef FUSE_VERSION
#ifndef FUSE_MAJOR_VERSION
#define FUSE_VERSION 11
#else							/* FUSE_MAJOR_VERSION */
#undef FUSE_MAKE_VERSION
#define FUSE_MAKE_VERSION(maj,min)  ((maj) * 10 + (min))
#define FUSE_VERSION FUSE_MAKE_VERSION(FUSE_MAJOR_VERSION,FUSE_MINOR_VERSION)
#endif							/* FUSE_MAJOR_VERSION */
#endif							/* FUSE_VERSION */

extern struct fuse_operations owfs_oper;

struct Fuse_option {
	int max_options;
	char **argv;
	int argc;
};

int Fuse_setup(struct Fuse_option *fo);
void Fuse_cleanup(struct Fuse_option *fo);
int Fuse_parse(char *opts, struct Fuse_option *fo);
int Fuse_add(char *opt, struct Fuse_option *fo);
char *Fuse_arg(char *opt_arg, char *entryname);

#endif							/* OWFS_H */
