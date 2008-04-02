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
    const ASCII *file;
    ASCII *prog;
    ASCII *opt;
    ASCII *val;
    int reverse_prog;
    int c;
    int line_number ;
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
	{"error_print", required_argument, NULL, 257},
	{"error-print", required_argument, NULL, 257},
	{"errorprint", required_argument, NULL, 257},
	{"error_level", required_argument, NULL, 258},
	{"error-level", required_argument, NULL, 258},
	{"errorlevel", required_argument, NULL, 258},
	{"autoserver", no_argument, &ow_Global.autoserver, 1},
	{"noautoserver", no_argument, &ow_Global.autoserver, 0},
	{"timeout_network", required_argument, NULL, 307,},	// timeout -- tcp wait
	{"timeout_server", required_argument, NULL, 308,},	// timeout -- server wait
	{0, 0, 0, 0},
};

// Called after "ParseTheLine" has filled the lineparse structure
static int ParseInterp(struct lineparse *lp)
{
    size_t option_name_length;
    const struct option *long_option_pointer;

    LEVEL_DEBUG("Configuration file (%s:%d) Program=%s%s, Option=%s, Value=%s\n",SAFESTRING(lp->file),lp->line_number,lp->reverse_prog?"Not ":"",SAFESTRING(lp->prog),SAFESTRING(lp->opt),SAFESTRING(lp->val)) ;
    // Check for program specification
    if ( lp->prog != NULL ) {
        ASCII * prog_char ;
        for ( prog_char = lp->prog ; *prog_char ; ++prog_char ) {
            *prog_char = tolower( *prog_char ) ;
        }
        if ( strstr( lp->prog, "server" ) != NULL && !lp->reverse_prog ) {
            return 0 ;
        } else if ( strstr( lp->prog, "http" ) != NULL  && !lp->reverse_prog ) {
            return 0 ;
        } else if ( strstr( lp->prog, "ftp" ) != NULL  && !lp->reverse_prog ) {
            return 0 ;
        } else if ( strstr( lp->prog, "fs" ) != NULL  && !lp->reverse_prog ) {
            return 0 ;
        } else {
            LEVEL_DEFAULT("Configuration file (%s:%d) Unrecognized program %s. Option=%s, Value=%s\n",SAFESTRING(lp->file),lp->line_number,SAFESTRING(lp->prog),SAFESTRING(lp->opt),SAFESTRING(lp->val)) ;
            return 0 ;
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
        if (strncasecmp(lp->opt, long_option_pointer->name, option_name_length) == 0 ) {
            LEVEL_DEBUG("Configuration file (%s:%d) Option %s recognized as %s. Value=%s\n",SAFESTRING(lp->file),lp->line_number,lp->opt,long_option_pointer->name,SAFESTRING(lp->val)) ;
            if ( long_option_pointer->flag != NULL) {   // immediate value mode
                //printf("flag c=%d flag=%p long_option_pointer->flag=%p\n",c,flag,long_option_pointer->flag);
                (long_option_pointer->flag)[0] = long_option_pointer->val;
                return 0 ;
            } else {
                return long_option_pointer->val;
            }
        }
    }
    LEVEL_DEFAULT("Configuration file (%s:%d) Unrecognized option %s. Value=%s\n",SAFESTRING(lp->file),lp->line_number,SAFESTRING(lp->opt),SAFESTRING(lp->val)) ;
    return 0 ;
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
    lp->reverse_prog = 0 ;
    
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
                    case ps_pre_value: // not ignored before values, of course
                        lp->val = current_char;
                        parse_state = ps_in_value;
                        break;
                    default:
                        break;
                }
                break ;
        // negate program state (even though program not yet assigned)
            case '!':
                switch (parse_state) {
                    case ps_pre_opt:
                        lp->reverse_prog = ! lp->reverse_prog ;
                        break;
                        case ps_pre_value: // not ignored before values, of course
                            lp->val = current_char;
                            parse_state = ps_in_value;
                            break;
                    default:
                        break;
                }
                break ;
        // colon special case -- assigns to a program
        // Since we had assumed this was an opt, we have to adjust
            case ':':
                switch (parse_state) {
                    case ps_in_opt:
                    case ps_pre_equals:
                        *current_char = '\0' ;
                        lp->prog = lp->opt ;
                        lp->opt = NULL ;
                        parse_state = ps_pre_opt;
                        break;
                        case ps_pre_value: // not ignored before values, of course
                            lp->val = current_char;
                            parse_state = ps_in_value;
                            break;
                    default:
                        break;
                }
                break ;
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
                        break ;
                        default:            // rogue '='
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
                        break ;
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
        lp.line_number = 0 ;
        lp.file = file ;
        
        while (fgets(lp.line, 256, configuration_file_pointer)) {
            ++lp.line_number ;
            // check line length
            if (strlen(lp.line) > 250) {
                LEVEL_DEFAULT("Configuration file (%s:%d) Line too long (>250 characters).\n", SAFESTRING(lp.file),lp.line_number);
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
        ERROR_DEFAULT("Cannot process configuration file %s\n", file);
        return 1;
    }
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
