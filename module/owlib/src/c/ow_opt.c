/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_opt -- owlib specific command line options processing */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_pid.h"

struct lineparse {
	ASCII line[256];
	ASCII *opt;
	ASCII *val;
	int c;
};

static void ParseTheLine(struct lineparse *lp);
static int ConfigurationFile(const ASCII * file);
static int ParseInterp(struct lineparse *lp);
static int OW_parsevalue(long long int *var, const ASCII * str);

static int OW_ArgSerial(const char *arg);
static int OW_ArgParallel(const char *arg);
static int OW_ArgI2C(const char *arg);
static int OW_ArgHA7(const char *arg);
static int OW_ArgEtherWeather(const char *arg);
static int OW_ArgFake(const char *arg);
static int OW_ArgTester(const char *arg);
static int OW_ArgLink(const char *arg);
static int OW_ArgPassive(char *adapter_type_name, const char *arg);

const struct option owopts_long[] = {
	{"configuration", required_argument, NULL, 'c'},
	{"device", required_argument, NULL, 'd'},
	{"usb", optional_argument, NULL, 'u'},
	{"USB", optional_argument, NULL, 'u'},
	{"help", optional_argument, NULL, 'h'},
	{"port", required_argument, NULL, 'p'},
	{"mountpoint", required_argument, NULL, 'm'},
	{"server", required_argument, NULL, 's'},
	{"readonly", no_argument, NULL, 'r'},
	{"write", no_argument, NULL, 'w'},
	{"Celsius", no_argument, NULL, 'C'},
	{"Fahrenheit", no_argument, NULL, 'F'},
	{"Kelvin", no_argument, NULL, 'K'},
	{"Rankine", no_argument, NULL, 'R'},
	{"version", no_argument, NULL, 'V'},
	{"format", required_argument, NULL, 'f'},
	{"pid_file", required_argument, NULL, 'P'},
	{"pid-file", required_argument, NULL, 'P'},
	{"background", no_argument, &Global.want_background, 1},
	{"foreground", no_argument, &Global.want_background, 0},
	{"error_print", required_argument, NULL, 257},
	{"error-print", required_argument, NULL, 257},
	{"errorprint", required_argument, NULL, 257},
	{"error_level", required_argument, NULL, 258},
	{"error-level", required_argument, NULL, 258},
	{"errorlevel", required_argument, NULL, 258},
	{"cache_size", required_argument, NULL, 260},	/* max cache size */
	{"cache-size", required_argument, NULL, 260},	/* max cache size */
	{"cachesize", required_argument, NULL, 260},	/* max cache size */
	{"fuse_opt", required_argument, NULL, 266},	/* owfs, fuse mount option */
	{"fuse-opt", required_argument, NULL, 266},	/* owfs, fuse mount option */
	{"fuseopt", required_argument, NULL, 266},	/* owfs, fuse mount option */
	{"fuse_open_opt", required_argument, NULL, 267},	/* owfs, fuse open option */
	{"fuse-open-opt", required_argument, NULL, 267},	/* owfs, fuse open option */
	{"fuseopenopt", required_argument, NULL, 267},	/* owfs, fuse open option */
	{"max_clients", required_argument, NULL, 269},	/* ftp max connections */
	{"max-clients", required_argument, NULL, 269},	/* ftp max connections */
	{"maxclients", required_argument, NULL, 269},	/* ftp max connections */
	{"HA7", optional_argument, NULL, 271},	/* HA7Net */
	{"ha7", optional_argument, NULL, 271},	/* HA7Net */
	{"HA7NET", optional_argument, NULL, 271},
	{"HA7Net", optional_argument, NULL, 271},
	{"ha7net", optional_argument, NULL, 271},
	{"FAKE", required_argument, NULL, 272},	/* Fake */
	{"Fake", required_argument, NULL, 272},	/* Fake */
	{"fake", required_argument, NULL, 272},	/* Fake */
	{"link", required_argument, NULL, 273},	/* link in ascii mode */
	{"LINK", required_argument, NULL, 273},	/* link in ascii mode */
	{"HA3", required_argument, NULL, 274},
	{"ha3", required_argument, NULL, 274},
	{"HA4B", required_argument, NULL, 275},
	{"HA4b", required_argument, NULL, 275},
	{"ha4b", required_argument, NULL, 275},
	{"HA5", required_argument, NULL, 276},
	{"ha5", required_argument, NULL, 276},
	{"ha7e", required_argument, NULL, 277},
	{"HA7e", required_argument, NULL, 277},
	{"HA7E", required_argument, NULL, 277},
	{"TESTER", required_argument, NULL, 278},	/* Tester */
	{"Tester", required_argument, NULL, 278},	/* Tester */
	{"tester", required_argument, NULL, 278},	/* Tester */
	{"etherweather", required_argument, NULL, 279},	/* EtherWeather */
	{"EtherWeather", required_argument, NULL, 279},	/* EtherWeather */
	{"zero", no_argument, &Global.announce_off, 0},
	{"nozero", no_argument, &Global.announce_off, 1},
	{"autoserver", no_argument, &Global.autoserver, 1},
	{"noautoserver", no_argument, &Global.autoserver, 0},
	{"announce", required_argument, NULL, 280},
	{"allow_other", no_argument, NULL, 298},
	{"altUSB", no_argument, &Global.altUSB, 1},	/* Willy Robison's tweaks */
	{"altusb", no_argument, &Global.altUSB, 1},	/* Willy Robison's tweaks */
	{"usb_flextime", no_argument, &Global.usb_flextime, 1},
	{"USB_flextime", no_argument, &Global.usb_flextime, 1},
	{"usb_regulartime", no_argument, &Global.usb_flextime, 0},
	{"USB_regulartime", no_argument, &Global.usb_flextime, 0},

	{"timeout_volatile", required_argument, NULL, 301,},	// timeout -- changing cached values
	{"timeout_stable", required_argument, NULL, 302,},	// timeout -- unchanging cached values
	{"timeout_directory", required_argument, NULL, 303,},	// timeout -- direcory cached values
	{"timeout_presence", required_argument, NULL, 304,},	// timeout -- device location
	{"timeout_serial", required_argument, NULL, 305,},	// timeout -- serial wait
	{"timeout_usb", required_argument, NULL, 306,},	// timeout -- usb wait
	{"timeout_network", required_argument, NULL, 307,},	// timeout -- tcp wait
	{"timeout_server", required_argument, NULL, 308,},	// timeout -- server wait
	{"timeout_ftp", required_argument, NULL, 309,},	// timeout -- ftp wait
	{"timeout_HA7", required_argument, NULL, 310,},	// timeout -- HA7Net wait
	{"timeout_ha7", required_argument, NULL, 310,},	// timeout -- HA7Net wait
	{"timeout_HA7Net", required_argument, NULL, 310,},	// timeout -- HA7Net wait
	{"timeout_ha7net", required_argument, NULL, 310,},	// timeout -- HA7Net wait
	{"timeout_persistent_low", required_argument, NULL, 311,},
	{"timeout_persistent_high", required_argument, NULL, 312,},
	{"clients_persistent_low", required_argument, NULL, 313,},
	{"clients_persistent_high", required_argument, NULL, 314,},

	{"one_device", no_argument, &Global.one_device, 1},
	{"1_device", no_argument, &Global.one_device, 1},

	{"pingcrazy", no_argument, &Global.pingcrazy, 1},
	{"no_dirall", no_argument, &Global.no_dirall, 1},
	{"no_get", no_argument, &Global.no_get, 1},
	{"no_persistence", no_argument, &Global.no_persistence, 1},
	{"8bit", no_argument, &Global.eightbit_serial, 1},
	{"6bit", no_argument, &Global.eightbit_serial, 0},
	{0, 0, 0, 0},
};

