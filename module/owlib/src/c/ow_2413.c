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

/* Changes
    7/2004 Extensive improvements based on input from Serg Oskin
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2413.h"

/* ------- Prototypes ----------- */

/* DS2413 switch */
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);
READ_FUNCTION(FS_r_latch);

/* ------- Structures ----------- */

struct aggregate A2413 = { 2, ag_letters, ag_aggregate, };
struct filetype DS2413[] = {
	F_STANDARD,
  {"PIO", PROPERTY_LENGTH_BITFIELD, &A2413, ft_bitfield, fc_volatile, FS_r_pio, FS_w_pio, {v:NULL},},
  {"sensed", PROPERTY_LENGTH_BITFIELD, &A2413, ft_bitfield, fc_volatile, FS_sense, NO_WRITE_FUNCTION, {v:NULL},},
  {"latch", PROPERTY_LENGTH_BITFIELD, &A2413, ft_bitfield, fc_volatile, FS_r_latch, NO_WRITE_FUNCTION, {v:NULL},},
};

DeviceEntryExtended(3A, DS2413, DEV_resume | DEV_ovdr);

#define _1W_PIO_ACCESS_READ 0xF5
#define _1W_PIO_ACCESS_WRITE 0x5A

/* ------- Functions ------------ */

/* DS2413 */
static int OW_write(BYTE data, const struct parsedname *pn);
static int OW_read(BYTE * data, const struct parsedname *pn);

/* 2413 switch */
/* complement of sense */
static int FS_r_pio(struct one_wire_query *owq)
{
    OWQ_allocate_struct_and_pointer(owq_sibling);

    OWQ_create_shallow_single(owq_sibling, owq);

    if (FS_read_sibling("sensed", owq_sibling)) {
        return -EINVAL;
    }
    
    OWQ_U(owq) = OWQ_U(owq_sibling) ^ 0x03 ;
	return 0;
}

/* 2413 switch PIO sensed*/
/* bits 0 and 2 */
static int FS_sense(struct one_wire_query *owq)
{
    BYTE data;
    if ( OW_read( &data, PN(owq) ) ) {
        return -EINVAL ;
    }

    // bits 0->0 and 2->1
    OWQ_U(owq) = (data & 0x01) | ((data>>1) & 0x02) ;
	return 0;
}

/* 2413 switch activity latch*/
/* bites 1 and 3 */
static int FS_r_latch(struct one_wire_query *owq)
{
    BYTE data;
    if ( OW_read( &data, PN(owq) ) ) {
        return -EINVAL ;
    }

    // bits 1->0 and 3->1
    OWQ_U(owq) = ((data>>1) & 0x01) | ((data>>2) & 0x02) ;
    return 0;
}

/* write 2413 switch -- 2 values*/
static int FS_w_pio(struct one_wire_query *owq)
{
	/* reverse bits */
	BYTE data = OWQ_U(owq) & 0x03 ;
    if (OW_write(data ^ 0x03, PN(owq))) {
		return -EINVAL;
    }
	return 0;
}

/* read status byte */
static int OW_read(BYTE * data, const struct parsedname *pn)
{
	BYTE p[] = { _1W_PIO_ACCESS_READ, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_READ1(data),
		TRXN_END,
	};

    if (BUS_transaction(t, pn)) {
		return 1;
    }
    
    // High nibble the complement of low nibble?
    if ((data[0] & 0x0F) + ((data[0] >> 4) & 0x0F) != 0x0F) {
		return 1;
    }

	return 0;
}

/* write status byte */
/* top 6 bits are set to 1, complement then sent */
static int OW_write(BYTE data, const struct parsedname *pn)
{
	BYTE p[] = { _1W_PIO_ACCESS_WRITE, data | 0xFC, (~data) & 0x03, };
	BYTE q[2];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ2(q),
		TRXN_END,
	};

    if (BUS_transaction(t, pn)) {
		return 1;
    }
    if (q[0] != 0xAA) {
		return 1;
    }

	return 0;
}
