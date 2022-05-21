/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

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
    Implementation:
    25-05-2003 iButtonLink device
*/

/* Cannot stand alone -- part of ow.h but separated for clarity */

#ifndef OW_FUNCTION_H			/* tedious wrapper */
#define OW_FUNCTION_H

#include "ow_parse_sn.h" // sub-includes

/* Prototypes */
#define  READ_FUNCTION( fname )  static int fname( struct one_wire_query * owq )
#define  WRITE_FUNCTION( fname )  static int fname( struct one_wire_query * owq )
#define  VISIBLE_FUNCTION( fname )  static enum e_visibility fname( const struct parsedname * pn );

/* Prototypes for owlib.c -- libow overall control */
void LibSetup(enum enum_program_type op);
GOOD_OR_BAD LibStart(void * v);
void HandleSignals(void);
void LibStop(void);
void LibClose(void);
GOOD_OR_BAD EnterBackground(void);

/* Initial sorting or the device and filetype lists */
void DeviceSort(void);
void DeviceDestroy(void);
/* Pasename processing -- URL/path comprehension */
int filetype_cmp(const void *name, const void *ex);
ZERO_OR_ERROR FS_ParsedNamePlus(const char *path, const char *file, struct parsedname *pn);
ZERO_OR_ERROR FS_ParsedNamePlusExt(const char *path, const char *file, int extension, enum ag_index alphanumeric, struct parsedname *pn);
ZERO_OR_ERROR FS_ParsedNamePlusText(const char *path, const char *file, const char *extension, struct parsedname *pn);
ZERO_OR_ERROR FS_ParsedName(const char *fn, struct parsedname *pn);
ZERO_OR_ERROR FS_ParsedName_BackFromRemote(const char *fn, struct parsedname *pn);
void FS_ParsedName_destroy(struct parsedname *pn);
void FS_ParsedName_Placeholder( struct parsedname * pn ) ;

size_t FileLength(const struct parsedname *pn);
size_t FullFileLength(const struct parsedname *pn);
INDEX_OR_ERROR CheckPresence(struct parsedname *pn);
INDEX_OR_ERROR ReCheckPresence(struct parsedname *pn);
INDEX_OR_ERROR RemoteAlias(struct parsedname *pn);
void FS_devicename(char *buffer, const size_t length, const BYTE * sn, const struct parsedname *pn);
void FS_devicefind(const char *code, struct parsedname *pn);
struct device * FS_devicefindhex(BYTE f, struct parsedname *pn);

const char *FS_DirName(const struct parsedname *pn);

/* Utility functions */
BYTE CRC8(const BYTE * bytes, const size_t length);
BYTE CRC8seeded(const BYTE * bytes, const size_t length, const UINT seed);
BYTE CRC8compute(const BYTE * bytes, const size_t length, const UINT seed);
int CRC16(const BYTE * bytes, const size_t length);
uint16_t CRC16compute(const BYTE * bytes, const size_t length, const UINT seed);
int CRC16seeded(const BYTE * bytes, const size_t length, const UINT seed);
BYTE char2num(const char *s);
BYTE string2num(const char *s);
char num2char(const BYTE n);
void num2string(char *s, const BYTE n);
void string2bytes(const char *str, BYTE * b, const int bytes);
void bytes2string(char *str, const BYTE * b, const int bytes);

int UT_getbit(const BYTE * buf, int loc);
void UT_setbit(BYTE * buf, int loc, int bit);

int UT_getbit_U(UINT U, int loc);
void UT_setbit_U(UINT * U, int loc, int bit);

int UT_get2bit(const BYTE * buf, int loc);
void UT_set2bit(BYTE * buf, int loc, int bits);

void UT_fromDate(const _DATE D, BYTE * data);
_DATE UT_toDate(const BYTE * date);

void Test_and_Close( FILE_DESCRIPTOR_OR_ERROR * file_descriptor ) ;
void Test_and_Close_Pipe( FILE_DESCRIPTOR_OR_ERROR * pipe_fd ) ;
void Init_Pipe( FILE_DESCRIPTOR_OR_ERROR * pipe_fd ) ;

/* Cache  and Storage functions */
#include "ow_cache.h"
void FS_LoadDirectoryOnly(struct parsedname *pn_directory, const struct parsedname *pn_original);

GOOD_OR_BAD FS_Test_Simultaneous( const struct internal_prop *ip, UINT delay, const struct parsedname * pn) ;

// ow_locks.c
void LockSetup(void);
ZERO_OR_ERROR DeviceLockGet(struct parsedname *pn);
void DeviceLockRelease(struct parsedname *pn);

/* 1-wire lowlevel */
void UT_delay(const UINT len);
void UT_delay_us(const unsigned long len);

ZERO_OR_ERROR tcp_read(FILE_DESCRIPTOR_OR_ERROR file_descriptor, BYTE * buffer, size_t n, const struct timeval *ptv, size_t * actual_read);
void tcp_read_flush(FILE_DESCRIPTOR_OR_ERROR file_descriptor);
GOOD_OR_BAD tcp_wait(FILE_DESCRIPTOR_OR_ERROR file_descriptor, const struct timeval *ptv);
ssize_t udp_read(FILE_DESCRIPTOR_OR_ERROR file_descriptor, void *vptr, size_t n, const struct timeval * ptv, struct sockaddr_in *from, socklen_t *fromlen) ;

