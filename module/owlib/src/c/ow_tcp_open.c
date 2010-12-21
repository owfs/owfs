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
#include "ow_connection.h"

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

//open tcp port
GOOD_OR_BAD tcp_open(struct connection_in *connection)
{
	struct termios newSerialTio;	/*new serial port settings */
	FILE_DESCRIPTOR_OR_ERROR fd ;

	fd = open( SOC(connection)->devicename, O_RDWR | O_NONBLOCK | O_NOCTTY) ;
	connection->soc.file_descriptor = fd ;
	if ( FILE_DESCRIPTOR_NOT_VALID( fd ) ) {
		// state doesn't change
		ERROR_DEFAULT("Cannot open port: %s Permissions problem?", SAFESTRING(SOC(connection)->devicename));
		return gbBAD;
	}

	if ( SOC(connection)->state == cs_virgin ) {
		char * def_port = NULL ;
		switch( connection->busmode ) {
			case bus_link:
			case bus_elink:
				def_port = DEFAULT_LINK_PORT ;
				break ;
			case bus_server:
			case bus_zero:
				def_port = DEFAULT_SERVER_PORT ;
				break ;
			case bus_ha7net:
				def_port = DEFAULT_HA7_PORT ;
				break ;
			case bus_xport:
			case bus_serial:
				def_port = DEFAULT_XPORT_PORT ;
				break ;
			case bus_etherweather:
				def_port = DEFAULT_ETHERWEATHER_POST ;
				break ;
			case bus_enet:
				def_port = DEFAULT_ENET_PORT ;
				break ;
			default:
				break ;
		}
		RETURN_BAD_IF_BAD( ClientAddr( SOC(connection)->devicename, def_port, connection ) ;
	}

	SOC(connection)->file_descriptor = ClientConnect(connection) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(SOC(connection)->file_descriptor) ) {
		return gbBAD;
	}
	return gbGOOD ;
}
