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
#include "ow_iblss.h"

/* ------- Prototypes ----------- */

/* iButtonLink SmartSlave */
READ_FUNCTION(FS_TH_humidity);
READ_FUNCTION(FS_TH_latesthumidity);
READ_FUNCTION(FS_TH_temperature);
READ_FUNCTION(FS_TH_latesttemp);
READ_FUNCTION(FS_TH_version);
WRITE_FUNCTION(FS_TH_w_led);


/* ------- Structures ----------- */
static struct filetype IBLSS[] = {
	F_STANDARD,
	{"TH",                PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir,      fc_subdir,   NO_READ_FUNCTION,     NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TH/humidity",       PROPERTY_LENGTH_FLOAT,  NON_AGGREGATE, ft_float,       fc_volatile, FS_TH_humidity,       NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TH/latesthumidity", PROPERTY_LENGTH_FLOAT,  NON_AGGREGATE, ft_float,       fc_volatile, FS_TH_latesthumidity, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TH/temperature",    PROPERTY_LENGTH_TEMP,   NON_AGGREGATE, ft_temperature, fc_volatile, FS_TH_temperature,    NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TH/latesttemp",     PROPERTY_LENGTH_TEMP,   NON_AGGREGATE, ft_temperature, fc_volatile, FS_TH_latesttemp,     NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TH/firmware",       5,                      NON_AGGREGATE, ft_ascii,       fc_stable,   FS_TH_version,        NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"TH/led",            PROPERTY_LENGTH_YESNO,  NON_AGGREGATE, ft_yesno,       fc_stable,   NO_READ_FUNCTION,     FS_TH_w_led,       VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(FE, IBLSS, DEV_temp, NO_GENERIC_READ, NO_GENERIC_WRITE);


/* Internal properties */
#define _1W_IBL_CONVERT 0xB4
#define _1W_IBL_READ_MEMORY 0xF0
#define _1W_IBL_SET_LED 0xA5

#define _IBL_TH 0x04
#define _IBL_PAGE_SIZE 32


/* ------- Functions ------------ */

/* iButtonLink SmartSlave */
static GOOD_OR_BAD OW_r_mem(BYTE * p, struct parsedname *pn);
static GOOD_OR_BAD OW_convert(struct parsedname *pn);
static GOOD_OR_BAD OW_w_LED(int state, struct parsedname *pn);


/* read humidty value from conversion result. */
static ZERO_OR_ERROR FS_TH_latesthumidity(struct one_wire_query *owq)
{
	BYTE p[_IBL_PAGE_SIZE+2];

	/* Read memory page from device. */
	RETURN_ERROR_IF_BAD( OW_r_mem(p, PN(owq)) ) ;

	/* Fail if this isn't a temperature/humidity SmartSlave. */
	if (p[0] != _IBL_TH) {
		return -ENOENT ;
	}

	/* Fail if no conversion was triggered since powerup. */
	if (p[8] == 0) {
		return -EINVAL ;
	}

	/* Fail if conversion isn't completed yet. */
	if (p[2] != 0 || p[3] != 0) {
		return -EAGAIN ;
	}

	/* Calculate humidity in % from raw data. */
	OWQ_F(owq) = ((float)((int16_t) (p[6] << 8 | p[7]))) / 128;

	/* Success. */
	return 0;
}


/* convert and read humidty value. */
static ZERO_OR_ERROR FS_TH_humidity(struct one_wire_query *owq)
{
	/* Start a conversion, wait. */
	RETURN_ERROR_IF_BAD( OW_convert(PN(owq)) ) ;

	/* Return humidity from conversion result. */
	return FS_TH_latesthumidity(owq);
}


/* read temperature value from conversion result. */
static ZERO_OR_ERROR FS_TH_latesttemp(struct one_wire_query *owq)
{
	BYTE p[_IBL_PAGE_SIZE+2];

	/* Read memory page from device. */
	RETURN_ERROR_IF_BAD( OW_r_mem(p, PN(owq)) ) ;

	/* Fail if this isn't a temperature/humidity SmartSlave. */
	if (p[0] != _IBL_TH) {
		return -ENOENT ;
	}

	/* Fail if no conversion was triggered since powerup. */
	if (p[8] == 0) {
		return -EINVAL ;
	}

	/* Fail if conversion isn't completed yet. */
	if (p[2] != 0 || p[3] != 0) {
		return -EAGAIN ;
	}

	/* Calculate temperature in °C from raw data. */
	OWQ_F(owq) = ((float)((int16_t) (p[4] << 8 | p[5]))) / 128;

	/* Success. */
	return 0;
}

/* read firmware version from memory page. */
static ZERO_OR_ERROR FS_TH_version(struct one_wire_query *owq)
{
	BYTE p[_IBL_PAGE_SIZE+2];
	ASCII v[6];

	/* Read memory page from device. */
	RETURN_ERROR_IF_BAD( OW_r_mem(p, PN(owq)) ) ;

	/* Fail if this isn't a temperature/humidity SmartSlave. */
	if (p[0] != _IBL_TH) {
		return -ENOENT ;
	}

	/* Calculate firmware version from raw data. */
	snprintf(v, sizeof(v), "%d.%d", (p[1] >> 4), (p[1] & 0xf));
	return OWQ_format_output_offset_and_size((ASCII *) v, strlen((ASCII *) v), owq);

}


/* convert and read temperature value. */
static ZERO_OR_ERROR FS_TH_temperature(struct one_wire_query *owq)
{
	/* Start a conversion, wait. */
	RETURN_ERROR_IF_BAD( OW_convert(PN(owq)) ) ;

	/* Return temperature from conversion result. */
	return FS_TH_latesttemp(owq);
}

/* iButtonLink SmartSlave LED */
static ZERO_OR_ERROR FS_TH_w_led(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_w_LED( OWQ_Y(owq), PN(owq)) ) ;
}


/* read result from SmartSlave */
static GOOD_OR_BAD OW_r_mem(BYTE * p, struct parsedname *pn)
{
	BYTE rmem[] = { _1W_IBL_READ_MEMORY, 0 };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(rmem),
		TRXN_READ(p, _IBL_PAGE_SIZE+2),
		TRXN_END,
	};

	/* Read slave memory. */
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn));

	/* Fail if crc16 failed. */
	return (CRC16(p, _IBL_PAGE_SIZE+2) == -1)?gbGOOD:gbBAD;
}


/* send A/D conversion command */
static GOOD_OR_BAD OW_convert(struct parsedname *pn)
{
	BYTE convert[] = { _1W_IBL_CONVERT, };
	struct transaction_log tconv[] = {
		TRXN_START,
		TRXN_WRITE1(convert),
		TRXN_DELAY(1000),
		TRXN_END,
	};

	/* Start conversion. */
	RETURN_BAD_IF_BAD(BUS_transaction(tconv, pn));
	return gbGOOD;
}


/* turn LED on/off */
static GOOD_OR_BAD OW_w_LED( int state, struct parsedname *pn)
{
	BYTE led[] = { _1W_IBL_SET_LED, 0, state };
	struct transaction_log tconv[] = {
		TRXN_START,
		TRXN_WRITE3(led),
		TRXN_END,
	};

	/* Set LED state. */
	return BUS_transaction(tconv, pn);
}
