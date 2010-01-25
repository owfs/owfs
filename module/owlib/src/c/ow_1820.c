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
#include "ow_1820.h"

/* ------- Prototypes ----------- */
/* DS18S20&2 Temperature */
// READ_FUNCTION( FS_tempdata ) ;
READ_FUNCTION(FS_10temp);
READ_FUNCTION(FS_22temp);
READ_FUNCTION(FS_fasttemp);
READ_FUNCTION(FS_slowtemp);
READ_FUNCTION(FS_power);
READ_FUNCTION(FS_r_templimit);
WRITE_FUNCTION(FS_w_templimit);
READ_FUNCTION(FS_r_die);
READ_FUNCTION(FS_r_trim);
WRITE_FUNCTION(FS_w_trim);
READ_FUNCTION(FS_r_trimvalid);
READ_FUNCTION(FS_r_blanket);
WRITE_FUNCTION(FS_w_blanket);
READ_FUNCTION(FS_r_ad);
READ_FUNCTION(FS_r_piostate);
READ_FUNCTION(FS_r_pio);
READ_FUNCTION(FS_r_latch);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);

/* -------- Structures ---------- */
struct filetype DS18S20[] = {
	F_STANDARD,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_10temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1},},
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0},},
	{"errata", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_stable, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"errata/trim", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_trim, FS_w_trim, VISIBLE, NO_FILETYPE_DATA,},
	{"errata/die", 2, NON_AGGREGATE, ft_ascii, fc_static, FS_r_die, NO_WRITE_FUNCTION, VISIBLE, {i:1},},
	{"errata/trimvalid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_trimvalid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"errata/trimblanket", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_blanket, FS_w_blanket, VISIBLE, NO_FILETYPE_DATA,},
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
}

;
DeviceEntryExtended(10, DS18S20, DEV_temp | DEV_alarm);

