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
#include "ow_2438.h"

/* ------- Prototypes ----------- */

/* DS2438 Battery */
bREAD_FUNCTION(FS_r_page);
bWRITE_FUNCTION(FS_w_page);
fREAD_FUNCTION(FS_temp);
fREAD_FUNCTION(FS_volts);
fREAD_FUNCTION(FS_Humid);
fREAD_FUNCTION(FS_Humid_1735);
fREAD_FUNCTION(FS_Humid_4000);
fREAD_FUNCTION(FS_Current);
uREAD_FUNCTION(FS_r_Ienable);
uWRITE_FUNCTION(FS_w_Ienable);
iREAD_FUNCTION(FS_r_Offset);
iWRITE_FUNCTION(FS_w_Offset);
uREAD_FUNCTION(FS_r_counter);
uWRITE_FUNCTION(FS_w_counter);
dREAD_FUNCTION(FS_r_date);
dWRITE_FUNCTION(FS_w_date);
aREAD_FUNCTION(FS_MStype);

/* ------- Structures ----------- */

struct aggregate A2437 = { 8, ag_numbers, ag_separate, };
struct filetype DS2437[] = {
	F_STANDARD,
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 8, &A2437, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
  {"VDD", 12, NULL, ft_float, fc_volatile, {f: FS_volts}, {v: NULL}, {i:1},},
  {"VAD", 12, NULL, ft_float, fc_volatile, {f: FS_volts}, {v: NULL}, {i:0},},
  {"temperature", 12, NULL, ft_temperature, fc_volatile, {f: FS_temp}, {v: NULL}, {v:NULL},},
  {"vis", 12, NULL, ft_float, fc_volatile, {f: FS_Current}, {v: NULL}, {v:NULL},},
  {"Ienable", 12, NULL, ft_unsigned, fc_stable, {u: FS_r_Ienable}, {u: FS_w_Ienable}, {v:NULL},},
  {"udate", 12, NULL, ft_unsigned, fc_second, {u: FS_r_counter}, {u: FS_w_counter}, {s:0x08},},
  {"date", 24, NULL, ft_date, fc_second, {d: FS_r_date}, {d: FS_w_date}, {s:0x08},},
  {"disconnect", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"disconnect/udate", 12, NULL, ft_unsigned, fc_volatile, {u: FS_r_counter}, {u: FS_w_counter}, {s:0x10},},
  {"disconnect/date", 24, NULL, ft_date, fc_volatile, {d: FS_r_date}, {d: FS_w_date}, {s:0x10},},
  {"endcharge", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"endcharge/udate", 12, NULL, ft_unsigned, fc_volatile, {u: FS_r_counter}, {u: FS_w_counter}, {s:0x14},},
  {"endcharge/date", 24, NULL, ft_date, fc_volatile, {d: FS_r_date}, {d: FS_w_date}, {s:0x14},},
};

DeviceEntryExtended(1E, DS2437, DEV_temp | DEV_volt);


struct aggregate A2438 = { 8, ag_numbers, ag_separate, };
struct filetype DS2438[] = {
	F_STANDARD,
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 8, &A2438, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
  {"VDD", 12, NULL, ft_float, fc_volatile, {f: FS_volts}, {v: NULL}, {i:1},},
  {"VAD", 12, NULL, ft_float, fc_volatile, {f: FS_volts}, {v: NULL}, {i:0},},
  {"temperature", 12, NULL, ft_temperature, fc_volatile, {f: FS_temp}, {v: NULL}, {v:NULL},},
  {"humidity", 12, NULL, ft_float, fc_volatile, {f: FS_Humid}, {v: NULL}, {v:NULL},},
  {"vis", 12, NULL, ft_float, fc_volatile, {f: FS_Current}, {v: NULL}, {v:NULL},},
  {"Ienable", 12, NULL, ft_unsigned, fc_stable, {u: FS_r_Ienable}, {u: FS_w_Ienable}, {v:NULL},},
  {"offset", 12, NULL, ft_unsigned, fc_stable, {i: FS_r_Offset}, {i: FS_w_Offset}, {v:NULL},},
  {"udate", 12, NULL, ft_unsigned, fc_second, {u: FS_r_counter}, {u: FS_w_counter}, {s:0x08},},
  {"date", 24, NULL, ft_date, fc_second, {d: FS_r_date}, {d: FS_w_date}, {s:0x08},},
  {"disconnect", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"disconnect/udate", 12, NULL, ft_unsigned, fc_volatile, {u: FS_r_counter}, {u: FS_w_counter}, {s:0x10},},
  {"disconnect/date", 24, NULL, ft_date, fc_volatile, {d: FS_r_date}, {d: FS_w_date}, {s:0x10},},
  {"endcharge", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"endcharge/udate", 12, NULL, ft_unsigned, fc_volatile, {u: FS_r_counter}, {u: FS_w_counter}, {s:0x14},},
  {"endcharge/date", 24, NULL, ft_date, fc_volatile, {d: FS_r_date}, {d: FS_w_date}, {s:0x14},},
  {"HTM1735", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"HTM1735/humidity", 12, NULL, ft_float, fc_volatile, {f: FS_Humid_1735}, {v: NULL}, {v:NULL},},
  {"HIH4000", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"HIH4000/humidity", 12, NULL, ft_float, fc_volatile, {f: FS_Humid_4000}, {v: NULL}, {v:NULL},},
  {"MultiSensor", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"MultiSensor/type", 12, NULL, ft_vascii, fc_stable, {a: FS_MStype}, {v: NULL}, {v:NULL},},
};

