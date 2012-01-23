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

#ifndef OW_CHANNEL_H			/* tedious wrapper */
#define OW_CHANNEL_H

/* included in ow_connection.h as the bus-master specific portion of the connection_in structure */
void OW_channel_init( struct connection_in * in ) ;
void OW_channel_close( struct connection_in * in ) ;
void OW_channel_lock( struct connection_in * in ) ;
void OW_channel_unlock( struct connection_in * in ) ;

#endif							/* OW_CHANNEL_H */
