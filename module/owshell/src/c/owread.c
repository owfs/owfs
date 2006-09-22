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

#include "owshell.h"

/* ---------------------------------------------- */
/* Command line parsing and result generation     */
/* ---------------------------------------------- */
int main(int argc, char *argv[]) {
    int c ;
    int detect_needed = 1 ;

    Setup() ;
    /* process command line arguments */
    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        if ( owopt(c,optarg) ) exit(1) ; /* rest of message */
    }

    /* non-option arguments */
    while ( optind < argc ) {
        if ( indevice==NULL ) {
            if ( OW_ArgNet(argv[optind]) ) exit (1) ;
        } else {
            if ( detect_needed ) {
                if ( Server_detect() ) {
                    fprintf(stderr,"Could not connect with owserver %s\n",indevice->name) ;
                    exit(1) ;
                }
                detect_needed = 0 ;
            }
            ServerRead(argv[optind]) ;
        }
        ++optind ;
    }
    exit(0) ;
}