DeviceEntryExtended(26, DS2438, DEV_temp | DEV_volt);

/* ------- Functions ------------ */

/* DS2438 */
static int OW_r_page(BYTE * p, const int page,
					 const struct parsedname *pn);
static int OW_w_page(const BYTE * p, const int page,
					 const struct parsedname *pn);
static int OW_temp(_FLOAT * T, const struct parsedname *pn);
static int OW_volts(_FLOAT * V, const int src,
					const struct parsedname *pn);
static int OW_current(_FLOAT * I, const struct parsedname *pn);
static int OW_r_Ienable(unsigned *u, const struct parsedname *pn);
static int OW_w_Ienable(const unsigned u, const struct parsedname *pn);
static int OW_r_int(int *I, const UINT address,
					const struct parsedname *pn);
static int OW_w_int(const int I, const UINT address,
					const struct parsedname *pn);
static int OW_w_offset(const int I, const struct parsedname *pn);

/* 2438 A/D */
static int FS_r_page(BYTE * buf, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	BYTE data[8];
	if (OW_r_page(data, pn->extension, pn))
		return -EINVAL;
	memcpy(buf, &data[offset], size);
	return size;
}

static int FS_w_page(const BYTE * buf, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	if (size != 8) {			/* partial page */
		BYTE data[8];
		if (OW_r_page(data, pn->extension, pn))
			return -EINVAL;
		memcpy(&data[offset], buf, size);
		if (OW_w_page(data, pn->extension, pn))
			return -EFAULT;
	} else {					/* complete page */
		if (OW_w_page(buf, pn->extension, pn))
			return -EFAULT;
	}
	return 0;
}

static int FS_MStype(ASCII * buf, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	BYTE data[8];
	ASCII *t;
	if (OW_r_page(data, 0, pn))
		return -EINVAL;
	switch (data[0]) {
	case 0x00:
		t = "MS-T";
		break;
	case 0x19:
		t = "MS-TH";
		break;
	case 0x1A:
		t = "MS-TV";
		break;
	case 0x1B:
		t = "MS-TL";
		break;
	case 0x1C:
		t = "MS-TC";
		break;
	case 0x1D:
		t = "MS-TW";
		break;
	default:
		t = "unknown";
		break;
	}
	return FS_output_ascii_z(buf, size, offset, t);
}

static int FS_temp(_FLOAT * T, const struct parsedname *pn)
{
	if (OW_temp(T, pn))
		return -EINVAL;
	return 0;
}

static int FS_volts(_FLOAT * V, const struct parsedname *pn)
{
	/* data=1 VDD data=0 VAD */
	if (OW_volts(V, pn->ft->data.i, pn))
		return -EINVAL;
	return 0;
}