static int ParseInterp(struct lineparse *lp)
{
	int c = 0;					// default character found
	int f = 0;					// initialize to avoid compiler warning
	size_t option_name_length;
	const struct option *long_option_pointer;
	int *flag = NULL;

	if (lp->opt == NULL)
		return 0;
	option_name_length = strlen(lp->opt);
	if (option_name_length == 1)
		return lp->opt[0];
	for (long_option_pointer = owopts_long; long_option_pointer->name != NULL; ++long_option_pointer) {
		if (strncasecmp(lp->opt, long_option_pointer->name, option_name_length))
			continue;			// no match
		//LEVEL_DEBUG("Configuration option %s recognized as %s. Value=%s\n",lp->opt,long_option_pointer->name,SAFESTRING(lp->val)) ;
		//printf("Configuration option %s recognized as %s. Value=%s\n",lp->opt,long_option_pointer->name,SAFESTRING(lp->val)) ;
		if (long_option_pointer->flag) {	// immediate value mode
			//printf("flag c=%d flag=%p long_option_pointer->flag=%p\n",c,flag,long_option_pointer->flag);
			if ((c != 0)
				|| (flag != NULL && flag != long_option_pointer->flag)) {
				//fprintf(stderr,"Ambiguous option %s in configuration file.\n",lp->opt ) ;
				return -1;
			}
			flag = long_option_pointer->flag;
			f = long_option_pointer->val;
		} else if (c == 0) {	// char mode first match
			c = long_option_pointer->val;
		} else if (c != long_option_pointer->val) {	// char mode -- second match
			//fprintf(stderr,"Ambiguous option %s in configuration file.\n",lp->opt ) ;
			return -1;
		}
	}
	if (flag) {
		flag[0] = f;
		return 0;
	}
	if (c == 0) {
		//fprintf(stderr,"Configuration option %s not recognized. Value=%s\n",lp->opt,SAFESTRING(lp->val)) ;
	}
	return c;
}

