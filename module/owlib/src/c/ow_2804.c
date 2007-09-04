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
#include "ow_2804.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);
READ_FUNCTION(FS_r_latch);
WRITE_FUNCTION(FS_w_latch);
READ_FUNCTION(FS_r_s_alarm);
WRITE_FUNCTION(FS_w_s_alarm);
READ_FUNCTION(FS_power);
READ_FUNCTION(FS_r_por);
READ_FUNCTION(FS_polarity);
WRITE_FUNCTION(FS_w_por);

/* ------- Structures ----------- */

struct aggregate A2804 = { 2, ag_numbers, ag_aggregate, };
struct aggregate A2804p = { 16, ag_numbers, ag_separate, };
struct filetype DS28E04[] = {
	F_STANDARD,
  {"memory", 550, NULL, ft_binary, fc_stable,   FS_r_mem, FS_w_mem, {v:NULL},} ,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"pages/page", 32, &A2804p, ft_binary, fc_stable,   FS_r_page, FS_w_page, {v:NULL},} ,
  {"polarity",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_polarity, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_power, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"por",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_r_por, FS_w_por, {v:NULL},} ,
  {"PIO",PROPERTY_LENGTH_BITFIELD, &A2804, ft_bitfield, fc_stable,   FS_r_pio, FS_w_pio, {v:NULL},} ,
  {"sensed",PROPERTY_LENGTH_BITFIELD, &A2804, ft_bitfield, fc_volatile,   FS_sense, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"latch",PROPERTY_LENGTH_BITFIELD, &A2804, ft_bitfield, fc_volatile,   FS_r_latch, FS_w_latch, {v:NULL},} ,
  {"set_alarm",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,   FS_r_s_alarm, FS_w_s_alarm, {v:NULL},} ,
};

DeviceEntryExtended(1C, DS28E04, DEV_alarm | DEV_resume | DEV_ovdr);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_READ_MEMORY 0xF0
#define _1W_PIO_ACCESS_READ 0xF5
#define _1W_PIO_ACCESS_WRITE 0x5A
#define _1W_READ_ACTIVE_LATCH 0xA5
#define _1W_WRITE_REGISTER 0xCC
#define _1W_RESET_ACTIVITY_LATCHES 0xC3

#define _1W_PIO_CONFIRMATION 0xAA

#define _ADDRESS_PIO_LOGIC 0x0220
#define _ADDRESS_PIO_OUTPUT 0x0221
#define _ADDRESS_PIO_ACTIVITY 0x0222
#define _ADDRESS_CONDITIONAL_SEARCH_PIO 0x0223
#define _ADDRESS_CONDITIONAL_SEARCH_CONTROL 0x0225

/* ------- Functions ------------ */

/* DS2804 */
static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn);
static int OW_w_scratch( BYTE * data,  size_t size,
						 off_t offset,  struct parsedname *pn);
static int OW_w_pio( BYTE data,  struct parsedname *pn);
//static int OW_access( BYTE * data , const struct parsedname * pn ) ;
static int OW_clear( struct parsedname *pn);
static int OW_w_reg( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn);

/* 2804 memory read */
static int FS_r_mem(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	/* read is not a "paged" endeavor, the CRC comes after a full read */
    if (OW_r_mem_simple( owq, 0, pagesize ))
		return -EINVAL;
    return OWQ_size(owq);
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	/* write is "byte at a time" -- not paged */
    if (OW_readwrite_paged(owq, 0, pagesize, OW_w_mem))
		return -EINVAL;
	return 0;
}

/* 2804 memory write */
static int FS_r_page(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
    if (OW_r_mem_simple( owq, OWQ_pn(owq).extension, pagesize))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	/* write is "byte at a time" -- not paged */
    if (OW_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem))
		return -EINVAL;
	return 0;
}

/* 2804 switch */
static int FS_r_pio(struct one_wire_query * owq)
{
	BYTE data;
	OWQ_make( owq_pio ) ;
    OWQ_create_temporary( owq_pio, (char *) &data, 1, _ADDRESS_PIO_OUTPUT, PN(owq) ) ;
    if (OW_r_mem_simple( owq_pio, 0, 0 ))
		return -EINVAL;
    OWQ_U(owq) = BYTE_INVERSE(data) & 0x03;	/* reverse bits */
	return 0;
}

