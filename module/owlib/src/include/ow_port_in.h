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

// For forward references
struct connection_in;
struct port_in ;

struct port_in {
	struct port_in * next ;
	struct connection_in *first;
	int connections ;
	
	pthread_mutex_t port_mutex;
};

void RemovePort( struct port_in * pin ) ;
struct port_in * AllocPort( const struct port_in * old_pin ) ;
struct port_in *LinkPort(struct port_in *pin) ;
struct port_in *NewPort(const struct port_in *pin) ;

#endif							/* OW_PORT_IN_H */
