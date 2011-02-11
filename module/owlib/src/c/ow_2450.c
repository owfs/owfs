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

#include <config.h>
#include "owfs_config.h"
#include "ow_2450.h"

/* ------- Prototypes ----------- */

/* DS2450 A/D */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_volts);
READ_FUNCTION(FS_r_power);
WRITE_FUNCTION(FS_w_power);
READ_FUNCTION(FS_r_enable);
WRITE_FUNCTION(FS_w_enable);
READ_FUNCTION(FS_r_PIO);
WRITE_FUNCTION(FS_w_PIO);
READ_FUNCTION(FS_r_setvolt);
WRITE_FUNCTION(FS_w_setvolt);
READ_FUNCTION(FS_r_flag);
WRITE_FUNCTION(FS_w_flag);
WRITE_FUNCTION(FS_w_por);

READ_FUNCTION(FS_CO2_ppm);
READ_FUNCTION(FS_CO2_status);
READ_FUNCTION(FS_CO2_power);

/* ------- Structures ----------- */
enum V_resolution {
	r_5V_16bit, // 5V full scale. 16 bits is higher than hardware resolution
	r_2V_16bit, // actually 2.5V full scale
	r_5V_8bit, // 5V full scale. 16 bits is higher than hardware resolution
	r_2V_8bit, // actually 2.5V full scale
} ;

enum alarm_level {
	ae_low,
	ae_high,
} ;

enum V_alarm_level {
	V5_ae_low,
	V5_ae_high,
	V2_ae_low,
	V2_ae_high,
} ;

#define _1W_2450_PAGES	4
#define _1W_2450_PAGESIZE	8
#define _1W_2450_REGISTERS	4

struct aggregate A2450p = { _1W_2450_PAGES, ag_numbers, ag_separate, };
struct aggregate A2450 = { _1W_2450_REGISTERS, ag_letters, ag_separate, };
struct aggregate A2450v = { _1W_2450_REGISTERS, ag_letters, ag_aggregate, };
struct filetype DS2450[] = {
	F_STANDARD,
	{"memory", _1W_2450_PAGESIZE*_1W_2450_PAGES, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", _1W_2450_PAGESIZE, &A2450p, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},

	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_power, FS_w_power, VISIBLE, NO_FILETYPE_DATA,},
	{"PIO", PROPERTY_LENGTH_YESNO, &A2450, ft_yesno, fc_stable, FS_r_PIO, FS_w_PIO, VISIBLE, {i:0},},
	{"volt", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {i:r_5V_16bit},},
	{"volt2", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {i:r_2V_16bit},},

	{"8bit", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"8bit/volt", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {i:r_5V_8bit},},
	{"8bit/volt2", PROPERTY_LENGTH_FLOAT, &A2450v, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {i:r_2V_8bit},},

