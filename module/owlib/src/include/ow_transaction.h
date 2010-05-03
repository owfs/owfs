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

#ifndef OW_TRANSACTION_H		/* tedious wrapper */
#define OW_TRANSACTION_H


enum transaction_type {
	trxn_select,
	trxn_match,
	trxn_modify,
	trxn_compare,
	trxn_read,
	trxn_blind,
	trxn_power,
	trxn_program,
	trxn_reset,
	trxn_crc8,
	trxn_crc8seeded,
	trxn_crc16,
	trxn_crc16seeded,
	trxn_end,
	trxn_verify,
	trxn_nop,
	trxn_delay,
	trxn_udelay,
};
struct transaction_log {
	const BYTE *out;
	BYTE *in;
	size_t size;
	enum transaction_type type;
};
#define TRXN_NVERIFY { NULL, NULL, _1W_SEARCH_ROM, trxn_verify, }
#define TRXN_AVERIFY { NULL, NULL, _1W_CONDITIONAL_SEARCH_ROM, trxn_verify, }
#define TRXN_START   { NULL, NULL, 0, trxn_select, }
#define TRXN_END     { NULL, NULL, 0, trxn_end, }
#define TRXN_RESET   { NULL, NULL, 0, trxn_reset, }
#define TRXN_WRITE(writedata,length)   { writedata, NULL, length, trxn_match, }
#define TRXN_READ(readdata,length)    { NULL, readdata, length, trxn_read, }
#define TRXN_MODIFY(writedata,readdata,length) { writedata, readdata, length, trxn_modify, }
#define TRXN_COMPARE(data1, data2, length) { data1, data2, length, trxn_compare }
#define TRXN_CRC8(data,length) { data, NULL, length, trxn_crc8, }
#define TRXN_CRC16(data,length) { data, NULL, length, trxn_crc16, }
#define TRXN_CRC16_seeded(data,length,seed) { data, seed, length, trxn_crc16, }
#define TRXN_BLIND(writedata,length)  { writedata, NULL, length, trxn_blind, }
#define TRXN_POWER(byte_pointer, msec)  { byte_pointer, byte_pointer, msec, trxn_power, }

#define TRXN_DELAY(msec) { NULL, NULL, msec, trxn_delay }

#define TRXN_WRITE1(writedata)  TRXN_WRITE(writedata,1)
#define TRXN_READ1(readdata)    TRXN_READ(readdata,1)
#define TRXN_WRITE2(writedata)  TRXN_WRITE(writedata,2)
#define TRXN_READ2(readdata)    TRXN_READ(readdata,2)
#define TRXN_WRITE3(writedata)  TRXN_WRITE(writedata,3)
#define TRXN_READ3(readdata)    TRXN_READ(readdata,3)

#define TRXN_WR_CRC16(pointer,writelength,readlength) \
                TRXN_WRITE((BYTE *)pointer,writelength), \
                TRXN_READ(((BYTE *)pointer)+(writelength),readlength+2), \
                TRXN_CRC16((BYTE *)pointer,writelength+readlength+2)

#define TRXN_WR_CRC16_SEEDED(pointer,seed, writelength,readlength) \
                TRXN_WRITE((BYTE *)pointer,writelength), \
                TRXN_READ(((BYTE *)pointer)+(writelength),readlength+2), \
                TRXN_CRC16_seeded((BYTE *)pointer, writelength+readlength+2, (BYTE *) (seed) )

#define TRXN_PROGRAM   { NULL, NULL, 0, trxn_program, }

GOOD_OR_BAD BUS_transaction(const struct transaction_log *tl, const struct parsedname *pn);
GOOD_OR_BAD BUS_transaction_nolock(const struct transaction_log *tl, const struct parsedname *pn);

#endif							/* OW_TRANSACTION_H */
