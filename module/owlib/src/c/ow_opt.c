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
	const ASCII *file;
	ASCII *prog;
	ASCII *opt;
	ASCII *val;
	int reverse_prog;
	int c;
	int line_number;
};

static void ParseTheLine(struct lineparse *lp);
static int ConfigurationFile(const ASCII * file);
static int ParseInterp(struct lineparse *lp);
static int OW_parsevalue_I(long long int *var, const ASCII * str);
static int OW_parsevalue_F(_FLOAT *var, const ASCII * str);

const struct option owopts_long[] = {
	{"alias", required_argument, NULL, 'a'},
	{"aliases", required_argument, NULL, 'a'},
	{"aliasfile", required_argument, NULL, 'a'},
	{"configuration", required_argument, NULL, 'c'},
	{"device", required_argument, NULL, 'd'},
	{"usb", optional_argument, NULL, 'u'},
	{"USB", optional_argument, NULL, 'u'},
	{"help", optional_argument, NULL, 'h'},
	{"port", required_argument, NULL, 'p'},
	{"mountpoint", required_argument, NULL, 'm'},
	{"server", required_argument, NULL, 's'},
	{"readonly", no_argument, NULL, 'r'},
	{"safemode", no_argument, NULL, e_safemode},
	{"safe_mode", no_argument, NULL, e_safemode},
	{"safe", no_argument, NULL, e_safemode},
	{"write", no_argument, NULL, 'w'},

	{"Celsius", no_argument, NULL, 'C'},
	{"celsius", no_argument, NULL, 'C'},
	{"Fahrenheit", no_argument, NULL, 'F'},
	{"fahrenheit", no_argument, NULL, 'F'},
	{"Kelvin", no_argument, NULL, 'K'},
	{"kelvin", no_argument, NULL, 'K'},
	{"Rankine", no_argument, NULL, 'R'},
	{"rankine", no_argument, NULL, 'R'},

	{"mbar", no_argument, NULL, e_pressure_mbar},
	{"millibar", no_argument, NULL, e_pressure_mbar},
	{"mBar", no_argument, NULL, e_pressure_mbar},
	{"atm", no_argument, NULL, e_pressure_atm},
	{"mmhg", no_argument, NULL, e_pressure_mmhg},
	{"mmHg", no_argument, NULL, e_pressure_mmhg},
	{"torr", no_argument, NULL, e_pressure_mmhg},
	{"inhg", no_argument, NULL, e_pressure_inhg},
	{"inHg", no_argument, NULL, e_pressure_inhg},
	{"psi", no_argument, NULL, e_pressure_psi},
	{"psia", no_argument, NULL, e_pressure_psi},
	{"psig", no_argument, NULL, e_pressure_psi},
	{"Pa", no_argument, NULL, e_pressure_Pa},
	{"pa", no_argument, NULL, e_pressure_Pa},
	{"pascal", no_argument, NULL, e_pressure_Pa},

	{"version", no_argument, NULL, 'V'},
	{"format", required_argument, NULL, 'f'},
	{"pid_file", required_argument, NULL, 'P'},
	{"pid-file", required_argument, NULL, 'P'},
	{"background", no_argument, &Globals.want_background, 1},
	{"foreground", no_argument, &Globals.want_background, 0},
	{"fatal_debug", no_argument, &Globals.fatal_debug, 1},
	{"fatal-debug", no_argument, &Globals.fatal_debug, 1},
	{"fatal_debug_file", required_argument, NULL, e_fatal_debug_file},
	{"fatal-debug-file", required_argument, NULL, e_fatal_debug_file},
	{"error_print", required_argument, NULL, e_error_print},
	{"error-print", required_argument, NULL, e_error_print},
	{"errorprint", required_argument, NULL, e_error_print},
	{"error_level", required_argument, NULL, e_error_level},
	{"error-level", required_argument, NULL, e_error_level},
	{"errorlevel", required_argument, NULL, e_error_level},
	{"concurrent_connections", required_argument, NULL, e_concurrent_connections},
	{"concurrent-connections", required_argument, NULL, e_concurrent_connections},
	{"max-connections", required_argument, NULL, e_concurrent_connections},
	{"max_connections", required_argument, NULL, e_concurrent_connections},
	{"cache_size", required_argument, NULL, e_cache_size},	/* max cache size */
	{"cache-size", required_argument, NULL, e_cache_size},	/* max cache size */
	{"cachesize", required_argument, NULL, e_cache_size},	/* max cache size */
	{"fuse_opt", required_argument, NULL, e_fuse_opt},	/* owfs, fuse mount option */
	{"fuse-opt", required_argument, NULL, e_fuse_opt},	/* owfs, fuse mount option */
	{"fuseopt", required_argument, NULL, e_fuse_opt},	/* owfs, fuse mount option */
	{"fuse_open_opt", required_argument, NULL, e_fuse_open_opt},	/* owfs, fuse open option */
	{"fuse-open-opt", required_argument, NULL, e_fuse_open_opt},	/* owfs, fuse open option */
	{"fuseopenopt", required_argument, NULL, e_fuse_open_opt},	/* owfs, fuse open option */
	{"max_clients", required_argument, NULL, e_max_clients},	/* ftp max connections */
	{"max-clients", required_argument, NULL, e_max_clients},	/* ftp max connections */
	{"maxclients", required_argument, NULL, e_max_clients},	/* ftp max connections */

	{"sidetap", required_argument, NULL, e_sidetap},	/* sidetap address */
	{"side", required_argument, NULL, e_sidetap},	/* sidetap address */
	{"Sidetap", required_argument, NULL, e_sidetap},	/* sidetap address */
	{"SideTap", required_argument, NULL, e_sidetap},	/* sidetap address */
	{"SIDE", required_argument, NULL, e_sidetap},	/* sidetap address */

	{"passive", required_argument, NULL, e_passive},	/* DS9097 passive */
	{"PASSIVE", required_argument, NULL, e_passive},	/* DS9097 passive */
	{"i2c", optional_argument, NULL, e_i2c},	/* i2c adapters */
	{"I2C", optional_argument, NULL, e_i2c},	/* i2c adapters */
	{"HA7", optional_argument, NULL, e_ha7},	/* HA7Net */
	{"ha7", optional_argument, NULL, e_ha7},	/* HA7Net */
	{"HA7NET", optional_argument, NULL, e_ha7},
	{"HA7Net", optional_argument, NULL, e_ha7},
	{"ha7net", optional_argument, NULL, e_ha7},
	{"enet", optional_argument, NULL, e_enet},
	{"Enet", optional_argument, NULL, e_enet},
	{"ENET", optional_argument, NULL, e_enet},
	{"OW-Server-Enet", optional_argument, NULL, e_enet},
	{"FAKE", required_argument, NULL, e_fake},	/* Fake */
	{"Fake", required_argument, NULL, e_fake},	/* Fake */
	{"fake", required_argument, NULL, e_fake},	/* Fake */
	{"link", required_argument, NULL, e_link},	/* link in ascii mode */
	{"LINK", required_argument, NULL, e_link},	/* link in ascii mode */
	{"HA3", required_argument, NULL, e_ha3},
	{"ha3", required_argument, NULL, e_ha3},
	{"HA4B", required_argument, NULL, e_ha4b},
	{"HA4b", required_argument, NULL, e_ha4b},
	{"ha4b", required_argument, NULL, e_ha4b},
	{"HA5", required_argument, NULL, e_ha5},
	{"ha5", required_argument, NULL, e_ha5},
	{"ha7e", required_argument, NULL, e_ha7e},
	{"HA7e", required_argument, NULL, e_ha7e},
	{"HA7E", required_argument, NULL, e_ha7e},
	{"ha7s", required_argument, NULL, e_ha7e},
	{"HA7s", required_argument, NULL, e_ha7e},
	{"HA7S", required_argument, NULL, e_ha7e},
	{"xport", required_argument, NULL, e_xport, },
	{"telnet", required_argument, NULL, e_xport, },
	{"TELNET", required_argument, NULL, e_xport, },
	{"Xport", required_argument, NULL, e_xport, },
	{"XPort", required_argument, NULL, e_xport, },
	{"XPORT", required_argument, NULL, e_xport, },
	{"telnet", required_argument, NULL, e_xport, },
	{"TESTER", required_argument, NULL, e_tester},	/* Tester */
	{"Tester", required_argument, NULL, e_tester},	/* Tester */
	{"tester", required_argument, NULL, e_tester},	/* Tester */
	{"mock", required_argument, NULL, e_mock},	/* Mock */
	{"Mock", required_argument, NULL, e_mock},	/* Mock */
	{"MOCK", required_argument, NULL, e_mock},	/* Mock */
	{"etherweather", required_argument, NULL, e_etherweather},	/* EtherWeather */
	{"EtherWeather", required_argument, NULL, e_etherweather},	/* EtherWeather */
	{"zero", no_argument, &Globals.announce_off, 0},
	{"nozero", no_argument, &Globals.announce_off, 1},
	{"autoserver", no_argument, &Globals.autoserver, 1},
	{"noautoserver", no_argument, &Globals.autoserver, 0},
	{"w1", no_argument, &Globals.w1, 1},
	{"W1", no_argument, &Globals.w1, 1},
	{"announce", required_argument, NULL, e_announce},
	{"allow_other", no_argument, NULL, e_allow_other},
	{"altUSB", no_argument, &Globals.altUSB, 1},	/* Willy Robison's tweaks */
	{"altusb", no_argument, &Globals.altUSB, 1},	/* Willy Robison's tweaks */
	{"usb_flextime", no_argument, &Globals.usb_flextime, 1},
	{"USB_flextime", no_argument, &Globals.usb_flextime, 1},
	{"usb_regulartime", no_argument, &Globals.usb_flextime, 0},
	{"USB_regulartime", no_argument, &Globals.usb_flextime, 0},
	{"serial_flex", no_argument, &Globals.serial_flextime, 1},
	{"serial_flextime", no_argument, &Globals.serial_flextime, 1},
	{"serial_regulartime", no_argument, &Globals.serial_flextime, 0},
	{"serial_regular", no_argument, &Globals.serial_flextime, 0},
	{"serial_standardtime", no_argument, &Globals.serial_flextime, 0},
	{"serial_standard", no_argument, &Globals.serial_flextime, 0},
	{"serial_stdtime", no_argument, &Globals.serial_flextime, 0},
	{"serial_std", no_argument, &Globals.serial_flextime, 0},
	{"reverse", no_argument, &Globals.serial_reverse, 1},
	{"reverse_polarity", no_argument, &Globals.serial_reverse, 1},
	{"straight_polarity", no_argument, &Globals.serial_reverse, 0},
	{"baud_rate", required_argument, NULL, e_baud},
	{"baud", required_argument, NULL, e_baud},
	{"Baud", required_argument, NULL, e_baud},
	{"BAUD", required_argument, NULL, e_baud},

	{"timeout_volatile", required_argument, NULL, e_timeout_volatile,},	// timeout -- changing cached values
	{"timeout_stable", required_argument, NULL, e_timeout_stable,},	// timeout -- unchanging cached values
	{"timeout_directory", required_argument, NULL, e_timeout_directory,},	// timeout -- direcory cached values
	{"timeout_presence", required_argument, NULL, e_timeout_presence,},	// timeout -- device location
	{"timeout_serial", required_argument, NULL, e_timeout_serial,},	// timeout -- serial wait (read and write)
	{"timeout_usb", required_argument, NULL, e_timeout_usb,},	// timeout -- usb wait
	{"timeout_network", required_argument, NULL, e_timeout_network,},	// timeout -- tcp wait
	{"timeout_server", required_argument, NULL, e_timeout_server,},	// timeout -- server wait
	{"timeout_ftp", required_argument, NULL, e_timeout_ftp,},	// timeout -- ftp wait
	{"timeout_HA7", required_argument, NULL, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_ha7", required_argument, NULL, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_HA7Net", required_argument, NULL, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_ha7net", required_argument, NULL, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_w1", required_argument, NULL, e_timeout_w1,},	// timeout -- w1 netlink
	{"timeout_W1", required_argument, NULL, e_timeout_w1,},	// timeout -- w1 netlink
	{"timeout_persistent_low", required_argument, NULL, e_timeout_persistent_low,},
	{"timeout_persistent_high", required_argument, NULL, e_timeout_persistent_high,},
	{"clients_persistent_low", required_argument, NULL, e_clients_persistent_low,},
	{"clients_persistent_high", required_argument, NULL, e_clients_persistent_high,},

	{"temperature_low", required_argument, NULL, e_templow,},
	{"low_temperature", required_argument, NULL, e_templow,},
	{"templow", required_argument, NULL, e_templow,},

	{"temperature_high", required_argument, NULL, e_temphigh,},
	{"high_temperature", required_argument, NULL, e_temphigh,},
	{"temphigh", required_argument, NULL, e_temphigh,},
	{"temperature_hi", required_argument, NULL, e_temphigh,},
	{"hi_temperature", required_argument, NULL, e_temphigh,},
	{"temphi", required_argument, NULL, e_temphigh,},

	{"one_device", no_argument, &Globals.one_device, 1},
	{"1_device", no_argument, &Globals.one_device, 1},

	{"pingcrazy", no_argument, &Globals.pingcrazy, 1},
	{"no_dirall", no_argument, &Globals.no_dirall, 1},
	{"no_get", no_argument, &Globals.no_get, 1},
	{"no_persistence", no_argument, &Globals.no_persistence, 1},
	{"8bit", no_argument, &Globals.eightbit_serial, 1},
	{"6bit", no_argument, &Globals.eightbit_serial, 0},
	{"checksum", no_argument, &Globals.checksum, 1},
	{"no_checksum", no_argument, &Globals.checksum, 0},
	{"ActivePullUp", no_argument, &Globals.i2c_APU, 1},
	{"activepullup", no_argument, &Globals.i2c_APU, 1},
	{"APU", no_argument, &Globals.i2c_APU, 1},
	{"apu", no_argument, &Globals.i2c_APU, 1},
	{"no_ActivePullUp", no_argument, &Globals.i2c_APU, 0},
	{"no_activepullup", no_argument, &Globals.i2c_APU, 0},
	{"no_APU", no_argument, &Globals.i2c_APU, 0},
	{"no_apu", no_argument, &Globals.i2c_APU, 0},
	{"PresencePulseMasking", no_argument, &Globals.i2c_PPM, 1},
	{"presencepulsemasking", no_argument, &Globals.i2c_PPM, 1},
	{"PPM", no_argument, &Globals.i2c_PPM, 1},
	{"ppm", no_argument, &Globals.i2c_PPM, 1},
	{"no_PresencePulseMasking", no_argument, &Globals.i2c_PPM, 0},
	{"no_presencepulsemasking", no_argument, &Globals.i2c_PPM, 0},
	{"no_PPM", no_argument, &Globals.i2c_PPM, 0},
	{"no_ppm", no_argument, &Globals.i2c_PPM, 0},
	{0, 0, 0, 0},
};

