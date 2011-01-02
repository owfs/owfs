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
#include "telnet.h"


GOOD_OR_BAD telnet_set(struct connection_in *in)
{
#pragma pack(push) /* Save current */
#pragma pack(1) /* byte alignment */

	struct {
//		BYTE do_sga[3] ;
//		BYTE do_echo[3] ;
		
		BYTE i_will[3] ;
//		BYTE you_do[3] ;
		
		BYTE pre_baud[4] ;
		uint32_t baud ;
		BYTE post_baud[2] ;

		BYTE pre_size[4] ;
		BYTE size ;
		BYTE post_size[2] ;

		BYTE pre_parity[4] ;
		BYTE parity ;
		BYTE post_parity[2] ;

		BYTE pre_stop[4] ;
		BYTE stop ;
		BYTE post_stop[2] ;

		BYTE pre_flow[4] ;
		BYTE flow ;
		BYTE post_flow[2] ;
	} telnet_string = {
//		.do_sga = { TELNET_IAC, TELNET_DO, TELOPT_SGA, } ,
//		.do_echo = { TELNET_IAC, TELNET_DO, TELOPT_ECHO, } ,
		.i_will = { TELNET_IAC, TELNET_WILL, TELOPT_COM_PORT, } ,
//		.you_do = { TELNET_IAC, TELNET_DO, TELOPT_COM_PORT, } ,
		.pre_baud = { TELNET_IAC, TELNET_SB, TELOPT_COM_PORT, COMOPT_BAUDRATE, } ,
		.post_baud = {TELNET_IAC, TELNET_SE, } ,
		.pre_size = { TELNET_IAC, TELNET_SB, TELOPT_COM_PORT, COMOPT_DATASIZE, } ,
		.post_size = {TELNET_IAC, TELNET_SE, } ,
		.pre_parity = { TELNET_IAC, TELNET_SB, TELOPT_COM_PORT, COMOPT_PARITY, } ,
		.post_parity = {TELNET_IAC, TELNET_SE, } ,
		.pre_stop = { TELNET_IAC, TELNET_SB, TELOPT_COM_PORT, COMOPT_STOPSIZE, } ,
		.post_stop = {TELNET_IAC, TELNET_SE, } ,
		.pre_flow = { TELNET_IAC, TELNET_SB, TELOPT_COM_PORT, COMOPT_CONTROL, } ,
		.post_flow = {TELNET_IAC, TELNET_SE, } ,
	} ;

#pragma pack(pop)  /* restore */

	switch ( SOC(in)->baud ) {
		case B115200:
			telnet_string.baud = htonl( 115200 ) ;
			break ;
		case B57600:
			telnet_string.baud = htonl( 57600 ) ;
			break ;
		case B19200:
			telnet_string.baud = htonl( 19200 ) ;
			break ;
		case B9600:
		default:
			telnet_string.baud = htonl( 9600 ) ;
			SOC(in)->baud = B9600 ;
			break ;
	}

	telnet_string.size = SOC(in)->bits ;

	switch ( SOC(in)->parity ) {
		case parity_none:
			telnet_string.parity = 1 ;
			break ;
		case parity_odd:
			telnet_string.parity = 2 ;
			break ;
		case parity_even:
			telnet_string.parity = 3 ;
			break ;
		case parity_mark:
			telnet_string.parity = 4 ;
			break ;
	}

	switch ( SOC(in)->stop ) {
		case stop_1 :
			telnet_string.stop = 1 ;
			break ;
		case stop_2 :
			telnet_string.stop = 2 ;
			break ;
		case stop_15 :
			telnet_string.stop = 3 ;
			break ;
	}

	switch ( SOC(in)->flow ) {
		case flow_none :
			telnet_string.flow = 1 ;
			break ;
		case flow_soft :
			telnet_string.flow = 2 ;
			break ;
		case flow_hard :
			telnet_string.flow = 3 ;
			break ;
	}

	return COM_write_simple( &telnet_string, sizeof( telnet_string ) , in ) ;
}
	