	{"set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"set_alarm/volthigh", PROPERTY_LENGTH_FLOAT, &A2450, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:V5_ae_high},},
	{"set_alarm/volt2high", PROPERTY_LENGTH_FLOAT, &A2450, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:V2_ae_high},},
	{"set_alarm/voltlow", PROPERTY_LENGTH_FLOAT, &A2450, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:V5_ae_low},},
	{"set_alarm/volt2low", PROPERTY_LENGTH_FLOAT, &A2450, ft_float, fc_stable, FS_r_setvolt, FS_w_setvolt, VISIBLE, {i:V2_ae_low},},
	{"set_alarm/high", PROPERTY_LENGTH_YESNO, &A2450, ft_yesno, fc_stable, FS_r_enable, FS_w_enable, VISIBLE, {i:ae_high},},
	{"set_alarm/low", PROPERTY_LENGTH_YESNO, &A2450, ft_yesno, fc_stable, FS_r_enable, FS_w_enable, VISIBLE, {i:ae_low},},
	{"set_alarm/unset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_por, VISIBLE, NO_FILETYPE_DATA,},

	{"alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"alarm/high", PROPERTY_LENGTH_YESNO, &A2450, ft_yesno, fc_volatile, FS_r_flag, FS_w_flag, VISIBLE, {i:ae_high},},
	{"alarm/low", PROPERTY_LENGTH_YESNO, &A2450, ft_yesno, fc_volatile, FS_r_flag, FS_w_flag, VISIBLE, {i:ae_low},},

	{"CO2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"CO2/ppm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_CO2_ppm, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"CO2/Vdd", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_yesno, fc_link, FS_CO2_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"CO2/status", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_float, fc_link, FS_CO2_status, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(20, DS2450, DEV_volt | DEV_alarm | DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Internal properties */
Make_SlaveSpecificTag(RES, fc_stable);	// resolution
Make_SlaveSpecificTag(RAN, fc_stable);	// range
Make_SlaveSpecificTag(POW, fc_stable);	// power status

#define _1W_READ_MEMORY 0x44
#define _1W_WRITE_MEMORY 0x55
#define _1W_CONVERT 0x3C

#define _1W_2450_POWERED 0x40
#define _1W_2450_UNPOWERED 0x00

#define _ADDRESS_CONVERSION_PAGE 0x00
#define _ADDRESS_CONTROL_PAGE 0x08
#define _ADDRESS_ALARM_PAGE 0x10
#define _ADDRESS_POWERED 0x1C

#define _1W_2450_REG_A 0
#define _1W_2450_REG_B 2
#define _1W_2450_REG_C 4
#define _1W_2450_REG_D 6

#define _1W_2450_RC_MASK 0x0F // resolution mask in control page
#define _1W_2450_OC 0x40 // Output control
#define _1W_2450_OE 0x80 // Outout enable

#define _1W_2450_IR 0x01 // Input DAC range
#define _1W_2450_AEL 0x04 // Alarm low enable
#define _1W_2450_AEH 0x08 // Alarm high enable 
#define _1W_2450_AFL 0x10 // Alarm low flag
#define _1W_2450_AFH 0x20 // Alarm high flag 
#define _1W_2450_POR 0x80 // Power On Reset bit 

/* ------- Functions ------------ */

/* DS2450 */
static GOOD_OR_BAD OW_r_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_volts(_FLOAT * f, struct parsedname *pn);
static GOOD_OR_BAD OW_convert( int simul_good, int delay, struct parsedname *pn);
static GOOD_OR_BAD OW_r_pio(int *pio, struct parsedname *pn);
static GOOD_OR_BAD OW_w_pio( int pio, struct parsedname *pn);
static GOOD_OR_BAD OW_r_vset( _FLOAT * V, enum V_alarm_level ae, struct parsedname *pn);
static GOOD_OR_BAD OW_w_vset( _FLOAT V, enum V_alarm_level ae, struct parsedname *pn) ;
static GOOD_OR_BAD OW_r_enable(int *y, enum alarm_level ae, struct parsedname *pn);
static GOOD_OR_BAD OW_w_enable( int y, enum alarm_level ae, struct parsedname *pn);
static GOOD_OR_BAD OW_r_mask(int *y, BYTE mask, struct parsedname *pn);
static GOOD_OR_BAD OW_w_mask( int y, BYTE mask, struct parsedname *pn);
static GOOD_OR_BAD OW_r_flag(int *y, enum alarm_level ae, struct parsedname *pn);
static GOOD_OR_BAD OW_w_flag( int y, enum alarm_level ae, struct parsedname *pn);
static GOOD_OR_BAD OW_w_por( int por, struct parsedname *pn);
static GOOD_OR_BAD OW_set_resolution( int resolution, struct parsedname * pn );
static GOOD_OR_BAD OW_set_range( int range, struct parsedname * pn );
static GOOD_OR_BAD OW_get_power( struct parsedname * pn );

/* read a page of memory (8 bytes) */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	return COMMON_offset_process( FS_r_mem, owq, OWQ_pn(owq).extension*_1W_2450_PAGESIZE) ;
}

/* write an 8-byte page */
static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	return COMMON_offset_process(FS_w_mem, owq, OWQ_pn(owq).extension*_1W_2450_PAGESIZE) ;
}

/* read powered flag */
static ZERO_OR_ERROR FS_r_power(struct one_wire_query *owq)
{
	BYTE p;
	RETURN_ERROR_IF_BAD( OW_r_mem(&p, 1, _ADDRESS_POWERED, PN(owq)) ) ;
	OWQ_Y(owq) = (p == _1W_2450_POWERED);
	Cache_Add_SlaveSpecific(&OWQ_Y(owq), sizeof(int), SlaveSpecificTag(POW), PN(owq));
	return 0;
}

/* write powered flag */
static ZERO_OR_ERROR FS_w_power(struct one_wire_query *owq)
{
	BYTE p = _1W_2450_POWERED;	/* powered */
	BYTE q = _1W_2450_UNPOWERED;	/* parasitic */
	if (OW_w_mem(OWQ_Y(owq) ? &p : &q, 1, _ADDRESS_POWERED, PN(owq))) {
		return -EINVAL;
	}
	Cache_Add_SlaveSpecific(&OWQ_Y(owq), sizeof(int), SlaveSpecificTag(POW), PN(owq));
	return 0;
}

/* write "unset" PowerOnReset flag */
static ZERO_OR_ERROR FS_w_por(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_w_por( OWQ_Y(owq), PN(owq) ) ) ;
}

/* read high/low voltage alarm flags */
static ZERO_OR_ERROR FS_r_enable(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E( OW_r_enable( &OWQ_Y(owq), pn->selected_filetype->data.i, pn) ) ;
}

/* write high/low voltage alarm flags */
static ZERO_OR_ERROR FS_w_enable(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E( OW_w_enable( OWQ_Y(owq), pn->selected_filetype->data.i, pn) ) ;
}

/* read high/low voltage triggered state alarm flags */
static ZERO_OR_ERROR FS_r_flag(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E( OW_r_flag( &OWQ_Y(owq), pn->selected_filetype->data.i, pn) ) ;
}

/* write high/low voltage triggered state alarm flags */
static ZERO_OR_ERROR FS_w_flag(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E(OW_w_flag( OWQ_Y(owq), pn->selected_filetype->data.i, pn)) ;
}

/* 2450 A/D */
// separate
static ZERO_OR_ERROR FS_r_PIO(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_r_pio( &OWQ_Y(owq), PN(owq)) ) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_w_PIO(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_w_pio( OWQ_Y(owq), PN(owq)) ) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, 0, _1W_2450_PAGESIZE, COMMON_read_memory_crc16_AA)) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, _1W_2450_PAGESIZE, OW_w_mem)) ;
}