// Called after "ParseTheLine" has filled the lineparse structure
static int ParseInterp(struct lineparse *lp)
{
	size_t option_name_length;
	const struct option *long_option_pointer;

	LEVEL_DEBUG("Configuration file (%s:%d) Program=%s%s, Option=%s, Value=%s", SAFESTRING(lp->file), lp->line_number,
				lp->reverse_prog ? "Not " : "", SAFESTRING(lp->prog), SAFESTRING(lp->opt), SAFESTRING(lp->val));
	// Check for program specification
	if (lp->prog != NULL) {
		ASCII *prog_char;
		for (prog_char = lp->prog; *prog_char; ++prog_char) {
			*prog_char = tolower(*prog_char);
		}
		if (strstr(lp->prog, "server") != NULL) {
			if ((Globals.opt == opt_server) == lp->reverse_prog) {
				return 0;
			}
		} else if (strstr(lp->prog, "http") != NULL) {
			if ((Globals.opt == opt_httpd) == lp->reverse_prog) {
				return 0;
			}
		} else if (strstr(lp->prog, "ftp") != NULL) {
			if ((Globals.opt == opt_ftpd) == lp->reverse_prog) {
				return 0;
			}
		} else if (strstr(lp->prog, "fs") != NULL) {
			if ((Globals.opt == opt_owfs) == lp->reverse_prog) {
				return 0;
			}
		} else {
			LEVEL_DEFAULT("Configuration file (%s:%d) Unrecognized program %s. Option=%s, Value=%s", SAFESTRING(lp->file), lp->line_number,
						  SAFESTRING(lp->prog), SAFESTRING(lp->opt), SAFESTRING(lp->val));
			return 0;
		}
	}
	// empty option field -- probably comment or blank line
	if (lp->opt == NULL) {
		return 0;
	}

	option_name_length = strlen(lp->opt);

	// single character option
	if (option_name_length == 1) {
		return lp->opt[0];
	}
	// long option -- search in owopts_long array
	for (long_option_pointer = owopts_long; long_option_pointer->name != NULL; ++long_option_pointer) {
		if (strncasecmp(lp->opt, long_option_pointer->name, option_name_length) == 0) {
			LEVEL_DEBUG("Configuration file (%s:%d) Option %s recognized as %s. Value=%s", SAFESTRING(lp->file), lp->line_number, lp->opt,
						long_option_pointer->name, SAFESTRING(lp->val));
			if (long_option_pointer->flag != NULL) {	// immediate value mode
				//printf("flag c=%d flag=%p long_option_pointer->flag=%p\n",c,flag,long_option_pointer->flag);
				(long_option_pointer->flag)[0] = long_option_pointer->val;
				return 0;
			} else {
				return long_option_pointer->val;
			}
		}
	}
	LEVEL_DEFAULT("Configuration file (%s:%d) Unrecognized option %s. Value=%s", SAFESTRING(lp->file), lp->line_number, SAFESTRING(lp->opt),
				  SAFESTRING(lp->val));
	return 0;
}