/* write 2804 switch -- 2 values*/
static int FS_w_pio(struct one_wire_query * owq)
{
	BYTE data = 0;
	/* reverse bits */
    data = BYTE_INVERSE(OWQ_U(owq)) | 0xFC;	/* Set bits 2-7 to "1" */
    if (OW_w_pio(data, PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2804 switch -- is Vcc powered?*/
static int FS_power(struct one_wire_query * owq)
{
	BYTE data;
	OWQ_make( owq_power ) ;
    OWQ_create_temporary( owq_power, (char *) &data, 1, _ADDRESS_CONDITIONAL_SEARCH_CONTROL, PN(owq) ) ;
    if (OW_r_mem_simple( owq_power, 0, 0 ))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data, 7);
	return 0;
}

/* 2904 switch -- power-on status of polrity pin */
static int FS_polarity(struct one_wire_query * owq)
{
	BYTE data;
	OWQ_make( owq_polarity ) ;
    OWQ_create_temporary( owq_polarity, (char *) &data, 1, _ADDRESS_CONDITIONAL_SEARCH_CONTROL, PN(owq) ) ;
    if (OW_r_mem_simple( owq_polarity, 0, 0 ))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data, 6);
	return 0;
}

/* 2804 switch -- power-on status of polrity pin */
static int FS_r_por(struct one_wire_query * owq)
{
	BYTE data;
	OWQ_make( owq_por ) ;
    OWQ_create_temporary( owq_por, (char *) &data, 1, _ADDRESS_CONDITIONAL_SEARCH_CONTROL, PN(owq) ) ;
    if (OW_r_mem_simple( owq_por, 0, 0 ))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data, 3);
	return 0;
}

/* 2804 switch -- power-on status of polrity pin */
static int FS_w_por(struct one_wire_query * owq)
{
	BYTE data;
    struct parsedname * pn = PN(owq) ;
	OWQ_make( owq_por ) ;
    OWQ_create_temporary( owq_por, (char *) &data, 1, _ADDRESS_CONDITIONAL_SEARCH_CONTROL, pn ) ;
    if (OW_r_mem_simple( owq_por, 0, 0 ))
		return -EINVAL;			/* get current register */
	if (UT_getbit(&data, 3)) {	/* needs resetting? bit3==1 */
		data ^= 0x08;			/* flip bit 3 */
        if (OW_w_reg(&data, 1, _ADDRESS_CONDITIONAL_SEARCH_CONTROL, pn))
			return -EINVAL;		/* reset */
		if (FS_r_por(owq))
			return -EINVAL;		/* reread */
        if (OWQ_Y(owq))
			return -EINVAL;		/* not reset despite our try */
	}
	return 0;					/* good */
}

/* 2804 switch PIO sensed*/
static int FS_sense(struct one_wire_query * owq)
{
	BYTE data;
	OWQ_make( owq_sense ) ;
    OWQ_create_temporary( owq_sense, (char *) &data, 1, _ADDRESS_PIO_LOGIC, PN(owq) ) ;
    if (OW_r_mem_simple( owq_sense, 0, 0 ))
		return -EINVAL;
    OWQ_U(owq) = (data) & 0x03;
	return 0;
}

/* 2804 switch activity latch*/
static int FS_r_latch(struct one_wire_query * owq)
{
	BYTE data;
	OWQ_make( owq_latch ) ;
    OWQ_create_temporary( owq_latch, (char *) &data, 1, _ADDRESS_PIO_ACTIVITY, PN(owq) ) ;
    if (OW_r_mem_simple( owq_latch, 0, 0 ))
		return -EINVAL;
    OWQ_U(owq) = data & 0x03;
	return 0;
}