/* 2450 A/D */
static ZERO_OR_ERROR FS_volts(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int resolution ;
	int range ;
	_FLOAT V[4] ;

	switch ( pn->selected_filetype->data.i ) {
		case r_2V_8bit:
			range = 2 ;
			resolution = 8 ;
			break ;
		case r_5V_8bit:
			range = 5 ;
			resolution = 8 ;
			break ;
		case r_2V_16bit:
			range = 2 ;
			resolution = 16 ;
			break ;
		case r_5V_16bit:
		default:
			range = 5 ;
			resolution = 16 ;
			break ;
	}
	if ( BAD( OW_set_resolution( resolution, pn ) ) ) {
		return -EINVAL ;
	}	
	if ( BAD( OW_set_range( range, pn ) ) ) {
		return -EINVAL ;
	}	
	// Start A/D process if needed
	if ( BAD( OW_convert( OWQ_SIMUL_TEST(owq), (int)(.5+.16+4.*resolution*.08), pn) )) {
		return -EINVAL ;
	}	
	
	if ( BAD( OW_volts( V, pn ) )) {
		return -EINVAL ;
	}
	switch ( pn->selected_filetype->data.i ) {
		case r_2V_8bit:
		case r_2V_16bit:
			OWQ_array_F(owq, 0) = V[0]*.5;
			OWQ_array_F(owq, 1) = V[1]*.5;
			OWQ_array_F(owq, 2) = V[2]*.5;
			OWQ_array_F(owq, 3) = V[3]*.5;
			break ;
		case r_5V_8bit:
		case r_5V_16bit:
		default:
			OWQ_array_F(owq, 0) = V[0];
			OWQ_array_F(owq, 1) = V[1];
			OWQ_array_F(owq, 2) = V[2];
			OWQ_array_F(owq, 3) = V[3];
			break ;
	}
	return 0 ;
}

static ZERO_OR_ERROR FS_r_setvolt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E( OW_r_vset( &OWQ_F(owq), pn->selected_filetype->data.i, pn) ) ;
}

static ZERO_OR_ERROR FS_w_setvolt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	RETURN_ERROR_IF_BAD( FS_w_sibling_Y( 0, "set_alarm/unset", owq ) ) ;
	return GB_to_Z_OR_E(OW_w_vset( OWQ_F(owq), pn->selected_filetype->data.i, pn)) ;
}

static GOOD_OR_BAD OW_r_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn)
{
	OWQ_allocate_struct_and_pointer(owq_read);

	OWQ_create_temporary(owq_read, (char *) p, size, offset, pn);
	return COMMON_read_memory_crc16_AA(owq_read, 0, _1W_2450_PAGESIZE);
}

