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
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_sense);

/* -------- Structures ---------- */
struct filetype DS18S20[] = {
	F_STANDARD,
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_10temp}, {v: NULL}, {v:NULL},},
  {"templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:1},},
  {"temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:0},},
  {"trim",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_trim}, {o: FS_w_trim}, {v:NULL},},
  {"die", 2, NULL, ft_ascii, fc_static, {o: FS_r_die}, {v: NULL}, {i:1},},
  {"trimvalid",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_trimvalid}, {v: NULL}, {v:NULL},},
  {"trimblanket",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_blanket}, {o: FS_w_blanket}, {v:NULL},},
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_power}, {v: NULL}, {v:NULL},},
}

;
DeviceEntryExtended(10, DS18S20, DEV_temp | DEV_alarm);

struct filetype DS18B20[] = {
	F_STANDARD,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile, FS_tempdata   , NULL, NULL, NULL,} ,
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"temperature9",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"temperature10",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:10},},
  {"temperature11",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:11},},
  {"temperature12",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"fasttemp",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:1},},
  {"temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:0},},
  {"trim",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_trim}, {o: FS_w_trim}, {v:NULL},},
  {"die", 2, NULL, ft_ascii, fc_static, {o: FS_r_die}, {v: NULL}, {i:2},},
  {"trimvalid",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_trimvalid}, {v: NULL}, {v:NULL},},
  {"trimblanket",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_blanket}, {o: FS_w_blanket}, {v:NULL},},
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_power}, {v: NULL}, {v:NULL},},
};

DeviceEntryExtended(28, DS18B20, DEV_temp | DEV_alarm);

struct filetype DS1822[] = {
	F_STANDARD,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile, FS_tempdata   , NULL, NULL, } ,
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"temperature9",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"temperature10",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:10},},
  {"temperature11",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:11},},
  {"temperature12",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"fasttemp",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:1},},
  {"temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:0},},
  {"trim",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_trim}, {o: FS_w_trim}, {v:NULL},},
  {"die", 2, NULL, ft_ascii, fc_static, {o: FS_r_die}, {v: NULL}, {i:0},},
  {"trimvalid",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_trimvalid}, {v: NULL}, {v:NULL},},
  {"trimblanket",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_blanket}, {o: FS_w_blanket}, {v:NULL},},
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_power}, {v: NULL}, {v:NULL},},
};

DeviceEntryExtended(22, DS1822, DEV_temp | DEV_alarm);

struct filetype DS1825[] = {
	F_STANDARD,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile, FS_tempdata   , NULL, NULL, } ,
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"temperature9",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"temperature10",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:10},},
  {"temperature11",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:11},},
  {"temperature12",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"fasttemp",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:1},},
  {"temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:0},},
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_power}, {v: NULL}, {v:NULL},},
  {"prog_addr",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_ad}, {v: NULL}, {v:NULL},},
};

DeviceEntryExtended(3B, DS1825, DEV_temp | DEV_alarm);

struct aggregate A28EA00 = { 2, ag_letters, ag_aggregate, };

struct filetype DS28EA00[] = {
	F_STANDARD,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile, FS_tempdata   , NULL, NULL, } ,
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"temperature9",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"temperature10",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:10},},
  {"temperature11",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:11},},
  {"temperature12",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:12},},
  {"fasttemp",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_22temp}, {v: NULL}, {i:9},},
  {"templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:1},},
  {"temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable, {o: FS_r_templimit}, {o: FS_w_templimit}, {i:0},},
  {"power",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_power}, {v: NULL}, {v:NULL},},
  {"PIO",PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_stable, {o: FS_r_pio}, {o: FS_w_pio}, {v:NULL},},
  {"sensed",PROPERTY_LENGTH_BITFIELD, &A28EA00, ft_bitfield, fc_volatile, {o: FS_sense}, {v: NULL}, {v:NULL},},
};

DeviceEntryExtended(42, DS28EA00, DEV_temp | DEV_alarm | DEV_chain);

/* Internal properties */
static struct internal_prop ip_resolution = { "RES", fc_stable };
static struct internal_prop ip_power = { "POW", fc_stable };

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

