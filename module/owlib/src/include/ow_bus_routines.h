/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions

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

#ifndef OW_BUS_ROUTINES_H			/* tedious wrapper */
#define OW_BUS_ROUTINES_H


/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
struct interface_routines {
	/* Detect if adapter is present, and open -- usually called outside of this routine */
	GOOD_OR_BAD (*detect) (struct connection_in * in);
	/* reset the interface -- actually the 1-wire bus */
	RESET_TYPE (*reset) (const struct parsedname * pn);
	/* Bulk of search routine, after set ups for first or alarm or family */
	enum search_status (*next_both) (struct device_search * ds, const struct parsedname * pn);
	/* Send a byte with bus power to follow */
	GOOD_OR_BAD (*PowerByte) (const BYTE data, BYTE * resp, const UINT delay, const struct parsedname * pn);
	/* Send a 12V 480msec oulse to program EEPROM */
	GOOD_OR_BAD (*ProgramPulse) (const struct parsedname * pn);
	/* send and recieve data -- byte at a time */
	GOOD_OR_BAD (*sendback_data) (const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn);
	/* send and recieve data -- byte at a time */
	GOOD_OR_BAD (*select_and_sendback) (const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn);
	/* send and recieve data -- bit at a time */
	GOOD_OR_BAD (*sendback_bits) (const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn);
	/* select a device */
	GOOD_OR_BAD (*select) (const struct parsedname * pn);
	/* reconnect with a balky device */
	GOOD_OR_BAD (*reconnect) (const struct parsedname * pn);
	/* Close the connection (port) */
	void (*close) (struct connection_in * in);
	/* capabilities flags */
	UINT flags;
};

/* placed in iroutines.flags */

// Adapter is usable
#define ADAP_FLAG_default     0x00000000

// Adapter supports overdrive mode
#define ADAP_FLAG_overdrive     0x00000001

// Adapter doesn't support the DS2409 microlan hub
#define ADAP_FLAG_no2409path      0x00000010

// Adapter doesn't need DS2404 delay (i.e. not a local device)
#define ADAP_FLAG_no2404delay      0x00000020

// Adapter gets a directory all at once rather than one at a time
#define ADAP_FLAG_dirgulp       0x00000100

// Adapter benefits from coalescing reads and writes into a longer string
#define ADAP_FLAG_bundle        0x00001000

// Adapter automatically performs a reset before read/writes
#define ADAP_FLAG_dir_auto_reset 0x00002000

// Adapter doesn't support "presence" -- use the last dirblob instead.
#define ADAP_FLAG_presence_from_dirblob 0x00004000

// Adapter is a sham.
#define ADAP_FLAG_sham 0x00008000

#define AdapterSupports2409(pn)	(((pn)->selected_connection->iroutines.flags&ADAP_FLAG_no2409path)==0)

GOOD_OR_BAD BUS_detect( struct connection_in * in ) ;

RESET_TYPE BUS_reset(const struct parsedname *pn);

GOOD_OR_BAD BUS_select(const struct parsedname *pn);
GOOD_OR_BAD BUS_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);

GOOD_OR_BAD BUS_sendback_bits( const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn );
GOOD_OR_BAD BUS_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
GOOD_OR_BAD BUS_sendback_cmd(const BYTE * cmd, BYTE * resp, const size_t len, const struct parsedname *pn);
GOOD_OR_BAD BUS_send_data(const BYTE * data, const size_t len, const struct parsedname *pn);
GOOD_OR_BAD BUS_readin_data(BYTE * data, const size_t len, const struct parsedname *pn);

GOOD_OR_BAD BUS_verify(BYTE search, const struct parsedname *pn);

GOOD_OR_BAD BUS_PowerByte(const BYTE data, BYTE * resp, UINT delay, const struct parsedname *pn);
GOOD_OR_BAD BUS_ProgramPulse(const struct parsedname *pn);

void BUS_close( struct connection_in * in );

#endif							/* OW_BUS_ROUTINES_H */