/* write to 2450 */
static GOOD_OR_BAD OW_w_mem(BYTE * p, size_t size, off_t offset, struct parsedname *pn)
{
	// command, address(2) , data , crc(2), databack
	BYTE buf[6] = { _1W_WRITE_MEMORY, LOW_HIGH_ADDRESS(offset), p[0], };
	BYTE echo[1];
	size_t i;
	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WR_CRC16(buf, 4, 0),
		TRXN_READ1(echo),
		TRXN_COMPARE(echo, p, 1),
		TRXN_END,
	};
	struct transaction_log trest[] = {	// note no TRXN_START
		TRXN_WRITE1(buf),
		TRXN_READ2(&buf[1]),
		TRXN_READ1(echo),
		TRXN_END,
	};
//printf("2450 W mem size=%d location=%d\n",size,location) ;

	if (size == 0) {
		return gbGOOD;
	}

	/* Send the first byte (handled differently) */
	RETURN_BAD_IF_BAD(BUS_transaction(tfirst, pn)) ;
	/* rest of the bytes */
	for (i = 1; i < size; ++i) {
		buf[0] = p[i];
		RETURN_BAD_IF_BAD( BUS_transaction(trest, pn) ) ;
		if ( CRC16seeded(buf, 3, offset + i) || (echo[0] != p[i]) ) {
			return gbBAD;
		}
	}
	return gbGOOD;
}

/* Read A/D from 2450 */
/* range adjustment made elsewhere */
static GOOD_OR_BAD OW_volts(_FLOAT * V, struct parsedname *pn)
{
	BYTE data[_1W_2450_PAGESIZE];

	// read data
	RETURN_BAD_IF_BAD( OW_r_mem(data, _1W_2450_PAGESIZE, _ADDRESS_CONVERSION_PAGE, pn) ) ;

	// data conversions
	V[0] = 7.8126192E-5 * ((((UINT) data[1]) << 8) | data[0]);
	V[1] = 7.8126192E-5 * ((((UINT) data[3]) << 8) | data[2]);
	V[2] = 7.8126192E-5 * ((((UINT) data[5]) << 8) | data[4]);
	V[3] = 7.8126192E-5 * ((((UINT) data[7]) << 8) | data[6]);
	return gbGOOD;
}

/* send A/D conversion command */
static GOOD_OR_BAD OW_convert( int simul_good, int delay, struct parsedname *pn)
{
	BYTE convert[] = { _1W_CONVERT, 0x0F, 0x00, 0xFF, 0xFF, };
	struct transaction_log tpower[] = {
		TRXN_START,
		TRXN_WR_CRC16(convert, 3, 0),
		TRXN_END,
	};
	struct transaction_log tdead[] = {
		TRXN_START,
		TRXN_WRITE3(convert),
		TRXN_READ1(&convert[3]),
		TRXN_POWER( &convert[4], delay ) ,
		TRXN_CRC16(convert, 5),
		TRXN_END,
	};

	/* See if a conversion was globally triggered */
	if ( GOOD(OW_get_power(pn) ) ) {
		if ( simul_good ) { 
			return FS_Test_Simultaneous( simul_volt, delay, pn) ;
		} 
		// Start conversion
		// 6 msec for 16bytex4channel (5.2)
		RETURN_BAD_IF_BAD(BUS_transaction(tpower, pn));
		UT_delay(delay);			/* don't need to hold line for conversion! */
	} else {
		// Start conversion
		// 6 msec for 16bytex4channel (5.2)
		RETURN_BAD_IF_BAD(BUS_transaction(tdead, pn)) ;
	}
	return gbGOOD;
}

/* read a pio register */
static GOOD_OR_BAD OW_r_pio(int *pio, struct parsedname *pn)
{
	BYTE p[1];
	// 2 bytes per register
	RETURN_BAD_IF_BAD( OW_r_mem(p, 1, _ADDRESS_CONTROL_PAGE + 2 * pn->extension, pn) ) ;
	
	pio[0] = ((p[0] & (_1W_2450_OC|_1W_2450_OE)) != _1W_2450_OE);
	return gbGOOD;
}