/* 2804 switch activity latch*/
static int FS_w_latch(struct one_wire_query * owq)
{
    if (OW_clear(PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2804 alarm settings*/
static int FS_r_s_alarm(struct one_wire_query * owq)
{
	BYTE data[3];
	OWQ_make( owq_alarm ) ;
    OWQ_create_temporary( owq_alarm, (char *) data, 3, _ADDRESS_CONDITIONAL_SEARCH_PIO, PN(owq) ) ;
    if (OW_r_mem_simple( owq_alarm, 0, 0 ))
		return -EINVAL;
    OWQ_U(owq) = (data[2] & 0x03) * 100;
    OWQ_U(owq) += UT_getbit(&data[1], 0) | (UT_getbit(&data[0], 0) << 1);
    OWQ_U(owq) += UT_getbit(&data[1], 1) | (UT_getbit(&data[0], 1) << 1) * 10;
	return 0;
}

/* 2804 alarm settings*/
static int FS_w_s_alarm(struct one_wire_query * owq)
{
	BYTE data[3] = { 0, 0, 0, };
    UINT U = OWQ_U(owq) ;
	OWQ_make( owq_alarm ) ;
    OWQ_create_temporary( owq_alarm, (char *) &data[2], 1, _ADDRESS_CONDITIONAL_SEARCH_CONTROL, PN(owq) ) ;
    if (OW_r_mem_simple( owq_alarm, 0, 0 ))
		return -EINVAL;
    data[2] |= (U / 100 % 10) & 0x03;
	UT_setbit(&data[1], 0, (int) (U % 10) & 0x01);
	UT_setbit(&data[1], 1, (int) (U / 10 % 10) & 0x01);
	UT_setbit(&data[0], 0, ((int) (U % 10) & 0x02) >> 1);
	UT_setbit(&data[0], 1, ((int) (U / 10 % 10) & 0x02) >> 1);
    if (OW_w_reg(data, 3, _ADDRESS_CONDITIONAL_SEARCH_PIO, PN(owq)))
		return -EINVAL;
	return 0;
}

static int OW_w_scratch( BYTE * data,  size_t size,
						 off_t offset,  struct parsedname *pn)
{
	BYTE p[3 + 32 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE(p,3+size),
		TRXN_END,
	};
	struct transaction_log tcrc[] = {
		TRXN_START,
        TRXN_WR_CRC16(p,3+size,0),
		TRXN_END,
	};

	memcpy(&p[3], data, size);
	if (((size + offset) & 0x1F) == 0) {	/* Check CRC if write is to end of page */
		return BUS_transaction(tcrc, pn);
	} else {
		return BUS_transaction(t, pn);
	}
}

/* pre-paged */
static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn)
{
	BYTE p[4 + 32 + 2] = { _1W_READ_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log tread[] = {
		TRXN_START,
        TRXN_WR_CRC16(p,3,1+size),
        TRXN_COMPARE(&p[4],data,size),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
        TRXN_WRITE3(p),
		{&p[3], &p[3], 10, trxn_power,},
		TRXN_END,
	};

	if (OW_w_scratch(data, size, offset, pn)
		|| BUS_transaction(tread, pn)
		)
		return 1;
	p[0] = _1W_COPY_SCRATCHPAD;
	return BUS_transaction(tcopy, pn);
}

//* write status byte */
static int OW_w_reg( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn)
{
	BYTE p[3] = { _1W_WRITE_REGISTER, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE3(p),
        TRXN_READ(data,size),
		TRXN_END,
	};
	return BUS_transaction(t, pn);
}

/* set PIO state bits: bit0=A bit1=B, value: open=1 closed=0 */
static int OW_w_pio( BYTE data,  struct parsedname *pn)
{
	BYTE p[3] = { _1W_PIO_ACCESS_WRITE, BYTE_MASK(data), BYTE_INVERSE(data), };
    BYTE confirm[] = { _1W_PIO_CONFIRMATION, } ;
    BYTE resp[1];
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE3(p),
        TRXN_READ1(resp),
        TRXN_COMPARE(resp,confirm,1),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

/* Clear latches */
static int OW_clear( struct parsedname *pn)
{
    BYTE cmd[] = { _1W_RESET_ACTIVITY_LATCHES, } ;
    BYTE resp[1] ;
    BYTE confirm[] = { _1W_PIO_CONFIRMATION, } ;
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE1(cmd),
        TRXN_READ1(resp),
        TRXN_COMPARE(resp,confirm,1),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}
