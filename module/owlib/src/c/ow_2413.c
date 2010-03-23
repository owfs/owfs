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
READ_FUNCTION(FS_r_piostate);
WRITE_FUNCTION(FS_w_piostate);
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);
READ_FUNCTION(FS_r_latch);

/* ------- Structures ----------- */

struct aggregate A2413 = { 2, ag_letters, ag_aggregate, };
struct filetype DS2413[] = {
	F_STANDARD,
	{"piostate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_piostate, FS_w_piostate, INVISIBLE, NO_FILETYPE_DATA, },
	{"PIO", PROPERTY_LENGTH_BITFIELD, &A2413, ft_bitfield, fc_link, FS_r_pio, FS_w_pio, VISIBLE, NO_FILETYPE_DATA,},
	{"sensed", PROPERTY_LENGTH_BITFIELD, &A2413, ft_bitfield, fc_link, FS_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"latch", PROPERTY_LENGTH_BITFIELD, &A2413, ft_bitfield, fc_link, FS_r_latch, FS_w_pio, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(3A, DS2413, DEV_resume | DEV_ovdr);

#define _1W_PIO_ACCESS_READ 0xF5
#define _1W_PIO_ACCESS_WRITE 0x5A

#define _1W_2413_LATCH_MASK 0x0A
#define _1W_2413_CONFIRMATION 0xAA

/* ------- Functions ------------ */

/* DS2413 */
static int OW_write(BYTE data, const struct parsedname *pn);
static int OW_read(BYTE * data, const struct parsedname *pn);
static UINT SENSED_state(UINT status_bits);
static UINT LATCH_state(UINT status_bits);

static ZERO_OR_ERROR FS_r_piostate(struct one_wire_query *owq)
{
	/* surrogate property
	bit 0 PIOA pin state
	bit 1 PIOA latch state
	bit 2 PIOB pin state
	bit 3 PIOB latch state
	*/
	BYTE piostate ;

	if ( OW_read( &piostate, PN(owq) ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = piostate ;
	return 0 ;
}

/* write 2413 switch -- 2 values*/
static ZERO_OR_ERROR FS_w_piostate(struct one_wire_query *owq)
{
	UINT pio = OWQ_U(owq) ;

	FS_del_sibling( "piostate", owq ) ;
	return OW_write( pio, PN(owq) ) ? -EINVAL : 0 ;
}

/* 2413 switch */
/* complement of sense */
/* bits 0 and 2 */
static ZERO_OR_ERROR FS_r_pio(struct one_wire_query *owq)
{
	UINT piostate ;
	if ( FS_r_sibling_U( &piostate, "piostate", owq ) != 0 ) {
		return -EINVAL ;
	}
	// bits 0->0 and 2->1
	// complement
	OWQ_U(owq) = SENSED_state(piostate) ^ 0x03 ;
	return 0 ;
}

/* write 2413 switch -- 2 values*/
static ZERO_OR_ERROR FS_w_pio(struct one_wire_query *owq)
{
	// complement
	return FS_w_sibling_U( OWQ_U(owq) ^ 0x03 , "piostate", owq );
}

/* 2413 switch PIO sensed*/
/* bits 0 and 2 */
static ZERO_OR_ERROR FS_sense(struct one_wire_query *owq)
{
	UINT piostate ;
	if ( FS_r_sibling_U( &piostate, "piostate", owq ) != 0 ) {
		return -EINVAL ;
	}
	// bits 0->0 and 2->1
	OWQ_U(owq) = SENSED_state(piostate) ;
	return 0 ;
}

/* 2413 switch activity latch*/
/* bites 1 and 3 */
static ZERO_OR_ERROR FS_r_latch(struct one_wire_query *owq)
{
	UINT piostate ;
	if ( FS_r_sibling_U( &piostate, "piostate", owq ) != 0 ) {
		return -EINVAL ;
	}
	// bits 1>0 and 3->1
	OWQ_U(owq) = LATCH_state( piostate ) ;
	return 0 ;
}

/* read status byte */
static int OW_read(BYTE * data, const struct parsedname *pn)
{
	BYTE cmd[] = { _1W_PIO_ACCESS_READ, };
	BYTE resp[1] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(cmd),
		TRXN_READ1(resp),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}
	// High nibble the complement of low nibble?
	// Fix thanks to josef_heiler
	if ((resp[0] & 0x0F) != ((~resp[0] >> 4) & 0x0F)) {
		return 1;
	}

	data[0] = resp[0] & 0x0F ;
	return 0;
}

/* write status byte */
/* top 6 bits are set to 1, complement then sent */
static int OW_write(BYTE data, const struct parsedname *pn)
{
	BYTE data_masked = data | 0xFC ;
	BYTE cmd[] = { _1W_PIO_ACCESS_WRITE, data_masked, ~data_masked, };
	BYTE chk[1];
	BYTE confirm[] = { _1W_2413_CONFIRMATION, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(cmd),
		TRXN_READ1(chk),
		TRXN_COMPARE(chk,confirm,1),
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}

// piostate -> sense
static UINT SENSED_state(UINT status_bits)
{
	return ((status_bits & 0x01) >> 0) | ((status_bits & 0x04) >>1);
}

// piostate -> latch
static UINT LATCH_state(UINT status_bits)
{
	return ((status_bits & 0x02) >> 1) | ((status_bits & 0x08) >>2);
}
