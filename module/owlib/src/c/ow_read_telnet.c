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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "telnet.h"

/* Telnet handling concepts from Jerry Scharf:

You should request the number of bytes you expect. When you scan it,
start looking for FF FA codes. If that shows up, see if the F0 is in the
read buffer. If so, move the char pointer back to where the FF point was
and ask for the rest of the string (expected - FF...F0 bytes.) If the F0
isn't in the read buffer, start reading byte by byte into a separate
variable looking for F0. Once you find that, then do the move the
pointer and read the rest piece as above. Finally, don't forget to scan
the rest of the strings for more FF FA blocks. It's most sloppy when the
FF is the last character of the initial read buffer...

You can also scan and remove the patterns FF F1 - FF F9 as these are 2
byte commands that the transmitter can send at any time. It is also
possible that you could see FF FB xx - FF FE xx 3 byte codes, but this
would be in response to FF FA codes that you would send, so that seems
unlikely. Handling these would be just the same as the FF FA codes above.

*/

/* Read from a telnet device
*/
GOOD_OR_BAD telnet_read(BYTE * buf, const size_t size, struct connection_in *in)
{
	struct port_in * pin ;
	// temporary buffer (add some extra space)
	BYTE readin_buf[size+2] ;

	// state machine for telnet escape chars
	// handles TELNET protocol, specifically RFC854
	// http://www.ietf.org/rfc/rfc854.txt
	enum {
		telnet_regular,
		telnet_iac,
		telnet_sb,
		telnet_sb_opt,
		telnet_sb_val,
		telnet_sb_iac,
		telnet_will,
		telnet_wont,
		telnet_do,
		telnet_dont,
	} telnet_read_state = telnet_regular ;

	size_t actual_readin = 0 ;
	size_t current_index = 0 ;
	size_t still_needed = size ;

	// test inputs
	if ( size == 0 ) {
		return gbGOOD ;
	}
	
	if ( in == NO_CONNECTION ) {
		return gbBAD ;
	}
	pin = in->pown ;

	if ( FILE_DESCRIPTOR_NOT_VALID(pin->file_descriptor) ) {
		return gbBAD ;
	}
	
	// loop and look for escape sequances
	while ( still_needed > 0 ) {

		// see if the state requires a longer read than currently scheduled
		size_t minimum_chars  = still_needed ;
		switch( telnet_read_state ) {
			case telnet_sb:
				minimum_chars += 4 ;
				break ;
			case telnet_sb_opt:
				minimum_chars += 3 ;
				break ;
			case telnet_sb_val:
				minimum_chars += 2 ;
				break ;
			case telnet_iac:
			case telnet_sb_iac:
			case telnet_will:
			case telnet_wont:
			case telnet_do:
			case telnet_dont:
				minimum_chars += 1 ;
				break ;
			case telnet_regular:
				break ;
		}

		if ( current_index >= actual_readin ) {
			// need to read more -- just read what we think we need -- escape chars may require repeat
			if ( tcp_read( pin->file_descriptor, readin_buf, minimum_chars, &(pin->timeout), &actual_readin) < 0 ) {
				LEVEL_DEBUG("tcp seems closed") ;
				Test_and_Close( &(pin->file_descriptor) ) ;
				return gbBAD ;
			}

			if (actual_readin < minimum_chars) {
				LEVEL_CONNECT("Telnet (ethernet) error");
				Test_and_Close( &(pin->file_descriptor) ) ;
				return gbBAD;
			}

			current_index = 0 ;
		}

		switch ( telnet_read_state ) {
			case telnet_regular :
				if ( readin_buf[current_index] == TELNET_IAC ) {
#if OW_SHOW_TRAFFIC
					LEVEL_DEBUG("TELNET: IAC");
#endif /* OW_SHOW_TRAFFIC */
					// starting escape sequence
					// following bytes will better characterize
					telnet_read_state = telnet_iac ;
				} else {
					// normal processing
					// move byte to response and decrement needed bytes
					// stay in current state
					buf[size - still_needed] = readin_buf[current_index] ;
					-- still_needed ;
				}
				break ;
			case telnet_iac:
				//printf("TELNET: IAC %d\n",readin_buf[current_index]);
				switch ( readin_buf[current_index] ) {
					case TELNET_EOF:
					case TELNET_SUSP:
					case TELNET_ABORT:
					case TELNET_EOR:
					case TELNET_SE:
					case TELNET_NOP:
					case TELNET_DM:
					case TELNET_BREAK:
					case TELNET_IP:
					case TELNET_AO:
					case TELNET_AYT:
					case TELNET_EC:
					case TELNET_EL:
					case TELNET_GA:
						// 2 byte sequence
						// just read 2nd character
#if OW_SHOW_TRAFFIC
						LEVEL_DEBUG("TELNET: End 2-byte sequence");
#endif /* OW_SHOW_TRAFFIC */
						telnet_read_state = telnet_regular ;
						break ;
					case TELNET_SB:
						// multibyte squence
						// start scanning for 0xF0
						telnet_read_state = telnet_sb ;
						break ;
					case TELNET_WILL:
						// 3 byte sequence
						// just read 2nd char
						telnet_read_state = telnet_will ;
						break ;
					case TELNET_WONT:
						// 3 byte sequence
						// just read 2nd char
						telnet_read_state = telnet_wont ;
						break ;
					case TELNET_DO:
						// 3 byte sequence
						// just read 2nd char
						telnet_read_state = telnet_do ;
						break ;
					case TELNET_DONT:
						// 3 byte sequence
						// just read 2nd char
						telnet_read_state = telnet_dont ;
						break ;
					case TELNET_IAC:
						// escape the FF character
						// make this a single regular FF char
						buf[size - still_needed] = 0xFF ;
						-- still_needed ;
#if OW_SHOW_TRAFFIC
						LEVEL_DEBUG("TELNET: FF escape sequence");
#endif /* OW_SHOW_TRAFFIC */
						telnet_read_state = telnet_regular ;
						break ;
					default:
						LEVEL_DEBUG("Unexpected telnet sequence");
						return gbBAD ;
				}
				break ;
			case telnet_sb:
				switch ( readin_buf[current_index] ) {
					case TELNET_IAC:
						LEVEL_DEBUG("Unexpected telnet sequence");
						return gbBAD ;
					default:
						//printf("TELNET: IAC SB opt=%d\n",readin_buf[current_index]);
						// stay in this mode
						telnet_read_state = telnet_sb_opt ;
						break ;
				}
				break ;
			case telnet_sb_opt:
				switch ( readin_buf[current_index] ) {
					case TELNET_IAC:
						LEVEL_DEBUG("Unexpected telnet sequence");
						return gbBAD ;
					default:
						//printf("TELNET: IAC SB sub_opt=%d\n",readin_buf[current_index]);
						// stay in this mode
						telnet_read_state = telnet_sb_val ;
						break ;
				}
				break ;
			case telnet_sb_val:
				switch ( readin_buf[current_index] ) {
					case TELNET_IAC:
						// stay in this mode
						telnet_read_state = telnet_sb_iac ;
						break ;
					default:
						//printf("TELNET: IAC SB val=%d\n",readin_buf[current_index]);
						// stay in this mode
						break ;
				}
				break ;
			case telnet_sb_iac:
				switch ( readin_buf[current_index] ) {
					case TELNET_SE:
						//printf("TELNET: IAC SE\n");
#if OW_SHOW_TRAFFIC
						LEVEL_DEBUG("TELNET: End multi-byte sequence");
#endif /* OW_SHOW_TRAFFIC */
						telnet_read_state = telnet_regular ;
						break ;					
					default:
						LEVEL_DEBUG("Unexpected telnet sequence");
						return gbBAD ;
				}
				break ;
			case telnet_will:
				//printf("TELNET: IAC WILL %d\n",readin_buf[current_index]);
				// 3 byte sequence
				// now reading 3rd char
#if OW_SHOW_TRAFFIC
				LEVEL_DEBUG("TELNET: End 3-byte sequence");
#endif /* OW_SHOW_TRAFFIC */
				telnet_read_state = telnet_regular ;
				break ;
			case telnet_wont:
				//printf("TELNET: IAC WONT %d\n",readin_buf[current_index]);
				// 3 byte sequence
				// now reading 3rd char
#if OW_SHOW_TRAFFIC
				LEVEL_DEBUG("TELNET: End 3-byte sequence");
#endif /* OW_SHOW_TRAFFIC */
				telnet_read_state = telnet_regular ;
				break ;
			case telnet_do:
				//printf("TELNET: IAC DO %d\n",readin_buf[current_index]);
				// 3 byte sequence
				// now reading 3rd char
#if OW_SHOW_TRAFFIC
				LEVEL_DEBUG("TELNET: End 3-byte sequence");
#endif /* OW_SHOW_TRAFFIC */
				telnet_read_state = telnet_regular ;
				break ;
			case telnet_dont:
				// 3 byte sequence
				// now reading 3rd char
#if OW_SHOW_TRAFFIC
				LEVEL_DEBUG("TELNET: End 3-byte sequence");
#endif /* OW_SHOW_TRAFFIC */
				telnet_read_state = telnet_regular ;
				break ;
		}
		++ current_index ;
	}
	return gbGOOD ;
}