// Parse the configuration file line
// Basically look for lines of form opt=val
// optional white space
// optional '='
// optional val
// Comment with #
// set opt and val as pointers into line (or NULL if not there)
// set NULL char at end of opt and val
static void ParseTheLine(struct lineparse *lp)
{
	ASCII *p;
	// state machine -- pretty self-explanatory.
	enum pstate { pspreopt, psinopt, pspreeq, pspreval, psinval } ps = pspreopt;
	lp->opt = NULL;
	lp->val = NULL;
	for (p = lp->line; *p; ++p) {
		switch (*p) {
			// end of line characters
		case '#':
		case '\n':
		case '\r':
			*p = '\0';
			return;
			// whitespace characters
		case ' ':
		case '\t':
			switch (ps) {
			case psinopt:
				*p = '\0';
				++ps;			// inopt->preeq
				break;
			case psinval:
				*p = '\0';
				return;
			default:
				break;
			}
			break;
			// equals special case: jump state
		case '=':
			switch (ps) {
			case psinopt:
				++ps;			// inopt->preeq
				// fall through intentionally
			case pspreeq:
				*p = '\0';
				++ps;			// preeq->preval
				break;
			default:			// rogue '='
				*p = '\0';
				return;
			}
			break;
			// real character, start or continue parameter, jump past "="
		default:
			switch (ps) {
			case pspreopt:
				lp->opt = p;
				++ps;			// preopt->inopt
				break;
			case pspreeq:
				++ps;			// preeq->preval
				// fall through intentionally
			case pspreval:
				lp->val = p;
				++ps;			// preval->inval
				// token fall through
			default:
				break;
			}
			break;
		}
	}
}

static int ConfigurationFile(const ASCII * file)
{
	FILE *configuration_file_pointer = fopen(file, "r");
	if (configuration_file_pointer) {
		int ret = 0;
		struct lineparse lp;
		while (fgets(lp.line, 256, configuration_file_pointer)) {
			// check line length
			if (strlen(lp.line) > 250) {
				ERROR_DEFAULT("Line too long (>250 characters) in file %s.\n%s\n", file, lp.line);
				ret = 1;
				break;
			}
			ParseTheLine(&lp);
			ret = owopt(ParseInterp(&lp), lp.val);
			if (ret)
				break;
		}
		fclose(configuration_file_pointer);
		return ret;
	} else {
		ERROR_DEFAULT("Cannot process configuration file %s\n", file);
		return 1;
	}
}

int owopt_packed(const char *params)
{
	char *params_copy;			// copy of the input line
	char *current_location_in_params_copy;	// pointers into the input line copy
	char *next_location_in_params_copy;	// pointers into the input line copy
	char **argv = NULL;
	int argc = 0;
	int ret = 0;
	int allocated = 0;
	int option_char;

	if (params == NULL)
		return 0;
	current_location_in_params_copy = params_copy = strdup(params);
	if (params_copy == NULL)
		return -ENOMEM;

	// Stuffs arbitrary first value since argv[0] ignored by getopt
	// create a synthetic argv/argc for get_opt
	for (next_location_in_params_copy = "X";
		 next_location_in_params_copy != NULL; next_location_in_params_copy = strsep(&current_location_in_params_copy, " ")) {
		// make room
		if (argc >= allocated - 1) {
			char **larger_argv = realloc(argv, (allocated + 10) * sizeof(char *));
			if (larger_argv != NULL) {
				allocated += 10;
				argv = larger_argv;
			} else {
				ret = -ENOMEM;
				break;
			}
		}
		argv[argc] = next_location_in_params_copy;
		++argc;
		argv[argc] = NULL;
	}

	// analyze argv/argc as if real comman line arguments
	while (ret == 0) {
		if ((option_char = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) == -1)
			break;
		ret = owopt(option_char, optarg);
	}

	/* non-option arguments */
	while ((ret == 0) && (optind < argc)) {
		//printf("optind = %d arg=%s\n",optind,SAFESTRING(argv[optind]));
		OW_ArgGeneric(argv[optind]);
		++optind;
	}

	if (argv != NULL)
		free(argv);
	free(params_copy);
	return ret;
}

