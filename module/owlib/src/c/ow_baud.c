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

/* ow_baud -- baud rate interpret and print */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

speed_t COM_MakeBaud( int raw_baud )
{
	switch ( raw_baud ) {
		case 12:
		case 1200:
			return B1200 ;
		case 24:
		case 2400:
			return B2400 ;
		case 48:
		case 4800:
			return B4800 ;
		case 96:
		case 9600:
			return B9600 ;
		case 19:
		case 19000:
		case 19200:
			return B19200 ;
		case 38:
		case 38000:
		case 38400:
			return B38400 ;
		case 56:
		case 57:
		case 56000:
		case 57000:
		case 57600:
			return B57600 ;
		case 115:
		case 115000:
		case 115200:
			return B115200 ;
		case 230:
		case 230000:
		case 230400:
			return B230400 ;
		default:
			return B9600 ;
	}
}

int COM_BaudRate( speed_t B_baud )
{
	switch ( B_baud ) {
		case B1200:
			return 1200 ;
		case B2400:
			return 2400 ;
		case B4800:
			return 4800 ;
		case B9600:
			return 9600 ;
		case B19200:
			return 19200 ;
		case B38400:
			return 38400 ;
		case B57600:
			return 57600 ;
		case B115200:
			return 115200 ;
		case B230400:
			return 230400 ;
		default:
			return 9600 ;
	}
}

// Find the best choice for baud rate among allowable choices.
// Must end list with a 0
void COM_BaudRestrict( speed_t * B_baud, ... )
{
	va_list baud_list ;

	speed_t B_original = B_baud[0] ;
	int original_baudrate = COM_BaudRate( B_original ) ;

	speed_t B_best = B9600 ;
	int best_baudrate = COM_BaudRate( B_best ) ;

	speed_t B_current ;
	
	va_start( baud_list, B_baud ) ;

	while ( (B_current=va_arg( baud_list, speed_t)) ) {
		int current_baudrate = COM_BaudRate( B_current ) ;

		if ( current_baudrate == original_baudrate ) {
			// perfect match
			B_best = B_current ;
			break ;
		} else if ( current_baudrate > original_baudrate ) {
			// too fast
			continue ;
		} else if ( current_baudrate > best_baudrate ) {
			// better choice
			B_best = B_current ;
			best_baudrate = current_baudrate ;
		}
	}
	
	va_end( baud_list ) ;
	
	B_baud[0] = B_best ;
}
