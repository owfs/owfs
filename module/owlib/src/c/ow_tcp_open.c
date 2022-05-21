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
#include "ow_connection.h"

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

//open tcp port
/* Called on head of multibus group */
GOOD_OR_BAD tcp_open(struct connection_in *connection)
{
	struct port_in * pin = connection->pown ;
	
	if ( pin->state == cs_virgin ) {
		char * def_port = "" ;
		switch( get_busmode(connection) ) {
			case bus_link:
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
				def_port = DEFAULT_ETHERWEATHER_PORT ;
				break ;
			case bus_enet:
				def_port = DEFAULT_ENET_PORT ;
				break ;
			case bus_xport_control:
				def_port = DEFAULT_XPORT_CONTROL_PORT ;
				break ;
			default:
				break ;
		}
		RETURN_BAD_IF_BAD( ClientAddr( DEVICENAME(connection), def_port, connection ) ) ;
		pin->file_descriptor = FILE_DESCRIPTOR_BAD ;
	}

	pin->state = cs_deflowered ;
	pin->file_descriptor = ClientConnect(connection) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(pin->file_descriptor) ) {
		return gbBAD;
	}
	return gbGOOD ;
}
