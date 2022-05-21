/*
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

/* Write to a telnet device
*/

GOOD_OR_BAD telnet_write_binary( const BYTE * buf, const size_t size, struct connection_in *in)
{
	size_t new_size = 0 ;
	size_t i = 0 ;
	// temporary buffer (add some extra space)
	BYTE new_buf[2*size] ;
	
	while ( i < size ) {
		if ( buf[i] == TELNET_IAC ) {
			new_buf[ new_size++ ] = TELNET_IAC ; // to double it
		}
		new_buf[ new_size++ ] = buf[ i++ ] ;
	}
	return COM_write( new_buf, new_size, in ) ;
}
