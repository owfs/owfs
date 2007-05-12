/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

// DS1954 Cryptographics iButton

#include <config.h>
#include "owfs_config.h"
#include "ow_1954.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
READ_FUNCTION(FS_r_ipr);
WRITE_FUNCTION(FS_w_ipr);
READ_FUNCTION(FS_r_io);
WRITE_FUNCTION(FS_w_io);
READ_FUNCTION(FS_r_status);
WRITE_FUNCTION(FS_w_status);
WRITE_FUNCTION(FS_reset);

/* ------- Structures ----------- */

struct filetype DS1954[] = {
	F_STANDARD,
  {"ipr", 128, NULL, ft_binary, fc_volatile, {o: FS_r_ipr}, {o: FS_w_ipr}, {v:NULL},},
  {"io_buffer", 8, NULL, ft_binary, fc_volatile, {o: FS_r_io}, {o: FS_w_io}, {v:NULL},},
  {"status", 4, NULL, ft_binary, fc_volatile, {o: FS_r_status}, {o: FS_w_status}, {v:NULL},},
  {"reset", 1, NULL, ft_yesno, fc_volatile, {o:NULL}, {o: FS_reset}, {v:NULL},},
};

DeviceEntryExtended(16, DS1954, DEV_ovdr );

#define _1W_WRITE_IPR 0x0F
#define _1W_READ_IPR 0xAA
#define _1W_WRITE_IO_BUFFER 0x2D
#define _1W_READ_IO_BUFFER 0x22
#define _1W_START_PROGRAM 0x77
#define _1W_CONTINUE_PROGRAM 0x87
#define _1W_RESET_MICRO 0xDD
#define _1W_WRITE_STATUS 0xD2
#define _1W_READ_STATUS 0xE1

#define _1W_WRITE_RELEASE_SEQUENCE 0x9D,0xB3
#define _1W_READ_RELEASE_SEQUENCE 0x62,0x4C
#define _1W_START_RELEASE_SEQUENCE 0x6D,0x43
#define _1W_CONTINUE_RELEASE_SEQUENCE 0x5D,0x73
#define _1W_RESET_RELEASE_SEQUENCE 0x92,0xBC
#define _1W_STATUS_RELEASE_SEQUENCE 0x51,0x7F

/* ------- Functions ------------ */

/* DS1902 */
static int OW_w_ipr( struct one_wire_query * owq ) ;
static int OW_r_ipr( struct one_wire_query * owq ) ;
static int OW_w_io( struct one_wire_query * owq ) ;
static int OW_r_io( struct one_wire_query * owq ) ;
static int OW_w_status( struct one_wire_query * owq ) ;
static int OW_r_status( struct one_wire_query * owq ) ;
static int OW_reset( struct one_wire_query * owq ) ;

/* 1954 */
static int FS_w_ipr(struct one_wire_query * owq)
{
    if (OW_w_ipr(owq))
		return -EINVAL;
	return 0;
}

static int FS_r_ipr(struct one_wire_query * owq)
{
    if (OW_r_ipr(owq))
		return -EINVAL;
	return 0;
}

static int FS_w_io(struct one_wire_query * owq)
{
    if (OW_w_io(owq))
		return -EINVAL;
	return 0;
}

static int FS_r_io(struct one_wire_query * owq)
{
    if (OW_r_io(owq))
		return -EINVAL;
	return 0;
}

static int FS_w_status(struct one_wire_query * owq)
{
    if (OW_w_status(owq))
		return -EINVAL;
	return 0;
}

static int FS_r_status(struct one_wire_query * owq)
{
    if (OW_r_status(owq))
		return -EINVAL;
	return 0;
}

static int FS_reset(struct one_wire_query * owq)
{
    if (OW_reset(owq))
		return -EINVAL;
	return 0;
}

static int OW_w_ipr( struct one_wire_query * owq ) {
    int size = OWQ_size(owq) ;
    BYTE p[2+128+2] = { _1W_WRITE_IPR, (size&0xFF), } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 2+size, trxn_match, } ,
        { NULL, &p[2+size], 2, trxn_read, } ,
        { p, NULL, 2+size+2, trxn_crc16, } ,
        TRXN_END,
    } ;
    return BUS_transaction( t, PN(owq) ) ;
} ;

