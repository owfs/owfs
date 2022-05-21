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

#include "owshell.h"

/* ---------------------------------------------- */
/* Command line parsing and result generation     */
/* ---------------------------------------------- */
int main(int argc, char *argv[])
{
	int fd ;
	
	Setup();

	/* process command line arguments */
	while (1) {
		int c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL);
		if (c == -1) {
			break;
		}
		owopt(c, optarg);
	}

	DefaultOwserver();
	Server_detect(); // Exit(1) if problem.
	
	fd = ClientConnect() ;
	if ( fd == -1 ) {
		PRINT_ERROR("Cannot connect\n");
		Exit(1);
	}
	
	// Success!
	close(fd);
	Exit(0) ;
	return 0 ;
}
