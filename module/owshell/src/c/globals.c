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

#include "owshell.h"

/* Globals for port and bus communication */
/* connections globals stored in ow_connect.c */
/* i.e. connection_in * owserver_connection ...         */


/* State informatoin, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
int32_t SemiGlobal =
	((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8;

struct global Global;

/* Globals */
int count_inbound_connections = 0 ;
struct connection_in s_owserver_connection ;
struct connection_in *owserver_connection = & s_owserver_connection ;


/* All ow library setup */
void Setup(void)
{
#if OW_ZERO
	OW_Load_dnssd_library();
#endif
	memset( owserver_connection, 0, sizeof(s_owserver_connection) ) ;

	// global structure of configuration parameters
	memset(&Global, 0, sizeof(struct global));
	Global.timeout_volatile = 15;
	Global.timeout_stable = 300;
	Global.timeout_directory = 60;
	Global.timeout_presence = 120;
	Global.timeout_serial = 5;
	Global.timeout_usb = 5000;	// 5 seconds
	Global.timeout_network = 1;
	Global.timeout_server = 10;
	Global.timeout_ftp = 900;
	errno = 0;					/* set error level none */
}

static void Cleanup(void)
{
#if OW_ZERO
	OW_Free_dnssd_library();
#endif
}

void Exit( int exit_code )
{
	Cleanup();
	exit( exit_code ) ;
}