/* Parses one argument */
/* return 0 if ok */
int owopt(const int option_char, const char *arg)
{
	static int config_depth = 0;
	//printf("Option %c (%d) Argument=%s\n",c,c,SAFESTRING(arg)) ;
	switch (option_char) {
	case 'c':
		if (config_depth > 4) {
			LEVEL_DEFAULT("Configuration file layered too deeply (>%d)\n", config_depth);
			return 1;
		} else {
			int ret;
			++config_depth;
			ret = ConfigurationFile(arg);
			--config_depth;
			return ret;
		}
	case 'h':
		ow_help(arg);
		return 1;
	case 'u':
		return OW_ArgUSB(arg);
	case 'd':
		return OW_ArgDevice(arg);
	case 't':
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			Global.timeout_volatile = (int) i;
		}
		break;
	case 'r':
		Global.readonly = 1;
		break;
	case 'w':
		Global.readonly = 0;
		break;
	case 'C':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
		break;
	case 'F':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit);
		break;
	case 'R':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine);
		break;
	case 'K':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin);
		break;
	case 'V':
		printf("libow version:\n\t" VERSION "\n");
		return 1;
	case 's':
		return OW_ArgNet(arg);
	case 'p':
		switch (Global.opt) {
		case opt_httpd:
		case opt_server:
		case opt_ftpd:
			return OW_ArgServer(arg);
		default:
			return 0;
		}
	case 'm':
		switch (Global.opt) {
		case opt_owfs:
			return OW_ArgServer(arg);
		default:
			return 0;
		}
	case 'f':
		if (!strcasecmp(arg, "f.i"))
			set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
		else if (!strcasecmp(arg, "fi"))
			set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fi);
		else if (!strcasecmp(arg, "f.i.c"))
			set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdidc);
		else if (!strcasecmp(arg, "f.ic"))
			set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdic);
		else if (!strcasecmp(arg, "fi.c"))
			set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fidc);
		else if (!strcasecmp(arg, "fic"))
			set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fic);
		else {
			LEVEL_DEFAULT("Unrecognized format type %s\n", arg);
			return 1;
		}
		break;
	case 'P':
		if (arg == NULL || strlen(arg) == 0) {
			LEVEL_DEFAULT("No PID file specified\n");
			return 1;
		} else if ((pid_file = strdup(arg)) == NULL) {
			fprintf(stderr, "Insufficient memory to store the PID filename: %s\n", arg);
			return 1;
		}
		break;
	case 257:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			Global.error_print = (int) i;
		}
		break;
	case 258:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			Global.error_level = (int) i;
		}
		break;
	case 260:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			Global.cache_size = (size_t) i;
		}
		break;
	case 266:					/* fuse_opt, handled in owfs.c */
		break;
	case 267:					/* fuse_open_opt, handled in owfs.c */
		break;
	case 269:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			Global.max_clients = (int) i;
		}
		break;
	case 271:
		return OW_ArgHA7(arg);
	case 272:
		return OW_ArgFake(arg);
	case 273:
		return OW_ArgLink(arg);
	case 274:
		return OW_ArgPassive("HA3", arg);
	case 275:
		return OW_ArgPassive("HA4B", arg);
	case 278:
		return OW_ArgTester(arg);
	case 279:
		return OW_ArgEtherWeather(arg);
	case 280:
		Global.announce_name = strdup(arg);
		break;
	case 298:					/* allow_other */
		break;
		// TIMEOUTS
	case 301:
	case 302:
	case 303:
	case 304:
	case 305:
	case 306:
	case 307:
	case 308:
	case 309:
	case 310:
	case 311:
	case 312:
	case 313:
	case 314:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			// Using the character as a numeric value -- convenient but risky
			(&Global.timeout_volatile)[option_char - 301] = (int) i;
		}
		break;
	case 0:
		break;
	default:
		return 1;
	}
	return 0;
}

int OW_ArgNet(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_server;
	return 0;
}

static int OW_ArgHA7(const char *arg)
{
#if OW_HA7
	if (arg) {
		struct connection_in *in = NewIn(NULL);
		if (in == NULL)
			return 1;
		in->name = arg ? strdup(arg) : NULL;
		in->busmode = bus_ha7net;
		return 0;
	} else {					// Try multicast discovery
		//printf("Find HA7\n");
		return FS_FindHA7();
	}
#else							/* OW_HA7 */
	LEVEL_DEFAULT("HA7 support (intentionally) not included in compilation. Reconfigure and recompile.\n");
	return 1;
#endif							/* OW_HA7 */
}

