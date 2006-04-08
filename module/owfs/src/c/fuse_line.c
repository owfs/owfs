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

#include "owfs_config.h"
#include "ow.h"
#include "owfs.h"
#include <fuse.h>

/* Fuse_main is called with a standard C-style argc / argv parameters */

/* These routines build the char **argv */

int Fuse_setup( struct Fuse_option * fo ) {
    int i ;
    fo->max_options = 10 ;
    fo->argc = 0 ;
    fo->argv = (char**) calloc( fo->max_options+1, sizeof( char * ) ) ;
    if ( fo->argv == NULL ) return -ENOMEM ;
    for ( i=0 ; i<=fo->max_options ; ++i ) fo->argv[i] = NULL ;
    /* create "0th" item -- the program name */
    return Fuse_add( "OWFS", fo ) ;
}

void Fuse_cleanup( struct Fuse_option * fo ) {
    int i ;
    if ( fo->argv ) {
        for ( i=0 ; i<fo->max_options ; ++i ) if ( fo->argv[i] ) free(fo->argv[i]) ;
        free( fo->argv ) ;
    }
}

int Fuse_add( char * opt, struct Fuse_option * fo ) {
    //printf("Adding option %s\n",opt);
    if ( fo->argc >= fo->max_options ) { // need to allocate more space
        int i = fo->max_options ;
        fo->max_options += 10 ;
        fo->argv = (char**) realloc( fo->argv, (fo->max_options+1)*sizeof( char * ) ) ;
        if ( fo->argv == NULL ) return -ENOMEM ; // allocated ok?
        for ( ; i<=fo->max_options ; ++i ) fo->argv[i] = NULL ; // now clear the new pointers
    }
    fo->argv[fo->argc++] = strdup(opt) ;
    //printf("Added option %d %s\n",fo->argc-1,fo->argv[fo->argc-1]);
    return 0 ;
}
