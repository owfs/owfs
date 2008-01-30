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

    LICENSE (As of version 2.5p4 2-Oct-2006)
    owlib: GPL v2
    owfs, owhttpd, owftpd, owserver: GPL v2
    owshell(owdir owread owwrite owpresent): GPL v2
    owcapi (libowcapi): GPL v2
    owperl: GPL v2
    owtcl: LGPL v2
    owphp: GPL v2
    owpython: GPL v2
    owsim.tcl: GPL v2
    where GPL v2 is the "Gnu General License version 2"
    and "LGPL v2" is the "Lesser Gnu General License version 2"


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

/* Cannot stand alone -- part of ow.h but separated for clarity */

#ifndef OW_FUNCTION_H			/* tedious wrapper */
#define OW_FUNCTION_H

/* Prototypes for owlib.c -- libow overall control */
void LibSetup(void);
int LibStart(void);
void LibStop(void);
void LibClose(void);

/* Utility functions */
BYTE char2num(const char *s);
BYTE string2num(const char *s);
char num2char(const BYTE n);
void num2string(char *s, const BYTE n);
void string2bytes(const char *str, BYTE * b, const int bytes);
void bytes2string(char *str, const BYTE * b, const int bytes);
int UT_getbit(const BYTE * buf, const int loc);
int UT_get2bit(const BYTE * buf, const int loc);
void UT_setbit(BYTE * buf, const int loc, const int bit);
void UT_set2bit(BYTE * buf, const int loc, const int bits);

// ow_api.c
int OWLIB_can_init_start(void);
void OWLIB_can_init_end(void);
int OWLIB_can_access_start(void);
void OWLIB_can_access_end(void);
int OWLIB_can_finish_start(void);
void OWLIB_can_finish_end(void);

void LockSetup(void);

ssize_t tcp_read(int file_descriptor, void *vptr, size_t n, const struct timeval *ptv);
void tcp_read_flush(int file_descriptor);
int tcp_wait(int file_descriptor, const struct timeval *ptv);
int ClientAddr(char *sname, struct connection_in *in);
int ClientConnect(struct connection_in *in);
void FreeClientAddr(struct connection_in *in);

int OW_ArgNet(const char *arg);
int OW_ArgGeneric(const char *arg);

void BUS_lock_in(struct connection_in *in);
void BUS_unlock_in(struct connection_in *in);

#endif							/* OW_FUNCTION_H */