#define _1W_WRITE_SCRATCHPAD 0x4E
#define _1W_READ_SCRATCHPAD 0xBE
#define _1W_COPY_SCRATCHPAD 0x48
#define _1W_CONVERT_T 0x44
#define _1W_READ_POWERMODE 0xB4
#define _1W_RECALL_EEPROM 0xB8
#define _1W_PIO_ACCESS_READ 0xF5
#define _1W_PIO_ACCESS_WRITE 0xA5
#define _1W_CHAIN_COMMAND 0x99
#define _1W_CHAIN_SUBCOMMAND_OFF 0x3C
#define _1W_CHAIN_SUBCOMMAND_ON 0x5A
#define _1W_CHAIN_SUBCOMMAND_DONE 0x96


/* ------- Functions ------------ */

/* DS1820&2*/
static int OW_10temp(_FLOAT * temp, const struct parsedname *pn);
static int OW_22temp(_FLOAT * temp, const int resolution,
					 const struct parsedname *pn);
static int OW_power(BYTE * data, const struct parsedname *pn);
static int OW_r_templimit(_FLOAT * T, const int Tindex,
						  const struct parsedname *pn);
static int OW_w_templimit(const _FLOAT T, const int Tindex,
						  const struct parsedname *pn);
static int OW_r_scratchpad(BYTE * data, const struct parsedname *pn);
static int OW_w_scratchpad(const BYTE * data, const struct parsedname *pn);
static int OW_r_trim(BYTE * trim, const struct parsedname *pn);
static int OW_w_trim(const BYTE * trim, const struct parsedname *pn);
static enum eDie OW_die(const struct parsedname *pn);
static int OW_read_pio( BYTE * pio, BYTE * latch, const struct parsedname * pn) ;
static int OW_w_pio( BYTE pio, const struct parsedname * pn ) ;