struct filetype DS18B20[] = {
	F_STANDARD,
	//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,   FS_tempdata, NO_WRITE_FUNCTION, VISIBLE, NULL, } ,//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,  {o:FS_tempdata} ,
{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10},},
{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11},},
{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1},},
{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0},},
{"errata", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_stable, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
{"errata/trim", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_trim, FS_w_trim, VISIBLE, NO_FILETYPE_DATA,},
{"errata/die", 2, NON_AGGREGATE, ft_ascii, fc_static, FS_r_die, NO_WRITE_FUNCTION, VISIBLE, {i:2},},
{"errata/trimvalid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_trimvalid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
{"errata/trimblanket", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_blanket, FS_w_blanket, VISIBLE, NO_FILETYPE_DATA,},
{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(28, DS18B20, DEV_temp | DEV_alarm);

struct filetype DS1822[] = {
	F_STANDARD,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,   FS_tempdata, NO_WRITE_FUNCTION, NULL, } ,//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,  {o:FS_tempdata} ,
{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10},},
{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11},},
{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1},},
{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0},},
{"errata", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_stable, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
{"errata/trim", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_trim, FS_w_trim, VISIBLE, NO_FILETYPE_DATA,},
{"errata/die", 2, NULL, ft_ascii, fc_static, FS_r_die, NO_WRITE_FUNCTION, VISIBLE, {i:0},},
	{"errata/trimvalid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_trimvalid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"errata/trimblanket", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_blanket, FS_w_blanket, VISIBLE, NO_FILETYPE_DATA,},
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(22, DS1822, DEV_temp | DEV_alarm);

struct filetype DS1825[] = {
	F_STANDARD,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,   FS_tempdata, NO_WRITE_FUNCTION, NULL, } ,//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,  {o:FS_tempdata} ,
{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10},},
{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11},},
{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1},},
{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0},},
{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
{"prog_addr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_ad, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(3B, DS1825, DEV_temp | DEV_alarm);

struct aggregate A28EA00 = { 2, ag_letters, ag_aggregate, };

struct filetype DS28EA00[] = {
	F_STANDARD,
	//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,   FS_tempdata, NO_WRITE_FUNCTION, VISIBLE, NULL, } ,//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile,  {o:FS_tempdata} ,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_slowtemp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
	{"temperature9", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
	{"temperature10", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:10},},
	{"temperature11", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:11},},
	{"temperature12", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_22temp, NO_WRITE_FUNCTION, VISIBLE, {i:12},},
	{"fasttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_fasttemp, NO_WRITE_FUNCTION, VISIBLE, {i:9},},
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1},},
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0},},
	{"power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_power, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"piostate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_piostate, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"PIO", PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_link, FS_r_pio, FS_w_pio, VISIBLE, NO_FILETYPE_DATA,},
	{"latch", PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_link, FS_r_latch, FS_w_pio, VISIBLE, NO_FILETYPE_DATA,},
	{"sensed", PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_link, FS_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(42, DS28EA00, DEV_temp | DEV_alarm | DEV_chain);

/* Internal properties */
//static struct internal_prop ip_resolution = { "RES", fc_stable };
MakeInternalProp(RES, fc_stable);	// resolution
//static struct internal_prop ip_power = { "POW", fc_stable };
MakeInternalProp(POW, fc_stable);	// power status

struct tempresolution {
	BYTE config;
	UINT delay;
	BYTE mask;
};
struct tempresolution Resolutions[] = {
	{0x1F, 110, 0xF8},			/*  9 bit */
	{0x3F, 200, 0xFC},			/* 10 bit */
	{0x5F, 400, 0xFE},			/* 11 bit */
	{0x7F, 1000, 0xFF},			/* 12 bit */
};

struct die_limits {
	BYTE B7[6];
	BYTE C2[6];
};

enum eDie { eB6, eB7, eC2, eC3, };

// ID ranges for the different chip dies
struct die_limits DIE[] = {
	{							// DS1822 Family code 22
	 {0x00, 0x00, 0x00, 0x08, 0x97, 0x8A},
	 {0x00, 0x00, 0x00, 0x0C, 0xB8, 0x1A},
	 },
	{							// DS18S20 Family code 10
	 {0x00, 0x08, 0x00, 0x59, 0x1D, 0x20},
	 {0x00, 0x08, 0x00, 0x80, 0x88, 0x60},
	 },
	{							// DS18B20 Family code 28
	 {0x00, 0x00, 0x00, 0x54, 0x50, 0x10},
	 {0x00, 0x00, 0x00, 0x66, 0x2B, 0x50},
	 },
};

/* Intermediate cached values  -- start with unlikely asterisk */
/* RES -- resolution
   POW -- power
*/

#define _1W_WRITE_SCRATCHPAD      0x4E
#define _1W_READ_SCRATCHPAD       0xBE
#define _1W_COPY_SCRATCHPAD       0x48
#define _1W_CONVERT_T             0x44
#define _1W_READ_POWERMODE        0xB4
#define _1W_RECALL_EEPROM         0xB8
#define _1W_PIO_ACCESS_READ       0xF5
#define _1W_PIO_ACCESS_WRITE      0xA5
#define _1W_CHAIN_COMMAND         0x99
#define _1W_CHAIN_SUBCOMMAND_OFF  0x3C
#define _1W_CHAIN_SUBCOMMAND_ON   0x5A
#define _1W_CHAIN_SUBCOMMAND_DONE 0x96

#define _1W_READ_TRIM_1           0x93
#define _1W_READ_TRIM_2           0x68
#define _1W_WRITE_TRIM_1          0x95
#define _1W_WRITE_TRIM_2          0x63
#define _1W_ACTIVATE_TRIM_1       0x94
#define _1W_ACTIVATE_TRIM_2       0x64
#define _DEFAULT_BLANKET_TRIM_1   0x9D
#define _DEFAULT_BLANKET_TRIM_2   0xBB

/* ------- Functions ------------ */

/* DS1820&2*/
static int OW_10temp(_FLOAT * temp, const struct parsedname *pn);
static int OW_22temp(_FLOAT * temp, const int resolution, const struct parsedname *pn);
static int OW_power(BYTE * data, const struct parsedname *pn);
static int OW_r_templimit(_FLOAT * T, const int Tindex, const struct parsedname *pn);
static int OW_w_templimit(const _FLOAT T, const int Tindex, const struct parsedname *pn);
static int OW_r_scratchpad(BYTE * data, const struct parsedname *pn);
static int OW_w_scratchpad(const BYTE * data, const struct parsedname *pn);
static int OW_r_trim(BYTE * trim, const struct parsedname *pn);
static int OW_w_trim(const BYTE * trim, const struct parsedname *pn);
static enum eDie OW_die(const struct parsedname *pn);
static int OW_w_pio(BYTE pio, const struct parsedname *pn);
static int OW_read_piostate(UINT * piostate, const struct parsedname *pn) ;
static _FLOAT OW_masked_temperature( BYTE * data, BYTE mask ) ;

static int FS_10temp(struct one_wire_query *owq)
{
	if (OW_10temp(&OWQ_F(owq), PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
static int FS_22temp(struct one_wire_query *owq)
{
	int resolution = OWQ_pn(owq).selected_filetype->data.i;
	switch (resolution) {
	case 9:
	case 10:
	case 11:
	case 12:
		if (OW_22temp(&OWQ_F(owq), resolution, PN(owq))) {
			return -EINVAL;
		}
		return 0;
	}
	return -ENODEV;
}

// use sibling function for fasttemp to keep cache value consistent
static int FS_fasttemp(struct one_wire_query *owq)
{
	_FLOAT temperature ;

	if ( FS_r_sibling_F( &temperature, "temperature9", owq ) ) {
		return -EINVAL ;
	}

	OWQ_F(owq) = temperature ;

	return 0 ;
}

// use sibling function for temperature to keep cache value consistent
static int FS_slowtemp(struct one_wire_query *owq)
{
	_FLOAT temperature ;

	if ( FS_r_sibling_F( &temperature, "temperature12", owq ) ) {
		return -EINVAL ;
	}

	OWQ_F(owq) = temperature ;

	return 0 ;
}

static int FS_power(struct one_wire_query *owq)
{
	BYTE data;
	if (OW_power(&data, PN(owq))) {
		return -EINVAL;
	}
	OWQ_Y(owq) = (data != 0x00);
	return 0;
}


/* 28EA00 switch */
static int FS_w_pio(struct one_wire_query *owq)
{
	BYTE data = BYTE_INVERSE(OWQ_U(owq) & 0x03);	/* reverse bits, set unused to 1s */
	//printf("Write pio raw=%X, stored=%X\n",OWQ_U(owq),data) ;
	FS_del_sibling( "piostate", owq ) ;
	if (OW_w_pio(data, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int FS_sense(struct one_wire_query *owq)
{
	UINT piostate ;

	if ( FS_r_sibling_U( &piostate, "piostate", owq ) ) {
		return -EINVAL ;
	}

	// bits 0->0 and 2->1
	OWQ_U(owq) = ( (piostate & 0x01) | ((piostate & 0x04)>>1) ) & 0x03  ;

	return 0;
}

static int FS_r_pio(struct one_wire_query *owq)
{
	UINT piostate ;

	if ( FS_r_sibling_U( &piostate, "piostate", owq ) ) {
		return -EINVAL ;
	}

	// bits 0->0 and 2->1
	OWQ_U(owq) = BYTE_INVERSE( (piostate & 0x01) | ((piostate & 0x04)>>1) ) & 0x03  ;

	return 0;
}

static int FS_r_piostate(struct one_wire_query *owq)
{
	if (OW_read_piostate(&(OWQ_U(owq)), PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_latch(struct one_wire_query *owq)
{
	UINT piostate ;

	if ( FS_r_sibling_U( &piostate, "piostate", owq ) ) {
		return -EINVAL ;
	}

	// bits 1->0 and 3->1
	OWQ_U(owq) = BYTE_INVERSE( ((piostate & 0x02)>>1) | ((piostate & 0x08)>>2) ) & 0x03  ;

	return 0;
}

static int FS_r_templimit(struct one_wire_query *owq)
{
	if (OW_r_templimit(&OWQ_F(owq), PN(owq)->selected_filetype->data.i, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

/* DS1825 hardware programmable address */
static int FS_r_ad(struct one_wire_query *owq)
{
	BYTE data[9];
	if (OW_r_scratchpad(data, PN(owq))) {
		return -EINVAL;
	}
	OWQ_U(owq) = data[4] & 0x0F;
	return 0;
}

static int FS_w_templimit(struct one_wire_query *owq)
{
	if (OW_w_templimit(OWQ_F(owq), PN(owq)->selected_filetype->data.i, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_die(struct one_wire_query *owq)
{
	char *d;
	switch (OW_die(PN(owq))) {
	case eB6:
		d = "B6";
		break;
	case eB7:
		d = "B7";
		break;
	case eC2:
		d = "C2";
		break;
	case eC3:
		d = "C3";
		break;
	default:
		return -EINVAL;
	}
	return Fowq_output_offset_and_size(d, 2, owq);
}

static int FS_r_trim(struct one_wire_query *owq)
{
	BYTE t[2];
	if (OW_r_trim(t, PN(owq))) {
		return -EINVAL;
	}
	OWQ_U(owq) = (t[1] << 8) | t[0];
	return 0;
}

static int FS_w_trim(struct one_wire_query *owq)
{
	UINT trim = OWQ_U(owq);
	BYTE t[2] = { LOW_HIGH_ADDRESS(trim), };
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		if (OW_w_trim(t, PN(owq))) {
			return -EINVAL;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

/* Are the trim values valid-looking? */
static int FS_r_trimvalid(struct one_wire_query *owq)
{
	BYTE trim[2];
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		if (OW_r_trim(trim, PN(owq))) {
			return -EINVAL;
		}
		OWQ_Y(owq) = (((trim[0] & 0x07) == 0x05)
					  || ((trim[0] & 0x07) == 0x03))
			&& (trim[1] == 0xBB);
		break;
	default:
		OWQ_Y(owq) = 1;			/* Assume true */
	}
	return 0;
}

/* Put in a blank trim value if non-zero */
static int FS_r_blanket(struct one_wire_query *owq)
{
	BYTE trim[2];
	BYTE blanket[] = { _DEFAULT_BLANKET_TRIM_1, _DEFAULT_BLANKET_TRIM_2 };
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		if (OW_r_trim(trim, PN(owq))) {
			return -EINVAL;
		}
		OWQ_Y(owq) = (memcmp(trim, blanket, 2) == 0);
		return 0;
	default:
		return -EINVAL;
	}
}

/* Put in a blank trim value if non-zero */
static int FS_w_blanket(struct one_wire_query *owq)
{
	BYTE blanket[] = { _DEFAULT_BLANKET_TRIM_1, _DEFAULT_BLANKET_TRIM_2 };
	switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		if (OWQ_Y(owq)) {
			if (OW_w_trim(blanket, PN(owq))) {
				return -EINVAL;
			}
		}
		return 0;
	default:
		return -EINVAL;
	}
}

/* get the temp from the scratchpad buffer after starting a conversion and waiting */
static int OW_10temp(_FLOAT * temp, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE convert[] = { _1W_CONVERT_T, };
	UINT delay = 1100;			// hard wired
	BYTE pow;
	struct transaction_log tunpowered[] = {
		TRXN_START,
		{convert, convert, delay, trxn_power},
		TRXN_END,
	};
	struct transaction_log tpowered[] = {
		TRXN_START,
		TRXN_WRITE1(convert),
		TRXN_END,
	};

	if (OW_power(&pow, pn)) {
		pow = 0x00;				/* assume unpowered if cannot tell */
	}

	/* Select particular device and start conversion */
	if (!pow) {					// unpowered, deliver power, no communication allowed
		if (BUS_transaction(tunpowered, pn)) {
			return 1;
		}
	} else if (FS_Test_Simultaneous( simul_temp, delay, pn)) {	// powered
		// Simultaneous not valid, so do a conversion
		int ret;
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		if (ret) {
			return ret;
		}
	}

	if (OW_r_scratchpad(data, pn)) {
		return 1;
	}

#if 0
	/* Check for error condition */
	if (data[0] == 0xAA && data[1] == 0x00 && data[6] == 0x0C) {
		/* repeat the conversion (only once) */
		/* Do it the most conservative way -- unpowered */
		if (!pow) {				// unpowered, deliver power, no communication allowed
			if (BUS_transaction(tunpowered, pn)) {
				return 1;
			}
		} else {				// powered, so release bus immediately after issuing convert
			if (BUS_transaction(tpowered, pn)) {
				return 1;
			}
			UT_delay(delay);
		}
		if (OW_r_scratchpad(data, pn)) {
			return 1;
		}
	}
#endif
// Correction thanks to Nathan D. Holmes
	//temp[0] = (_FLOAT) ((int16_t)(data[1]<<8|data[0])) * .5 ; // Main conversion
	// Further correction, using "truncation" thanks to Wim Heirman
	//temp[0] = (_FLOAT) ((int16_t)(data[1]<<8|data[0])>>1); // Main conversion
	temp[0] = (_FLOAT) ((UT_int16(data)) >> 1);	// Main conversion -- now with helper function
	if (data[7]) {				// only if COUNT_PER_C non-zero (supposed to be!)
//        temp[0] += (_FLOAT)(data[7]-data[6]) / (_FLOAT)data[7] - .25 ; // additional precision
		temp[0] += .75 - (_FLOAT) data[6] / (_FLOAT) data[7];	// additional precision
	}
	return 0;
}

static int OW_power(BYTE * data, const struct parsedname *pn)
{
	if (IsUncachedDir(pn)
		|| Cache_Get_Internal_Strict(data, sizeof(BYTE), InternalProp(POW), pn)) {
		BYTE b4[] = { _1W_READ_POWERMODE, };
		struct transaction_log tpower[] = {
			TRXN_START,
			TRXN_WRITE1(b4),
			TRXN_READ1(data),
			TRXN_END,
		};
	
		if (BUS_transaction(tpower, pn)) {
			return 1;
		}
		Cache_Add_Internal(data, sizeof(BYTE), InternalProp(POW), pn);
	}
	return 0;
}

static int OW_22temp(_FLOAT * temp, const int resolution, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE convert[] = { _1W_CONVERT_T, };
	BYTE pow;
	UINT delay = Resolutions[resolution - 9].delay;
	UINT longdelay = delay * 1.5 ; // failsafe
	BYTE mask = Resolutions[resolution - 9].mask;
	int stored_resolution ;
	int must_convert = 0 ;

	struct transaction_log tunpowered[] = {
		TRXN_START,
		{convert, convert, delay, trxn_power},
		TRXN_END,
	};
	struct transaction_log tpowered[] = {
		TRXN_START,
		TRXN_WRITE1(convert),
		TRXN_END,
	};
	// failsafe
	struct transaction_log tunpowered_long[] = {
		TRXN_START,
		{convert, convert, longdelay, trxn_power},
		TRXN_END,
	};

	/* Resolution */
	if (Cache_Get_Internal_Strict(&stored_resolution, sizeof(stored_resolution), InternalProp(RES), pn)
		|| stored_resolution != resolution) {
		BYTE resolution_register = Resolutions[resolution - 9].config;
		/* Get existing settings */
		if (OW_r_scratchpad(data, pn)) {
			return 1;
		}
		/* Put in new settings (if different) */
		if ((data[4] | 0x1F) != resolution_register) {	// ignore lower 5 bits
			must_convert = 1 ; // resolution has changed
			data[4] = (resolution_register & 0x60) | 0x1F ;
			if (OW_w_scratchpad(&data[2], pn)) {
				return 1;
			}
			Cache_Add_Internal(&resolution, sizeof(int), InternalProp(RES), pn);
		}
	}

	/* Conversion */
	// first time
	/* powered? */
	if (OW_power(&pow, pn)) {
		pow = 0x00;				/* assume unpowered if cannot tell */
	}

	if (!pow) {					// unpowered, deliver power, no communication allowed
		LEVEL_DEBUG("Unpowered temperature conversion -- %d msec", delay);
		// If not powered, no Simultaneous for this chip
		must_convert = 1 ;
		if (BUS_transaction(tunpowered, pn)) {
			return 1;
		}
	} else if ( must_convert || FS_Test_Simultaneous( simul_temp, delay, pn) ) {
		// No Simultaneous active, so need to "convert"
		// powered, so release bus immediately after issuing convert
		int ret;
		LEVEL_DEBUG("Powered temperature conversion");
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		if (ret) {
			return ret;
		}
	}

	if (OW_r_scratchpad(data, pn)) {
		return 1;
	}

	if ( data[1]!=0x05 || data[0]!=0x50 ) { // not 85C
		temp[0] = OW_masked_temperature( data, mask) ;
		return 0;
	}

	// second time
	LEVEL_DEBUG("Temp error. Try unpowered temperature conversion -- %d msec", delay);
	if (BUS_transaction(tunpowered, pn)) {
		return 1;
	}
	if (OW_r_scratchpad(data, pn)) {
		return 1;
	}
	if ( data[1]!=0x05 || data[0]!=0x50 ) { // not 85C
		temp[0] = OW_masked_temperature( data, mask) ;
		return 0;
	}

	// third and last time
	LEVEL_DEBUG("Temp error. Try unpowered long temperature conversion -- %d msec", longdelay);
	if (BUS_transaction(tunpowered_long, pn)) {
		return 1;
	}
	if (OW_r_scratchpad(data, pn)) {
		return 1;
	}
	temp[0] = OW_masked_temperature( data, mask) ;
	return 0;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit(_FLOAT * T, const int Tindex, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE recall[] = { _1W_READ_POWERMODE, };
	struct transaction_log trecall[] = {
		TRXN_START,
		TRXN_WRITE1(recall),
		TRXN_END,
	};

	if (BUS_transaction(trecall, pn)) {
		return 1;
	}

	UT_delay(10);

	if (OW_r_scratchpad(data, pn)) {
		return 1;
	}
	T[0] = (_FLOAT) ((int8_t) data[2 + Tindex]);
	return 0;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit(const _FLOAT T, const int Tindex, const struct parsedname *pn)
{
	BYTE data[9];

	if (OW_r_scratchpad(data, pn)) {
		return 1;
	}
	data[2 + Tindex] = (uint8_t) T;
	return OW_w_scratchpad(&data[2], pn);
}

/* read 9 bytes, includes CRC8 which is checked */
static int OW_r_scratchpad(BYTE * data, const struct parsedname *pn)
{
	/* data is 9 bytes long */
	BYTE be[] = { _1W_READ_SCRATCHPAD, };
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE1(be),
		TRXN_READ(data, 9),
		TRXN_CRC8(data, 9),
		TRXN_END,
	};
	return BUS_transaction(tread, pn);
}

/* write 3 bytes (byte2,3,4 of register) */
static int OW_w_scratchpad(const BYTE * data, const struct parsedname *pn)
{
	/* data is 3 bytes long */
	BYTE d[4] = { _1W_WRITE_SCRATCHPAD, data[0], data[1], data[2], };
	BYTE pow[] = { _1W_COPY_SCRATCHPAD, };
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(d, 4),
		TRXN_END,
	};
	struct transaction_log tpower[] = {
		TRXN_START,
		{pow, pow, 10, trxn_power},
		TRXN_END,
	};

	/* different processing for DS18S20 and both DS19B20 and DS1822 */
	if (pn->sn[0] == 0x10) {
		twrite->size = 4;
	}

	if (BUS_transaction(twrite, pn)) {
		return 1;
	}

	return BUS_transaction(tpower, pn);
}

/* Trim values -- undocumented except in AN247.pdf */
static int OW_r_trim(BYTE * trim, const struct parsedname *pn)
{
	BYTE cmd0[] = { _1W_READ_TRIM_1, };
	BYTE cmd1[] = { _1W_READ_TRIM_2, };
	struct transaction_log t0[] = {
		TRXN_START,
		TRXN_WRITE1(cmd0),
		TRXN_READ1(&trim[0]),
		TRXN_END,
	};
	struct transaction_log t1[] = {
		TRXN_START,
		TRXN_WRITE1(cmd1),
		TRXN_READ1(&trim[1]),
		TRXN_END,
	};

	if (BUS_transaction(t0, pn)) {
		return 1;
	}

	return BUS_transaction(t1, pn);
}

static int OW_w_trim(const BYTE * trim, const struct parsedname *pn)
{
	BYTE cmd0[] = { _1W_WRITE_TRIM_1, trim[0], };
	BYTE cmd1[] = { _1W_WRITE_TRIM_2, trim[1], };
	BYTE cmd2[] = { _1W_ACTIVATE_TRIM_1, };
	BYTE cmd3[] = { _1W_ACTIVATE_TRIM_2, };
	struct transaction_log t0[] = {
		TRXN_START,
		TRXN_WRITE2(cmd0),
		TRXN_END,
	};
	struct transaction_log t1[] = {
		TRXN_START,
		TRXN_WRITE2(cmd1),
		TRXN_END,
	};
	struct transaction_log t2[] = {
		TRXN_START,
		TRXN_WRITE1(cmd2),
		TRXN_END,
	};
	struct transaction_log t3[] = {
		TRXN_START,
		TRXN_WRITE1(cmd3),
		TRXN_END,
	};

	if (BUS_transaction(t0, pn)) {
		return 1;
	}
	if (BUS_transaction(t1, pn)) {
		return 1;
	}
	if (BUS_transaction(t2, pn)) {
		return 1;
	}
	if (BUS_transaction(t3, pn)) {
		return 1;
	}
	return 0;
}

static enum eDie OW_die(const struct parsedname *pn)
{
	BYTE die[6] = { pn->sn[6], pn->sn[5], pn->sn[4], pn->sn[3], pn->sn[2], pn->sn[1],
	};
	// data gives index into die matrix
	if (memcmp(die, DIE[pn->selected_filetype->data.i].C2, 6) > 0) {
		return eC2;
	}
	if (memcmp(die, DIE[pn->selected_filetype->data.i].B7, 6) > 0) {
		return eB7;
	}
	return eB6;
}

/* Powered temperature measurements -- need to poll line since it is held low during measurement */
/* We check every 10 msec (arbitrary) up to 1.25 seconds */
int FS_poll_convert(const struct parsedname *pn)
{
	int i;
	BYTE p[1];
	struct transaction_log t[] = {
		{NULL, NULL, 10, trxn_delay,},
		TRXN_READ1(p),
		TRXN_END,
	};

	// the first test is faster for just DS2438 (10 msec)
	// subsequent polling is slower since the DS18x20 is a slower converter
	for (i = 0; i < 22; ++i) {
		if (BUS_transaction_nolock(t, pn)) {
			LEVEL_DEBUG("BUS_transaction failed");
			break;
		}
		if (p[0] != 0) {
			LEVEL_DEBUG("BUS_transaction done after %dms", (i + 1) * 10);
			return 0;
		}
		t[0].size = 50;			// 50 msec for rest of delays
	}
	LEVEL_DEBUG("Temperature measurement failed");
	return 1;
}

/* read PIO pins for the DS28EA00 */
static int OW_read_piostate(UINT * piostate, const struct parsedname *pn)
{
	BYTE data[1];
	BYTE cmd[] = { _1W_PIO_ACCESS_READ, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(cmd),
		TRXN_READ1(data),
		TRXN_END,
	};
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	/* compare lower and upper nibble to be complements */
	// High nibble the complement of low nibble?
	// Fix thanks to josef_heiler
	if ((data[0] & 0x0F) != ((~data[0] >> 4) & 0x0F)) {
		return 1;
	}

	piostate[0] = data[0] & 0x0F ;

	return 0;
}
/* Write to PIO -- both channels. Already inverted and other fields set to 1 */
static int OW_w_pio(BYTE pio, const struct parsedname *pn)
{
	BYTE cmd[] = { _1W_PIO_ACCESS_WRITE, pio, BYTE_INVERSE(pio) };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(cmd),
		TRXN_END,
	};
	return BUS_transaction(t, pn);
}

static _FLOAT OW_masked_temperature( BYTE * data, BYTE mask )
{
	// Torsten Godau <tg@solarlabs.de> found a problem with 9-bit resolution
	return (_FLOAT) ((int16_t) ((data[1] << 8) | (data[0] & mask))) * .0625 ;
}