static int FS_Humid(_FLOAT * H, const struct parsedname *pn)
{
	_FLOAT T, VAD, VDD;
	if (OW_volts(&VDD, 1, pn) || OW_volts(&VAD, 0, pn) || OW_temp(&T, pn))
		return -EINVAL;
	//*H = (VAD/VDD-.16)/(.0062*(1.0546-.00216*T)) ;
	/*
	   From: Vincent Fleming <vincef@penmax.com>
	   To: owfs-developers@lists.sourceforge.net
	   Date: Jun 7, 2006 8:53 PM
	   Subject: [Owfs-developers] Error in Humidity calculation in ow_2438.c

	   OK, this is a nit, but it will make owfs a little more accurate (admittedly, it’s not very significant difference)…
	   The calculation given by Dallas for a DS2438/HIH-3610 combination is:
	   Sensor_RH = (VAD/VDD) -0.16 / 0.0062
	   And Honeywell gives the following:
	   VAD = VDD (0.0062(sensor_RH) + 0.16), but they specify that VDD = 5V dc, at 25 deg C.
	   Which is exactly what we have in owfs code (solved for Humidity, of course).
	   Honeywell’s documentation explains that the HIH-3600 series humidity sensors produce a liner voltage response to humidity that is in the range of 0.8 Vdc to 3.8 Vdc (typical) and is proportional to the input voltage.
	   So, the error is, their listed calculations don’t correctly adjust for varying input voltage.
	   The .16 constant is 1/5 of 0.8 – the minimum voltage produced.  When adjusting for voltage (such as (VAD/VDD) portion), this constant should also be divided by the input voltage, not by 5 (the calibrated input voltage), as shown in their documentation.
	   So, their documentation is a little wrong.
	   The level of error this produces would be proportional to how far from 5 Vdc your input voltage is.  In my case, I seem to have a constant 4.93 Vdc input, so it doesn’t have a great effect (about .25 degrees RH)
	 */
	H[0] = (VAD / VDD - (0.8 / VDD)) / (.0062 * (1.0546 - .00216 * T));

	return 0;
}

static int FS_Humid_4000(_FLOAT * H, const struct parsedname *pn)
{
	_FLOAT T, VAD, VDD;
	if (OW_volts(&VDD, 1, pn) || OW_volts(&VAD, 0, pn) || OW_temp(&T, pn))
		return -EINVAL;
	H[0] =
		(VAD / VDD -
		 (0.8 / VDD)) / (.0062 * (1.0305 + .000044 * T +
								  .0000011 * T * T));

	return 0;
}

/*
 * Willy Robison's contribution
 *      HTM1735 from Humirel (www.humirel.com) hooked up like everyone
 *      else.  The datasheet seems to suggest that the humidity
 *      measurement isn't too sensitive to VCC (VCC=5.0V +/- 0.25V).
 *      The conversion formula is derived directly from the datasheet
 *      (page 2).  VAD is assumed to be volts and *H is relative
 *      humidity in percent.
 */
static int FS_Humid_1735(_FLOAT * H, const struct parsedname *pn)
{
	_FLOAT VAD;
	if (OW_volts(&VAD, 0, pn))
		return -EINVAL;
	H[0] = 38.92 * VAD - 41.98;

	return 0;
}

static int FS_Current(_FLOAT * I, const struct parsedname *pn)
{
	if (OW_current(I, pn))
		return -EINVAL;
	return 0;
}

static int FS_r_Ienable(unsigned *u, const struct parsedname *pn)
{
	if (OW_r_Ienable(u, pn))
		return -EINVAL;
	return 0;
}

static int FS_w_Ienable(const unsigned *u, const struct parsedname *pn)
{
	if (*u > 3)
		return -EINVAL;
	if (OW_w_Ienable(*u, pn))
		return -EINVAL;
	return 0;
}

static int FS_r_Offset(int *i, const struct parsedname *pn)
{
	if (OW_r_int(i, 0x0D, pn))
		return -EINVAL;			/* page1 byte 5 */
	*i >>= 3;
	return 0;
}

static int FS_w_Offset(const int *i, const struct parsedname *pn)
{
	int I = *i;
	if (I > 255 || I < -256)
		return -EINVAL;
	if (OW_w_offset(I << 3, pn))
		return -EINVAL;
	return 0;
}

/* set clock */
static int FS_w_counter(const UINT * u, const struct parsedname *pn)
{
	_DATE d = (_DATE) u[0];
	return FS_w_date(&d, pn);
}

/* set clock */
static int FS_w_date(const _DATE * d, const struct parsedname *pn)
{
	int page = ((uint32_t) (pn->ft->data.s)) >> 3;
	int offset = ((uint32_t) (pn->ft->data.s)) & 0x07;
	BYTE data[8];
	if (OW_r_page(data, page, pn))
		return -EINVAL;
	UT_fromDate(d[0], &data[offset]);
	if (OW_w_page(data, page, pn))
		return -EINVAL;
	return 0;
}

