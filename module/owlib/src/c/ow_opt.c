/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_opt -- owlib specific command line options processing */

#define _GNU_SOURCE
#include <stdio.h> // for getline
#undef _GNU_SOURCE

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_pid.h"
#include "ow_external.h"

struct lineparse {
	char * line;
	size_t line_length ;
	const ASCII *file;
	ASCII *prog;
	ASCII *opt;
	ASCII *val;
	int reverse_prog;
	int c;
	int line_number;
};

static void ParseTheLine(struct lineparse *lp);
static GOOD_OR_BAD ConfigurationFile(const ASCII * file);
static int ParseInterp(struct lineparse *lp);
static GOOD_OR_BAD OW_parsevalue_I(long long int *var, const ASCII * str);
static GOOD_OR_BAD OW_parsevalue_F(_FLOAT *var, const ASCII * str);

#define NO_LINKED_VAR NULL

const struct option owopts_long[] = {
	{"alias", required_argument, NO_LINKED_VAR, 'a'},
	{"aliases", required_argument, NO_LINKED_VAR, 'a'},
	{"aliasfile", required_argument, NO_LINKED_VAR, 'a'},
	{"configuration", required_argument, NO_LINKED_VAR, 'c'},
	{"device", required_argument, NO_LINKED_VAR, 'd'},
	{"usb", optional_argument, NO_LINKED_VAR, 'u'},
	{"USB", optional_argument, NO_LINKED_VAR, 'u'},
	{"help", optional_argument, NO_LINKED_VAR, 'h'},
	{"port", required_argument, NO_LINKED_VAR, 'p'},
	{"mountpoint", required_argument, NO_LINKED_VAR, 'm'},
	{"server", required_argument, NO_LINKED_VAR, 's'},
	{"readonly", no_argument, NO_LINKED_VAR, 'r'},
	{"safemode", no_argument, NO_LINKED_VAR, e_safemode},
	{"safe_mode", no_argument, NO_LINKED_VAR, e_safemode},
	{"safe", no_argument, NO_LINKED_VAR, e_safemode},
	{"write", no_argument, NO_LINKED_VAR, 'w'},

	{"Celsius", no_argument, NO_LINKED_VAR, 'C'},
	{"celsius", no_argument, NO_LINKED_VAR, 'C'},
	{"Fahrenheit", no_argument, NO_LINKED_VAR, 'F'},
	{"fahrenheit", no_argument, NO_LINKED_VAR, 'F'},
	{"Kelvin", no_argument, NO_LINKED_VAR, 'K'},
	{"kelvin", no_argument, NO_LINKED_VAR, 'K'},
	{"Rankine", no_argument, NO_LINKED_VAR, 'R'},
	{"rankine", no_argument, NO_LINKED_VAR, 'R'},

	{"mbar", no_argument, NO_LINKED_VAR, e_pressure_mbar},
	{"millibar", no_argument, NO_LINKED_VAR, e_pressure_mbar},
	{"mBar", no_argument, NO_LINKED_VAR, e_pressure_mbar},
	{"atm", no_argument, NO_LINKED_VAR, e_pressure_atm},
	{"mmhg", no_argument, NO_LINKED_VAR, e_pressure_mmhg},
	{"mmHg", no_argument, NO_LINKED_VAR, e_pressure_mmhg},
	{"torr", no_argument, NO_LINKED_VAR, e_pressure_mmhg},
	{"inhg", no_argument, NO_LINKED_VAR, e_pressure_inhg},
	{"inHg", no_argument, NO_LINKED_VAR, e_pressure_inhg},
	{"psi", no_argument, NO_LINKED_VAR, e_pressure_psi},
	{"psia", no_argument, NO_LINKED_VAR, e_pressure_psi},
	{"psig", no_argument, NO_LINKED_VAR, e_pressure_psi},
	{"Pa", no_argument, NO_LINKED_VAR, e_pressure_Pa},
	{"pa", no_argument, NO_LINKED_VAR, e_pressure_Pa},
	{"pascal", no_argument, NO_LINKED_VAR, e_pressure_Pa},

	{"version", no_argument, NO_LINKED_VAR, 'V'},
	{"format", required_argument, NO_LINKED_VAR, 'f'},
	{"pid_file", required_argument, NO_LINKED_VAR, 'P'},
	{"pid-file", required_argument, NO_LINKED_VAR, 'P'},
	
	{"unaliased", no_argument, &Globals.unaliased, 1 },
	{"aliased", no_argument, &Globals.unaliased, 0 },

	{"uncached", no_argument, &Globals.uncached, 1 },
	{"cached", no_argument, &Globals.uncached, 0 },

	{"external", no_argument, &Globals.allow_external, 1},
	{"no_external", no_argument, &Globals.allow_external, 0},

	{"background", no_argument, NO_LINKED_VAR, e_want_background},
	{"foreground", no_argument, NO_LINKED_VAR, e_want_foreground},
	{"fatal_debug", no_argument, &Globals.fatal_debug, 1},
	{"fatal-debug", no_argument, &Globals.fatal_debug, 1},
	{"fatal_debug_file", required_argument, NO_LINKED_VAR, e_fatal_debug_file},
	{"fatal-debug-file", required_argument, NO_LINKED_VAR, e_fatal_debug_file},
	{"error_print", required_argument, NO_LINKED_VAR, e_error_print},
	{"error-print", required_argument, NO_LINKED_VAR, e_error_print},
	{"errorprint", required_argument, NO_LINKED_VAR, e_error_print},
	{"error_level", required_argument, NO_LINKED_VAR, e_error_level},
	{"error-level", required_argument, NO_LINKED_VAR, e_error_level},
	{"errorlevel", required_argument, NO_LINKED_VAR, e_error_level},
	{"debug", no_argument, NO_LINKED_VAR, e_debug},
	{"concurrent_connections", required_argument, NO_LINKED_VAR, e_concurrent_connections},
	{"concurrent-connections", required_argument, NO_LINKED_VAR, e_concurrent_connections},
	{"max-connections", required_argument, NO_LINKED_VAR, e_concurrent_connections},
	{"max_connections", required_argument, NO_LINKED_VAR, e_concurrent_connections},
	{"cache_size", required_argument, NO_LINKED_VAR, e_cache_size},	/* max cache size */
	{"cache-size", required_argument, NO_LINKED_VAR, e_cache_size},	/* max cache size */
	{"cachesize", required_argument, NO_LINKED_VAR, e_cache_size},	/* max cache size */
	{"fuse_opt", required_argument, NO_LINKED_VAR, e_fuse_opt},	/* owfs, fuse mount option */
	{"fuse-opt", required_argument, NO_LINKED_VAR, e_fuse_opt},	/* owfs, fuse mount option */
	{"fuseopt", required_argument, NO_LINKED_VAR, e_fuse_opt},	/* owfs, fuse mount option */
	{"fuse_open_opt", required_argument, NO_LINKED_VAR, e_fuse_open_opt},	/* owfs, fuse open option */
	{"fuse-open-opt", required_argument, NO_LINKED_VAR, e_fuse_open_opt},	/* owfs, fuse open option */
	{"fuseopenopt", required_argument, NO_LINKED_VAR, e_fuse_open_opt},	/* owfs, fuse open option */
	{"max_clients", required_argument, NO_LINKED_VAR, e_max_clients},	/* ftp max connections */
	{"max-clients", required_argument, NO_LINKED_VAR, e_max_clients},	/* ftp max connections */
	{"maxclients", required_argument, NO_LINKED_VAR, e_max_clients},	/* ftp max connections */

	{"passive", required_argument, NO_LINKED_VAR, e_passive},	/* DS9097 passive */
	{"PASSIVE", required_argument, NO_LINKED_VAR, e_passive},	/* DS9097 passive */
	{"i2c", optional_argument, NO_LINKED_VAR, e_i2c},	/* i2c adapters */
	{"I2C", optional_argument, NO_LINKED_VAR, e_i2c},	/* i2c adapters */
	{"HA7", optional_argument, NO_LINKED_VAR, e_ha7},	/* HA7Net */
	{"ha7", optional_argument, NO_LINKED_VAR, e_ha7},	/* HA7Net */
	{"HA7NET", optional_argument, NO_LINKED_VAR, e_ha7},
	{"HA7Net", optional_argument, NO_LINKED_VAR, e_ha7},
	{"ha7net", optional_argument, NO_LINKED_VAR, e_ha7},
	{"enet", optional_argument, NO_LINKED_VAR, e_enet},
	{"Enet", optional_argument, NO_LINKED_VAR, e_enet},
	{"ENET", optional_argument, NO_LINKED_VAR, e_enet},
	{"OW-Server-Enet", optional_argument, NO_LINKED_VAR, e_enet},
	{"FAKE", required_argument, NO_LINKED_VAR, e_fake},	/* Fake */
	{"Fake", required_argument, NO_LINKED_VAR, e_fake},	/* Fake */
	{"fake", required_argument, NO_LINKED_VAR, e_fake},	/* Fake */
	{"link", required_argument, NO_LINKED_VAR, e_link},	/* link in ascii mode */
	{"LINK", required_argument, NO_LINKED_VAR, e_link},	/* link in ascii mode */
	{"PBM", required_argument, NO_LINKED_VAR, e_pbm}, /* Dirk Opfer's elabnet "power bus master" */
	{"pbm", required_argument, NO_LINKED_VAR, e_pbm}, /* Dirk Opfer's elabnet "power bus master" */
	{"HA3", required_argument, NO_LINKED_VAR, e_ha3},
	{"ha3", required_argument, NO_LINKED_VAR, e_ha3},
	{"HA4B", required_argument, NO_LINKED_VAR, e_ha4b},
	{"HA4b", required_argument, NO_LINKED_VAR, e_ha4b},
	{"ha4b", required_argument, NO_LINKED_VAR, e_ha4b},
	{"HA5", required_argument, NO_LINKED_VAR, e_ha5},
	{"ha5", required_argument, NO_LINKED_VAR, e_ha5},
	{"ha7e", required_argument, NO_LINKED_VAR, e_ha7e},
	{"HA7e", required_argument, NO_LINKED_VAR, e_ha7e},
	{"HA7E", required_argument, NO_LINKED_VAR, e_ha7e},
	{"ha7s", required_argument, NO_LINKED_VAR, e_ha7e},
	{"HA7s", required_argument, NO_LINKED_VAR, e_ha7e},
	{"HA7S", required_argument, NO_LINKED_VAR, e_ha7e},
	{"xport", required_argument, NO_LINKED_VAR, e_xport, },
	{"telnet", required_argument, NO_LINKED_VAR, e_xport, },
	{"TELNET", required_argument, NO_LINKED_VAR, e_xport, },
	{"Xport", required_argument, NO_LINKED_VAR, e_xport, },
	{"XPort", required_argument, NO_LINKED_VAR, e_xport, },
	{"XPORT", required_argument, NO_LINKED_VAR, e_xport, },
	{"telnet", required_argument, NO_LINKED_VAR, e_xport, },
	{"TESTER", required_argument, NO_LINKED_VAR, e_tester},	/* Tester */
	{"Tester", required_argument, NO_LINKED_VAR, e_tester},	/* Tester */
	{"tester", required_argument, NO_LINKED_VAR, e_tester},	/* Tester */
	{"mock", required_argument, NO_LINKED_VAR, e_mock},	/* Mock */
	{"Mock", required_argument, NO_LINKED_VAR, e_mock},	/* Mock */
	{"MOCK", required_argument, NO_LINKED_VAR, e_mock},	/* Mock */
	{"etherweather", required_argument, NO_LINKED_VAR, e_etherweather},	/* EtherWeather */
	{"EtherWeather", required_argument, NO_LINKED_VAR, e_etherweather},	/* EtherWeather */
	{"zero", no_argument, &Globals.announce_off, 0},
	{"nozero", no_argument, &Globals.announce_off, 1},
	{"autoserver", no_argument, NO_LINKED_VAR, e_browse},
	{"auto", no_argument, NO_LINKED_VAR, e_browse},
	{"AUTO", no_argument, NO_LINKED_VAR, e_browse},
	{"browse", no_argument, NO_LINKED_VAR, e_browse},
	{"w1", no_argument, NO_LINKED_VAR, e_w1_monitor},
	{"W1", no_argument, NO_LINKED_VAR, e_w1_monitor},
	{"announce", required_argument, NO_LINKED_VAR, e_announce},
	{"allow_other", no_argument, &Globals.allow_other, 1},
	{"altUSB", no_argument, &Globals.altUSB, 1},	/* Willy Robison's tweaks */
	{"altusb", no_argument, &Globals.altUSB, 1},	/* Willy Robison's tweaks */
	{"usb_flextime", no_argument, &Globals.usb_flextime, 1},
	{"USB_flextime", no_argument, &Globals.usb_flextime, 1},
	{"usb_regulartime", no_argument, &Globals.usb_flextime, 0},
	{"USB_regulartime", no_argument, &Globals.usb_flextime, 0},
	{"usb_scan", optional_argument, NO_LINKED_VAR, e_usb_monitor,},
	{"USB_scan", optional_argument, NO_LINKED_VAR, e_usb_monitor,},
	{"serial_flex", no_argument, &Globals.serial_flextime, 1},
	{"serial_flextime", no_argument, &Globals.serial_flextime, 1},
	{"serial_regulartime", no_argument, &Globals.serial_flextime, 0},
	{"serial_regular", no_argument, &Globals.serial_flextime, 0},
	{ "no_hard", no_argument, &Globals.serial_hardflow, 0 }, // hardware flow control
	{ "flow_none", no_argument, &Globals.serial_hardflow, 0 }, // hardware flow control
	{ "no_hardflow", no_argument, &Globals.serial_hardflow, 0 }, // hardware flow control
	{ "hard", no_argument, &Globals.serial_hardflow, 1 }, // hardware flow control
	{ "hardware", no_argument, &Globals.serial_hardflow, 1 }, // hardware flow control
	{ "hardflow", no_argument, &Globals.serial_hardflow, 1 }, // hardware flow control
	{"serial_standardtime", no_argument, &Globals.serial_flextime, 0},
	{"serial_standard", no_argument, &Globals.serial_flextime, 0},
	{"serial_stdtime", no_argument, &Globals.serial_flextime, 0},
	{"serial_std", no_argument, &Globals.serial_flextime, 0},
	{"reverse", no_argument, &Globals.serial_reverse, 1},
	{"reverse_polarity", no_argument, &Globals.serial_reverse, 1},
	{"straight_polarity", no_argument, &Globals.serial_reverse, 0},
	{"baud_rate", required_argument, NO_LINKED_VAR, e_baud},
	{"baud", required_argument, NO_LINKED_VAR, e_baud},
	{"Baud", required_argument, NO_LINKED_VAR, e_baud},
	{"BAUD", required_argument, NO_LINKED_VAR, e_baud},

	{"timeout_volatile", required_argument, NO_LINKED_VAR, e_timeout_volatile,},	// timeout -- changing cached values
	{"timeout_stable", required_argument, NO_LINKED_VAR, e_timeout_stable,},	// timeout -- unchanging cached values
	{"timeout_directory", required_argument, NO_LINKED_VAR, e_timeout_directory,},	// timeout -- direcory cached values
	{"timeout_presence", required_argument, NO_LINKED_VAR, e_timeout_presence,},	// timeout -- device location
	{"timeout_serial", required_argument, NO_LINKED_VAR, e_timeout_serial,},	// timeout -- serial wait (read and write)
	{"timeout_usb", required_argument, NO_LINKED_VAR, e_timeout_usb,},	// timeout -- usb wait
	{"timeout_network", required_argument, NO_LINKED_VAR, e_timeout_network,},	// timeout -- tcp wait
	{"timeout_server", required_argument, NO_LINKED_VAR, e_timeout_server,},	// timeout -- server wait
	{"timeout_ftp", required_argument, NO_LINKED_VAR, e_timeout_ftp,},	// timeout -- ftp wait
	{"timeout_HA7", required_argument, NO_LINKED_VAR, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_ha7", required_argument, NO_LINKED_VAR, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_HA7Net", required_argument, NO_LINKED_VAR, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_ha7net", required_argument, NO_LINKED_VAR, e_timeout_ha7,},	// timeout -- HA7Net wait
	{"timeout_w1", required_argument, NO_LINKED_VAR, e_timeout_w1,},	// timeout -- w1 netlink
	{"timeout_W1", required_argument, NO_LINKED_VAR, e_timeout_w1,},	// timeout -- w1 netlink
	{"timeout_persistent_low", required_argument, NO_LINKED_VAR, e_timeout_persistent_low,},
	{"timeout_persistent_high", required_argument, NO_LINKED_VAR, e_timeout_persistent_high,},
	{"clients_persistent_low", required_argument, NO_LINKED_VAR, e_clients_persistent_low,},
	{"clients_persistent_high", required_argument, NO_LINKED_VAR, e_clients_persistent_high,},

	{"temperature_low", required_argument, NO_LINKED_VAR, e_templow,},
	{"low_temperature", required_argument, NO_LINKED_VAR, e_templow,},
	{"templow", required_argument, NO_LINKED_VAR, e_templow,},

	{"temperature_high", required_argument, NO_LINKED_VAR, e_temphigh,},
	{"high_temperature", required_argument, NO_LINKED_VAR, e_temphigh,},
	{"temphigh", required_argument, NO_LINKED_VAR, e_temphigh,},
	{"temperature_hi", required_argument, NO_LINKED_VAR, e_temphigh,},
	{"hi_temperature", required_argument, NO_LINKED_VAR, e_temphigh,},
	{"temphi", required_argument, NO_LINKED_VAR, e_temphigh,},

	{"one_device", no_argument, &Globals.one_device, 1},
	{"1_device", no_argument, &Globals.one_device, 1},

	{"pingcrazy", no_argument, &Globals.pingcrazy, 1},
	{"no_dirall", no_argument, &Globals.no_dirall, 1},
	{"no_get", no_argument, &Globals.no_get, 1},
	{"no_persistence", no_argument, &Globals.no_persistence, 1},
	{"8bit", no_argument, &Globals.eightbit_serial, 1},
	{"6bit", no_argument, &Globals.eightbit_serial, 0},
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
	{"trim", no_argument, &Globals.trim, 1},
	{"TRIM", no_argument, &Globals.trim, 1},
	{"detail", required_argument, NO_LINKED_VAR, e_detail},	/* slave detail */
	{"details", required_argument, NO_LINKED_VAR, e_detail},	/* slave detail */
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
		// lp-> prog already in lower case
		if (strstr(lp->prog, "server") != NULL) {
			if ((Globals.program_type == program_type_server) == lp->reverse_prog) {
				return 0; // ignore this line -- doesn't apply
			}
		} else if (strstr(lp->prog, "external") != NULL) {
			if ((Globals.program_type == program_type_external) == lp->reverse_prog) {
				return 0; // ignore this line -- doesn't apply
			}
		} else if (strstr(lp->prog, "http") != NULL) {
			if ((Globals.program_type == program_type_httpd) == lp->reverse_prog) {
				return 0; // ignore this line -- doesn't apply
			}
		} else if (strstr(lp->prog, "ftp") != NULL) {
			if ((Globals.program_type == program_type_ftpd) == lp->reverse_prog) {
				return 0; // ignore this line -- doesn't apply
			}
		} else if (strstr(lp->prog, "fs") != NULL) {
			if ((Globals.program_type == program_type_filesystem) == lp->reverse_prog) {
				return 0; // ignore this line -- doesn't apply
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
// optional white space
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
				{	// put in lower case
					ASCII *prog_char;
					for (prog_char = lp->prog; *prog_char; ++prog_char) {
						*prog_char = tolower(*prog_char);
					}
				}
				// special cases for sensor and property lines
				// they use a different syntax
				if (strstr(lp->prog, "sensor") != NULL) {
					// sensor line for external device
					LEVEL_DEBUG("SENSOR entry found <%s>", current_char+1);
					lp->prog = NULL ;
					AddSensor(current_char+1) ;
					return ;
				} else if (strstr(lp->prog, "script") != NULL) {
					// property line for external device
					LEVEL_DEBUG("SCRIPT entry found <%s>", current_char+1);
					lp->prog = NULL ;
					AddProperty(current_char+1,et_script) ;
					return ;
				} else if (strstr(lp->prog, "property") != NULL) {
					// property line for external device
					LEVEL_DEBUG("PROPERTY (SCRIPT) entry found <%s>", current_char+1);
					lp->prog = NULL ;
					AddProperty(current_char+1,et_script) ;
					return ;
				}
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

static GOOD_OR_BAD ConfigurationFile(const ASCII * file)
{
	FILE *configuration_file_pointer = fopen(file, "r");
	if (configuration_file_pointer) {
		int ret = gbGOOD;

		struct lineparse lp;
		lp.line_number = 0;
		lp.file = file;
		lp.line = NULL ; // setup for getline

		while (getline(&(lp.line), &(lp.line_length), configuration_file_pointer)>=0) {
			++lp.line_number;
			ParseTheLine(&lp);
			if ( BAD( owopt(ParseInterp(&lp), lp.val) ) ) {
				ret = gbBAD ;
				break ;
			}
		}
		fclose(configuration_file_pointer);
		if ( lp.line != NULL) {
			free(lp.line) ; // not owfree
		}
		return ret;
	} else {
		ERROR_DEFAULT("Cannot process configuration file %s", file);
		return gbBAD;
	}
}

GOOD_OR_BAD owopt_packed(const char *params)
{
	char *params_copy;			// copy of the input line
	char *current_location_in_params_copy;	// pointers into the input line copy
	char *next_location_in_params_copy;	// pointers into the input line copy
	char **argv = NULL;
	int argc = 0;
	GOOD_OR_BAD ret = 0;
	int allocated = 0;
	int option_char;

	if (params == NULL) {
		return gbGOOD;
	}
	current_location_in_params_copy = params_copy = owstrdup(params);
	if (params_copy == NULL) {
		return gbBAD;
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
				ret = gbBAD;
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

	SAFEFREE(argv) ;
	owfree(params_copy);
	return ret;
}

/* Parses one argument */
/* return 0 if ok */
GOOD_OR_BAD owopt(const int option_char, const char *arg)
{
	// depth of nesting for config files
	static int config_depth = 0;

	// utility variables for parsing options
	long long int arg_to_integer ;
	_FLOAT arg_to_float ;

	//printf("Option %c (%d) Argument=%s\n",c,c,SAFESTRING(arg)) ;
	switch (option_char) {
	case 'a' :
		return ReadAliasFile(arg) ;
	case 'c':
		if (config_depth > 4) {
			LEVEL_DEFAULT("Configuration file layered too deeply (>%d)", config_depth);
			return gbBAD;
		} else {
			GOOD_OR_BAD ret;
			++config_depth;
			ret = ConfigurationFile(arg);
			--config_depth;
			return ret;
		}
	case 'h':
		FS_help(arg);
		return gbBAD;
	case 'u':
		return ARG_USB(arg);
	case 'd':
		return ARG_Device(arg);
	case 't':
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.timeout_volatile = (int) arg_to_integer;
		break;
	case 'r':
		Globals.readonly = 1;
		break;
	case 'w':
		Globals.readonly = 0;
		break;
	case 'C':
		Globals.temp_scale = temp_celsius ;
		SetLocalControlFlags() ;
		break;
	case 'F':
		Globals.temp_scale = temp_fahrenheit ;
		SetLocalControlFlags() ;
		break;
	case 'R':
		Globals.temp_scale = temp_rankine ;
		SetLocalControlFlags() ;
		break;
	case 'K':
		Globals.temp_scale = temp_kelvin ;
		SetLocalControlFlags() ;
		break;
	case 'V':
		printf("libow version:\n\t" VERSION "\n");
		return gbBAD;
	case 's':
		return ARG_Net(arg);
	case 'p':
		switch (Globals.program_type) {
		case program_type_httpd:
		case program_type_server:
		case program_type_external:
		case program_type_ftpd:
			return ARG_Server(arg);
		default:
			return gbGOOD;
		}
	case 'm':
		switch (Globals.program_type) {
		case program_type_filesystem:
			return ARG_Server(arg);
		default:
			return gbGOOD;
		}
	case 'f':
		if (!strcasecmp(arg, "f.i")) {
			Globals.format = fdi ;
		} else if (!strcasecmp(arg, "fi")) {
			Globals.format = fi ;
		} else if (!strcasecmp(arg, "f.i.c")) {
			Globals.format = fdidc ;
		} else if (!strcasecmp(arg, "f.ic")) {
			Globals.format = fdic ;
		} else if (!strcasecmp(arg, "fi.c")) {
			Globals.format = fidc ;
		} else if (!strcasecmp(arg, "fic")) {
			Globals.format = fic ;
		} else {
			LEVEL_DEFAULT("Unrecognized format type %s", arg);
			return gbBAD;
		}
		SetLocalControlFlags() ;
		break;
	case 'P':
		if (arg == NULL || strlen(arg) == 0) {
			LEVEL_DEFAULT("No PID file specified");
			return gbBAD;
		} else if ((pid_file = owstrdup(arg)) == NULL) {
			fprintf(stderr, "Insufficient memory to store the PID filename: %s", arg);
			return gbBAD;
		}
		break;
	case e_fatal_debug_file:
		if (arg == NULL || strlen(arg) == 0) {
			LEVEL_DEFAULT("No fatal_debug_file specified");
			return gbBAD;
		} else if ((Globals.fatal_debug_file = owstrdup(arg)) == NULL) {
			LEVEL_DEBUG("Out of memory.");
			return gbBAD;
		}
		break;
	case e_error_print:
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.error_print = (int) arg_to_integer;
		break;
	case e_error_level:
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.error_level = (int) arg_to_integer;
		break;
	case e_debug:
		// shortcut for --foreground --error_level=9
		printf("DEBUG MODE\n");
		printf("libow version:\n\t" VERSION "\n");
		switch (Globals.daemon_status) {
			case e_daemon_want_bg:
				Globals.daemon_status = e_daemon_fg ;
				break ;
			default:
				break ;
		}		
		Globals.error_level = 9 ;
		Globals.error_level_restore = 9 ;
		break ;
	case e_concurrent_connections:
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.concurrent_connections = (int) arg_to_integer;
		break;
	case e_cache_size:
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.cache_size = (size_t) arg_to_integer;
		break;
	case e_fuse_opt:			/* fuse_opt, handled in owfs.c */
		break;
	case e_fuse_open_opt:		/* fuse_open_opt, handled in owfs.c */
		break;
	case e_max_clients:
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.max_clients = (int) arg_to_integer;
		break;
	case e_want_background:
		switch (Globals.daemon_status) {
			case e_daemon_fg:
			case e_daemon_unknown:
				Globals.daemon_status = e_daemon_want_bg ;
				break ;
			default:
				break ;
		}		
		break ;
	case e_want_foreground:
		switch (Globals.daemon_status) {
			case e_daemon_want_bg:
			case e_daemon_unknown:
				Globals.daemon_status = e_daemon_fg ;
				break ;
			default:
				break ;
		}		
		break ;
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
	case e_pbm:
		return ARG_PBM(arg);
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
	case e_w1_monitor:
		return ARG_W1_monitor();
	case e_usb_monitor:
		return ARG_USB_monitor(arg);
	case e_browse:
		return ARG_Browse();
	case e_announce:
		Globals.announce_name = owstrdup(arg);
		break;
		// Pressure scale
	case e_pressure_mbar:
		Globals.pressure_scale = pressure_mbar ;
		SetLocalControlFlags() ;
		break ;
	case e_pressure_atm: 
		Globals.pressure_scale = pressure_atm ;
		SetLocalControlFlags() ;
		break ;
	case e_pressure_mmhg: 
		Globals.pressure_scale = pressure_mmhg ;
		SetLocalControlFlags() ;
		break ;
	case e_pressure_inhg: 
		Globals.pressure_scale = pressure_inhg ;
		SetLocalControlFlags() ;
		break ;
	case e_pressure_psi: 
		Globals.pressure_scale = pressure_psi ;
		SetLocalControlFlags() ;
		break ;
	case e_pressure_Pa:
		Globals.pressure_scale = pressure_Pa ;
		SetLocalControlFlags() ;
		break ;
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
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		// Using the character as a numeric value -- convenient but risky
		(&Globals.timeout_volatile)[option_char - e_timeout_volatile] = (int) arg_to_integer;
		break;
	case e_baud:
		RETURN_BAD_IF_BAD(OW_parsevalue_I(&arg_to_integer, arg)) ;
		Globals.baud = COM_MakeBaud( arg_to_integer ) ;
		break ;
	case e_templow:
		RETURN_BAD_IF_BAD(OW_parsevalue_F(&arg_to_float, arg)) ;
		Globals.templow = arg_to_float;
		break;
	case e_temphigh:
		RETURN_BAD_IF_BAD(OW_parsevalue_F(&arg_to_float, arg)) ;
		Globals.temphigh = arg_to_float;
		break;
	case e_safemode:
		LocalControlFlags |= SAFEMODE ;
		break ;
	case e_detail:
		return Detail_Add(arg) ;
		break;
	case 0:
		break;
	default:
		return gbBAD;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_parsevalue_I(long long int *var, const ASCII * str)
{
	errno = 0;
	var[0] = strtol(str, NULL, 10);
	if (errno) {
		ERROR_DETAIL("Bad integer configuration value %s", str);
		return gbBAD;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_parsevalue_F(_FLOAT *var, const ASCII * str)
{
	errno = 0;
	var[0] = (_FLOAT) strtod(str, NULL);
	if (errno) {
		ERROR_DETAIL("Bad floating point configuration value %s", str);
		return gbBAD;
	}
	return gbGOOD;
}