/* Write a pio register */
static GOOD_OR_BAD OW_w_pio( int pio, struct parsedname *pn)
{
	BYTE p[0];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 1, _ADDRESS_CONTROL_PAGE + 2 * pn->extension, pn) ) ;
	p[0] |= _1W_2450_OE | _1W_2450_OC ;
	if ( pio==0 ) {
		p[0] &= ~_1W_2450_OC ;
	}
	return OW_w_mem(p, 1, _ADDRESS_CONTROL_PAGE + 2 * pn->extension, pn);
}

static GOOD_OR_BAD OW_r_vset(_FLOAT * V, enum V_alarm_level ae, struct parsedname *pn)
{
	BYTE p[2];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 2, _ADDRESS_ALARM_PAGE + 2 * pn->extension, pn) ) ;
	switch ( ae ) {
		case V2_ae_high:
			V[0] = .01 * p[1];
			break ;
		case V2_ae_low:
			V[0] = .01 * p[0];
			break ;
		case V5_ae_high:
			V[0] = .02 * p[1];
			break ;
		case V5_ae_low:
			V[0] = .02 * p[0];
			break ;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_vset( _FLOAT V, enum V_alarm_level ae, struct parsedname *pn)
{
	BYTE p[_1W_2450_PAGESIZE];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 2, _ADDRESS_ALARM_PAGE + 2 * pn->extension, pn) ) ;
	switch ( ae ) {
		case V2_ae_high:
			p[1] = 100. * V ;
			break ;
		case V2_ae_low:
			p[0] = 100. * V ;
			break ;
		case V5_ae_high:
			p[1] = 50. * V ;
			break ;
		case V5_ae_low:
			p[0] = 50. * V ;
			break ;
	}
	return OW_w_mem(p, 2, _ADDRESS_ALARM_PAGE + 2 * pn->extension, pn) ;
}