/* Read all 128 bytes, then transfer over what's needed */
static int OW_r_ipr( struct one_wire_query * owq ) {
    BYTE p[2+128+2] = { _1W_READ_IPR, 128, } ;
    int size = OWQ_size(owq) ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 2, trxn_match, } ,
        { NULL, &p[2], 128+2, trxn_read, } ,
        { p, NULL, 2+128+2, trxn_crc16, } ,
        TRXN_END,
    } ;
    if ( BUS_transaction( t, PN(owq) ) ) return 1 ;
    if ( size > 128 ) size = 128 ;
    memcpy( OWQ_buffer(owq), &p[2], size ) ;
    OWQ_length(owq) = size ;
    return 0 ;
} ;

static int OW_w_status( struct one_wire_query * owq ) {
    int size = OWQ_size(owq) ;
    BYTE p[1+1+2] = { _1W_WRITE_STATUS, (size&0xFF), } ;
    BYTE release[2] = { _1W_STATUS_RELEASE_SEQUENCE, } ;
    BYTE zero[2] = { 0x00, 0x00, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 1, trxn_match, } ,
        { NULL, &p[1], 3, trxn_read, } ,
        { p, NULL, 1+1+2, trxn_crc16, } ,
        { release, release, 2, trxn_read, },
        { release, zero, 2, trxn_match, },
        TRXN_END,
    } ;
    return BUS_transaction( t, PN(owq) ) ;
} ;

static int OW_r_status( struct one_wire_query * owq ) {
    BYTE p[1+4+2] = { _1W_READ_STATUS, } ;
    int size = OWQ_size(owq) ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 1, trxn_match, } ,
        { NULL, &p[1], 4+2, trxn_read, } ,
        { p, NULL, 1+4+2, trxn_crc16, } ,
        TRXN_END,
    } ;
    if ( BUS_transaction( t, PN(owq) ) ) return 1 ;
    if ( size > 4 ) size = 4 ;
    memcpy( OWQ_buffer(owq), &p[1], size ) ;
    OWQ_length(owq) = size ;
    return 0 ;
} ;

static int OW_w_io( struct one_wire_query * owq ) {
    int size = OWQ_size(owq) ;
    BYTE p[2+8+2] = { _1W_WRITE_IO_BUFFER, (size&0xFF), } ;
    BYTE release[2] = { _1W_WRITE_RELEASE_SEQUENCE, } ;
    BYTE zero[2] = { 0x00, 0x00, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 2+size, trxn_match, } ,
        { NULL, &p[2+size], 2, trxn_read, } ,
        { p, NULL, 2+size+2, trxn_crc16, } ,
        { release, release, 2, trxn_read, },
        { release, zero, 2, trxn_match, },
        TRXN_END,
    } ;
    return BUS_transaction( t, PN(owq) ) ;
} ;

static int OW_r_io( struct one_wire_query * owq ) {
    int size = OWQ_size(owq) ;
    BYTE p[2+8+2] = { _1W_READ_IO_BUFFER, (size&0xFF), } ;
    BYTE release[2] = { _1W_READ_RELEASE_SEQUENCE, } ;
    BYTE zero[2] = { 0x00, 0x00, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 2, trxn_match, } ,
        { NULL, &p[2], size+2, trxn_read, } ,
        { p, NULL, 2+size+2, trxn_crc16, } ,
        { release, release, 2, trxn_read, },
        { release, zero, 2, trxn_match, },
        TRXN_END,
    } ;
    if ( BUS_transaction( t, PN(owq) ) ) return 1 ;
    memcpy( OWQ_buffer(owq), &p[2], size ) ;
    OWQ_length(owq) = size ;
    return 0 ;
} ;

static int OW_reset( struct one_wire_query * owq ) {
    BYTE p[] = { _1W_RESET_MICRO, } ;
    BYTE release[2] = { _1W_RESET_RELEASE_SEQUENCE, } ;
    BYTE zero[2] = { 0x00, 0x00, } ;
    BYTE testbit[1] ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 1, trxn_match, } ,
        { release, release, 2, trxn_read, },
        { release, zero, 2, trxn_match, },
        { NULL, testbit, 1, trxn_read, } ,
        TRXN_END,
    } ;
    if ( BUS_transaction( t, PN(owq) ) || testbit[0]&0x80 ) return 1;
    return 0 ;
} ;

