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
int main(int argc, char *argv[])
{
	int c;
	int paths_found = 0;
	int rc = -1;

	Setup();
	/* process command line arguments */
	while ((c =
			getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1)
		owopt(c, optarg);

	/* non-option arguments */
	while (optind < argc) {
		if (head_inbound_list == NULL) {
			OW_ArgNet(argv[optind]);
		} else {
			if (paths_found++ == 0)
				Server_detect();
			rc = ServerPresence(argv[optind]);
		}
		++optind;
	}
	Cleanup();
	exit((rc >= 0 ? 0 : 1));
}
