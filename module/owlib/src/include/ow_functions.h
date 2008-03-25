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

_FLOAT Temperature(_FLOAT C, const struct parsedname *pn);
_FLOAT TemperatureGap(_FLOAT C, const struct parsedname *pn);
_FLOAT fromTemperature(_FLOAT T, const struct parsedname *pn);
_FLOAT fromTempGap(_FLOAT T, const struct parsedname *pn);
const char *TemperatureScaleName(enum temp_type t);

/* Prototypes */
#define  READ_FUNCTION( fname )  static int fname( struct one_wire_query * owq )
#define  WRITE_FUNCTION( fname )  static int fname( struct one_wire_query * owq )

/* Prototypes for owlib.c -- libow overall control */
void LibSetup(enum opt_program op);
int LibStart(void);
void LibStop(void);
void LibClose(void);
int EnterBackground(void) ;

/* Initial sorting or the device and filetype lists */
void DeviceSort(void);
void DeviceDestroy(void);
//  int filecmp(const void * name , const void * ex ) 
/* Pasename processing -- URL/path comprehension */
int filecmp(const void *name, const void *ex);
int FS_ParsedNamePlus(const char *path, const char *file, struct parsedname *pn);
int FS_ParsedName(const char *fn, struct parsedname *pn);
int FS_ParsedName_BackFromRemote(const char *fn, struct parsedname *pn);
void FS_ParsedName_destroy(struct parsedname *pn);
int FS_ParseProperty_for_sibling(char *filename, struct parsedname *pn);

size_t FileLength(const struct parsedname *pn);
size_t FullFileLength(const struct parsedname *pn);
int CheckPresence(struct parsedname *pn);
void FS_devicename(char *buffer, const size_t length, const BYTE * sn, const struct parsedname *pn);
void FS_devicefind(const char *code, struct parsedname *pn);
void FS_devicefindhex(BYTE f, struct parsedname *pn);

int FS_dirname_state(char *buffer, const size_t length, const struct parsedname *pn);
int FS_dirname_type(char *buffer, const size_t length, const struct parsedname *pn);
void FS_DirName(char *buffer, const size_t size, const struct parsedname *pn);
int FS_FileName(char *name, const size_t size, const struct parsedname *pn);

/* Utility functions */
BYTE CRC8(const BYTE * bytes, const size_t length);
BYTE CRC8seeded(const BYTE * bytes, const size_t length, const UINT seed);
BYTE CRC8compute(const BYTE * bytes, const size_t length, const UINT seed);
int CRC16(const BYTE * bytes, const size_t length);
int CRC16seeded(const BYTE * bytes, const size_t length, const UINT seed);
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
void UT_fromDate(const _DATE D, BYTE * data);
_DATE UT_toDate(const BYTE * date);
int FS_busless(char *path);

/* Cache  and Storage functions */
#include "ow_cache.h"
void FS_LoadDirectoryOnly(struct parsedname *pn_directory, const struct parsedname *pn_original);

int Simul_Test(const enum simul_type type, const struct parsedname *pn);
int FS_poll_convert(const struct parsedname *pn);

// ow_locks.c
void LockSetup(void);
int LockGet(const struct parsedname *pn);
void LockRelease(const struct parsedname *pn);

// ow_api.c
int OWLIB_can_init_start(void);
void OWLIB_can_init_end(void);
int OWLIB_can_access_start(void);
void OWLIB_can_access_end(void);
int OWLIB_can_finish_start(void);
void OWLIB_can_finish_end(void);

/* 1-wire lowlevel */
void UT_delay(const UINT len);
void UT_delay_us(const unsigned long len);

ssize_t tcp_read(int file_descriptor, void *vptr, size_t n, const struct timeval *ptv);
void tcp_read_flush(int file_descriptor);
int tcp_wait(int file_descriptor, const struct timeval *ptv);
int ClientAddr(char *sname, struct connection_in *in);
int ClientConnect(struct connection_in *in);
void ServerProcess(void (*HandlerRoutine) (int file_descriptor), void (*Exit) (int errcode));
void FreeClientAddr(struct connection_in *in);
int ServerOutSetup(struct connection_out *out);

int OW_ArgNet(const char *arg);
int OW_ArgServer(const char *arg);
int OW_ArgUSB(const char *arg);
int OW_ArgDevice(const char *arg);
int OW_ArgGeneric(const char *arg);

void ow_help(const char *arg);

void update_max_delay(const struct parsedname *pn);

int ServerPresence(const struct parsedname *pn);
int ServerRead(struct one_wire_query *owq);
int ServerWrite(struct one_wire_query *owq);
int ServerDir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn, uint32_t * flags);

/* High-level callback functions */
int FS_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn);
int FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn, uint32_t * flags);

int FS_write(const char *path, const char *buf, const size_t size, const off_t offset);
int FS_write_postparse(struct one_wire_query *owq);
int FS_write_sibling(char *property, struct one_wire_query *owq_shallow_copy);

int FS_read(const char *path, char *buf, const size_t size, const off_t offset);
//int FS_read_postparse(char *buf, const size_t size, const off_t offset,
//                    const struct parsedname *pn);
int FS_read_postparse(struct one_wire_query *owq);
int FS_read_distribute(struct one_wire_query *owq);
int FS_read_fake(struct one_wire_query *owq);
int FS_read_tester(struct one_wire_query *owq);
int FS_r_aggregate_all(struct one_wire_query *owq);
int FS_read_sibling(char *property, struct one_wire_query *owq_shallow_copy);

int Fowq_output_offset_and_size(char *string, size_t length, struct one_wire_query *owq);
int Fowq_output_offset_and_size_z(char *string, struct one_wire_query *owq);

int FS_fstat(const char *path, struct stat *stbuf);
int FS_fstat_postparse(struct stat *stbuf, const struct parsedname *pn);

/* iteration functions for splitting writes to buffers */
int OW_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, int (*readwritefunc) (BYTE *, size_t, off_t, struct parsedname *));
int OWQ_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, int (*readwritefunc) (struct one_wire_query *, size_t, size_t));

int OW_r_mem_simple(struct one_wire_query *owq, size_t page, size_t pagesize);
int OW_r_mem_crc16_A5(struct one_wire_query *owq, size_t page, size_t pagesize);
int OW_r_mem_crc16_AA(struct one_wire_query *owq, size_t page, size_t pagesize);
int OW_r_mem_crc16_F0(struct one_wire_query *owq, size_t page, size_t pagesize);
int OW_r_mem_toss8(struct one_wire_query *owq, size_t page, size_t pagesize);
int OW_r_mem_counter_bytes(BYTE * extra, size_t page, size_t pagesize, struct parsedname *pn);

int OW_w_eprom_mem(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn);
int OW_w_eprom_status(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn);

void BUS_lock(const struct parsedname *pn);
void BUS_unlock(const struct parsedname *pn);
void BUS_lock_in(struct connection_in *in);
void BUS_unlock_in(struct connection_in *in);

/* API wrappers for swig and owcapi */
void API_setup( enum opt_program opt ) ;
int API_init( const char * command_line ) ;
void API_finish( void ) ;
int API_access_start( void ) ;
void API_access_end( void ) ;


#endif							/* OW_FUNCTION_H */
