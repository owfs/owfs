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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

char * return_code_strings[] = {
	"Good result", // 0
	"Startup - command line parameters invalid",
	"legacy - No such entity",
	"Startup - a device could not be opened",
	"legacy - Interrupted",
	"legacy - IO error", // 5
	"Startup - network could not be opened",
	"Startup - Avahi library could not be loaded",
	"Startup - Bonjour library could not be loaded",
	"legacy - Bad filesystem",
	"Startup - zeroconf communication problem", // 10
	"legacy - Temporary interruption",
	"legacy - memory exhausted",
	"legacy - Access error",
	"legacy - Communication fault",
	"Startup - memory could not be allocated", // 15
	"legacy - Busy",
	"Program - not initialized",
	"Program - not yet ready",
	"legacy - No such device",
	"legacy - Not a directory", // 20
	"legacy - Is a directory",
	"legacy - Invalid transaction",
	"Program - closing",
	"Program - closed",
	"Program - cannot close", // 25
	"Path - input path too long",
	"Path - bad path syntax",
	"Device - Device name bad CRC8",
	"Device - Unrecognized device name or alias",
	"legacy - Read-only file system", // 30
	"Device - alias name too long",
	"Device - unrecognized device property",
	"Device - device property not an array",
	"legacy - Out of range",
	"Device - device property should be an array", // 35
	"legacy - Name too long",
	"Device - device not a bit field",
	"Device - array index out of range",
	"Device - device property not a subpath",
	"legacy - Loop discovered", // 40
	"Device - device not found",
	"legacy - No message",
	"Device - other device error",
	"Bus - bus short",
	"Bus - no such bus", // 45
	"Bus - bus not appropriate",
	"Bus - bus not responding",
	"Bus - bus being reset",
	"Bus - bus closed",
	"Bus - bus could not be opened", // 50
	"Bus - communication error on bus",
	"Bus - communication timeout",
	"Bus - telnet error",
	"Bus - tcp error",
	"Bus - bus is local", // 55
	"Bus - bus is remote",
	"Read - data too large",
	"Read - data communication error",
	"Read - not a property",
	"Read - not a readable property", // 60
	"Write - data too large",
	"Write - data too small",
	"Write - data wrong format",
	"Write - not a property",
	"Write - not a writable property", // 65
	"Write - read-only mode",
	"Write - data communication error",
	"Directory - output path too long",
	"Directory - not a directory", // 69
	"Presence - not a device", // 70
	"Unknown query type",
	"Owserver protocol - socket problem",
	"Owserver protocol - timeout",
	"legacy - Bad message",
	"Owserver protocol - version mismatch", // 75
	"Owserver protocol - packet size error",
	"Path - extra text in path",
	"Internal - unexpected null pointer",
	"Internal - unable to allocate memory",
	"Unassigned error 80", // 80
	"Unassigned error 81",
	"Unassigned error 82",
	"Unassigned error 83",
	"Unassigned error 84",
	"Unassigned error 85", // 85
	"Unassigned error 86",
	"Unassigned error 87",
	"Unassigned error 88",
	"Unassigned error 89",
	"legacy - Message size problem", // 90
	"Unassigned error 91",
	"Unassigned error 92",
	"Unassigned error 93",
	"Unassigned error 94",
	"legacy - Not supported", // 95
	"Unassigned error 96",
	"Unassigned error 97",
	"legacy - Address in use",
	"legacy - Address not available",
	"Unassigned error 100", // 100
	"Unassigned error 101",
	"Unassigned error 102",
	"legacy - Connection aborted",
	"Unassigned error 104",
	"legacy - No buffers", // 105
	"Unassigned error 106",
	"Unassigned error 107",
	"Unassigned error 108",
	"Unassigned error 109",
	"Unassigned error 110", // 110
	"Unassigned error 111",
	"Unassigned error 112",
	"Unassigned error 113",
	"Unassigned error 114",
	"Unassigned error 115", // 115
	"Unassigned error 116",
	"Unassigned error 117",
	"Unassigned error 118",
	"Unassigned error 119",
	"Unassigned error 120", // 120
	"Unassigned error 121",
	"Unassigned error 122",
	"Unassigned error 123",
	"Unassigned error 124",
	"Unassigned error 125", // 125
	"Unassigned error 126",
	"Unassigned error 127",
	"Unassigned error 128",
	"Unassigned error 129",
	"Unassigned error 130", // 130
	"Unassigned error 131",
	"Unassigned error 132",
	"Unassigned error 133",
	"Unassigned error 134",
	"Unassigned error 135", // 135
	"Unassigned error 136",
	"Unassigned error 137",
	"Unassigned error 138",
	"Unassigned error 139",
	"Unassigned error 140", // 140
	"Unassigned error 141",
	"Unassigned error 142",
	"Unassigned error 143",
	"Unassigned error 144",
	"Unassigned error 145", // 145
	"Unassigned error 146",
	"Unassigned error 147",
	"Unassigned error 148",
	"Unassigned error 149",
	"Unassigned error 150", // 150
	"Unassigned error 151",
	"Unassigned error 152",
	"Unassigned error 153",
	"Unassigned error 154",
	"Unassigned error 155", // 155
	"Unassigned error 156",
	"Unassigned error 157",
	"Unassigned error 158",
	"Unassigned error 159",
	"Unassigned error 160", // 160
	"Unassigned error 161",
	"Unassigned error 162",
	"Unassigned error 163",
	"Unassigned error 164",
	"Unassigned error 165", // 165
	"Unassigned error 166",
	"Unassigned error 167",
	"Unassigned error 168",
	"Unassigned error 169",
	"Unassigned error 170", // 170
	"Unassigned error 171",
	"Unassigned error 172",
	"Unassigned error 173",
	"Unassigned error 174",
	"Unassigned error 175", // 175
	"Unassigned error 176",
	"Unassigned error 177",
	"Unassigned error 178",
	"Unassigned error 179",
	"Unassigned error 180", // 180
	"Unassigned error 181",
	"Unassigned error 182",
	"Unassigned error 183",
	"Unassigned error 184",
	"Unassigned error 185", // 185
	"Unassigned error 186",
	"Unassigned error 187",
	"Unassigned error 188",
	"Unassigned error 189",
	"Unassigned error 190", // 190
	"Unassigned error 191",
	"Unassigned error 192",
	"Unassigned error 193",
	"Unassigned error 194",
	"Unassigned error 195", // 195
	"Unassigned error 196",
	"Unassigned error 197",
	"Unassigned error 198",
	"Unassigned error 199",
	"Unassigned error 200", // 200
	"Unassigned error 201",
	"Unassigned error 202",
	"Unassigned error 203",
	"Unassigned error 204",
	"Unassigned error 205", // 205
	"Unassigned error 206",
	"Unassigned error 207",
	"Unassigned error 208",
	"Unassigned error 209",
	"Error number out of range", // 210 return_code_out_of_bounds
// Note: If you add more return_codes, make sure you adjust 
// #define return_code_out_of_bounds 210
// it's defined in ow_return_code.h
} ;

