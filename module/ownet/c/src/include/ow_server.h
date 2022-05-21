/*
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
    of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#ifndef OW_SERVER_H				/* tedious wrapper */
#define OW_SERVER_H

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

struct request_packet {
	struct connection_in *owserver;
	const ASCII *path;
	unsigned char *read_value;
	const unsigned char *write_value;
	size_t data_length;
	off_t data_offset;
	int error_code;
	int tokens;					/* for anti-loop work */
	BYTE *tokenstring;			/* List of tokens from owservers passed */
};

extern struct ow_global {
	uint32_t control_flags;
	int timeout_network;
	int timeout_server;
	int autoserver;
} ow_Global;

#define SHOULD_RETURN_BUS_LIST    ( (UINT) 0x00000002 )
#define PERSISTENT_MASK    ( (UINT) 0x00000004 )
#define PERSISTENT_BIT     2
#define ALIAS_REQUEST      ( (UINT) 0x00000008 )
#define TEMPSCALE_MASK ( (UINT) 0x00FF0000 )
#define TEMPSCALE_BIT  16
#define DEVFORMAT_MASK ( (UINT) 0xFF000000 )
#define DEVFORMAT_BIT  24
#define TRIM                        ( (UINT) 0x00000040 )
#define IsPersistent         ( ow_Global.control_flags & PERSISTENT_MASK )
#define SetPersistent(b)      UT_Setbit(ow_Global.control_flags,PERSISTENT_BIT,(b))
#define TemperatureScale     ( (enum temp_type) ((ow_Global.control_flags & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define SGTemperatureScale(x)    ( (enum temp_type)(((x) & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define DeviceFormat         ( (enum deviceformat) ((ow_Global.control_flags & DEVFORMAT_MASK) >> DEVFORMAT_BIT) )
#define set_semiglobal(s, mask, bit, val) do { *(s) = (*(s) & ~(mask)) | ((val)<<bit); } while(0)

int ServerPresence(struct request_packet *rp);
int ServerRead(struct request_packet *rp);
int ServerWrite(struct request_packet *rp);
int ServerDir(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp);

#endif							/* OW_SERVER_H */
