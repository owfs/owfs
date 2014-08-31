/*
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

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

#ifndef OW_DETECT_H			/* tedious wrapper */
#define OW_DETECT_H

#include "ow_port_in.h"

GOOD_OR_BAD Server_detect(struct port_in * pin);
GOOD_OR_BAD Zero_detect(struct port_in * pin);
GOOD_OR_BAD DS2480_detect(struct port_in * pin);

#if OW_PARPORT
GOOD_OR_BAD DS1410_detect(struct port_in * pin);
#endif							/* OW_PARPORT */

GOOD_OR_BAD DS9097_detect(struct port_in * pin);
GOOD_OR_BAD LINK_detect(struct port_in * pin);
GOOD_OR_BAD PBM_detect(struct port_in * pin);
GOOD_OR_BAD HA7E_detect(struct port_in * pin);
GOOD_OR_BAD OWServer_Enet_detect(struct port_in * pin);
GOOD_OR_BAD OWServer_Enet2_detect(struct port_in * pin);
GOOD_OR_BAD HA5_detect(struct port_in * pin);
GOOD_OR_BAD BadAdapter_detect(struct port_in * pin);
GOOD_OR_BAD LINKE_detect(struct port_in * pin);
GOOD_OR_BAD Fake_detect(struct port_in * pin);
GOOD_OR_BAD Tester_detect(struct port_in * pin);
GOOD_OR_BAD Mock_detect(struct port_in * pin);
GOOD_OR_BAD EtherWeather_detect(struct port_in * pin);
GOOD_OR_BAD Browse_detect(struct port_in * pin);
GOOD_OR_BAD W1_monitor_detect(struct port_in * pin);
GOOD_OR_BAD External_detect(struct port_in * pin);

struct enet_member ;
struct enet_member {
	int version ;
	struct enet_member * next ;
	char name[0] ;
}; 

struct enet_list {
	int members ;
	struct enet_member * head ;
} ;

GOOD_OR_BAD HA7_detect(struct port_in * pin);
GOOD_OR_BAD FS_FindHA7(void);
void Find_ENET_all( struct enet_list * elist ) ;
void Find_ENET_Specific( char * addr, struct enet_list * elist ) ;
GOOD_OR_BAD OWServer_Enet_setup(char * enet_name, int enet_version, struct port_in *pin) ;
void enet_list_init( struct enet_list * elist ) ;
void enet_list_kill( struct enet_list * elist ) ;
void enet_list_add( char * ip, char * port, int version, struct enet_list * elist ) ;
GOOD_OR_BAD ENET_monitor_detect(struct port_in * pin) ;

#if OW_W1
GOOD_OR_BAD W1_detect(struct port_in * pin) ;
GOOD_OR_BAD W1_Browse( void ) ;
#endif /* OW_W1 */

#if OW_I2C
GOOD_OR_BAD DS2482_detect(struct port_in * pin);
#endif							/* OW_I2C */

#if OW_USB
GOOD_OR_BAD DS9490_detect(struct port_in * pin);
GOOD_OR_BAD USB_monitor_detect(struct port_in * pin) ;
#endif							/* OW_USB */

#endif							/* OW_DETECT_H */