// Parse the configuration file line
// Basically look for lines of form opt=val
// optional white space1
// optional '='
// optional val
// Comment with #
// set opt and val as pointers into line (or NULL if not there)
// set NULL char at end of opt and val
static void ParseTheLine(struct lineparse *lp)
{
	ASCII *current_char;
	// state machine -- pretty self-explanatory.
	// note that there is no program state, it is only deduced post-facto when a colon is seen.
	enum e_parse_state { ps_pre_opt, ps_in_opt, ps_pre_equals, ps_pre_value, ps_in_value } parse_state = ps_pre_opt;

	lp->prog = NULL;
	lp->opt = NULL;
	lp->val = NULL;
	lp->reverse_prog = 0;

	// plod through the line, char by char
	for (current_char = lp->line; *current_char; ++current_char) {
		// change states on special chars
		switch (*current_char) {
			// end of line characters
		case '#':
		case '\n':
		case '\r':
			*current_char = '\0';
			return;
			// whitespace characters
		case ' ':
		case '\t':
			switch (parse_state) {
			case ps_in_opt:
				*current_char = '\0';
				parse_state = ps_pre_equals;
				break;
			case ps_in_value:
				*current_char = '\0';
				return;
			default:
				break;
			}
			break;
			// dashes before options ignored (only needed in real command line)
		case '-':
			switch (parse_state) {
			case ps_pre_value:	// not ignored before values, of course
				lp->val = current_char;
				parse_state = ps_in_value;
				break;
			default:
				break;
			}
			break;
			// negate program state (even though program not yet assigned)
		case '!':
			switch (parse_state) {
			case ps_pre_opt:
				lp->reverse_prog = !lp->reverse_prog;
				break;
			case ps_pre_value:	// not ignored before values, of course
				lp->val = current_char;
				parse_state = ps_in_value;
				break;
			default:
				break;
			}
			break;
			// colon special case -- assigns to a program
			// Since we had assumed this was an opt, we have to adjust
		case ':':
			switch (parse_state) {
			case ps_in_opt:
			case ps_pre_equals:
				*current_char = '\0';
				lp->prog = lp->opt;
				lp->opt = NULL;
				parse_state = ps_pre_opt;
				break;
			case ps_pre_value:	// not ignored before values, of course
				lp->val = current_char;
				parse_state = ps_in_value;
				break;
			default:
				break;
			}
			break;
			// equals special case: jump state
		case '=':
			switch (parse_state) {
			case ps_in_opt:
				parse_state = ps_pre_equals;
				// fall through intentionally
			case ps_pre_equals:
				*current_char = '\0';
				parse_state = ps_pre_value;
				break;
			case ps_in_value:
				break;
			default:			// rogue '='
				*current_char = '\0';
				return;
			}
			break;
			// real character, start or continue parameter, jump past "="
		default:
			switch (parse_state) {
			case ps_pre_opt:
				lp->opt = current_char;
				parse_state = ps_in_opt;
				break;
			case ps_pre_equals:
				parse_state = ps_pre_value;
				// fall through intentionally
			case ps_pre_value:
				lp->val = current_char;
				parse_state = ps_in_value;
				break;
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
		lp.line_number = 0;
		lp.file = file;

		while (fgets(lp.line, 256, configuration_file_pointer)) {
			++lp.line_number;
			// check line length
			if (strlen(lp.line) > 250) {
				LEVEL_DEFAULT("Configuration file (%s:%d) Line too long (>250 characters).", SAFESTRING(lp.file), lp.line_number);
				ret = 1;
				break;
			}
			ParseTheLine(&lp);
			ret = owopt(ParseInterp(&lp), lp.val);
			if (ret) {
				break;
			}
		}
		fclose(configuration_file_pointer);
		return ret;
	} else {
		ERROR_DEFAULT("Cannot process configuration file %s", file);
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

	if (params == NULL) {
		return 0;
	}
	current_location_in_params_copy = params_copy = owstrdup(params);
	if (params_copy == NULL) {
		return -ENOMEM;
	}
	// Stuffs arbitrary first value since argv[0] ignored by getopt
	// create a synthetic argv/argc for get_opt
	for (next_location_in_params_copy = "X";
		 next_location_in_params_copy != NULL; next_location_in_params_copy = strsep(&current_location_in_params_copy, " ")) {
		// make room
		if (argc >= allocated - 1) {
			char **larger_argv = owrealloc(argv, (allocated + 10) * sizeof(char *));
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

	// analyze argv/argc as if real command line arguments
	while (ret == 0) {
		if ((option_char = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) == -1) {
			break;
		}
		ret = owopt(option_char, optarg);
	}

	/* non-option arguments */
	while ((ret == 0) && (optind < argc)) {
		//printf("optind = %d arg=%s\n",optind,SAFESTRING(argv[optind]));
		ARG_Generic(argv[optind]);
		++optind;
	}

	if (argv != NULL) {
		owfree(argv);
	}
	owfree(params_copy);
	return ret;
}

/* Parses one argument */
/* return 0 if ok */
int owopt(const int option_char, const char *arg)
{
	// depth of nesting for config files
	static int config_depth = 0;

	// utility variables for parsing options
	long long int arg_to_integer ;
	_FLOAT arg_to_float ;

	//printf("Option %c (%d) Argument=%s\n",c,c,SAFESTRING(arg)) ;
	switch (option_char) {
	case 'a' :
		return AliasFile(arg) ;
	case 'c':
		if (config_depth > 4) {
			LEVEL_DEFAULT("Configuration file layered too deeply (>%d)", config_depth);
			return 1;
		} else {
			int ret;
			++config_depth;
			ret = ConfigurationFile(arg);
			--config_depth;
			return ret;
		}
	case 'h':
		FS_help(arg);
		return 1;
	case 'u':
		return ARG_USB(arg);
	case 'd':
		return ARG_Device(arg);
	case 't':
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.timeout_volatile = (int) arg_to_integer;
		break;
	case 'r':
		Globals.readonly = 1;
		break;
	case 'w':
		Globals.readonly = 0;
		break;
	case 'C':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
		break;
	case 'F':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit);
		break;
	case 'R':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine);
		break;
	case 'K':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin);
		break;
	case 'V':
		printf("libow version:\n\t" VERSION "\n");
		return 1;
	case 's':
		return ARG_Net(arg);
	case 'p':
		switch (Globals.opt) {
		case opt_httpd:
		case opt_server:
		case opt_ftpd:
			return ARG_Server(arg);
		default:
			return 0;
		}
	case 'm':
		switch (Globals.opt) {
		case opt_owfs:
			return ARG_Server(arg);
		default:
			return 0;
		}
	case 'f':
		if (!strcasecmp(arg, "f.i")) {
			set_controlflags(&LocalControlFlags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
		} else if (!strcasecmp(arg, "fi")) {
			set_controlflags(&LocalControlFlags, DEVFORMAT_MASK, DEVFORMAT_BIT, fi);
		} else if (!strcasecmp(arg, "f.i.c")) {
			set_controlflags(&LocalControlFlags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdidc);
		} else if (!strcasecmp(arg, "f.ic")) {
			set_controlflags(&LocalControlFlags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdic);
		} else if (!strcasecmp(arg, "fi.c")) {
			set_controlflags(&LocalControlFlags, DEVFORMAT_MASK, DEVFORMAT_BIT, fidc);
		} else if (!strcasecmp(arg, "fic")) {
			set_controlflags(&LocalControlFlags, DEVFORMAT_MASK, DEVFORMAT_BIT, fic);
		} else {
			LEVEL_DEFAULT("Unrecognized format type %s", arg);
			return 1;
		}
		break;
	case 'P':
		if (arg == NULL || strlen(arg) == 0) {
			LEVEL_DEFAULT("No PID file specified");
			return 1;
		} else if ((pid_file = owstrdup(arg)) == NULL) {
			fprintf(stderr, "Insufficient memory to store the PID filename: %s", arg);
			return 1;
		}
		break;
	case e_fatal_debug_file:
		if (arg == NULL || strlen(arg) == 0) {
			LEVEL_DEFAULT("No fatal_debug_file specified");
			return 1;
		} else if ((Globals.fatal_debug_file = owstrdup(arg)) == NULL) {
			LEVEL_DEBUG("Out of memory.");
			return 1;
		}
		break;
	case e_error_print:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.error_print = (int) arg_to_integer;
		break;
	case e_error_level:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.error_level = (int) arg_to_integer;
		break;
	case e_concurrent_connections:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.concurrent_connections = (int) arg_to_integer;
		break;
	case e_cache_size:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.cache_size = (size_t) arg_to_integer;
		break;
	case e_fuse_opt:			/* fuse_opt, handled in owfs.c */
		break;
	case e_fuse_open_opt:		/* fuse_open_opt, handled in owfs.c */
		break;
	case e_sidetap:			/* sidetap address */
		switch (Globals.opt) {
		case opt_server:
			return ARG_Side(arg);
		default:
			return 0;
		}
	case e_max_clients:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.max_clients = (int) arg_to_integer;
		break;
	case e_i2c:
		return ARG_I2C(arg);
	case e_ha5:
		return ARG_HA5(arg);
	case e_ha7:
		return ARG_HA7(arg);
	case e_ha7e:
		return ARG_HA7E(arg);
	case e_enet:
		return ARG_ENET(arg);
	case e_fake:
		return ARG_Fake(arg);
	case e_link:
		return ARG_Link(arg);
	case e_passive:
		return ARG_Passive("Passive", arg);
	case e_ha3:
		return ARG_Passive("HA3", arg);
	case e_ha4b:
		return ARG_Passive("HA4B", arg);
	case e_xport:
		return ARG_Xport(arg);
	case e_tester:
		return ARG_Tester(arg);
	case e_mock:
		return ARG_Mock(arg);
	case e_etherweather:
		return ARG_EtherWeather(arg);
	case e_announce:
		Globals.announce_name = owstrdup(arg);
		break;
	case e_allow_other:		/* allow_other */
		break;
		// Pressure scale
	case e_pressure_mbar: 
	case e_pressure_atm: 
	case e_pressure_mmhg: 
	case e_pressure_inhg: 
	case e_pressure_psi: 
	case e_pressure_Pa:
		set_controlflags(&LocalControlFlags, PRESSURESCALE_MASK, PRESSURESCALE_BIT, option_char-e_pressure_mbar);
		break;
		// TIMEOUTS
	case e_timeout_volatile:
	case e_timeout_stable:
	case e_timeout_directory:
	case e_timeout_presence:
	case e_timeout_serial:
	case e_timeout_usb:
	case e_timeout_network:
	case e_timeout_server:
	case e_timeout_ftp:
	case e_timeout_ha7:
	case e_timeout_w1:
	case e_timeout_persistent_low:
	case e_timeout_persistent_high:
	case e_clients_persistent_low:
	case e_clients_persistent_high:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		// Using the character as a numeric value -- convenient but risky
		(&Globals.timeout_volatile)[option_char - e_timeout_volatile] = (int) arg_to_integer;
		break;
	case e_baud:
		if (OW_parsevalue_I(&arg_to_integer, arg)) {
			return 1;
		}
		Globals.baud = COM_MakeBaud( arg_to_integer ) ;
		break ;
	case e_templow:
		if (OW_parsevalue_F(&arg_to_float, arg)) {
			return 1;
		}
		Globals.templow = arg_to_float;
		break;
	case e_temphigh:
		if (OW_parsevalue_F(&arg_to_float, arg)) {
			return 1;
		}
		Globals.temphigh = arg_to_float;
		break;
	case e_safemode:
		LocalControlFlags |= SAFEMODE ;
		break;
	case 0:
		break;
	default:
		return 1;
	}
	return 0;
}

static int OW_parsevalue_I(long long int *var, const ASCII * str)
{
	errno = 0;
	var[0] = strtol(str, NULL, 10);
	if (errno) {
		ERROR_DETAIL("Bad integer configuration value %s", str);
		return 1;
	}
	return 0;
}

static int OW_parsevalue_F(_FLOAT *var, const ASCII * str)
{
	errno = 0;
	var[0] = (_FLOAT) strtod(str, NULL);
	if (errno) {
		ERROR_DETAIL("Bad floating point configuration value %s", str);
		return 1;
	}
	return 0;
}