static int OW_ArgEtherWeather(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = arg ? strdup(arg) : NULL;
	in->busmode = bus_etherweather;
	return 0;
}

static int OW_ArgFake(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_fake;
	return 0;
}

static int OW_ArgTester(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_tester;
	return 0;
}

int OW_ArgServer(const char *arg)
{
	struct connection_out *out = NewOut();
	if (out == NULL)
		return 1;
	out->name = arg ? strdup(arg) : NULL;
	return 0;
}

int OW_ArgDevice(const char *arg)
{
	struct stat sbuf;
	if (stat(arg, &sbuf)) {
		LEVEL_DEFAULT("Cannot access device %s\n", arg);
		return 1;
	}
	if (!S_ISCHR(sbuf.st_mode)) {
		LEVEL_DEFAULT("Not a \"character\" device %s (st_mode=%x)\n", arg, sbuf.st_mode);
		return 1;
	}
	if (major(sbuf.st_rdev) == 99)
		return OW_ArgParallel(arg);
	if (major(sbuf.st_rdev) == 89)
		return OW_ArgI2C(arg);
	return OW_ArgSerial(arg);
}

static int OW_ArgSerial(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_serial;
	return 0;
}

static int OW_ArgPassive(char *adapter_type_name, const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_passive;
	// special set name of adapter here 
	in->adapter_name = adapter_type_name;
	return 0;
}

static int OW_ArgLink(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = (arg[0] == '/') ? bus_link : bus_elink;
	return 0;
}

static int OW_ArgParallel(const char *arg)
{
#if OW_PARPORT
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_parallel;
	return 0;
#else							/* OW_PARPORT */
	LEVEL_DEFAULT("Parallel port support (intentionally) not included in compilation. For DS1410E. That's ok, it doesn't work anyways.\n");
	return 1;
#endif							/* OW_PARPORT */
}

static int OW_ArgI2C(const char *arg)
{
#if OW_I2C
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_i2c;
	return 0;
#else							/* OW_I2C */
	LEVEL_DEFAULT("I2C (smbus DS2482-X00) support (intentionally) not included in compilation. Reconfigure and recompile.\n");
	return 1;
#endif							/* OW_I2C */
}

int OW_ArgUSB(const char *arg)
{
#if OW_USB
	struct connection_in *in = NewIn(NULL);
	if (in == NULL)
		return 1;
	in->busmode = bus_usb;
	if (arg == NULL) {
		in->connin.usb.usb_nr = 1;
	} else if (strcasecmp(arg, "all") == 0) {
		int number_of_usb_adapters;
		number_of_usb_adapters = DS9490_enumerate();
		LEVEL_CONNECT("All USB adapters requested, %d found.\n", number_of_usb_adapters);
		// first one
		in->connin.usb.usb_nr = 1;
		// cycle through rest
		if (number_of_usb_adapters > 1) {
			int usb_adapter_index;
			for (usb_adapter_index = 2; usb_adapter_index <= number_of_usb_adapters; ++usb_adapter_index) {
				struct connection_in *in2 = NewIn(NULL);
				if (in2 == NULL)
					return 1;
				in2->busmode = bus_usb;
				in2->connin.usb.usb_nr = usb_adapter_index;
			}
		}
	} else {
		in->connin.usb.usb_nr = atoi(arg);
		//printf("ArgUSB file_descriptor=%d\n",in->file_descriptor);
		if (in->connin.usb.usb_nr < 1) {
			LEVEL_CONNECT("USB option %s implies no USB detection.\n", arg);
			in->connin.usb.usb_nr = 0;
		} else if (in->connin.usb.usb_nr > 1) {
			LEVEL_CONNECT("USB adapter %d requested.\n", in->connin.usb.usb_nr);
		}
	}
	return 0;
#else							/* OW_USB */
	LEVEL_DEFAULT("USB support (intentionally) not included in compilation. Check LIBUSB, then reconfigure and recompile.\n");
	return 1;
#endif							/* OW_USB */
}

int OW_ArgGeneric(const char *arg)
{
	if (arg && arg[0]) {
		switch (arg[0]) {
		case '/':
			return OW_ArgDevice(arg);
		case 'u':
		case 'U':
			return OW_ArgUSB(&arg[1]);
		default:
			return OW_ArgNet(arg);
		}
	}
	return 1;
}

static int OW_parsevalue(long long int *var, const ASCII * str)
{
	errno = 0;
	var[0] = strtol(str, NULL, 10);
	if (errno) {
		ERROR_DETAIL("Bad configuration value %s\n", str);
		return 1;
	}
	return 0;
}
