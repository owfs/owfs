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

#if OW_MT
pthread_t main_threadid;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else
#define IS_MAINTHREAD 1
#endif

static void ow_exit(int e)
{
	StateInfo.shutdown_in_progress = 1;
	LEVEL_DEBUG("owfs: ow_exit(%d)\n", e);
	if (IS_MAINTHREAD) {
		LibClose();
	}
	//LEVEL_DEBUG("owfs: call _exit(%d)\n", e);
#ifdef __UCLIBC__
	/* Process never die on WRT54G router with uClibc if exit() is used */
	_exit(e);
#else
	exit(e);
#endif
}

/*
    OW -- One Wire
    Global variables -- each invokation will have it's own data
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
	int allow_other = 0;
	struct Fuse_option fuse_options;

	/* Set up owlib */
	LibSetup(opt_owfs);

	/* grab our executable name */
    if (argc > 0) {
		Global.progname = strdup(argv[0]);
    }
	//mtrace() ;
	/* process command line arguments */
	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
        case e_fuse_opt:				/* fuse_mnt_opt */
            if (fuse_mnt_opt) {
                free(fuse_mnt_opt);
            }
            fuse_mnt_opt = Fuse_arg(optarg, "FUSE mount options");
            if (fuse_mnt_opt == NULL) {
                ow_exit(0);
            }
            break;
        case e_fuse_open_opt:				/* fuse_open_opt */
            if (fuse_open_opt) {
                free(fuse_open_opt);
            }
            fuse_open_opt = Fuse_arg(optarg, "FUSE open options");
            if (fuse_open_opt == NULL) {
                ow_exit(0);
            }
            break;
        case e_allow_other:				/* allow_other */
            allow_other = 1;
            break;
		}
        if (owopt(c, optarg)) {
			ow_exit(0);			/* rest of message */
        }
	}

	/* non-option arguments */
	while (optind < argc - 1) {
		OW_ArgGeneric(argv[optind]);
		++optind;
	}
	while (optind < argc) {
		if (count_outbound_connections) {
			OW_ArgGeneric(argv[optind]);
		} else {
			OW_ArgServer(argv[optind]);
		}
		++optind;
	}

	if (count_outbound_connections == 0) {
		LEVEL_DEFAULT("No mount point specified.\nTry '%s -h' for help.\n", argv[0]);
		ow_exit(1);
	}
	// FUSE directory mounting
	LEVEL_CONNECT("fuse mount point: %s\n", head_outbound_list->name);

	// Signal handler is set in fuse library
	//set_signal_handlers(exit_handler);

	/* Set up adapters */
    if (LibStart()) {
		ow_exit(1);
    }

#if OW_MT
	main_threadid = pthread_self();
#endif

#if FUSE_VERSION >= 14
	/* Set up "command line" for main fuse routines */
	Fuse_setup(&fuse_options);	// command line setup
	Fuse_add(head_outbound_list->name, &fuse_options);	// mount point
#if FUSE_VERSION >= 22
	Fuse_add("-o", &fuse_options);	// add "-o direct_io" to prevent buffering
	Fuse_add("direct_io", &fuse_options);
#endif							/* FUSE_VERSION >= 22 */
	if (!Global.want_background) {
		Fuse_add("-f", &fuse_options);	// foreground for fuse too
        if (Global.error_level > 2) {
			Fuse_add("-d", &fuse_options);	// debug for fuse too
        }
	}
	Fuse_parse(fuse_mnt_opt, &fuse_options);
	LEVEL_DEBUG("fuse_mnt_opt=[%s]\n", fuse_mnt_opt);
	Fuse_parse(fuse_open_opt, &fuse_options);
	LEVEL_DEBUG("fuse_open_opt=[%s]\n", fuse_open_opt);
	if (allow_other) {
		Fuse_add("-o", &fuse_options);	// add "-o allow_other" to permit other users access
		Fuse_add("allow_other", &fuse_options);
	}
#if OW_MT == 0
	Fuse_add("-s", &fuse_options);	// single threaded
#endif							/* OW_MT */
	Global.now_background = Global.want_background;	// tell "error" that we are background
#if 0
	{
		int i;
		for (i = 0; i < fuse_options.argc; i++) {
			LEVEL_DEBUG("fuse_options.argv[%d]=[%s]\n", i, fuse_options.argv[i]);
		}
	}
#endif
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
