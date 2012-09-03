/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

 */

#ifndef OW_PORT_IN_H			/* tedious wrapper */
#define OW_PORT_IN_H

/* struct connection_in (for each bus master) as part of ow_connection.h */

//enum server_type { srv_unknown, srv_direct, srv_client, src_
/* Network connection structure */
enum bus_mode {
	bus_unknown = 0,
	bus_serial,
	bus_xport,
	bus_passive,
	bus_usb,
	bus_usb_monitor,
	bus_parallel,
	bus_server,
	bus_zero,
	bus_browse,
	bus_i2c,
	bus_ha7net,
	bus_ha5,
	bus_ha7e,
	bus_enet,
	bus_fake,
	bus_tester,
	bus_mock,
	bus_link,
	bus_etherweather,
	bus_bad,
	bus_w1,
	bus_w1_monitor,
	bus_xport_control,
	bus_external,
};

enum com_state {
	cs_virgin,
	cs_deflowered,
} ;


// For forward references
struct connection_in;
struct port_in ;

struct port_in {
	struct port_in * next ;
	struct connection_in *first;
	int connections ;
	enum bus_mode busmode;
	char * init_data ;

	FILE_DESCRIPTOR_OR_PERSISTENT file_descriptor;
	enum com_state state ;
	
	pthread_mutex_t port_mutex;
};

/* This bug-fix/workaround function seem to be fixed now... At least on
 * the platforms I have tested it on... printf() in owserver/src/c/owserver.c
 * returned very strange result on c->busmode before... but not anymore */
enum bus_mode get_busmode(const struct connection_in *c);

void RemovePort( struct port_in * pin ) ;
struct port_in * AllocPort( const struct port_in * old_pin ) ;
struct port_in *LinkPort(struct port_in *pin) ;
struct port_in *NewPort(const struct port_in *pin) ;
struct connection_in * AddtoPort( struct port_in * pin, struct connection_in * template ) ;

#endif							/* OW_PORT_IN_H */