static int FS_10temp(struct one_wire_query * owq)
{
    if (OW_10temp(&OWQ_F(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
static int FS_22temp(struct one_wire_query * owq)
{
    int resolution = OWQ_pn(owq).ft->data.i ;
    switch (resolution) {
	case 9:
	case 10:
	case 11:
	case 12:
        if (OW_22temp(&OWQ_F(owq), resolution, PN(owq)))
			return -EINVAL;
		return 0;
	}
	return -ENODEV;
}

static int FS_power(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_power(&data, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = (data != 0x00);
	return 0;
}


/* 28EA00 switch */
static int FS_w_pio(struct one_wire_query * owq)
{
    BYTE data = (OWQ_U(owq)&0x03) ^ 0xFF ; /* reverse bits, set unused to 1s */
    if (OW_w_pio(data, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_sense(struct one_wire_query * owq)
{
	BYTE pio, latch;
    if (OW_read_pio(&pio, &latch, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = pio & 0x03;	/* don't reverse bits */
	return 0;
}

static int FS_r_pio(struct one_wire_query * owq)
{
	BYTE pio, latch;
    if (OW_read_pio(&pio, &latch, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = (latch ^ 0xFF) & 0x03;	/* reverse bits */
	return 0;
}

static int FS_r_templimit(struct one_wire_query * owq)
{
    if (OW_r_templimit(&OWQ_F(owq), PN(owq)->ft->data.i, PN(owq)))
		return -EINVAL;
	return 0;
}

/* DS1825 hardware programmable address */
static int FS_r_ad(struct one_wire_query * owq)
{
	BYTE data[9];
    if (OW_r_scratchpad(data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = data[4] & 0x0F;
	return 0;
}

static int FS_w_templimit(struct one_wire_query * owq)
{
    if (OW_w_templimit(OWQ_F(owq), PN(owq)->ft->data.i, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_die(struct one_wire_query * owq)
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

static int FS_r_trim(struct one_wire_query * owq)
{
	BYTE t[2];
    if (OW_r_trim(t, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = (t[1] << 8) | t[0];
	return 0;
}

static int FS_w_trim(struct one_wire_query * owq)
{
	BYTE t[2];
    UINT trim = OWQ_U(owq) ;
    switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
		t[0] = trim && 0xFF;
		t[1] = (trim >> 8) && 0xFF;
        if (OW_w_trim(t, PN(owq)))
			return -EINVAL;
		return 0;
	default:
		return -EINVAL;
	}
}

/* Are the trim values valid-looking? */
static int FS_r_trimvalid(struct one_wire_query * owq)
{
	BYTE trim[2];
    switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
        if (OW_r_trim(trim, PN(owq)))
			return -EINVAL;
        OWQ_Y(owq) = (((trim[0] & 0x07) == 0x05) || ((trim[0] & 0x07) == 0x03))
			&& (trim[1] == 0xBB);
		break;
	default:
        OWQ_Y(owq) = 1;				/* Assume true */
	}
	return 0;
}

/* Put in a black trim value if non-zero */
static int FS_r_blanket(struct one_wire_query * owq)
{
	BYTE trim[2];
	BYTE blanket[] = { 0x9D, 0xBB };
    switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
        if (OW_r_trim(trim, PN(owq)))
			return -EINVAL;
        OWQ_Y(owq) = (memcmp(trim, blanket, 2) == 0);
		return 0;
	default:
		return -EINVAL;
	}
}

/* Put in a black trim value if non-zero */
static int FS_w_blanket(struct one_wire_query * owq)
{
	BYTE blanket[] = { 0x9D, 0xBB };
    switch (OW_die(PN(owq))) {
	case eB7:
	case eC2:
        if (OWQ_Y(owq)) {
            if (OW_w_trim(blanket, PN(owq)))
				return -EINVAL;
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
		{convert, NULL, 1, trxn_match},
		TRXN_END,
	};

	if (OW_power(&pow, pn))
		pow = 0x00;				/* assume unpowered if cannot tell */

	/* Select particular device and start conversion */
	if (!pow) {					// unpowered, deliver power, no communication allowed
		if (BUS_transaction(tunpowered, pn))
			return 1;
	} else if (Simul_Test(simul_temp, pn)) {	// powered
		int ret;
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		if (ret)
			return ret;
	}

	if (OW_r_scratchpad(data, pn))
		return 1;

#if 0
	/* Check for error condition */
	if (data[0] == 0xAA && data[1] == 0x00 && data[6] == 0x0C) {
		/* repeat the conversion (only once) */
		/* Do it the most conservative way -- unpowered */
		if (!pow) {				// unpowered, deliver power, no communication allowed
			if (BUS_transaction(tunpowered, pn))
				return 1;
		} else {				// powered, so release bus immediately after issuing convert
			if (BUS_transaction(tpowered, pn))
				return 1;
			UT_delay(delay);
		}
		if (OW_r_scratchpad(data, pn))
			return 1;
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
	BYTE b4[] = { _1W_READ_POWERMODE, };
	struct transaction_log tpower[] = {
		TRXN_START,
		{b4, NULL, 1, trxn_match},
		{NULL, data, 1, trxn_read},
		TRXN_END,
	};
	//printf("POWER "SNformat", before check\n",SNvar(pn->sn)) ;
	if (IsUncachedDir(pn)
		|| Cache_Get_Internal_Strict(data, sizeof(BYTE), &ip_power, pn)) {
		//printf("POWER "SNformat", need to ask\n",SNvar(pn->sn)) ;
		if (BUS_transaction(tpower, pn))
			return 1;
		//printf("POWER "SNformat", asked\n",SNvar(pn->sn)) ;
		Cache_Add_Internal(data, sizeof(BYTE), &ip_power, pn);
		//printf("POWER "SNformat", cached\n",SNvar(pn->sn)) ;
	}
	//printf("POWER "SNformat", done\n",SNvar(pn->sn)) ;
	return 0;
}

static int OW_22temp(_FLOAT * temp, const int resolution,
					 const struct parsedname *pn)
{
	BYTE data[9];
	BYTE convert[] = { _1W_CONVERT_T, };
	BYTE pow;
	int res = Resolutions[resolution - 9].config;
	UINT delay = Resolutions[resolution - 9].delay;
	BYTE mask = Resolutions[resolution - 9].mask;
	int oldres;
	struct transaction_log tunpowered[] = {
		TRXN_START,
		{convert, convert, delay, trxn_power},
		TRXN_END,
	};
	struct transaction_log tpowered[] = {
		TRXN_START,
		{convert, NULL, 1, trxn_match},
		TRXN_END,
	};
	//LEVEL_DATA("OW_22temp\n");
	/* powered? */
	if (OW_power(&pow, pn))
		pow = 0x00;				/* assume unpowered if cannot tell */

	/* Resolution */
	if (Cache_Get_Internal_Strict
		(&oldres, sizeof(oldres), &ip_resolution, pn)
		|| oldres != resolution) {
		/* Get existing settings */
		if (OW_r_scratchpad(data, pn))
			return 1;
		/* Put in new settings */
		if ((data[4] | 0x0F) != res) {	// ignore lower nibble
			data[4] = (res & 0xF0) | (data[4] & 0x0F);
			if (OW_w_scratchpad(&data[2], pn))
				return 1;
			Cache_Add_Internal(&resolution, sizeof(int), &ip_resolution,
							   pn);
		}
	}

	/* Conversion */
	if (!pow) {					// unpowered, deliver power, no communication allowed
		LEVEL_DEBUG("Unpowered temperature conversion -- %d msec\n",
					delay);
		if (BUS_transaction(tunpowered, pn))
			return 1;
	} else if (Simul_Test(simul_temp, pn)) {	// powered, so release bus immediately after issuing convert
		int ret;
		LEVEL_DEBUG("Powered temperature conversion\n");
		BUSLOCK(pn);
		ret = BUS_transaction_nolock(tpowered, pn) || FS_poll_convert(pn);
		BUSUNLOCK(pn);
		if (ret)
			return ret;
	} else {
		LEVEL_DEBUG("Simultaneous temperature conversion\n");
	}
	if (OW_r_scratchpad(data, pn))
		return 1;
	//printf("Temperature Got bytes %.2X %.2X\n",data[0],data[1]) ;

	//*temp = .0625*(((char)data[1])<<8|data[0]) ;
	// Torsten Godau <tg@solarlabs.de> found a problem with 9-bit resolution
	temp[0] =
		(_FLOAT) ((int16_t) ((data[1] << 8) | (data[0] & mask))) * .0625;
	return 0;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit(_FLOAT * T, const int Tindex,
						  const struct parsedname *pn)
{
	BYTE data[9];
	BYTE recall[] = { _1W_READ_POWERMODE, };
	struct transaction_log trecall[] = {
		TRXN_START,
		{recall, NULL, 1, trxn_match},
		TRXN_END,
	};

	if (BUS_transaction(trecall, pn))
		return 1;

	UT_delay(10);

	if (OW_r_scratchpad(data, pn))
		return 1;
	T[0] = (_FLOAT) ((int8_t) data[2 + Tindex]);
	return 0;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit(const _FLOAT T, const int Tindex,
						  const struct parsedname *pn)
{
	BYTE data[9];

	if (OW_r_scratchpad(data, pn))
		return 1;
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
		{be, NULL, 1, trxn_match},
		{NULL, data, 9, trxn_read},
		{data, NULL, 9, trxn_crc8,},
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
		{d, NULL, 4, trxn_match},
		TRXN_END,
	};
	struct transaction_log tpower[] = {
		TRXN_START,
		{pow, pow, 10, trxn_power},
		TRXN_END,
	};

	/* different processing for DS18S20 and both DS19B20 and DS1822 */
	if (pn->sn[0] == 0x10)
		twrite->size = 4;

	if (BUS_transaction(twrite, pn))
		return 1;

	return BUS_transaction(tpower, pn);
}

/* Trim values -- undocumented except in AN247.pdf */
static int OW_r_trim(BYTE * trim, const struct parsedname *pn)
{
	BYTE cmd0[] = { 0x93, };
	BYTE cmd1[] = { 0x68, };
	struct transaction_log t0[] = {
		TRXN_START,
		{cmd0, NULL, 1, trxn_match},
		{NULL, &trim[0], 1, trxn_read},
		TRXN_END,
	};
	struct transaction_log t1[] = {
		TRXN_START,
		{cmd1, NULL, 1, trxn_match},
		{NULL, &trim[1], 1, trxn_read},
		TRXN_END,
	};

	if (BUS_transaction(t0, pn))
		return 1;

	return BUS_transaction(t1, pn);
}

static int OW_w_trim(const BYTE * trim, const struct parsedname *pn)
{
	BYTE cmd0[] = { 0x95, trim[0], };
	BYTE cmd1[] = { 0x63, trim[1], };
	BYTE cmd2[] = { 0x94, };
	BYTE cmd3[] = { 0x64, };
	struct transaction_log tt[] = {
		TRXN_START,
		{cmd0, NULL, 2, trxn_match},
		TRXN_END,
	};
	struct transaction_log t[] = {
		TRXN_START,
		{cmd2, NULL, 1, trxn_match},
		TRXN_END,
	};

	if (BUS_transaction(tt, pn))
		return 1;
	tt[1].out = cmd1;
	if (BUS_transaction(tt, pn))
		return 1;
	if (BUS_transaction(t, pn))
		return 1;

	t[1].out = cmd3;
	return BUS_transaction(t, pn);
}

static enum eDie OW_die(const struct parsedname *pn)
{
	BYTE die[6] =
		{ pn->sn[6], pn->sn[5], pn->sn[4], pn->sn[3], pn->sn[2], pn->sn[1],
	};
	// data gives index into die matrix
	if (memcmp(die, DIE[pn->ft->data.i].C2, 6) > 0)
		return eC2;
	if (memcmp(die, DIE[pn->ft->data.i].B7, 6) > 0)
		return eB7;
	return eB6;
}

/* Powered temperature measurements -- need to poll line since it is held low during measurement */
/* We check every 50 msec (arbitrary) up to 1.25 seconds */
int FS_poll_convert(const struct parsedname *pn)
{
	int i;
	BYTE p[1];
	struct transaction_log t[] = {
		{NULL, NULL, 50, trxn_delay,},
		{NULL, p, 1, trxn_read,},
		TRXN_END,
	};

	// the first test is faster for just DS2438 (10 msec)
	// subsequent polling is slower since the DS18x20 is a slower converter
	for (i = 0; i < 22; ++i) {
		if (BUS_transaction_nolock(t, pn)) {
			LEVEL_DEBUG("FS_poll_convert: BUS_transaction failed\n");
			break;
		}
		if (p[0] != 0) {
			LEVEL_DEBUG
				("FS_poll_convert: BUS_transaction done after %dms\n",
				 (i + 1) * 50);
			return 0;
		}
		t[0].size = 50;			// 50 msec for rest of delays
	}
	LEVEL_DEBUG("FS_poll_convert: failed\n");
	return 1;
}

/* read PIO pins for the DS28EA00 and rearrange slightly */
static int OW_read_pio( BYTE * pio, BYTE * latch, const struct parsedname * pn)
{
  BYTE data[1] ;
  BYTE cmd[] = { _1W_PIO_ACCESS_READ, } ;
  struct transaction_log t[] = {
    TRXN_START,
    { cmd, NULL, 1, trxn_match, } ,
    { NULL, data, 1, trxn_read, } ,
    TRXN_END ,
  } ;
  if ( BUS_transaction(t,pn) ) return 1 ;
  /* compare lower and upper nibble to be complements */
  if ( (data[0]&0x0F)^(data[0]>>4) ) return 1 ;
  switch ( data[0]&0x0F ) {
    case 0x0: pio[0] = 0 ; latch[0] = 0; break ;
    case 0x1: pio[0] = 1 ; latch[0] = 0; break ;
    case 0x2: pio[0] = 0 ; latch[0] = 1; break ;
    case 0x3: pio[0] = 1 ; latch[0] = 1; break ;
    case 0x4: pio[0] = 2 ; latch[0] = 0; break ;
    case 0x5: pio[0] = 3 ; latch[0] = 0; break ;
    case 0x6: pio[0] = 2 ; latch[0] = 1; break ;
    case 0x7: pio[0] = 3 ; latch[0] = 1; break ;
    case 0x8: pio[0] = 0 ; latch[0] = 2; break ;
    case 0x9: pio[0] = 1 ; latch[0] = 2; break ;
    case 0xA: pio[0] = 0 ; latch[0] = 3; break ;
    case 0xB: pio[0] = 1 ; latch[0] = 3; break ;
    case 0xC: pio[0] = 2 ; latch[0] = 2; break ;
    case 0xD: pio[0] = 3 ; latch[0] = 2; break ;
    case 0xE: pio[0] = 2 ; latch[0] = 3; break ;
    case 0xF: pio[0] = 3 ; latch[0] = 3; break ;
  }
  return 0 ;
}

static int OW_w_pio( BYTE pio, const struct parsedname * pn )
{
  BYTE cmd[] = { _1W_PIO_ACCESS_WRITE, pio, } ;
  struct transaction_log t[] = {
    TRXN_START,
    { cmd, NULL, 2, trxn_match, } ,
    TRXN_END ,
  } ;
  return BUS_transaction(t,pn) ;
}