/* read clock */
int FS_r_counter(UINT * u, const struct parsedname *pn)
{
	_DATE d;
	int ret = FS_r_date(&d, pn);
	u[0] = (UINT) d;
	return ret;
}

/* read clock */
int FS_r_date(_DATE * d, const struct parsedname *pn)
{
	int page = ((uint32_t) (pn->ft->data.s)) >> 3;
	int offset = ((uint32_t) (pn->ft->data.s)) & 0x07;
	BYTE data[8];
	if (OW_r_page(data, page, pn))
		return -EINVAL;
	d[0] = UT_toDate(&data[offset]);
	return 0;
}

/* DS2438 fancy battery */
static int OW_r_page(BYTE * p, const int page, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE recall[] = { 0xB8, page, };
	BYTE r[] = { 0xBE, page, };
	struct transaction_log t[] = {
		TRXN_START,
		{recall, NULL, 2, trxn_match},
		TRXN_START,
		{r, NULL, 2, trxn_match},
		{NULL, data, 9, trxn_read},
		{data, NULL, 9, trxn_crc8,},
		TRXN_END,
	};

	// read to scratch, then in
	if (BUS_transaction(t, pn))
		return 1;

	// copy to buffer
	memcpy(p, data, 8);
	return 0;
}

/* write 8 bytes */
static int OW_w_page(const BYTE * p, const int page,
					 const struct parsedname *pn)
{
	BYTE data[9];
	BYTE w[] = { 0x4E, page, };
	BYTE r[] = { 0xBE, page, };
	BYTE eeprom[] = { 0x48, page, };
	struct transaction_log t[] = {
		TRXN_START,				// 0
		{w, NULL, 2, trxn_match},	//1 write to scratch command
		{p, NULL, 8, trxn_match},	// write to scratch data
		TRXN_START,
		{r, NULL, 2, trxn_match},	//4 read back command
		{NULL, data, 9, trxn_read},	//5 read data
		{data, NULL, 9, trxn_crc8},	//6 crc8
		{p, data, 0, trxn_match,},	//7 match except page 0
		TRXN_START,
		{eeprom, NULL, 2, trxn_match},	//9 actual write
		TRXN_END,
	};

	if (page > 0)
		t[7].size = 8;			// full match for all but volatile page 0
	// write then read to scratch, then into EEPROM if scratch matches
	if (BUS_transaction(t, pn))
		return 1;

	UT_delay(10);
	return 0;					// timeout
}

static int OW_temp(_FLOAT * T, const struct parsedname *pn)
{
	BYTE data[9];
	static BYTE t[] = { 0x44, };
	struct transaction_log tconvert[] = {
		TRXN_START,
		{t, NULL, 1, trxn_match},
		TRXN_END,
	};
	// write conversion command
	if (Simul_Test(simul_temp, pn) != 0) {
		if (BUS_transaction(tconvert, pn))
			return 1;
		UT_delay(10);
	}
	// read back registers
	if (OW_r_page(data, 0, pn))
		return 1;
	//*T = ((int)((signed char)data[2])) + .00390625*data[1] ;
	T[0] = UT_int16(&data[1]) / 256.0;
	return 0;
}

static int OW_volts(_FLOAT * V, const int src, const struct parsedname *pn)
{
	// src deserves some explanation:
	//   1 -- VDD (battery) measured
	//   0 -- VAD (other) measured
	BYTE data[9];
	static BYTE v[] = { 0xB4, };
	static BYTE w[] = { 0x4E, 0x00, };
	struct transaction_log tsource[] = {
		TRXN_START,
		{w, NULL, 2, trxn_match},
		{data, NULL, 8, trxn_match},
		TRXN_END,
	};
	struct transaction_log tconvert[] = {
		TRXN_START,
		{v, NULL, 1, trxn_match},
		TRXN_END,
	};

	// set voltage source command
	if (OW_r_page(data, 0, pn))
		return 1;
	UT_setbit(data, 3, src);	// AD bit in status register
	if (BUS_transaction(tsource, pn))
		return 1;

	// write conversion command
	if (BUS_transaction(tconvert, pn))
		return 1;
	UT_delay(10);

	// read back registers
	if (OW_r_page(data, 0, pn))
		return 1;
	//printf("DS2438 current read %.2X %.2X %g\n",data[6],data[5],(_FLOAT)( ( ((int)data[6]) <<8 )|data[5] ));
	//V[0] = .01 * (_FLOAT)( ( ((int)data[4]) <<8 )|data[3] ) ;
	V[0] = .01 * (_FLOAT) UT_int16(&data[3]);
	return 0;
}

