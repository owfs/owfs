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

#ifndef OW_COMMUNICATION_H			/* tedious wrapper */
#define OW_COMMUNICATION_H

enum com_type {
	ct_unknown,
	ct_serial,
	ct_telnet,
	ct_tcp,
	ct_i2c,
} ;

enum com_state {
	cs_virgin,
	cs_good,
	cs_bad,
	cs_closed,
} ;

struct com_serial {
	struct termios oldSerialTio;    /*old serial port settings */
	enum { flow_none, flow_soft, flow_hard, } flow_control ;
} ;

struct communication {
	enum com_type type ;
	enum com_type state ;
	struct tileval 
	struct timeval timeout ; // for serial or tcp read
	FILE_DESCRIPTOR_OR_PERSISTENT file_descriptor;
	char * devicename ;
	union {
		struct com_serial serial ;
	} dev ;
} ;

#endif							/* OW_COMMUNICATION_H */
