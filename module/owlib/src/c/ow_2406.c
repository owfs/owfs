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
#include "ow_2406.h"

#define TAI8570					/* AAG barometer */

/* ------- Prototypes ----------- */

/* DS2406 switch */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_infobyte);
READ_FUNCTION(FS_r_flipflop);
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);
READ_FUNCTION(FS_r_latch);
WRITE_FUNCTION(FS_w_latch);
READ_FUNCTION(FS_r_s_alarm);
WRITE_FUNCTION(FS_w_s_alarm);
READ_FUNCTION(FS_power);
READ_FUNCTION(FS_channel);
#if OW_TAI8570
READ_FUNCTION(FS_sibling);
READ_FUNCTION(FS_temp);
READ_FUNCTION(FS_pressure);
#endif							/* OW_TAI8570 */
 READ_FUNCTION(FS_voltage);

/* ------- Structures ----------- */

struct aggregate A2406 = { 2, ag_letters, ag_aggregate, };
struct aggregate A2406p = { 4, ag_numbers, ag_separate, };
struct aggregate AT8Ac = { 8, ag_numbers, ag_separate, }; // 8 channel T8A volt meter
struct filetype DS2406[] = {
	F_STANDARD,
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_stable, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"pages/page", 32, &A2406p, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
  {"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"channels", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_channel, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"PIO", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_stable, FS_r_pio, FS_w_pio, VISIBLE, NO_FILETYPE_DATA,},
  {"sensed", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_link, FS_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"latch", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_volatile, FS_r_latch, FS_w_latch, VISIBLE, NO_FILETYPE_DATA,},
  {"flipflop", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_volatile, FS_r_flipflop, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"set_alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_s_alarm, FS_w_s_alarm, VISIBLE, NO_FILETYPE_DATA,},
  {"infobyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_infobyte, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
#if OW_TAI8570
{"TAI8570", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
{"TAI8570/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
{"TAI8570/pressure", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_pressure, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"TAI8570/sibling", 16, NON_AGGREGATE, ft_ascii, fc_stable, FS_sibling, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
#endif							/* OW_TAI8570 */
{"T8A", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
   {"T8A/volt", PROPERTY_LENGTH_FLOAT, &AT8Ac, ft_float, fc_volatile, FS_voltage, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(12, DS2406, DEV_alarm);

#define _1W_READ_MEMORY 0xF0
#define _1W_EXTENDED_READ_MEMORY 0xA5
#define _1W_WRITE_MEMORY 0x0F
#define _1W_WRITE_STATUS 0x55
#define _1W_READ_STATUS 0xAA
#define _1W_CHANNEL_ACCESS 0xF5

#define _DS2406_ALR  0x80
#define _DS2406_IM   0x40
#define _DS2406_TOG  0x20
#define _DS2406_IC   0x10
#define _DS2406_CHS1 0x08
#define _DS2406_CHS0 0x04
#define _DS2406_CRC1 0x02
#define _DS2406_CRC0 0x01

#define _DS2406_POWER_BIT     0x80
#define _DS2406_FLIPFLOP_BITS 0x60
#define _DS2406_ALARM_BITS    0x1F

#define _ADDRESS_STATUS_MEMORY_SRAM 0x0007

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
//static int OW_r_s_alarm( BYTE * data , const struct parsedname * pn ) ;
static int OW_w_s_alarm(const BYTE data, const struct parsedname *pn);
static int OW_r_control(BYTE * data, const struct parsedname *pn);
static int OW_w_control(const BYTE data, const struct parsedname *pn);
static int OW_w_pio(const BYTE data, const struct parsedname *pn);
static int OW_access(BYTE * data, const struct parsedname *pn);
static int OW_syncaccess(BYTE * data, const struct parsedname *pn);
static int OW_clear(const struct parsedname *pn);
static int OW_full_access(BYTE * data, const struct parsedname *pn);

/* 2406 memory read */
static int FS_r_mem(struct one_wire_query *owq)
{
	/* read is not a "paged" endeavor, the CRC comes after a full read */
	if (OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) {
		return -EINVAL;
	}
	return OWQ_size(owq);
}

/* 2406 memory write */
static int FS_r_page(struct one_wire_query *owq)
{
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
	if (OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) (OWQ_offset(owq) + (OWQ_pn(owq).extension << 5)), PN(owq))) {
		return -EINVAL;
	}
	return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query *owq)
{
	if (COMMON_write_eprom_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) (OWQ_offset(owq) + (OWQ_pn(owq).extension << 5)), PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(struct one_wire_query *owq)
{
	/* write is "byte at a time" -- not paged */
	if (COMMON_write_eprom_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* 2406 switch */
static int FS_r_pio(struct one_wire_query *owq)
{
	UINT infobyte ;

	if ( FS_r_sibling_U( &infobyte, "infobyte", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = BYTE_INVERSE(infobyte>>0) & 0x03;	/* reverse bits */
	return 0;
}

/* 2406 switch */
static int FS_r_flipflop(struct one_wire_query *owq)
{
	UINT infobyte ;

	if ( FS_r_sibling_U( &infobyte, "infobyte", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = (infobyte>>0) & 0x03;	/* reverse bits */
	return 0;
}

/* 2406 switch */
static int FS_r_infobyte(struct one_wire_query *owq)
{
	BYTE data;
	if (OW_syncaccess(&data, PN(owq))) {
		return -EINVAL;
	}
	OWQ_U(owq) = data;	/* reverse bits */
	return 0;
}

/* 2406 switch -- is Vcc powered?*/
static int FS_power(struct one_wire_query *owq)
{
	UINT infobyte ;

	if ( FS_r_sibling_U( &infobyte, "infobyte", owq ) ) {
		return -EINVAL ;
	}

	OWQ_Y(owq) = (infobyte & _DS2406_POWER_BIT) ? 1 : 0;
	return 0;
}

/* 2406 switch -- number of channels (actually, if Vcc powered)*/
static int FS_channel(struct one_wire_query *owq)
{
	UINT infobyte ;

	if ( FS_r_sibling_U( &infobyte, "infobyte", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = (infobyte & 0x40) ? 2 : 1;
	return 0;
}

/* 2406 switch PIO sensed*/
/* bits 2 and 3 */
static int FS_sense(struct one_wire_query *owq)
{
	UINT infobyte ;

	if ( FS_r_sibling_U( &infobyte, "infobyte", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = (infobyte>>2) & 0x03;

	return 0 ;
}

/* 2406 switch activity latch*/
/* bites 4 and 5 */
static int FS_r_latch(struct one_wire_query *owq)
{
	UINT infobyte ;

	if ( FS_r_sibling_U( &infobyte, "infobyte", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = (infobyte >> 4) & 0x03;

	return 0;
}

/* 2406 switch activity latch*/
static int FS_w_latch(struct one_wire_query *owq)
{
	FS_del_sibling( "infobyte", owq ) ;

	if (OW_clear(PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* 2406 alarm settings*/
static int FS_r_s_alarm(struct one_wire_query *owq)
{
	BYTE data;
	if (OW_r_control(&data, PN(owq))) {
		return -EINVAL;
	}
	OWQ_U(owq) = (data & 0x01) + ((data >> 1) & 0x03) * 10 + ((data >>3) & 0x03) * 100;
	return 0;
}

/* 2406 alarm settings*/
static int FS_w_s_alarm(struct one_wire_query *owq)
{
	BYTE data;
	UINT U = OWQ_U(owq);
	data = ((U % 10) & 0x01)
		| (((U / 10 % 10) & 0x03) << 1)
		| (((U / 100 % 10) & 0x03) << 3);
	if (OW_w_s_alarm(data, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* write 2406 switch -- 2 values*/
static int FS_w_pio(struct one_wire_query *owq)
{
	BYTE data = BYTE_INVERSE(OWQ_U(owq)) & 0x03 ; /* reverse bits */

	FS_del_sibling( "infobyte", owq ) ;

	if (OW_w_pio(data, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* Support for EmbeddedDataSystems's T8A 8 channel A/D
   Written by Chase Shimmin cshimmin@berkeley.edu */
static int FS_voltage(struct one_wire_query *owq)
{
	// channel select byte, based on zero-indexed channel number
	BYTE ch_select = (OWQ_pn(owq).extension << 2) + 0x02;
	BYTE channel_info ;
	BYTE data[8] = { ch_select, } ;

	// this is the complete byte sequence we want to write to the ds2406 so it will
	// select the appropriate channel and initiate adc, and also so it can write
	// back the results to the trailing 0xFF bytes.
	BYTE p[] = { _1W_CHANNEL_ACCESS,
		_DS2406_ALR|_DS2406_TOG|_DS2406_CHS0|_DS2406_CRC1, 0xFF, // Channel control
	} ;

	// most & least significant bytes. for the latter, we're actually only interested
	// in the least significant nibble.
	BYTE msb, lsb;

	// 'modify' transaction -- so we can read back the trailing bytes.
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ1(&channel_info),
		TRXN_WR_CRC16(data,1,7),
		TRXN_END,
	};

	FS_del_sibling( "infobyte", owq ) ;

	if (BUS_transaction(t, PN(owq))) {
		return -EINVAL;
	}

	// grab the msb from the adc (in this case, it will be written in the 6th byte sent out)
	// then take the ones complement and reverse bits.
	msb = data[5];
	msb = ~msb;
	msb = ((msb * 0x0802LU & 0x22110LU) | (msb * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;

	// grab the lsb (8th byte), 1s complement, reverse it, and take the 4 original L.S. bits.
	lsb = data[7];
	lsb = ~lsb;
	lsb = ((lsb * 0x0802LU & 0x22110LU) | (lsb * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
	lsb >>= 4;
	lsb &= 0xF;

	// convert the traslated and combined msb and lsb into voltages (this is a 0-5v 12-bit adc
	// so multiply by 5 and divide by 2^12. Then write it into the float field in the OWQ union & return
	OWQ_F(owq) = ((msb << 4) + lsb) * 5.0 / 4096;
	return 0;
}

static int OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[3 + 128] = { _1W_READ_MEMORY, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ(&p[3], size),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}

	memcpy(data, &p[3], size);
	return 0;
}

/* read status byte */
static int OW_r_control(BYTE * data, const struct parsedname *pn)
{
	BYTE p[3 + 1 + 2] = { _1W_READ_STATUS, LOW_HIGH_ADDRESS(_ADDRESS_STATUS_MEMORY_SRAM),
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 1),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}

	*data = p[3];
	return 0;
}

/* write status byte */
static int OW_w_control(const BYTE data, const struct parsedname *pn)
{
	BYTE p[3 + 1 + 2] = { _1W_WRITE_STATUS, LOW_HIGH_ADDRESS(_ADDRESS_STATUS_MEMORY_SRAM),
		data,
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, 0),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}

	return 0;
}

/* write alarm settings */
static int OW_w_s_alarm(const BYTE data, const struct parsedname *pn)
{
	BYTE b[1];
	if (OW_r_control(b, pn)) {
		return 1;
	}
    b[0] = ( b[0] & ~_DS2406_ALARM_BITS) | (data & _DS2406_ALARM_BITS) ;
	return OW_w_control(b[0], pn);
}

/* set PIO state bits: bit0=A bit1=B, value: open=1 closed=0 */
static int OW_w_pio(const BYTE data, const struct parsedname *pn)
{
	BYTE b[1];
	if (OW_r_control(b, pn)) {
		return 1;
	}
    b[0] = ( b[0] & ~_DS2406_FLIPFLOP_BITS) | ((data<<5) & _DS2406_FLIPFLOP_BITS) ;
    return OW_w_control(b[0], pn);
}

static int OW_syncaccess(BYTE * data, const struct parsedname *pn)
{
    BYTE d[2] = { _DS2406_IM|_DS2406_CHS1|_DS2406_CHS0|_DS2406_CRC0, 0xFF, };
    if (OW_full_access(d, pn)) {
		return 1;
    }
	data[0] = d[0];
	return 0;
}

static int OW_access(BYTE * data, const struct parsedname *pn)
{
    BYTE d[2] = { _DS2406_IM|_DS2406_IC|_DS2406_CHS1|_DS2406_CHS0|_DS2406_CRC0, 0xFF, };
    if (OW_full_access(d, pn)) {
		return 1;
    }
	data[0] = d[0];
	return 0;
}

/* Clear latches */
static int OW_clear(const struct parsedname *pn)
{
    BYTE data[2] = { _DS2406_ALR|_DS2406_IM|_DS2406_IC|_DS2406_CHS0|_DS2406_CRC0, 0xFF, };
	return OW_full_access(data, pn);;
}

// write both control bytes, and read both back
static int OW_full_access(BYTE * data, const struct parsedname *pn)
{
	BYTE p[3 + 2 + 2] = { _1W_CHANNEL_ACCESS, data[0], data[1], };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 2),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}
	//printf("DS2406 access %.2X %.2X -> %.2X %.2X \n",data[0],data[1],p[3],p[4]);
	//printf("DS2406 CRC ok\n");
	data[0] = p[3];
	data[1] = p[4];
	return 0;
}

#if OW_TAI8570
struct s_TAI8570 {
	BYTE master[8];
	BYTE sibling[8];
	BYTE reader[8];
	BYTE writer[8];
	UINT C[6];
};
/* Internal files */
//static struct internal_prop ip_bar = { "BAR", fc_persistent };
MakeInternalProp(BAR, fc_persistent);

// Read from the TAI8570 microcontroller vias the paired DS2406s
// Updated by Simon Melhuish, with ref. to AAG C++ code
static BYTE SEC_READW4[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04,
	0x04, 0x00
};

static BYTE SEC_READW2[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x0E, 0x04, 0x04, 0x04,
	0x04, 0x00
};
static BYTE SEC_READW1[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x04,
	0x04, 0x00
};
static BYTE SEC_READW3[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x0E, 0x04, 0x04, 0x0E, 0x04, 0x04,
	0x04, 0x00
};

static BYTE SEC_READD1[] = { 0x0E, 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x00
};
static BYTE SEC_READD2[] = { 0x0E, 0x0E, 0x0E, 0x0E, 0x04, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04,
	0x00
};
static BYTE SEC_RESET[] = { 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E,
	0x04, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00
};

static BYTE CFG_READ = _DS2406_ALR|_DS2406_IM|_DS2406_TOG|_DS2406_CHS1|_DS2406_CHS0;	// '11101100'   Configuraci� de lectura para DS2407
static BYTE CFG_WRITE = _DS2406_ALR|_DS2406_CHS1|_DS2406_CHS0;	// '10001100'  Configuraci� de Escritura para DS2407
static BYTE CFG_READPULSE = _DS2406_ALR|_DS2406_IM|_DS2406_CHS1;	// '11001000'  Configuraci� de lectura de Pulso de conversion para DS2407

static int ReadTmexPage(BYTE * data, size_t size, int page, const struct parsedname *pn);
static int TAI8570_Calibration(UINT * cal, const struct s_TAI8570 *tai, struct parsedname *pn);
static int testTAI8570(struct s_TAI8570 *tai, struct one_wire_query *owq);
static int TAI8570_Write(BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_Read(UINT * u, const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_Reset(const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_Check(const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_ClockPulse(const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_DataPulse(const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_CalValue(UINT * cal, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_SenseValue(UINT * val, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn);
static int TAI8570_A(struct parsedname *pn);
static int TAI8570_B(struct parsedname *pn);

static int FS_sibling(struct one_wire_query *owq)
{
	ASCII sib[16];
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if (testTAI8570(&tai, owq)) {
		return -ENOENT;
	}
	bytes2string(sib, tai.sibling, 8);
	return Fowq_output_offset_and_size(sib, 16, owq);
}

static int FS_temp(struct one_wire_query *owq)
{
	UINT D2;
	int UT1, dT;
	struct s_TAI8570 tai;
	struct parsedname pn_copy;

	FS_del_sibling( "infobyte", owq ) ;

	memcpy(&pn_copy, PN(owq), sizeof(struct parsedname));	//shallow copy
	if (testTAI8570(&tai, owq)) {
		return -ENOENT;
	}

	UT1 = 8 * tai.C[4] + 20224;
	if (TAI8570_SenseValue(&D2, SEC_READD2, &tai, &pn_copy)) {
		return -EINVAL;
	}
	LEVEL_DEBUG("TAI8570 Raw Temperature (D2) = %lu\n", D2);
	dT = D2 - UT1;
	OWQ_F(owq) = (200. + dT * (tai.C[5] + 50.) / 1024.) / 10.;
	return 0;
}

static int FS_pressure(struct one_wire_query *owq)
{
	_FLOAT TEMP ;

	FS_del_sibling( "infobyte", owq ) ;

	if ( FS_r_sibling_F( &TEMP, "TAI8570/temperature", owq )==0 ) {
		struct parsedname pn_copy;
		struct s_TAI8570 tai;
		UINT D1;

		memcpy(&pn_copy, PN(owq), sizeof(struct parsedname));	//shallow copy
		if (testTAI8570(&tai, owq)) {
			return -ENOENT;
		} else if (TAI8570_SenseValue(&D1, SEC_READD1, &tai, &pn_copy)) {
			return -EINVAL;
		} else {
			_FLOAT dT = (TEMP * 10. - 200.) * 1024. / (tai.C[5] + 50.);
			_FLOAT OFF = 4. * tai.C[1] + ((tai.C[3] - 512.) * dT) / 4096.;
			_FLOAT SENS = 24576. + tai.C[0] + (tai.C[2] * dT) / 1024.;
			_FLOAT X = (SENS * (D1 - 7168.)) / 16384. - OFF;

			LEVEL_DEBUG("TAI8570 Raw Pressure (D1) = %lu\n", D1);
			OWQ_F(owq) = 250. + X / 32.;
			return  0;
		}
	}

	return -EINVAL ;
}

// Read a page and confirm its a valid tmax page
// pn should be pointing to putative master device
static int ReadTmexPage(BYTE * data, size_t size, int page, const struct parsedname *pn)
{
	if (OW_r_mem(data, size, size * page, pn)) {
		LEVEL_DETAIL("Cannot read Tmex page %d\n", page);
		return 1;				// read page
	}
	if (data[0] > size) {
		LEVEL_DETAIL("Tmex page %d bad length %d\n", page, data[0]);
		return 1;				// check length
	}
	if (CRC16seeded(data, data[0] + 3, page)) {
		LEVEL_DETAIL("Tmex page %d CRC16 error\n", page);
		return 1;				// check length
	}
	return 0;
}

/* called with a copy of pn already set to the right device */
static int TAI8570_config(BYTE cfg, struct parsedname *pn)
{
	BYTE data[] = { _1W_CHANNEL_ACCESS, cfg, };
	BYTE dummy[] = { 0xFF, 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(data),
		TRXN_READ2(dummy),
		TRXN_END,
	};
	//printf("TAI8570_config\n");
	return BUS_transaction(t, pn);
}

static int TAI8570_A(struct parsedname *pn)
{
	BYTE b = 0xFF;
	int i;
	for (i = 0; i < 5; ++i) {
		if (OW_access(&b, pn)) {
			return 1;
		}
		if (OW_w_control(b | 0x20, pn)) {
			return 1;
		}
		if (OW_access(&b, pn)) {
			return 1;
		}
		if (b & 0xDF) {
			return 0;
		}
	}
	return 1;
}

static int TAI8570_B(struct parsedname *pn)
{
	BYTE b = 0xFF;
	int i;
	for (i = 0; i < 5; ++i) {
		if (OW_access(&b, pn)) {
			return 1;
		}
		if (OW_w_control(b | 0x40, pn)) {
			return 1;
		}
		if (OW_access(&b, pn)) {
			return 1;
		}
		if (b & 0xBF) {
			return 0;
		}
	}
	return 1;
}

/* called with a copy of pn */
static int TAI8570_ClockPulse(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	memcpy(pn->sn, tai->reader, 8);
	if (TAI8570_A(pn)) {
		return 1;
	}
	memcpy(pn->sn, tai->writer, 8);
	return TAI8570_A(pn);
}

/* called with a copy of pn */
static int TAI8570_DataPulse(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	memcpy(pn->sn, tai->reader, 8);
	if (TAI8570_B(pn)) {
		return 1;
	}
	memcpy(pn->sn, tai->writer, 8);
	return TAI8570_B(pn);
}

/* called with a copy of pn */
static int TAI8570_Reset(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	if (TAI8570_ClockPulse(tai, pn)) {
		return 1;
	}
	//printf("TAI8570 Clock ok\n") ;
	memcpy(pn->sn, tai->writer, 8);
	if (TAI8570_config(CFG_WRITE, pn)) {
		return 1;				// config write
	}
	//printf("TAI8570 config (reset) ok\n") ;
	return TAI8570_Write(SEC_RESET, tai, pn);
}

/* called with a copy of pn */
static int TAI8570_Write(BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	size_t len = strlen((ASCII *) cmd);
	BYTE zero = 0x04;
	struct transaction_log t[] = {
		TRXN_BLIND(cmd, len),
		TRXN_BLIND(&zero, 1),
		TRXN_END,
	};
	memcpy(pn->sn, tai->writer, 8);
	if (TAI8570_config(CFG_WRITE, pn)) {
		return 1;				// config write
	}
	//printf("TAI8570 config (write) ok\n") ;
	return BUS_transaction(t, pn);
}

/* called with a copy of pn */
static int TAI8570_Read(UINT * u, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	size_t i, j;
	BYTE data[32];
	UINT U = 0;
	struct transaction_log t[] = {
		TRXN_MODIFY(data, data, 32),
		TRXN_END,
	};
	//printf("TAI8570_Read\n");
	memcpy(pn->sn, tai->reader, 8);
	if (TAI8570_config(CFG_READ, pn)) {
		return 1;				// config write
	}
	//printf("TAI8570 read ") ;
	for (i = j = 0; i < 16; ++i) {
		data[j++] = 0xFF;
		data[j++] = 0xFA;
	}
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	for (j = 0; j < 32; j += 2) {
		U = U << 1;
		if (data[j] & 0x80) {
			++U;
		}
		//printf("%.2X%.2X ",data[j],data[j+1]) ;
	}
	//printf("\n") ;
	u[0] = U;
	return 0;
}

/* called with a copy of pn */
static int TAI8570_Check(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	size_t i;
	BYTE data[1];
	int ret = 1;
	struct transaction_log t[] = {
		TRXN_READ1(data),
		TRXN_END,
	};
	//printf("TAI8570_Check\n");
	UT_delay(30);				// conversion time in msec
	memcpy(pn->sn, tai->writer, 8);
	if (TAI8570_config(CFG_READPULSE, pn)) {
		return 1;				// config write
	}
	//printf("TAI8570 check ") ;
	for (i = 0; i < 100; ++i) {
		if (BUS_transaction(t, pn)) {
			return 1;
		}
		//printf("%.2X ",data[0]) ;
		if (data[0] != 0xFF) {
			ret = 0;
			break;
		}
	}
	//printf("TAI8570 conversion poll = %d\n",i) ;
	//printf("\n") ;
	return 0;
}

/* Called with a copy of pn */
static int TAI8570_SenseValue(UINT * val, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	if (TAI8570_Reset(tai, pn)) {
		return 1;
	}
	if (TAI8570_Write(cmd, tai, pn)) {
		return 1;
	}
	if (TAI8570_Check(tai, pn)) {
		return 1;
	}
	if (TAI8570_ClockPulse(tai, pn)) {
		return 1;
	}
	if (TAI8570_Read(val, tai, pn)) {
		return 1;
	}
	return TAI8570_DataPulse(tai, pn);
}

/* Called with a copy of pn */
static int TAI8570_CalValue(UINT * cal, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	if (TAI8570_ClockPulse(tai, pn)) {
		return 1;
	}
	if (TAI8570_Write(cmd, tai, pn)) {
		return 1;
	}
	if (TAI8570_ClockPulse(tai, pn)) {
		return 1;
	}
	if (TAI8570_Read(cal, tai, pn)) {
		return 1;
	}
	TAI8570_DataPulse(tai, pn);
	return 0;
}

/* read calibration constants and put in Cache, too
   return 0 on successful aquisition
 */
/* Called with a copy of pn */
static int TAI8570_Calibration(UINT * cal, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	UINT oldcal[4] = { 0, 0, 0, 0, };
	int rep;

	for (rep = 0; rep < 5; ++rep) {
		//printf("TAI8570_Calibration #%d\n",rep);
		if (TAI8570_Reset(tai, pn)) {
			return 1;
		}
		//printf("TAI8570 Pre_Cal_Value\n");
		TAI8570_CalValue(&cal[0], SEC_READW1, tai, pn);
		//printf("TAI8570 SIBLING cal[0]=%u ok\n",cal[0]);
		TAI8570_CalValue(&cal[1], SEC_READW2, tai, pn);
		//printf("TAI8570 SIBLING cal[1]=%u ok\n",cal[1]);
		TAI8570_CalValue(&cal[2], SEC_READW3, tai, pn);
		//printf("TAI8570 SIBLING cal[2]=%u ok\n",cal[2]);
		TAI8570_CalValue(&cal[3], SEC_READW4, tai, pn);
		//printf("TAI8570 SIBLING cal[3]=%u ok\n",cal[3]);
		if (memcmp(cal, oldcal, sizeof(oldcal)) == 0) {
			return 0;
		}
		memcpy(oldcal, cal, sizeof(oldcal));
	}
	return 1;					// couldn't get the same answer twice in a row
}

/* Called with a copy of pn */
static int testTAI8570(struct s_TAI8570 *tai, struct one_wire_query *owq)
{
	int pow;
	BYTE data[32];
	UINT cal[4];
	struct parsedname *pn = PN(owq);

	// see which DS2406 is powered
	if ( FS_r_sibling_Y( &pow, "power", owq ) ) {
		return -EINVAL ;
	}
	// See if already cached
	if (Cache_Get_Internal_Strict((void *) tai, sizeof(struct s_TAI8570), InternalProp(BAR), pn) == 0) {
		LEVEL_DEBUG("TAI8570 cache read: reader=" SNformat " writer=" SNformat "\n", SNvar(tai->reader), SNvar(tai->writer));
		LEVEL_DEBUG("TAI8570 cache read: C1=%u C2=%u C3=%u C4=%u C5=%u C6=%u\n", tai->C[0], tai->C[1], tai->C[2], tai->C[3], tai->C[4], tai->C[5]);
		return 0;
	}
	// Set master SN
	memcpy(tai->master, pn->sn, 8);

	// read page 0
	if (ReadTmexPage(data, 32, 0, pn)) {
		return 1;				// read page
	}
	if (memcmp("8570", &data[8], 4)) {	// check dir entry
		LEVEL_DETAIL("No 8570 Tmex file\n");
		return 1;
	}
	// See if page 1 is readable
	if (ReadTmexPage(data, 32, 1, pn)) {
		return 1;				// read page
	}

	memcpy(tai->sibling, &data[1], 8);
	if (pow) {
		memcpy(tai->writer, tai->master, 8);
		memcpy(tai->reader, tai->sibling, 8);
	} else {
		memcpy(tai->reader, tai->master, 8);
		memcpy(tai->writer, tai->sibling, 8);
	}
	LEVEL_DETAIL("TAI8570 reader=" SNformat " writer=" SNformat "\n", SNvar(tai->reader), SNvar(tai->writer));
	if (TAI8570_Calibration(cal, tai, pn)) {
		LEVEL_DETAIL("Trouble reading TAI8570 calibration constants\n");
		return 1;
	}
	tai->C[0] = ((cal[0]) >> 1);
	tai->C[1] = ((cal[3]) & 0x3F) | (((cal[2]) & 0x3F) << 6);
	tai->C[2] = ((cal[3]) >> 6);
	tai->C[3] = ((cal[2]) >> 6);
	tai->C[4] = ((cal[1]) >> 6) | (((cal[0]) & 0x01) << 10);
	tai->C[5] = ((cal[1]) & 0x3F);

	LEVEL_DETAIL("TAI8570 C1=%u C2=%u C3=%u C4=%u C5=%u C6=%u\n", tai->C[0], tai->C[1], tai->C[2], tai->C[3], tai->C[4], tai->C[5]);
	memcpy(pn->sn, tai->master, 8);	// restore original for cache
	return Cache_Add_Internal((const void *) tai, sizeof(struct s_TAI8570), InternalProp(BAR), pn);
}

#endif							/* OW_TAI8570 */