// Read current register
// turn on (temporary) A/D in scratchpad
static int OW_current(_FLOAT * I, const struct parsedname *pn)
{
	BYTE data[8];
	BYTE r[] = { 0xBE, 0x00, };
	BYTE w[] = { 0x4E, 0x00, };
	int enabled;
	struct transaction_log tread[] = {
		TRXN_START,
		{r, NULL, 2, trxn_match,},
		{NULL, data, 7, trxn_read,},
		TRXN_END,
	};
	struct transaction_log twrite[] = {
		TRXN_START,
		{w, NULL, 2, trxn_match,},
		{data, NULL, 1, trxn_match,},
		TRXN_END,
	};

	// set current readings on source command
	// Actual units are volts-- need to know sense resistor for current
	if (BUS_transaction(tread, pn))
		return 1;
	//printf("DS2438 current PREread %.2X %.2X %g\n",data[6],data[5],(_FLOAT)( ( ((int)data[6]) <<8 )|data[5] ));
	enabled = data[0] & 0x01;	// IAC bit
	if (!enabled) {				// need to temporariliy turn on current measurements
		//printf("DS2438 Current needs to be enabled\n");
		data[0] |= 0x01;
		if (BUS_transaction(twrite, pn))
			return 1;
		UT_delay(30);			// enough time for one conversion (30msec)
		if (BUS_transaction(tread, pn))
			return 1;			// reread
	}
	//printf("DS2438 current read %.2X %.2X %g\n",data[6],data[5],(_FLOAT)( ( ((int)data[6]) <<8 )|data[5] ));
	I[0] = .0002441 * (_FLOAT) ((((int) data[6]) << 8) | data[5]);
	if (!enabled) {				// need to restore no current measurements
		if (BUS_transaction(twrite, pn))
			return 1;
	}
	return 0;
}

static int OW_w_offset(const int I, const struct parsedname *pn)
{
	BYTE data[8];
	int enabled;

	// set current readings off source command
	if (OW_r_page(data, 0, pn))
		return 1;
	enabled = UT_getbit(data, 0);
	if (enabled) {
		UT_setbit(data, 0, 0);	// AD bit in status register
		if (OW_w_page(data, 0, pn))
			return 1;
	}
	// read back registers
	if (OW_w_int(I, 0x0D, pn))
		return 1;				/* page1 byte5 */

	if (enabled) {
		// if ( OW_r_page( data , 0 , pn ) ) return 1 ; /* Assume no change to these fields */
		UT_setbit(data, 0, 1);	// AD bit in status register
		if (OW_w_page(data, 0, pn))
			return 1;
	}
	return 0;
}

static int OW_r_Ienable(unsigned *u, const struct parsedname *pn)
{
	BYTE data[8];

	if (OW_r_page(data, 0, pn))
		return 1;
	if (UT_getbit(data, 0)) {
		if (UT_getbit(data, 1)) {
			if (UT_getbit(data, 2)) {
				*u = 3;
			} else {
				*u = 2;
			}
		} else {
			*u = 1;
		}
	} else {
		*u = 0;
	}
	return 0;
}

static int OW_w_Ienable(const unsigned u, const struct parsedname *pn)
{
	BYTE data[8];
	static BYTE iad[] = { 0x00, 0x01, 0x3, 0x7, };

	if (OW_r_page(data, 0, pn))
		return 1;
	if ((data[0] & 0x07) != iad[u]) {	/* only change if needed */
		data[0] = (data[0] & 0xF8) | iad[u];
		if (OW_w_page(data, 0, pn))
			return 1;
	}
	return 0;
}


static int OW_r_int(int *I, const UINT address,
					const struct parsedname *pn)
{
	BYTE data[8];

	// read back registers
	if (OW_r_page(data, address >> 3, pn))
		return 1;
	*I = ((int) ((signed char) data[(address & 0x07) + 1])) << 8 |
		data[address & 0x07];
	return 0;
}

static int OW_w_int(const int I, const UINT address,
					const struct parsedname *pn)
{
	BYTE data[8];

	// read back registers
	if (OW_r_page(data, address >> 3, pn))
		return 1;
	data[address & 0x07] = I & 0xFF;
	data[(address & 0x07) + 1] = (I >> 8) & 0xFF;
	return OW_w_page(data, address >> 3, pn);
}