int return_code_calls[N_RETURN_CODES] ;

void Return_code_setup(void)
{
	int i ;
	for ( i=0 ; i<N_RETURN_CODES ; ++i ) {
		return_code_calls[i] = 0 ;
	}
}

void return_code_set( int raw_rc, struct parsedname * pn, const char * d_file, int d_line, const char * d_func )
{
	int rc = raw_rc ;
	if ( rc < 0 ) {
		// make positive
		rc = -rc ;
	}

	if ( pn->return_code != 0 ) {
	// Already set?
	#if OW_DEBUG
		if (Globals.error_level>=e_err_debug) {
			err_msg( e_err_type_level, e_err_debug, d_file, d_line, d_func, "%s: Resetting error from %d <%s> to %d",SAFESTRING(pn->path),pn->return_code,return_code_strings[pn->return_code],rc); 
		}
	#endif
	}

	if ( rc > return_code_out_of_bounds || rc < 0 ) {
	// Out of bounds?
	#if OW_DEBUG
		if (Globals.error_level>=e_err_debug) {
			err_msg( e_err_type_level, e_err_debug, d_file, d_line, d_func, "%s: Reset out of bounds error from %d to %d <%s>",SAFESTRING(pn->path),rc,return_code_out_of_bounds,return_code_strings[return_code_out_of_bounds]); 
		}
	#endif
		pn->return_code = return_code_out_of_bounds ;
		++ return_code_calls[return_code_out_of_bounds] ;
	} else {
	// Error and success
		pn->return_code = rc ;
		++ return_code_calls[rc] ;
		if ( rc != 0 ) {
		// Error found
			-- return_code_calls[0] ;
		#if OW_DEBUG
			if (Globals.error_level>=e_err_debug) {
				err_msg( e_err_type_level, e_err_debug, d_file, d_line, d_func, "%s: Set error to %d <%s>",SAFESTRING(pn->path),rc,return_code_strings[rc]); 
			}
		#endif
		}
	}
}

void return_code_set_scalar( int raw_rc, int * pi, const char * d_file, int d_line, const char * d_func )
{
	int rc = raw_rc ;
	if ( rc < 0 ) {
		// make positive
		rc = -rc ;
	}

	if ( rc > return_code_out_of_bounds || rc < 0 ) {
	// Out of bounds?
	//printf("Return code out of bounds\n");
	#if OW_DEBUG
		if (Globals.error_level>=e_err_debug) {
			err_msg( e_err_type_level, e_err_debug, d_file, d_line, d_func, "Reset out of bounds error from %d to %d <%s>",rc,return_code_out_of_bounds,return_code_strings[return_code_out_of_bounds]);
		}
	#endif
		*pi = return_code_out_of_bounds ;
		++ return_code_calls[return_code_out_of_bounds] ;
	} else {
	// Error and success
	//printf("Return code in bounds\n");
		*pi = rc ;
		++ return_code_calls[rc] ;
		if ( rc != 0 ) {
			//printf("Return code still non-zero\n");
			// Error found
			-- return_code_calls[0] ;
		#if OW_DEBUG
			if (Globals.error_level>=e_err_debug) {
				err_msg( e_err_type_level, e_err_debug, d_file, d_line, d_func, "Set error to %d <%s>",rc,return_code_strings[rc]); 
			}
		#endif
		}
	}
	//printf("Return code done\n");
}