static GOOD_OR_BAD OW_r_enable(int *y, enum alarm_level ae, struct parsedname *pn)
{
	switch ( ae ) {
		case ae_low:
			return OW_r_mask( y, _1W_2450_AEL, pn ) ;
		case ae_high:
			return OW_r_mask( y, _1W_2450_AEH, pn );
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD OW_w_enable( int y, enum alarm_level ae, struct parsedname *pn)
{
	switch ( ae ) {
		case ae_low:
			return OW_w_mask( y, _1W_2450_AEL, pn );
		case ae_high:
			return OW_w_mask( y, _1W_2450_AEH, pn );
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD OW_r_flag(int *y, enum alarm_level ae, struct parsedname *pn)
{
	switch ( ae ) {
		case ae_low:
			return OW_r_mask( y, _1W_2450_AFL, pn ) ;
		case ae_high:
			return OW_r_mask( y, _1W_2450_AFH, pn );
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD OW_r_mask(int *y, BYTE mask, struct parsedname *pn)
{
	BYTE p[1];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 1, _ADDRESS_CONTROL_PAGE + 2 * pn->extension + 1, pn) ) ;
	y[0] = (p[0] & mask) ? 1 : 0 ;
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_mask( int y, BYTE mask, struct parsedname *pn)
{
	BYTE p[1];
	RETURN_BAD_IF_BAD( OW_r_mem(p, 1, _ADDRESS_CONTROL_PAGE + 2 * pn->extension + 1, pn) ) ;
	if ( y ) {
		p[0] |= mask ;
	} else {
		p[0] &= ~mask ;
	}
	// Clear POR as well
	p[0] &= ~_1W_2450_POR ;
	return OW_w_mem(p, 1, _ADDRESS_CONTROL_PAGE + 2 * pn->extension + 1, pn) ;
}

static GOOD_OR_BAD OW_w_flag( int y, enum alarm_level ae, struct parsedname *pn)
{
	switch ( ae ) {
		case ae_low:
			return OW_w_mask( y, _1W_2450_AFL, pn );
		case ae_high:
			return OW_w_mask( y, _1W_2450_AFH, pn );
		default:
			return gbBAD ;
	}
}

// Always clear
static GOOD_OR_BAD OW_w_por( int por, struct parsedname *pn)
{
	// use a NOP mask, since OW_w_mask always clears POR
	(void) por ;
	return OW_w_mask( 1, 0x00, pn ) ;
}


/* Functions for the CO2 sensor */
static ZERO_OR_ERROR FS_CO2_power( struct one_wire_query *owq)
{
	_FLOAT P = 0. ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F(&P,"volt.D",owq) ;

	OWQ_F(owq) = P ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_CO2_ppm( struct one_wire_query *owq)
{
	_FLOAT P = 0. ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F(&P,"volt.A",owq) ;

	OWQ_U(owq) = P*1000. ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_CO2_status( struct one_wire_query *owq)
{
	_FLOAT V = 0.;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F(&V,"volt.B",owq) ;

	OWQ_Y(owq) = (V>3.0) && (V<3.4) ;
	return z_or_e ;
}

static GOOD_OR_BAD OW_set_resolution( int resolution, struct parsedname * pn )
{
	int stored_resolution ;
	/* Resolution */
	if ( BAD( Cache_Get_SlaveSpecific(&stored_resolution, sizeof(stored_resolution), SlaveSpecificTag(RES), pn))
		|| stored_resolution != resolution) {
			// need to set resolution
		BYTE p[_1W_2450_PAGESIZE];
		RETURN_BAD_IF_BAD( OW_r_mem(p, _1W_2450_PAGESIZE, _ADDRESS_CONTROL_PAGE, pn) ) ;
		p[_1W_2450_REG_A] &= ~_1W_2450_RC_MASK ;
		p[_1W_2450_REG_A] |= ( resolution & _1W_2450_RC_MASK ) ;
		p[_1W_2450_REG_B] &= ~_1W_2450_RC_MASK ;
		p[_1W_2450_REG_B] |= ( resolution & _1W_2450_RC_MASK ) ;
		p[_1W_2450_REG_C] &= ~_1W_2450_RC_MASK ;
		p[_1W_2450_REG_C] |= ( resolution & _1W_2450_RC_MASK ) ;
		p[_1W_2450_REG_D] &= ~_1W_2450_RC_MASK ;
		p[_1W_2450_REG_D] |= ( resolution & _1W_2450_RC_MASK ) ;
		RETURN_BAD_IF_BAD( OW_w_mem(p, _1W_2450_PAGESIZE, _ADDRESS_CONTROL_PAGE, pn) );
		return Cache_Add_SlaveSpecific(&resolution, sizeof(int), SlaveSpecificTag(RES), pn);
	}
	return gbGOOD ;
}

// range is 5=5V or 2=2.5V
static GOOD_OR_BAD OW_set_range( int range, struct parsedname * pn )
{
	int stored_range ;
	/* Range */
	if ( BAD( Cache_Get_SlaveSpecific(&stored_range, sizeof(stored_range), SlaveSpecificTag(RAN), pn))
		|| stored_range != range) {
			// need to set resolution
		BYTE p[_1W_2450_PAGESIZE];
		RETURN_BAD_IF_BAD( OW_r_mem(p, _1W_2450_PAGESIZE, _ADDRESS_CONTROL_PAGE, pn) ) ;
		switch ( range ) {
			case 2:
				p[_1W_2450_REG_A+1] &= ~_1W_2450_IR ;
				p[_1W_2450_REG_B+1] &= ~_1W_2450_IR ;
				p[_1W_2450_REG_C+1] &= ~_1W_2450_IR ;
				p[_1W_2450_REG_D+1] &= ~_1W_2450_IR ;
				break ;
			case 5:
			default:
				p[_1W_2450_REG_A+1] |= _1W_2450_IR ;
				p[_1W_2450_REG_B+1] |= _1W_2450_IR ;
				p[_1W_2450_REG_C+1] |= _1W_2450_IR ;
				p[_1W_2450_REG_D+1] |= _1W_2450_IR ;
				break ;
		}
		RETURN_BAD_IF_BAD( OW_w_mem(p, _1W_2450_PAGESIZE, _ADDRESS_CONTROL_PAGE, pn) );
		return Cache_Add_SlaveSpecific(&range, sizeof(int), SlaveSpecificTag(RAN), pn);
	}
	return gbGOOD ;
}

// good if powered.
static GOOD_OR_BAD OW_get_power( struct parsedname * pn )
{
	int power ;
	/* power */
	if ( BAD( Cache_Get_SlaveSpecific(&power, sizeof(power), SlaveSpecificTag(POW), pn)) ) {
		BYTE p[1];
		/* get power flag -- to see if pullup can be avoided */
		RETURN_BAD_IF_BAD( OW_r_mem(p, 1, _ADDRESS_POWERED, pn) ) ;
		power = (p[1]==_1W_2450_POWERED) ? 1 : 0 ; 
		Cache_Add_SlaveSpecific(&power, sizeof(power), SlaveSpecificTag(POW), pn);
	}
	return power==1 ? gbGOOD : gbBAD ;
}
