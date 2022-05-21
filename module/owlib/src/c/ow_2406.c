/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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
READ_FUNCTION(FS_r_C);
READ_FUNCTION(FS_r_D1);
READ_FUNCTION(FS_r_D2);
READ_FUNCTION(FS_sibling);
READ_FUNCTION(FS_temperature);
READ_FUNCTION(FS_pressure);
READ_FUNCTION(FS_temperature_5534);
READ_FUNCTION(FS_pressure_5534);
READ_FUNCTION(FS_temperature_5540);
READ_FUNCTION(FS_pressure_5540);
 READ_FUNCTION(FS_voltage);

static enum e_visibility VISIBLE_T8A( const struct parsedname * pn ) ;

/* ------- Structures ----------- */

static struct aggregate A2406 = { 2, ag_letters, ag_aggregate, };
static struct aggregate A2406p = { 4, ag_numbers, ag_separate, };
static struct aggregate AT8Ac = { 8, ag_numbers, ag_separate, }; // 8 channel T8A volt meter
static struct aggregate ATAI8570 = { 6, ag_numbers, ag_aggregate, } ;
static struct filetype DS2406[] = {
	F_STANDARD,
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A2406p, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },

	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"channels", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_channel, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"PIO", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_stable, FS_r_pio, FS_w_pio, VISIBLE, NO_FILETYPE_DATA, },
	{"sensed", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_link, FS_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"latch", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_volatile, FS_r_latch, FS_w_latch, VISIBLE, NO_FILETYPE_DATA, },
	{"flipflop", PROPERTY_LENGTH_BITFIELD, &A2406, ft_bitfield, fc_volatile, FS_r_flipflop, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"set_alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_s_alarm, FS_w_s_alarm, VISIBLE, NO_FILETYPE_DATA, },
	{"infobyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_infobyte, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },

	// AAG Barometer
	{"TAI8570", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/C", PROPERTY_LENGTH_UNSIGNED, &ATAI8570, ft_unsigned, fc_static, FS_r_C, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/D1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_D1, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/D2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_D2, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/sibling", 16, NON_AGGREGATE, ft_ascii, fc_stable, FS_sibling, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"TAI8570/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_temperature, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_link, FS_pressure, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"TAI8570/DA5534", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/DA5534/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_temperature_5534, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/DA5534/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_link, FS_pressure_5534, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"TAI8570/DA5540", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/DA5540/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_temperature_5540, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TAI8570/DA5540/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_link, FS_pressure_5540, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	// EDS voltage
	{"T8A", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_T8A, NO_FILETYPE_DATA, },
	{"T8A/volt", PROPERTY_LENGTH_FLOAT, &AT8Ac, ft_float, fc_volatile, FS_voltage, NO_WRITE_FUNCTION, VISIBLE_T8A, NO_FILETYPE_DATA, },

	{"aux", 0, NON_AGGREGATE, ft_directory, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, {.i=eBranch_aux}, }, //3TA 2-line HUB
};

DeviceEntryExtended(12, DS2406, DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

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
static GOOD_OR_BAD OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_s_alarm(const BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_control(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_control(const BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_pio(const BYTE data, const struct parsedname *pn);
static GOOD_OR_BAD OW_access(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_syncaccess(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_clear(const struct parsedname *pn);
static GOOD_OR_BAD OW_full_access(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_voltage(_FLOAT * V, struct parsedname *pn );
static int VISIBLE_2406( const struct parsedname * pn );

/* finds the visibility of the DS2406-based T8A */
static int VISIBLE_2406( const struct parsedname * pn )
{
	int device_id = -1 ; // Unknown
	
	LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
	if ( BAD( GetVisibilityCache( &device_id, pn ) ) ) {
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		size_t memsize = 15 ;
		BYTE mem[memsize] ;
		if ( owq != NULL) {
			if ( FS_r_sibling_binary( mem, &memsize, "memory", owq ) == 0 ) {
				if ( memcmp( "A189" , &mem[1], 4 ) == 0 ) {
					device_id = 1 ; // T8A
				} else {
					device_id = 0 ; // non T8A
				}
				SetVisibilityCache( device_id, pn ) ;
			}
			OWQ_destroy(owq) ;
		}
	}
	return device_id ;
}

static enum e_visibility VISIBLE_T8A( const struct parsedname * pn )
{
	switch ( VISIBLE_2406(pn) ) {
		case 1:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

/* 2406 memory read */
static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	/* read is not a "paged" endeavor, no CRC */
	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E(OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

/* 2406 memory write */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32 ;
	return COMMON_offset_process( FS_r_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

/* Note, it's EPROM -- write once */
static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	/* write is "byte at a time" -- not paged */
	return COMMON_write_eprom_mem_owq(owq) ;
}

/* 2406 switch */
static ZERO_OR_ERROR FS_r_pio(struct one_wire_query *owq)
{
	UINT infobyte = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &infobyte, "infobyte", owq ) ;

	OWQ_U(owq) = BYTE_INVERSE(infobyte>>0) & 0x03;	/* reverse bits */
	return z_or_e;
}

/* 2406 switch */
static ZERO_OR_ERROR FS_r_flipflop(struct one_wire_query *owq)
{
	UINT infobyte = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &infobyte, "infobyte", owq ) ;

	OWQ_U(owq) = (infobyte>>0) & 0x03;	/* reverse bits */
	return z_or_e;
}

/* 2406 switch */
static ZERO_OR_ERROR FS_r_infobyte(struct one_wire_query *owq)
{
	BYTE data;
	RETURN_ERROR_IF_BAD( OW_syncaccess(&data, PN(owq)) );
	OWQ_U(owq) = data;	/* reverse bits */
	return 0;
}

/* 2406 switch -- is Vcc powered?*/
static ZERO_OR_ERROR FS_power(struct one_wire_query *owq)
{
	UINT infobyte = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &infobyte, "infobyte", owq ) ;

	OWQ_Y(owq) = (infobyte & _DS2406_POWER_BIT) ? 1 : 0;
	return z_or_e;
}

/* 2406 switch -- number of channels (actually, if Vcc powered)*/
static ZERO_OR_ERROR FS_channel(struct one_wire_query *owq)
{
	UINT infobyte = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &infobyte, "infobyte", owq ) ;

	OWQ_U(owq) = (infobyte & 0x40) ? 2 : 1;
	return z_or_e;
}

/* 2406 switch PIO sensed*/
/* bits 2 and 3 */
static ZERO_OR_ERROR FS_sense(struct one_wire_query *owq)
{
	UINT infobyte = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &infobyte, "infobyte", owq ) ;

	OWQ_U(owq) = (infobyte>>2) & 0x03;
	return z_or_e ;
}

/* 2406 switch activity latch*/
/* bites 4 and 5 */
static ZERO_OR_ERROR FS_r_latch(struct one_wire_query *owq)
{
	UINT infobyte = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &infobyte, "infobyte", owq ) ;

	OWQ_U(owq) = (infobyte >> 4) & 0x03;
	return z_or_e;
}

/* 2406 switch activity latch*/
static ZERO_OR_ERROR FS_w_latch(struct one_wire_query *owq)
{
	FS_del_sibling( "infobyte", owq ) ;

	return GB_to_Z_OR_E(OW_clear(PN(owq))) ;
}

/* 2406 alarm settings*/
static ZERO_OR_ERROR FS_r_s_alarm(struct one_wire_query *owq)
{
	BYTE data ;

	RETURN_ERROR_IF_BAD( OW_r_control(&data, PN(owq)) );

	OWQ_U(owq) = (data & 0x01) + ((data >> 1) & 0x03) * 10 + ((data >>3) & 0x03) * 100;
	return 0 ;
}

/* 2406 alarm settings*/
static ZERO_OR_ERROR FS_w_s_alarm(struct one_wire_query *owq)
{
	BYTE data;
	UINT U = OWQ_U(owq);
	data = ((U % 10) & 0x01)
		| (((U / 10 % 10) & 0x03) << 1)
		| (((U / 100 % 10) & 0x03) << 3);
	return GB_to_Z_OR_E(OW_w_s_alarm(data, PN(owq))) ;
}

/* write 2406 switch -- 2 values*/
static ZERO_OR_ERROR FS_w_pio(struct one_wire_query *owq)
{
	BYTE data = BYTE_INVERSE(OWQ_U(owq)) & 0x03 ; /* reverse bits */

	FS_del_sibling( "infobyte", owq ) ;

	return GB_to_Z_OR_E(OW_w_pio(data, PN(owq))) ;
}

/* Support for EmbeddedDataSystems's T8A 8 channel A/D
   Written by Chase Shimmin cshimmin@berkeley.edu */
static ZERO_OR_ERROR FS_voltage(struct one_wire_query *owq)
{
	_FLOAT V ;
	
	FS_del_sibling( "infobyte", owq ) ;

	RETURN_ERROR_IF_BAD( OW_voltage( &V, PN(owq) ) ) ;

	OWQ_F(owq) = V;
	return 0;
}

/* reads memory directly into the buffer, no CRC */
static GOOD_OR_BAD OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_READ_MEMORY, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ(data, size),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	return gbGOOD;
}

/* read status byte */
static GOOD_OR_BAD OW_r_control(BYTE * data, const struct parsedname *pn)
{
	BYTE p[3 + 1 + 2] = { _1W_READ_STATUS, LOW_HIGH_ADDRESS(_ADDRESS_STATUS_MEMORY_SRAM),
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 1),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	*data = p[3];
	return gbGOOD;
}

/* write status byte */
static GOOD_OR_BAD OW_w_control(const BYTE data, const struct parsedname *pn)
{
	BYTE p[3 + 1 + 2] = { _1W_WRITE_STATUS, LOW_HIGH_ADDRESS(_ADDRESS_STATUS_MEMORY_SRAM),
		data,
	};
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, 0),
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}

/* write alarm settings */
static GOOD_OR_BAD OW_w_s_alarm(const BYTE data, const struct parsedname *pn)
{
	BYTE b[1];
	RETURN_BAD_IF_BAD( OW_r_control(b, pn) );
	b[0] = ( b[0] & ~_DS2406_ALARM_BITS) | (data & _DS2406_ALARM_BITS) ;
	return OW_w_control(b[0], pn);
}

/* set PIO state bits: bit0=A bit1=B, value: open=1 closed=0 */
static GOOD_OR_BAD OW_w_pio(const BYTE data, const struct parsedname *pn)
{
	BYTE b[1];
	RETURN_BAD_IF_BAD( OW_r_control(b, pn) );
	b[0] = ( b[0] & ~_DS2406_FLIPFLOP_BITS) | ((data<<5) & _DS2406_FLIPFLOP_BITS) ;
	return OW_w_control(b[0], pn);
}

static GOOD_OR_BAD OW_syncaccess(BYTE * data, const struct parsedname *pn)
{
	BYTE d[2] = { _DS2406_IM|_DS2406_CHS1|_DS2406_CHS0|_DS2406_CRC0, 0xFF, };
	RETURN_BAD_IF_BAD( OW_full_access(d, pn) );
	data[0] = d[0];
	return gbGOOD;
}

static GOOD_OR_BAD OW_access(BYTE * data, const struct parsedname *pn)
{
	BYTE d[2] = { _DS2406_IM|_DS2406_IC|_DS2406_CHS1|_DS2406_CHS0|_DS2406_CRC0, 0xFF, };
	RETURN_BAD_IF_BAD( OW_full_access(d, pn) );
	data[0] = d[0];
	return gbGOOD;
}

/* Clear latches */
static GOOD_OR_BAD OW_clear(const struct parsedname *pn)
{
	BYTE data[2] = { _DS2406_ALR|_DS2406_IM|_DS2406_IC|_DS2406_CHS0|_DS2406_CRC0, 0xFF, };
	return OW_full_access(data, pn);;
}

// write both control bytes, and read both back
static GOOD_OR_BAD OW_full_access(BYTE * data, const struct parsedname *pn)
{
	BYTE p[3 + 2 + 2] = { _1W_CHANNEL_ACCESS, data[0], data[1], };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 2),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	//printf("DS2406 access %.2X %.2X -> %.2X %.2X \n",data[0],data[1],p[3],p[4]);
	//printf("DS2406 CRC ok\n");
	data[0] = p[3];
	data[1] = p[4];
	return gbGOOD;
}

/* Support for EmbeddedDataSystems's T8A 8 channel A/D
   Written by Chase Shimmin cshimmin@berkeley.edu */
static GOOD_OR_BAD OW_voltage(_FLOAT * V, struct parsedname * pn )
{
	// channel select byte, based on zero-indexed channel number
	BYTE ch_select = (pn->extension << 2) + 0x02;
	BYTE data[8+2] = { ch_select,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF, } ;

	// this is the complete byte sequence we want to write to the ds2406 so it will
	// select the appropriate channel and initiate adc, and also so it can write
	// back the results to the trailing 0xFF bytes.
	BYTE p[] = { _1W_CHANNEL_ACCESS,
		_DS2406_ALR|_DS2406_TOG|_DS2406_CHS0|_DS2406_CRC1, 0xFF, // Channel control
	} ;

	// 'modify' transaction -- so we can read back the trailing bytes.
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_MODIFY( data, data, 8+2 ),
		TRXN_CRC16( data, 8+2 ),
		TRXN_END,
	};

	// most & least significant bytes. for the latter, we're actually only interested
	// in the least significant nibble.
	BYTE msb, lsb;

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

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
	V[0] = ((msb << 4) + lsb) * 5.0 / 4096;
	return 0;
}

struct s_TAI8570 {
	BYTE master[8];
	BYTE sibling[8];
	BYTE reader[8];
	BYTE writer[8];
	UINT C[6];
};
/* Internal files */
Make_SlaveSpecificTag(BAR, fc_persistent);

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

static GOOD_OR_BAD ReadTmexPage(BYTE * data, size_t size, int page, const struct parsedname *pn);
static GOOD_OR_BAD TAI8570_Calibration(UINT * cal, const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD testTAI8570(struct s_TAI8570 *tai, struct one_wire_query *owq);
static GOOD_OR_BAD TAI8570_Write(BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_Read(UINT * u, const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_Reset(const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_Check(const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_ClockPulse(const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_DataPulse(const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_CalValue(UINT * cal, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_SenseValue(UINT * val, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn);
static GOOD_OR_BAD TAI8570_A(struct parsedname *pn);
static GOOD_OR_BAD TAI8570_B(struct parsedname *pn);

static ZERO_OR_ERROR FS_sibling(struct one_wire_query *owq)
{
	ASCII sib[16];
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
	bytes2string(sib, tai.sibling, 8);
	return OWQ_format_output_offset_and_size(sib, 16, owq);
}

static ZERO_OR_ERROR FS_r_D1(struct one_wire_query *owq)
{
	UINT D;
	struct s_TAI8570 tai;
	struct parsedname pn_copy;

	FS_del_sibling( "infobyte", owq ) ;

	memcpy(&pn_copy, PN(owq), sizeof(struct parsedname));	//shallow copy
	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}

	RETURN_BAD_IF_BAD( TAI8570_SenseValue(&D, SEC_READD1, &tai, &pn_copy) );
	LEVEL_DEBUG("TAI8570 Raw Pressure (D1) = %lu", D);
	OWQ_U(owq) = D ;
	return 0;
}

static ZERO_OR_ERROR FS_r_D2(struct one_wire_query *owq)
{
	UINT D;
	struct s_TAI8570 tai;
	struct parsedname pn_copy;

	FS_del_sibling( "infobyte", owq ) ;

	memcpy(&pn_copy, PN(owq), sizeof(struct parsedname));	//shallow copy
	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}

	RETURN_BAD_IF_BAD( TAI8570_SenseValue(&D, SEC_READD2, &tai, &pn_copy) );
	LEVEL_DEBUG("TAI8570 Raw Temperature (D2) = %lu", D);
	OWQ_U(owq) = D ;
	return 0;
}

static ZERO_OR_ERROR FS_r_C(struct one_wire_query *owq)
{
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}

	OWQ_array_U( owq, 0 ) = tai.C[0] ;
	OWQ_array_U( owq, 1 ) = tai.C[1] ;
	OWQ_array_U( owq, 2 ) = tai.C[2] ;
	OWQ_array_U( owq, 3 ) = tai.C[3] ;
	OWQ_array_U( owq, 4 ) = tai.C[4] ;
	OWQ_array_U( owq, 5 ) = tai.C[5] ;
	return 0;
}

static ZERO_OR_ERROR FS_temperature(struct one_wire_query *owq)
{
	UINT D2;
	UINT UT1 ;
	_FLOAT dT;
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
	
	if ( FS_r_sibling_U( &D2, "TAI8570/D2", owq ) != 0 ) {
		return -EINVAL ;
	} 

	UT1 = 8 * tai.C[4] + 20224;
	dT = (_FLOAT)D2 - (_FLOAT)UT1;

	OWQ_F(owq) = (200. + dT * (tai.C[5] + 50.) / 1024.) / 10.;

	return 0;
}

static ZERO_OR_ERROR FS_pressure(struct one_wire_query *owq)
{
	UINT D1 ;
	UINT D2 ;
	UINT UT1 ;
	_FLOAT dT ;
	_FLOAT OFF, SENS, X ;
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
		
	if ( FS_r_sibling_U( &D1, "TAI8570/D1", owq ) != 0 ) {
		return -EINVAL ;
	} 
	
	if ( FS_r_sibling_U( &D2, "TAI8570/D2", owq ) != 0 ) {
		return -EINVAL ;
	} 

	UT1 = 8 * tai.C[4] + 20224;
	dT = (_FLOAT)D2 - (_FLOAT)UT1;
	OFF = 4. * tai.C[1] + ((tai.C[3] - 512.) * dT) / 4096.;
	SENS = 24576. + tai.C[0] + (tai.C[2] * dT) / 1024.;
	X = (SENS * (D1 - 7168.)) / 16384. - OFF;

	OWQ_F(owq) = 250. + X / 32.;

	return  0;
}

static ZERO_OR_ERROR FS_temperature_5534(struct one_wire_query *owq)
{
	UINT D2;
	UINT UT1 ; 
	_FLOAT dT;
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
	
	if ( FS_r_sibling_U( &D2, "TAI8570/D2", owq ) != 0 ) {
		return -EINVAL ;
	} 

	UT1 = 8 * tai.C[4] + 20224;

	if ( D2 >= UT1 ) {
		dT = D2 - UT1;
		OWQ_F(owq) = (200. + dT * (tai.C[5] + 50.) / 1024.) / 10.;
	} else {
		dT = D2 - UT1;
		dT = dT - (dT/256.)*(dT/256.) ;
		OWQ_F(owq) = (200. + dT * (tai.C[5] + 50.) / 1024. + dT / 256. ) / 10.;
	}

	return 0;
}

static ZERO_OR_ERROR FS_pressure_5534(struct one_wire_query *owq)
{
	UINT D1 ;
	UINT D2 ;
	UINT UT1 ;
	_FLOAT dT ;
	_FLOAT OFF, SENS, X ;
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
		
	if ( FS_r_sibling_U( &D1, "TAI8570/D1", owq ) != 0 ) {
		return -EINVAL ;
	} 
	
	if ( FS_r_sibling_U( &D2, "TAI8570/D2", owq ) != 0 ) {
		return -EINVAL ;
	} 

	UT1 = 8 * tai.C[4] + 20224;
	if ( D2 >= UT1 ) {
		dT = D2 - UT1;
	} else {
		dT = D2 - UT1;
		dT = dT - (dT/256.)*(dT/256.) ;
	}
	OFF = 4. * tai.C[1] + ((tai.C[3] - 512.) * dT) / 4096.;
	SENS = 24576. + tai.C[0] + (tai.C[2] * dT) / 1024.;
	X = (SENS * (D1 - 7168.)) / 16384. - OFF;

	OWQ_F(owq) = 250. + X / 32.;

	return  0;
}

static ZERO_OR_ERROR FS_temperature_5540(struct one_wire_query *owq)
{
	UINT D2;
	int UT1 ;
	_FLOAT dT;
	_FLOAT T2, TEMP ;
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
	
	if ( FS_r_sibling_U( &D2, "TAI8570/D2", owq ) != 0 ) {
		return -EINVAL ;
	} 

	UT1 = 8 * tai.C[4] + 20224;
	dT = D2 - UT1;

	TEMP = (200. + dT * (tai.C[5] + 50.) / 1024.) ;
	
	if ( TEMP < 200 ) {
		T2 = 11 * ( tai.C[5] + 24 ) * ( (200 - TEMP) / 1024. )*( (200 - TEMP) / 1024. ) ;
	} else if ( TEMP <= 450 ) {
		T2 = 0. ;
	} else {
		T2 =  3 * ( tai.C[5] + 24 ) * ( (450 - TEMP) / 1024. )*( (450 - TEMP) / 1024. ) ;
	}

	OWQ_F(owq) = ( TEMP - T2 ) / 10. ;

	return 0;
}

static ZERO_OR_ERROR FS_pressure_5540(struct one_wire_query *owq)
{
	UINT D1 ;
	UINT D2 ;
	UINT UT1 ;
	_FLOAT dT ;
	_FLOAT T2, TEMP ;
	_FLOAT OFF, SENS, X ;
	_FLOAT P, P2 ;
	struct s_TAI8570 tai;

	FS_del_sibling( "infobyte", owq ) ;

	if ( BAD( testTAI8570(&tai, owq) ) ) {
		return -ENOENT;
	}
		
	if ( FS_r_sibling_U( &D1, "TAI8570/D1", owq ) != 0 ) {
		return -EINVAL ;
	} 
	
	if ( FS_r_sibling_U( &D2, "TAI8570/D2", owq ) != 0 ) {
		return -EINVAL ;
	} 

	UT1 = 8 * tai.C[4] + 20224;
	dT = D2 - UT1;

	TEMP = (200. + dT * (tai.C[5] + 50.) / 1024.) ;
	
	OFF = 4. * tai.C[1] + ((tai.C[3] - 512.) * dT) / 4096.;
	SENS = 24576. + tai.C[0] + (tai.C[2] * dT) / 1024.;
	X = (SENS * (D1 - 7168.)) / 16384. - OFF;
	P = 10 * (250. + X / 32. );

	if ( TEMP < 200 ) {
		T2 = 11 * ( tai.C[5] + 24 ) * ( (200 - TEMP) / 1024. )*( (200 - TEMP) / 1024. ) ;
		P2 = 3 * T2 * ( P - 3500 ) / 16384. ;
	} else if ( TEMP <= 450 ) {
		P2 = 0. ;
	} else {
		T2 =  3 * ( tai.C[5] + 24 ) * ( (450 - TEMP) / 1024. )*( (450 - TEMP) / 1024. ) ;
		P2 =  T2 * ( P - 10000 ) / 8192. ;
	}

	OWQ_F(owq) = ( P - P2 ) / 10. ;

	return  0;
}

// Read a page and confirm its a valid tmax page
// pn should be pointing to putative master device
static GOOD_OR_BAD ReadTmexPage(BYTE * data, size_t size, int page, const struct parsedname *pn)
{
	RETURN_BAD_IF_BAD( OW_r_mem(data, size, size * page, pn) );
	if (data[0] > size) {
		LEVEL_DETAIL("Tmex page %d bad length %d", page, data[0]);
		return gbBAD;				// check length
	}
	if (CRC16seeded(data, data[0] + 3, page)) {
		LEVEL_DETAIL("Tmex page %d CRC16 error", page);
		return gbBAD;				// check length
	}
	return gbGOOD;
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

static GOOD_OR_BAD TAI8570_A(struct parsedname *pn)
{
	BYTE b = 0xFF;
	int i;
	for (i = 0; i < 5; ++i) {
		RETURN_BAD_IF_BAD( OW_access(&b, pn) );
		RETURN_BAD_IF_BAD( OW_w_control(b | 0x20, pn) );
		RETURN_BAD_IF_BAD( OW_access(&b, pn) );
		if (b & 0xDF) {
			return gbGOOD;
		}
	}
	return gbBAD;
}

static GOOD_OR_BAD TAI8570_B(struct parsedname *pn)
{
	BYTE b = 0xFF;
	int i;
	for (i = 0; i < 5; ++i) {
		RETURN_BAD_IF_BAD( OW_access(&b, pn) );
		RETURN_BAD_IF_BAD( OW_w_control(b | 0x40, pn) ) ;
		RETURN_BAD_IF_BAD( OW_access(&b, pn) ) ;
		if (b & 0xBF) {
			return gbGOOD;
		}
	}
	return gbBAD;
}

/* called with a copy of pn */
static GOOD_OR_BAD TAI8570_ClockPulse(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	memcpy(pn->sn, tai->reader, 8);
	RETURN_BAD_IF_BAD( TAI8570_A(pn) );
	memcpy(pn->sn, tai->writer, 8);
	return TAI8570_A(pn);
}

/* called with a copy of pn */
static GOOD_OR_BAD TAI8570_DataPulse(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	memcpy(pn->sn, tai->reader, 8);
	RETURN_BAD_IF_BAD( TAI8570_B(pn) );
	memcpy(pn->sn, tai->writer, 8);
	return TAI8570_B(pn);
}

/* called with a copy of pn */
static GOOD_OR_BAD TAI8570_Reset(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	RETURN_BAD_IF_BAD( TAI8570_ClockPulse(tai, pn) );
	//printf("TAI8570 Clock ok\n") ;
	memcpy(pn->sn, tai->writer, 8);
	RETURN_BAD_IF_BAD( TAI8570_config(CFG_WRITE, pn) );	// config write
	//printf("TAI8570 config (reset) ok\n") ;
	return TAI8570_Write(SEC_RESET, tai, pn);
}

/* called with a copy of pn */
static GOOD_OR_BAD TAI8570_Write(BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	size_t len = strlen((ASCII *) cmd);
	BYTE zero = 0x04;
	struct transaction_log t[] = {
		TRXN_BLIND(cmd, len),
		TRXN_BLIND(&zero, 1),
		TRXN_END,
	};
	memcpy(pn->sn, tai->writer, 8);
	RETURN_BAD_IF_BAD( TAI8570_config(CFG_WRITE, pn) );	// config write
	//printf("TAI8570 config (write) ok\n") ;
	return BUS_transaction(t, pn);
}

/* called with a copy of pn */
static GOOD_OR_BAD TAI8570_Read(UINT * u, const struct s_TAI8570 *tai, struct parsedname *pn)
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
	RETURN_BAD_IF_BAD( TAI8570_config(CFG_READ, pn) );	// config write
	//printf("TAI8570 read ") ;
	for (i = j = 0; i < 16; ++i) {
		data[j++] = 0xFF;
		data[j++] = 0xFA;
	}
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	for (j = 0; j < 32; j += 2) {
		U = U << 1;
		if (data[j] & 0x80) {
			++U;
		}
		//printf("%.2X%.2X ",data[j],data[j+1]) ;
	}
	//printf("\n") ;
	u[0] = U;
	return gbGOOD;
}

/* called with a copy of pn */
static GOOD_OR_BAD TAI8570_Check(const struct s_TAI8570 *tai, struct parsedname *pn)
{
	size_t i;
	BYTE data[1];
	struct transaction_log t[] = {
		TRXN_DELAY(1),
		TRXN_READ1(data),
		TRXN_END,
	};
	UT_delay(29);				// conversion time in msec
	memcpy(pn->sn, tai->writer, 8);
	RETURN_BAD_IF_BAD( TAI8570_config(CFG_READPULSE, pn) );		// config write
	for (i = 0; i < 100; ++i) {
		RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
		//printf("%.2X ",data[0]) ;
		if (data[0] != 0xFF) {
			break;
		}
	}
	//printf("TAI8570 conversion poll = %d\n",i) ;
	return gbGOOD;
}

/* Called with a copy of pn */
static GOOD_OR_BAD TAI8570_SenseValue(UINT * val, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	RETURN_BAD_IF_BAD( TAI8570_Reset(tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_Write(cmd, tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_Check(tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_ClockPulse(tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_Read(val, tai, pn) );
	return TAI8570_DataPulse(tai, pn);
}

/* Called with a copy of pn */
static GOOD_OR_BAD TAI8570_CalValue(UINT * cal, BYTE * cmd, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	RETURN_BAD_IF_BAD( TAI8570_ClockPulse(tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_Write(cmd, tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_ClockPulse(tai, pn) );
	RETURN_BAD_IF_BAD( TAI8570_Read(cal, tai, pn) );
	TAI8570_DataPulse(tai, pn);
	return gbGOOD;
}

/* read calibration constants and put in Cache, too
   return 0 on successful aquisition
 */
/* Called with a copy of pn */
static GOOD_OR_BAD TAI8570_Calibration(UINT * cal, const struct s_TAI8570 *tai, struct parsedname *pn)
{
	UINT oldcal[4] = { 0, 0, 0, 0, };
	int rep;

	for (rep = 0; rep < 5; ++rep) {
		//printf("TAI8570_Calibration #%d\n",rep);
		RETURN_BAD_IF_BAD( TAI8570_Reset(tai, pn) ) ;
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
			return gbGOOD;
		}
		memcpy(oldcal, cal, sizeof(oldcal));
	}
	return gbBAD;					// couldn't get the same answer twice in a row
}

/* Called with a copy of pn */
static GOOD_OR_BAD testTAI8570(struct s_TAI8570 *tai, struct one_wire_query *owq)
{
	int pow;
	BYTE data[32];
	UINT cal[4];
	struct parsedname *pn = PN(owq);

	// see which DS2406 is powered
	if ( FS_r_sibling_Y( &pow, "power", owq ) != 0 ) {
		return gbBAD ;
	}
	// See if already cached
	if ( GOOD( Cache_Get_SlaveSpecific((void *) tai, sizeof(struct s_TAI8570), SlaveSpecificTag(BAR), pn)) ) {
		LEVEL_DEBUG("TAI8570 cache read: reader=" SNformat " writer=" SNformat, SNvar(tai->reader), SNvar(tai->writer));
		LEVEL_DEBUG("TAI8570 cache read: C1=%u C2=%u C3=%u C4=%u C5=%u C6=%u", tai->C[0], tai->C[1], tai->C[2], tai->C[3], tai->C[4], tai->C[5]);
		return gbGOOD;
	}
	// Set master SN
	memcpy(tai->master, pn->sn, 8);

	// read page 0
	RETURN_BAD_IF_BAD( ReadTmexPage(data, 32, 0, pn) ) ;
	if (memcmp("8570", &data[8], 4)) {	// check dir entry
		LEVEL_DETAIL("No 8570 Tmex file");
		return gbBAD;
	}
	// See if page 1 is readable
	RETURN_BAD_IF_BAD( ReadTmexPage(data, 32, 1, pn) ) ;

	memcpy(tai->sibling, &data[1], 8);
	if (pow) {
		memcpy(tai->writer, tai->master, 8);
		memcpy(tai->reader, tai->sibling, 8);
	} else {
		memcpy(tai->reader, tai->master, 8);
		memcpy(tai->writer, tai->sibling, 8);
	}
	LEVEL_DETAIL("TAI8570 reader=" SNformat " writer=" SNformat, SNvar(tai->reader), SNvar(tai->writer));
	if ( BAD( TAI8570_Calibration(cal, tai, pn) ) ) {
		LEVEL_DETAIL("Trouble reading TAI8570 calibration constants");
		return gbBAD;
	}
	
	// Set data
	tai->C[0] = ((cal[0]) >> 1);
	tai->C[1] = ((cal[3]) & 0x3F) | (((cal[2]) & 0x3F) << 6);
	tai->C[2] = ((cal[3]) >> 6);
	tai->C[3] = ((cal[2]) >> 6);
	tai->C[4] = (((cal[1]) >> 6) & 0x3FF) | (((cal[0]) & 0x01) << 10);
	tai->C[5] = ((cal[1]) & 0x3F);
	
	// Enforce data lenths
	tai->C[0] &= 0x7FFF; // 15 bit
	tai->C[1] &= 0x0FFF; // 12 bit
	tai->C[2] &= 0x03FF; // 10 bit
	tai->C[3] &= 0x03FF; // 10 bit
	tai->C[4] &= 0x07FF; // 11 bit
	tai->C[5] &= 0x003F; //  6 bit

	LEVEL_DETAIL("TAI8570 C1=%u C2=%u C3=%u C4=%u C5=%u C6=%u", tai->C[0], tai->C[1], tai->C[2], tai->C[3], tai->C[4], tai->C[5]);
	memcpy(pn->sn, tai->master, 8);	// restore original for cache
	return Cache_Add_SlaveSpecific((const void *) tai, sizeof(struct s_TAI8570), SlaveSpecificTag(BAR), pn);
}
