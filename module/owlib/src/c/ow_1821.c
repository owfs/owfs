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


#include <config.h>
#include "owfs_config.h"
#include "ow_1821.h"
#include <math.h>

/* ------- Prototypes ----------- */
/* DS18S20&2 Temperature */
// READ_FUNCTION( FS_tempdata ) ;
READ_FUNCTION(FS_temperature);
READ_FUNCTION(FS_r_polarity);
WRITE_FUNCTION(FS_w_polarity);
READ_FUNCTION(FS_r_templimit);
WRITE_FUNCTION(FS_w_templimit);
READ_FUNCTION(FS_r_thermomode);
WRITE_FUNCTION(FS_w_thermomode);
READ_FUNCTION(FS_r_oneshot);
WRITE_FUNCTION(FS_w_oneshot);
READ_FUNCTION(FS_r_tlf);
WRITE_FUNCTION(FS_w_tlf);
READ_FUNCTION(FS_r_thf);
WRITE_FUNCTION(FS_w_thf);

/* -------- Structures ---------- */
struct filetype DS1821[] = {
	F_type,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temperature, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"polarity", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_polarity, FS_w_polarity, VISIBLE, NO_FILETYPE_DATA,},
	{"templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:1},},
	{"temphigh", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_templimit, FS_w_templimit, VISIBLE, {i:0},},
	{"templowflag", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_tlf, FS_w_tlf, VISIBLE, NO_FILETYPE_DATA,},
	{"temphighflag", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_thf, FS_w_thf, VISIBLE, NO_FILETYPE_DATA,},
	{"thermostatmode", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_thermomode, FS_w_thermomode, VISIBLE, NO_FILETYPE_DATA,},
	{"1shot", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_oneshot, FS_w_oneshot, VISIBLE, NO_FILETYPE_DATA,}
}

;
DeviceEntry(thermostat, DS1821, NO_GENERIC_READ, NO_GENERIC_WRITE);

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

	/* Definition of status register bits. */
	/* Please see the DS1821 datasheet for further information. */
#define DS1821_STATUS_1SHOT 0	// 0: convert temp continuously; 1: one conversion only.
#define DS1821_STATUS_POL 1		// 0: in thermostat mode DQ output is active low; 1: active high.
#define DS1821_STATUS_TR 2		// 0: go 1-wire mode on next power-up; 1: go into thermostat mode.
#define DS1821_STATUS_TLF 3		// 0: temp has not been below the TL temp; 1: temp has.
#define DS1821_STATUS_THF 4		// 0: temp has not been above the TH temp; 1: temp has.
#define DS1821_STATUS_NVB 5		// 0: EEPROM write not in progress; 1: EEPROM write in progress.
					// Bit 6 always returns 1.
#define DS1821_STATUS_DONE 7	// 0: temp conversion in progress; 1: conversion finished.
	/* End of status register bits. */


/* ------- Functions ------------ */

