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
READ_FUNCTION(FS_temperature);
READ_FUNCTION(FS_r_polarity);
WRITE_FUNCTION(FS_w_polarity);
READ_FUNCTION(FS_r_templimit);
WRITE_FUNCTION(FS_w_templimit);

/* -------- Structures ---------- */
struct filetype DS1821[] = {
	F_type,
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,   FS_temperature, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"polarity",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   FS_r_polarity, FS_w_polarity, {v:NULL},} ,
  {"templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable,   FS_r_templimit, FS_w_templimit, {i:1},} ,
  {"temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_stable,   FS_r_templimit, FS_w_templimit, {i:0},} ,
}

;
DeviceEntry(thermostat, DS1821);

#define _1W_READ_TEMPERATURE 0xAA
#define _1W_START_CONVERT_T 0xEE
#define _1W_STOP_CONVERT_T 0x22
#define _1W_WRITE_TH 0x01
#define _1W_WRITE_TL 0x02
#define _1W_READ_TH 0xA1
#define _1W_READ_TL 0xA2
#define _1W_WRITE_STATUS 0x0C
#define _1W_READ_STATUS 0xAC
#define _1W_READ_COUNTER 0xA0
#define _1W_LOAD_COUNTER 0x41

/* ------- Functions ------------ */

/* DS1821*/
static int OW_temperature(_FLOAT * temp, const struct parsedname *pn);
static int OW_current_temperature(_FLOAT * temp,
								  const struct parsedname *pn);
static int OW_r_status(BYTE * data, const struct parsedname *pn);
static int OW_w_status(BYTE * data, const struct parsedname *pn);
static int OW_r_templimit(_FLOAT * T, const int Tindex,
						  const struct parsedname *pn);
static int OW_w_templimit(const _FLOAT T, const int Tindex,
						  const struct parsedname *pn);

/* Internal properties */
//static struct internal_prop ip_continuous = { "CON", fc_stable };
MakeInternalProp(CON,fc_stable) ; // continuous conversions

static int FS_temperature(struct one_wire_query * owq)
{
    if (OW_temperature(&OWQ_F(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_polarity(struct one_wire_query * owq)
{
	BYTE data;

    if (OW_r_status(&data, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = (data & 0x02) >> 1;
	return 0;
}

static int FS_w_polarity(struct one_wire_query * owq)
{
	BYTE data;

    if (OW_r_status(&data, PN(owq)))
		return -EINVAL;
    UT_setbit(&data, 1, OWQ_Y(owq));
    if (OW_w_status(&data, PN(owq)))
		return -EINVAL;
	return 0;
}


static int FS_r_templimit(struct one_wire_query * owq)
{
    if (OW_r_templimit(&OWQ_F(owq), OWQ_pn(owq).selected_filetype->data.i, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_templimit(struct one_wire_query * owq)
{
    if (OW_w_templimit(OWQ_F(owq), OWQ_pn(owq).selected_filetype->data.i, PN(owq)))
		return -EINVAL;
	return 0;
}

static int OW_r_status(BYTE * data, const struct parsedname *pn)
{
    BYTE p[] = { _1W_READ_STATUS, };
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE1(p),
        TRXN_READ1(data),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_w_status(BYTE * data, const struct parsedname *pn)
{
    BYTE p[] = { _1W_WRITE_STATUS, data[0] };
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE2(p),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_temperature(_FLOAT * temp, const struct parsedname *pn)
{
    BYTE c[] = { _1W_START_CONVERT_T, };
	BYTE status;
	int continuous;
	int need_to_trigger = 0;
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE1(c),
		TRXN_END,
	};

	if (OW_r_status(&status, pn))
		return 1;

	if (status & 0x01) {		/* 1-shot, convert and wait 1 second */
		need_to_trigger = 1;
	} else {					/* continuous conversion mode */
		if (Cache_Get_Internal_Strict
			(&continuous, sizeof(continuous), InternalProp(CON), pn)
			|| continuous == 0) {
			need_to_trigger = 1;
		}
	}

	if (need_to_trigger) {
		if (BUS_transaction(t, pn))
			return 1;
		UT_delay(1000);
		if ((status & 0x01) == 0) {	/* continuous mode, just triggered */
			continuous = 1;
			Cache_Add_Internal(&continuous, sizeof(int), InternalProp(CON), pn);	/* Flag as continuous */
		}
	}
	return OW_current_temperature(temp, pn);
}

static int OW_current_temperature(_FLOAT * temp,
								  const struct parsedname *pn)
{
    BYTE read_temp[] = { _1W_READ_TEMPERATURE, };
    BYTE read_counter[] = { _1W_READ_COUNTER, };
    BYTE load_counter[] = { _1W_LOAD_COUNTER, };
	BYTE temp_read;
	BYTE count_per_c;
	BYTE count_remain;
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE1(read_temp),
        TRXN_READ1(&temp_read),
		TRXN_START,
        TRXN_WRITE1(read_counter),
        TRXN_READ1(&count_remain),
		TRXN_START,
        TRXN_WRITE1(load_counter),
		TRXN_START,
        TRXN_WRITE1(read_counter),
        TRXN_READ1(&count_per_c),
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;
	if (count_per_c) {
		temp[0] =
			(_FLOAT) ((int8_t) temp_read) + .5 -
			((_FLOAT) count_remain) / ((_FLOAT) count_per_c);
	} else {					/* Bad count_per_c -- use lower resolution */
		temp[0] = (_FLOAT) ((int8_t) temp_read);
	}
	return 0;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit(_FLOAT * T, const int Tindex,
						  const struct parsedname *pn)
{
    BYTE p[] = { _1W_READ_TH, _1W_READ_TL, };
	BYTE data;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[Tindex]),
		TRXN_READ1(&data),
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;
	T[0] = (_FLOAT) ((int8_t) data);
	return 0;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit(const _FLOAT T, const int Tindex,
						  const struct parsedname *pn)
{
    BYTE p[] = { _1W_WRITE_TH, _1W_WRITE_TL, };
	BYTE data = BYTE_MASK((int) (T + .49)) ;	// round off
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[Tindex]),
		TRXN_WRITE1(&data),
		TRXN_END,
	};
	return BUS_transaction(t, pn);;
}
