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
#include "ow_counters.h"
#include "ow_connection.h"

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

static struct timeval tvnet = { 0, 300000, };


/* Read from a telnet device
*/
GOOD_OR_BAD telnet_read(BYTE * buf, const size_t size, const struct parsedname *pn)
{
	// temporary buffer (guess the extra escape chars based on prior experience
	size_t allocated_size = size + pn->selected_connection->default_discard ;
	BYTE readin_buf[allocated_size] ;

	// state machine for telnet escape chars
	// handles TELNET protocol, specifically RFC854
	// http://www.ietf.org/rfc/rfc854.txt
	enum { telnet_regular, telnet_ff, telnet_fffa, telnet_fffb, } telnet_read_state = telnet_regular ;

	size_t actual_readin ;
	size_t current_index = 0 ;
	size_t total_discard = 0 ;
	size_t still_needed = size ;
	
	// initial read
	//printf("LINKE_READ getting default_discard = %d\n",pn->selected_connection->default_discard);
	tcp_read(pn->selected_connection->file_descriptor, readin_buf, allocated_size, &tvnet, &actual_readin) ;
	if (actual_readin < size) {
		LEVEL_CONNECT("Telnet (ethernet) error");
		return gbBAD;
	}

	// loop and look for escape sequances
	while ( still_needed > 0 ) {
		if ( current_index >= allocated_size ) {
			// need to read more -- just read what we think we need -- escape chars may require repeat
			tcp_read(pn->selected_connection->file_descriptor, readin_buf, still_needed, &tvnet, &actual_readin) ;
			if (actual_readin != still_needed ) {
				LEVEL_CONNECT("Telnet (ethernet) error");
				return gbBAD;
			}
			current_index = 0 ;
			allocated_size = still_needed ;
		}
		switch ( telnet_read_state ) {
			case telnet_regular :
				//printf("LINKE_READ regular char=%.2X index=%d disacard=%d size=%d allocated=%d\n",readin_buf[current_index],current_index,total_discard,size,allocated_size);
				if ( readin_buf[current_index] == 0xFF ) {
					// starting escape sequence
					// following bytes will better characterize
					++ total_discard ;
					telnet_read_state = telnet_ff ;
				} else {
					// normal processing
					// move byte to response and decrement needed bytes
					// stay in current state
					buf[size - still_needed] = readin_buf[current_index] ;
					-- still_needed ;
				}
				break ;
			case telnet_ff:
				//printf("LINKE_READ FF      char=%.2X index=%d disacard=%d size=%d allocated=%d\n",readin_buf[current_index],current_index,total_discard,size,allocated_size);
				switch ( readin_buf[current_index] ) {
					case 0xF1:
					case 0xF2:
					case 0xF3:
					case 0xF4:
					case 0xF5:
					case 0xF6:
					case 0xF7:
					case 0xF8:
					case 0xF9:
						// 2 byte sequence
						// just read 2nd character
						++ total_discard ;
						telnet_read_state = telnet_regular ;
						break ;
					case 0xFA:
						// multibyte squence
						// start scanning for 0xF0
						++ total_discard ;
						telnet_read_state = telnet_fffa ;
						break ;
					case 0xFB:
					case 0xFC:
					case 0xFD:
					case 0xFE:
						// 3 byte sequence
						// just read 2nd char
						++ total_discard ;
						telnet_read_state = telnet_fffb ;
						break ;
					case 0xFF:
						// escape the FF character
						// make this a single regular FF char
						buf[size - still_needed] = 0xFF ;
						-- total_discard ;
						-- still_needed ;
						telnet_read_state = telnet_regular ;
						break ;
					default:
						LEVEL_DEBUG("Unexpected telnet sequence");
						return gbBAD ;
				}
				break ;
			case telnet_fffa:
				switch ( readin_buf[current_index] ) {
					case 0xF0:
						// end of escape sequence
						++ total_discard ;
						telnet_read_state = telnet_regular ;
						break ;
					default:
						// stay in this mode
						++ total_discard ;
						break ;
				}
				break ;
			case telnet_fffb:
				// 3 byte sequence
				// now reading 3rd char
				++ total_discard ;
				telnet_read_state = telnet_regular ;
				break ;
		}
		++ current_index ;
	}
	// store this extra length for the next read attempt
	//printf("LINKE_READ setting default_discard = %d to %d\n",pn->selected_connection->default_discard,total_discard);
	pn->selected_connection->default_discard = total_discard ;
	return gbGOOD ;
}