/* DS1821*/
static GOOD_OR_BAD OW_temperature(_FLOAT * temp, const struct parsedname *pn);
static GOOD_OR_BAD OW_current_temperature(_FLOAT * temp, const struct parsedname *pn);
static GOOD_OR_BAD OW_current_temperature_lowres(_FLOAT * temp, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_status(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_status(BYTE * data, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_templimit(_FLOAT * T, const int Tindex, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_templimit(const _FLOAT T, const int Tindex, const struct parsedname *pn);

static ZERO_OR_ERROR FS_temperature(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_temperature(&OWQ_F(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_thf(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	OWQ_Y(owq) = (data >> DS1821_STATUS_THF) & 0x01;
//    OWQ_Y(owq) = (data & 0x10) >> 4;
	return 0;
}

static ZERO_OR_ERROR FS_w_thf(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	UT_setbit(&data, DS1821_STATUS_THF, OWQ_Y(owq));
	return GB_to_Z_OR_E(OW_w_status(&data, PN(owq))) ;
}


static ZERO_OR_ERROR FS_r_tlf(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	OWQ_Y(owq) = (data >> DS1821_STATUS_TLF) & 0x01;
//OWQ_Y(owq) = (data & 0x08) >> 3;
	return 0;
}

static ZERO_OR_ERROR FS_w_tlf(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	UT_setbit(&data, DS1821_STATUS_TLF, OWQ_Y(owq));
	return GB_to_Z_OR_E(OW_w_status(&data, PN(owq))) ;
}


static ZERO_OR_ERROR FS_r_thermomode(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	OWQ_Y(owq) = (data >> DS1821_STATUS_TR) & 0x01;
//    OWQ_Y(owq) = (data & 0x04) >> 2;
	return 0;
}

static ZERO_OR_ERROR FS_w_thermomode(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	UT_setbit(&data, DS1821_STATUS_TR, OWQ_Y(owq));
	return GB_to_Z_OR_E(OW_w_status(&data, PN(owq))) ;
}


static ZERO_OR_ERROR FS_r_polarity(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	OWQ_Y(owq) = (data >> DS1821_STATUS_POL) & 0x01;
	return 0;
}

static ZERO_OR_ERROR FS_w_polarity(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	UT_setbit(&data, DS1821_STATUS_POL, OWQ_Y(owq));
	return GB_to_Z_OR_E(OW_w_status(&data, PN(owq))) ;
}


static ZERO_OR_ERROR FS_r_oneshot(struct one_wire_query *owq)
{
	BYTE data;

	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;
	OWQ_Y(owq) = (data >> DS1821_STATUS_1SHOT) & 0x01;

	return 0;
}

/* Set or reset the "one shot" bit of the status register.
 *
 * One of several idiosyncrasies of this chip is it's "Continuous Conversion/1Shot" mode.
 * If the 1Shot bit is zero at the time a temperature conversion is finished then the chip starts another
 * conversion immediately without needing a Start Conversion command. It continues to do this until
 * it hears a Stop Conversion command. This means that the chip doesn't do anything when the 1Shot
 * bit is cleared - it waits until the next Start Conversion command before starting continuous conversions.
 * I can't see how this waiting behaviour is useful so I have issued Start Conversion and Stop Conversion
 * commands at the appropriate times on the assumption that the reason you are setting the continuous conversion
 * mode is because you want continuous conversions.
 */
static ZERO_OR_ERROR FS_w_oneshot(struct one_wire_query *owq)
{
	BYTE pstop[] = { _1W_STOP_CONVERT_T, };
	struct transaction_log tstop[] = {
		TRXN_START,
		TRXN_WRITE1(pstop),
		TRXN_END,
	};

	BYTE pstart[] = { _1W_START_CONVERT_T, };
	struct transaction_log tstart[] = {
		TRXN_START,
		TRXN_WRITE1(pstart),
		TRXN_END,
	};

	BYTE data;
	int oneshotmode;
	
	RETURN_ERROR_IF_BAD(OW_r_status(&data, PN(owq))) ;

	oneshotmode = (data >> DS1821_STATUS_1SHOT) & 0x01;

	UT_setbit(&data, DS1821_STATUS_1SHOT, OWQ_Y(owq));
	RETURN_ERROR_IF_BAD(OW_w_status(&data, PN(owq))) ;

	if (!oneshotmode && OWQ_Y(owq)) {
		/* 1Shot mode was off and we are setting it on; i.e. we are ending continuous mode.
		 * Continuous conversions may be in progress.
		 * Since we are turning this mode off, it's probably a reasonable assumption
		 * that we don't want continuous conversions anymore, 
		 * so issue a STOP CONVERSION to halt the conversions.
		 */
		if ( BUS_transaction(tstop, PN(owq)) ) {
			return -EINVAL;
		}
	} else if (oneshotmode && !OWQ_Y(owq)) {
		/* 1Shot mode was on and we are turning it off; i.e. we are starting continuous mode.
		 * Continuous conversions are probably not in progress.
		 * Presumably, we are doing this because we want continuous conversions so
		 * issue a START CONVERSION command to kick things off.
		 */
		GOOD_OR_BAD ret = BUS_transaction(tstart, PN(owq));

		/* We have just kicked off a conversion. If we try to read the temperature
		 * right now we will get some previous temperature, which we are not probably
		 * expecting.  So wait for the first conversion only, just in case. We will now
		 * be in continuous conversion mode so after the first one we no longer have to 
		 * worry about it.
		 */
		UT_delay(1000);
		if ( BAD(ret) ) {
			return -EINVAL;
		}
	}
	// else ;   /* We are setting the bit to what it already was so do nothing special. */

	return 0;
}


static ZERO_OR_ERROR FS_r_templimit(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_templimit(&OWQ_F(owq), OWQ_pn(owq).selected_filetype->data.i, PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_templimit(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_templimit(OWQ_F(owq), OWQ_pn(owq).selected_filetype->data.i, PN(owq))) ;
}

static GOOD_OR_BAD OW_r_status(BYTE * data, const struct parsedname *pn)
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

static GOOD_OR_BAD OW_w_status(BYTE * data, const struct parsedname *pn)
{
	BYTE p[] = { _1W_WRITE_STATUS, data[0] };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

/* Do a transaction to get the temperature, increasing the resolution if possible.
 *
 * When we are in continuous conversion mode we don't have to wait for a conversion to finish
 * because, apart from the first one, one has always already been completed and the temperature stored in the
 * temperature register.  
 *
 * However, a tricky wrinkle is that you cannot use the "high resolution" temperature calculation in continuous
 * conversion mode because the counter is constantly in use and reading it disrupts the temperature measurement
 * in progress. The result is incorrect temperature readings in the temperature register even if we don't attempt
 * to use the counter value to increase resolution.
 */
static GOOD_OR_BAD OW_temperature(_FLOAT * temp, const struct parsedname *pn)
{
	BYTE c[] = { _1W_START_CONVERT_T, };
	BYTE status;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(c),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(OW_r_status(&status, pn)) ;

	if ((status >> DS1821_STATUS_1SHOT) & 0x01) {	/* 1-shot, convert and wait 1 second */
		RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
		UT_delay(1000);
		return OW_current_temperature(temp, pn);
	} else {
		/* in continuous conversion mode. No need to delay but can't get full resolution. */
		return OW_current_temperature_lowres(temp, pn);
	}
}

/* Do a transaction to get the temperature, the remaining count, and the count-per-c registers.
 * Perform the calculation to increase the temperature resolution.
 */
static GOOD_OR_BAD OW_current_temperature(_FLOAT * temp, const struct parsedname *pn)
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

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	if (count_per_c) {
		/* Perform calculation to get high temperature resolution - see datasheet. */
		temp[0] = (_FLOAT) ((int8_t) temp_read) + .5 - ((_FLOAT) count_remain) / ((_FLOAT) count_per_c);
	} else {					/* Bad count_per_c -- use lower resolution */
		temp[0] = (_FLOAT) ((int8_t) temp_read);
	}

	return gbGOOD;
}

/* Read temperature register but do not attempt to increase the resolution,
 * because, for instance, the chip is in continuous conversion mode.
 */
static GOOD_OR_BAD OW_current_temperature_lowres(_FLOAT * temp, const struct parsedname *pn)
{
	BYTE read_temp[] = { _1W_READ_TEMPERATURE, };
	BYTE temp_read;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(read_temp),
		TRXN_READ1(&temp_read),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	temp[0] = (_FLOAT) ((int8_t) temp_read);

	return gbGOOD;
}

/* Limits Tindex=0 high 1=low */
static GOOD_OR_BAD OW_r_templimit(_FLOAT * T, const int Tindex, const struct parsedname *pn)
{
	BYTE p[] = { _1W_READ_TH, _1W_READ_TL, };
	BYTE data;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[Tindex]),
		TRXN_READ1(&data),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	T[0] = (_FLOAT) ((int8_t) data);
	return gbGOOD;
}

/* Limits Tindex=0 high 1=low */
static GOOD_OR_BAD OW_w_templimit(const _FLOAT T, const int Tindex, const struct parsedname *pn)
{
	BYTE p[] = { _1W_WRITE_TH, _1W_WRITE_TL, };
#ifdef HAVE_LRINT
	BYTE data = BYTE_MASK(lrint(T));	// round off
#else
	BYTE data = BYTE_MASK((int) (T + .49));	// round off
#endif
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&p[Tindex]),
		TRXN_WRITE1(&data),
		TRXN_END,
	};
	return BUS_transaction(t, pn);;
}
