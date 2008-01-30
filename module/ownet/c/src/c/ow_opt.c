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
#include "ow_server.h"

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

const struct option owopts_long[] = {
	{"configuration", required_argument, NULL, 'c'},
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
	{"zero", no_argument, &Global.announce_off, 0},
	{"nozero", no_argument, &Global.announce_off, 1},
	{"autoserver", no_argument, &ow_Global.autoserver, 1},
	{"noautoserver", no_argument, &ow_Global.autoserver, 0},
	{"timeout_network", required_argument, NULL, 307,},	// timeout -- tcp wait
	{"timeout_server", required_argument, NULL, 308,},	// timeout -- server wait
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
			if ((c != 0) || (flag != NULL && flag != long_option_pointer->flag)) {
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

#if 0
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
#endif

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
	case 'r':
		Global.readonly = 1;
		break;
	case 'w':
		Global.readonly = 0;
		break;
	case 'C':
		set_semiglobal(&ow_Global.sg, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
		break;
	case 'F':
		set_semiglobal(&ow_Global.sg, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit);
		break;
	case 'R':
		set_semiglobal(&ow_Global.sg, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine);
		break;
	case 'K':
		set_semiglobal(&ow_Global.sg, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin);
		break;
	case 'V':
		printf("libownet version:\n\t" VERSION "\n");
		return 1;
	case 's':
		return OW_ArgNet(arg);
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
	case 307:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			ow_Global.timeout_network = (int) i;
		}
		break;
	case 308:
		{
			long long int i;
			if (OW_parsevalue(&i, arg))
				return 1;
			ow_Global.timeout_server = (int) i;
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
	struct connection_in *in = NewIn();
	if (in == NULL)
		return 1;
	in->name = strdup(arg);
	in->busmode = bus_server;
	return 0;
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