GOOD_OR_BAD ClientAddr(char *sname, char * default_port, struct connection_in *in);
FILE_DESCRIPTOR_OR_ERROR ClientConnect(struct connection_in *in);
void FreeClientAddr(struct connection_in *in);

void ServerProcess(void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor));
GOOD_OR_BAD ServerOutSetup(struct connection_out *out);
void InterruptListening( void ) ;

void Setup_Systemd( void ) ;
void Announce_Systemd( void ) ;

GOOD_OR_BAD ReadAliasFile(const ASCII * file) ;
GOOD_OR_BAD Test_and_Add_Alias( char * name, BYTE * sn ) ;

speed_t COM_MakeBaud( int raw_baud ) ;
int COM_BaudRate( speed_t B_baud ) ;
void COM_BaudRestrict( speed_t * B_baud, ... ) ;

void FS_help(const char *arg);

INDEX_OR_ERROR ServerPresence( struct parsedname *pn);
SIZE_OR_ERROR ServerRead(struct one_wire_query *owq);
ZERO_OR_ERROR ServerWrite(struct one_wire_query *owq);
ZERO_OR_ERROR ServerDir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn, uint32_t * flags);

/* High-level callback functions */
ZERO_OR_ERROR FS_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, struct parsedname *pn);
ZERO_OR_ERROR FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn, uint32_t * flags);
void FS_dir_entry_aliased(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn) ;

SIZE_OR_ERROR FS_write(const char *path, const char *buf, const size_t size, const off_t offset);
SIZE_OR_ERROR FS_write_postparse(struct one_wire_query *owq);
ZERO_OR_ERROR FS_write_local(struct one_wire_query *owq);

SIZE_OR_ERROR FS_get(const char *path, char **return_buffer, size_t * buffer_length) ;

SIZE_OR_ERROR FS_read(const char *path, char *buf, const size_t size, const off_t offset);
SIZE_OR_ERROR FS_read_postparse(struct one_wire_query *owq);
ZERO_OR_ERROR FS_read_fake(struct one_wire_query *owq);
ZERO_OR_ERROR FS_read_tester(struct one_wire_query *owq);
ZERO_OR_ERROR FS_r_aggregate_all(struct one_wire_query *owq);
SIZE_OR_ERROR FS_read_local( struct one_wire_query *owq);

size_t FileLength_vascii(struct one_wire_query *owq);

ZERO_OR_ERROR FS_r_external( struct one_wire_query * owq ) ;
ZERO_OR_ERROR FS_w_external( struct one_wire_query * owq ) ;

#include "ow_sibling.h"

ZERO_OR_ERROR FS_fstat(const char *path, struct stat *stbuf);
ZERO_OR_ERROR FS_fstat_postparse(struct stat *stbuf, const struct parsedname *pn);

/* iteration functions for splitting writes to buffers */
GOOD_OR_BAD COMMON_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, GOOD_OR_BAD (*readwritefunc) (BYTE *, size_t, off_t, struct parsedname *));
GOOD_OR_BAD COMMON_OWQ_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, GOOD_OR_BAD (*readwritefunc) (struct one_wire_query *, size_t, size_t));

ZERO_OR_ERROR COMMON_r_date( struct one_wire_query * owq ) ;
ZERO_OR_ERROR COMMON_w_date( struct one_wire_query * owq ) ;

GOOD_OR_BAD COMMON_read_memory_F0(struct one_wire_query *owq, size_t page, size_t pagesize);
GOOD_OR_BAD COMMON_read_memory_crc16_A5(struct one_wire_query *owq, size_t page, size_t pagesize);
GOOD_OR_BAD COMMON_read_memory_crc16_AA(struct one_wire_query *owq, size_t page, size_t pagesize);
GOOD_OR_BAD COMMON_read_memory_toss_counter(struct one_wire_query *owq, size_t page, size_t pagesize);
GOOD_OR_BAD COMMON_read_memory_plus_counter(BYTE * extra, size_t page, size_t pagesize, struct parsedname *pn);

ZERO_OR_ERROR COMMON_write_eprom_mem_owq(struct one_wire_query * owq) ;

ZERO_OR_ERROR COMMON_offset_process( ZERO_OR_ERROR (*func) (struct one_wire_query *), struct one_wire_query * owq, off_t shift_offset) ;

void BUS_lock(const struct parsedname *pn);
void BUS_unlock(const struct parsedname *pn);
void BUS_lock_in(struct connection_in *in);
void BUS_unlock_in(struct connection_in *in);
void CHANNEL_lock_in(struct connection_in *in);
void CHANNEL_unlock_in(struct connection_in *in);
void PORT_lock_in(struct connection_in *in);
void PORT_unlock_in(struct connection_in *in);

/* API wrappers for swig and owcapi */
enum restart_init { restart_if_repeat, continue_if_repeat, } ; // behavior if init called a second time
void API_setup(enum enum_program_type opt);
GOOD_OR_BAD API_init(const char *command_line, enum restart_init repeat);
GOOD_OR_BAD API_init_args(int argc, char **argv, enum restart_init repeat) ;
void API_set_error_level(const char *params);
void API_set_error_print(const char *params);
void API_finish(void);
int API_access_start(void);
void API_access_end(void);

#endif							/* OW_FUNCTION_H */
