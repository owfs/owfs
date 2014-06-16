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

#include "owfs.h"
#include "ow_connection.h"

/*
    OW -- One Wire
    Globals variables -- each invokation will have it's own data
*/
struct fuse *fuse;
int fuse_fd = -1;
char umount_cmd[1024] = "";
char *fuse_mnt_opt = NULL;
char *fuse_open_opt = NULL;

/* ---------------------------------------------- */
/* Command line parsing and mount handler to FUSE */
/* ---------------------------------------------- */
int main(int argc, char *argv[])
{
	int c;
	struct Fuse_option fuse_options;

	/* Set up owlib */
	LibSetup(program_type_filesystem);

	/* grab our executable name */
	if (argc > 0) {
		Globals.progname = owstrdup(argv[0]);
	}
	//mtrace() ;
	/* process command line arguments */
	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
		case e_fuse_opt:		/* fuse_mnt_opt */
			if (fuse_mnt_opt) {
				owfree(fuse_mnt_opt);
			}
			fuse_mnt_opt = Fuse_arg(optarg, "FUSE mount options");
			if (fuse_mnt_opt == NULL) {
				ow_exit(0);
			}
			break;
		case e_fuse_open_opt:	/* fuse_open_opt */
			if (fuse_open_opt) {
				owfree(fuse_open_opt);
			}
			fuse_open_opt = Fuse_arg(optarg, "FUSE open options");
			if (fuse_open_opt == NULL) {
				ow_exit(0);
			}
			break;
		}
		if ( BAD( owopt(c, optarg) ) ) {
			ow_exit(0);			/* rest of message */
		}
	}

	/* non-option arguments */
	while (optind < argc - 1) {
		ARG_Generic(argv[optind]);
		++optind;
	}
	while (optind < argc) {
		if (Outbound_Control.active) {
			ARG_Generic(argv[optind]);
		} else {
			ARG_Server(argv[optind]);
		}
		++optind;
	}

	if (Outbound_Control.active == 0) {
		LEVEL_DEFAULT("No mount point specified.\nTry '%s -h' for help.", argv[0]);
		ow_exit(1);
	}
	// FUSE directory mounting
	LEVEL_CONNECT("fuse mount point: %s", Outbound_Control.head->name);

	// Signal handler is set in fuse library
	//set_signal_handlers(exit_handler);

	/* Set up adapters */
	if ( BAD(LibStart()) ) {
		ow_exit(1);
	}

#if FUSE_VERSION >= 14
	/* Set up "command line" for main fuse routines */
	Fuse_setup(&fuse_options);	// command line setup
	Fuse_add(Outbound_Control.head->name, &fuse_options);	// mount point
#if FUSE_VERSION >= 22
	Fuse_add("-o", &fuse_options);	// add "-o direct_io" to prevent buffering
	Fuse_add("direct_io", &fuse_options);
#endif							/* FUSE_VERSION >= 22 */
	switch (Globals.daemon_status) {
		case e_daemon_want_bg:
		case e_daemon_bg:
			Fuse_add("-f", &fuse_options);	// foreground for fuse too
			if (Globals.error_level > 2) {
				Fuse_add("-d", &fuse_options);	// debug for fuse too
			}
			Globals.daemon_status = e_daemon_bg;
			break ;
		default:
			break ;
	}		

	Fuse_parse(fuse_mnt_opt, &fuse_options);
	LEVEL_DEBUG("fuse_mnt_opt=[%s]", fuse_mnt_opt);
	Fuse_parse(fuse_open_opt, &fuse_options);
	LEVEL_DEBUG("fuse_open_opt=[%s]", fuse_open_opt);
	if (Globals.allow_other) {
		Fuse_add("-o", &fuse_options);	// add "-o allow_other" to permit other users access
		Fuse_add("allow_other", &fuse_options);
	}

#if FUSE_VERSION > 25
	fuse_main(fuse_options.argc, fuse_options.argv, &owfs_oper, NULL);
#else							/* FUSE_VERSION <= 25 */
	fuse_main(fuse_options.argc, fuse_options.argv, &owfs_oper);
#endif							/* FUSE_VERSION <= 25 */
	Fuse_cleanup(&fuse_options);
#endif

	ow_exit(0);
	return 0;
}
