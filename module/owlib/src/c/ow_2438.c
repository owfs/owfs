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
#include <math.h>
#include "owfs_config.h"
#include "ow_2438.h"

/* ------- Prototypes ----------- */

/* DS2438 Battery */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_temp);
READ_FUNCTION(FS_latesttemp);
READ_FUNCTION(FS_volts);
READ_FUNCTION(FS_Humid);
READ_FUNCTION(FS_Humid_1735);
READ_FUNCTION(FS_Humid_3600);
READ_FUNCTION(FS_Humid_4000);
READ_FUNCTION(FS_Humid_5030);
READ_FUNCTION(FS_Humid_datanab);
WRITE_FUNCTION(FS_reset_datanab);
READ_FUNCTION(FS_Current);
READ_FUNCTION(FS_r_status);
WRITE_FUNCTION(FS_w_status);
READ_FUNCTION(FS_r_Offset);
WRITE_FUNCTION(FS_w_Offset);
READ_FUNCTION(FS_r_counter);
WRITE_FUNCTION(FS_w_counter);
READ_FUNCTION(FS_MStype);
READ_FUNCTION(FS_B1R1A_pressure);
READ_FUNCTION(FS_r_B1R1A_offset);
WRITE_FUNCTION(FS_w_B1R1A_offset);
READ_FUNCTION(FS_r_B1R1A_gain);
WRITE_FUNCTION(FS_w_B1R1A_gain);
READ_FUNCTION(FS_S3R1A_current);
READ_FUNCTION(FS_S3R1A_illuminance);
READ_FUNCTION(FS_r_S3R1A_gain);
WRITE_FUNCTION(FS_w_S3R1A_gain);

static enum e_visibility VISIBLE_DATANAB( const struct parsedname * pn ) ;

// src deserves some explanation:
//   1 -- VDD (battery) measured
//   0 -- VAD (other) measured
enum voltage_source {
	voltage_source_VAD = 0,
	voltage_source_VDD = 1,
} ;

/* ------- Structures ----------- */

static struct aggregate A2437 = { 8, ag_numbers, ag_separate, };
static struct filetype DS2437[] = {
	F_STANDARD,
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 8, &A2437, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },

	{"VDD", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {.i=voltage_source_VAD}, },
	{"VAD", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {.i=voltage_source_VAD}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"latesttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_latesttemp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_Current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"IAD", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_status, FS_w_status, VISIBLE, {.i=0}, },
	{"CA", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_status, FS_w_status, VISIBLE, {.i=1}, },
	{"EE", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_status, FS_w_status, VISIBLE, {.i=2}, },
	{"udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter, FS_w_counter, VISIBLE, {.s=0x08}, },
	{"date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {.a="udate"}, },

	{"disconnect", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"disconnect/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_counter, FS_w_counter, VISIBLE, {.s=0x10}, },
	{"disconnect/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {.a="disconnect/udate"}, },

	{"endcharge", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"endcharge/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_counter, FS_w_counter, VISIBLE, {.s=0x14}, },
	{"endcharge/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {.a="endcharge/udate"}, },
};

DeviceEntryExtended(1E, DS2437, DEV_temp | DEV_volt, NO_GENERIC_READ, NO_GENERIC_WRITE);


static struct aggregate A2438 = { 8, ag_numbers, ag_separate, };
static struct filetype DS2438[] = {
	F_STANDARD,
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 8, &A2438, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },

	{"VDD", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {.i=voltage_source_VDD}, },
	{"VAD", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, {.i=voltage_source_VAD}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_simultaneous_temperature, FS_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"latesttemp", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_latesttemp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_Humid, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_Current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"IAD", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_status, FS_w_status, VISIBLE, {.i=0}, },
	{"CA", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_status, FS_w_status, VISIBLE, {.i=1}, },
	{"EE", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_status, FS_w_status, VISIBLE, {.i=2}, },
	{"offset", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_Offset, FS_w_Offset, VISIBLE, NO_FILETYPE_DATA, },
	{"udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter, FS_w_counter, VISIBLE, {.s=0x08}, },
	{"date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {.a="udate"}, },

	{"disconnect", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"disconnect/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_counter, FS_w_counter, VISIBLE, {.s=0x10}, },
	{"disconnect/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {.a="disconnect/udate"}, },

	{"endcharge", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"endcharge/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_counter, FS_w_counter, VISIBLE, {.s=0x14}, },
	{"endcharge/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {.a="endcharge/udate"}, },

	{"HTM1735", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"HTM1735/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_Humid_1735, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"HIH3600", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"HIH3600/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_Humid_3600, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"HIH4000", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"HIH4000/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_Humid_4000, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"HIH5030", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"HIH5030/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_Humid_5030, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"DATANAB", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_DATANAB, NO_FILETYPE_DATA, },
	{"DATANAB/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_Humid_datanab, NO_WRITE_FUNCTION, VISIBLE_DATANAB, NO_FILETYPE_DATA, },
	{"DATANAB/reset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset_datanab, VISIBLE_DATANAB, NO_FILETYPE_DATA, },

	{"MultiSensor", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"MultiSensor/type", 12, NON_AGGREGATE, ft_vascii, fc_stable, FS_MStype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"B1-R1-A", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"B1-R1-A/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_volatile, FS_B1R1A_pressure, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"B1-R1-A/offset", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_stable, FS_r_B1R1A_offset, FS_w_B1R1A_offset, VISIBLE, NO_FILETYPE_DATA, },
	{"B1-R1-A/gain", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_stable, FS_r_B1R1A_gain, FS_w_B1R1A_gain, VISIBLE, NO_FILETYPE_DATA, },

	{"S3-R1-A", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"S3-R1-A/current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_S3R1A_current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"S3-R1-A/illuminance", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_S3R1A_illuminance, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"S3-R1-A/gain", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_float, fc_stable, FS_r_S3R1A_gain, FS_w_S3R1A_gain, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(26, DS2438, DEV_temp | DEV_volt, NO_GENERIC_READ, NO_GENERIC_WRITE);
DeviceEntryExtendedSecondary(A6, DS2438, DEV_temp | DEV_volt, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x4E
#define _1W_READ_SCRATCHPAD 0xBE
#define _1W_COPY_SCRATCHPAD 0x48
#define _1W_RECALL_SCRATCHPAD 0xB8
#define _1W_CONVERT_T 0x44
#define _1W_CONVERT_V 0xB4

/* ------- Functions ------------ */

/* DS2438 */
static GOOD_OR_BAD OW_r_page(BYTE * p, const int page, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_page(const BYTE * p, const int page, const struct parsedname *pn);
static GOOD_OR_BAD OW_latesttemp(_FLOAT * T, const struct parsedname *pn);
static GOOD_OR_BAD OW_temp(_FLOAT * T, int simul_good, const struct parsedname *pn);
static GOOD_OR_BAD OW_volts(_FLOAT * V, enum voltage_source src, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_int(int *I, const UINT address, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_int(const int I, const UINT address, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_offset(const int I, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_uint(UINT *U, const UINT address, const struct parsedname *pn);
static GOOD_OR_BAD OW_set_AD( enum voltage_source src, const struct parsedname *pn);

/* 8 Byte pages */
#define DS2438_ADDRESS_TO_PAGE(a)	((a)>>3)
#define DS2438_ADDRESS_TO_OFFSET(a)	((a)&0x07)

/* Datanab data */
struct s_datanab {
	_FLOAT slope ;
	_FLOAT offset ;
};
/* Internal files */
Make_SlaveSpecificTag(NAB, fc_persistent);

/* finds the visibility for DATANAB */
static enum e_visibility VISIBLE_DATANAB( const struct parsedname * pn )
{
	int device_id = -1 ;
	
	LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
	if ( BAD( GetVisibilityCache( &device_id, pn ) ) ) {
		BYTE page3[8] ;
		if ( GOOD( OW_r_page( page3, 3, pn ) ) ) {
			if ( memcmp( page3, "HUMIDIT3", 8 ) == 0 ) {
				device_id = 1 ; // tag for datanab device
			} else {
				device_id = 0 ; // not a datanab
			}
			SetVisibilityCache( device_id, pn ) ;
		}
	}
	return device_id==1 ? visible_now : visible_not_now ;
}

/* 2438 A/D */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE data[8];
	RETURN_ERROR_IF_BAD( OW_r_page(data, pn->extension, pn) ) ;
	memcpy((BYTE *) OWQ_buffer(owq), &data[OWQ_offset(owq)], OWQ_size(owq));
	OWQ_length(owq) = OWQ_size(owq);
	return 0;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	LEVEL_DEBUG("size=%d offset=%d", OWQ_size(owq), OWQ_offset(owq));
	if (OWQ_size(owq) < 8) {	/* partial page */
		BYTE data[8];
		RETURN_ERROR_IF_BAD( OW_r_page(data, pn->extension, pn) );
		memcpy(&data[OWQ_offset(owq)], (BYTE *) OWQ_buffer(owq), OWQ_size(owq));
		RETURN_ERROR_IF_BAD( OW_w_page(data, pn->extension, pn) ) ;
	} else {					/* complete page */
		RETURN_ERROR_IF_BAD( OW_w_page((BYTE *) OWQ_buffer(owq), pn->extension, pn) ) ;

	}
	return 0;
}

static ZERO_OR_ERROR FS_MStype(struct one_wire_query *owq)
{
	BYTE data[8];
	ASCII *t;
	// Read page 3 for type -- Michael Markstaller
	RETURN_ERROR_IF_BAD( OW_r_page(data, 3, PN(owq)) );
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
	return OWQ_format_output_offset_and_size_z(t, owq);
}

static ZERO_OR_ERROR FS_temp(struct one_wire_query *owq)
{
	// Double try temperature, with and without simultaneous
	if ( GOOD( OW_temp(&OWQ_F(owq), OWQ_SIMUL_TEST(owq), PN(owq))) ) {
		return 0 ;
	}
	return GB_to_Z_OR_E( OW_temp(&OWQ_F(owq), 0, PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_latesttemp(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_latesttemp(&OWQ_F(owq), PN(owq)) );
}

static ZERO_OR_ERROR FS_volts(struct one_wire_query *owq)
{
	/* data=1 VDD data=0 VAD */
	return GB_to_Z_OR_E( OW_volts(&OWQ_F(owq), OWQ_pn(owq).selected_filetype->data.i, PN(owq)) );
}

static ZERO_OR_ERROR FS_Humid(struct one_wire_query *owq)
{
	_FLOAT H = 0.;
	ZERO_OR_ERROR z_or_e ;

	if ( VISIBLE_DATANAB(PN(owq)) == visible_not_now ) {
		z_or_e = FS_r_sibling_F( &H, "HIH3600/humidity", owq ) ;
	} else {
		z_or_e = FS_r_sibling_F( &H, "DATANAB/humidity", owq ) ;
	}

	OWQ_F(owq) = H ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_Humid_3600(struct one_wire_query *owq)
{
	_FLOAT T, VAD, VDD;
	_FLOAT humidity_uncompensated ;
	_FLOAT temperature_compensation ;

	if (
		FS_r_sibling_F( &T, "temperature", owq ) != 0
		|| FS_r_sibling_F( &VAD, "VAD", owq ) != 0
		|| FS_r_sibling_F( &VDD, "VDD", owq ) != 0
	) {
		return -EINVAL ;
	}

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
	if ( VDD < .01 ) {
		LEVEL_DEBUG("Low measured VDD %g",VDD);
		return -EINVAL ;
	}
	humidity_uncompensated = (VAD / VDD - (0.8 / VDD)) / .0062 ;
	temperature_compensation = 1.0546 - 0.00216 * T ;
	OWQ_F(owq) = humidity_uncompensated / temperature_compensation ;
    //printf( "%g,%g,%g,%g\n", VAD, VDD, T, OWQ_F(owq) ) ;
	return 0;
}

/* The HIH-4010 and HIH-4020 are newer versions of the HIH-4000 */
/* Used in the Hobbyboards humidity product */
/* Formula from Honeywell datasheet (2007)
 * http://sensing.honeywell.com/index.cfm/ci_id/142534/la_id/1/document/1/re_id/0
 *
 * */ 
static ZERO_OR_ERROR FS_Humid_4000(struct one_wire_query *owq)
{
	_FLOAT T, VAD, VDD;
	_FLOAT humidity_uncompensated ;
	_FLOAT temperature_compensation ;

	if (
		FS_r_sibling_F( &T, "temperature", owq ) != 0
		|| FS_r_sibling_F( &VAD, "VAD", owq ) != 0
		|| FS_r_sibling_F( &VDD, "VDD", owq ) != 0
	) {
		return -EINVAL ;
	}

	if ( VDD < .01 ) {
		LEVEL_DEBUG("Low measured VDD %g",VDD);
		return -EINVAL ;
	}
	humidity_uncompensated = ((VAD/VDD) - 0.16) / 0.0062 ;
	// temperature compensation
	temperature_compensation = 1.0546 - 0.00216 * T ;
	OWQ_F(owq) = humidity_uncompensated / temperature_compensation ;

	return 0;
}

/* Calculations and compensation for HIH5030/HIH5031
 * Formula from Datasheet
 * https://sensing.honeywell.com/honeywell-sensing-hih5030-5031-series-product-sheet-009050-2-en.pdf
 * */ 
static ZERO_OR_ERROR FS_Humid_5030(struct one_wire_query *owq)
{
	_FLOAT T, VAD, VDD;
	_FLOAT humidity_uncompensated ;
	_FLOAT temperature_compensation ;

	if (
		FS_r_sibling_F( &T, "temperature", owq ) != 0
		|| FS_r_sibling_F( &VAD, "VAD", owq ) != 0
		|| FS_r_sibling_F( &VDD, "VDD", owq ) != 0
	) {
		return -EINVAL ;
	}

	if ( VDD < .01 ) {
		LEVEL_DEBUG("Low measured VDD %g",VDD);
		return -EINVAL ;
	}
	humidity_uncompensated = ((VAD/VDD) - 0.1515) / 0.00636 ;
	// temperature compensation
	temperature_compensation = 1.0546 - 0.00216 * T ;
	OWQ_F(owq) = humidity_uncompensated / temperature_compensation ;

	return 0;
}


/* The Datanab verion of the HIH-4000 */
static ZERO_OR_ERROR FS_Humid_datanab(struct one_wire_query *owq)
{
	_FLOAT T, vis, VAD;
	_FLOAT humidity_uncompensated ;
	_FLOAT temperature_adjust ;
	struct s_datanab nab ;
	struct parsedname * pn = PN(owq) ;

	if ( VISIBLE_DATANAB(pn) == visible_not_now ) {
		return -ENOTSUP ;
	}

	// make sure permanent cache has calibration values
	// this also serves as a check that device setup was done.
	if ( BAD( Cache_Get_SlaveSpecific((void *) &nab, sizeof(struct s_datanab), SlaveSpecificTag(NAB), pn) )) {
		// need to call device setup
		FS_w_sibling_Y( 1, "DATANAB/reset", owq ) ;
		if ( BAD( Cache_Get_SlaveSpecific((void *) &nab, sizeof(struct s_datanab), SlaveSpecificTag(NAB), pn) )) {
			return -EINVAL ;
		}
	}

	if (
		FS_r_sibling_F( &T, "temperature", owq ) != 0
		|| FS_r_sibling_F( &VAD, "VAD", owq ) != 0
		|| FS_r_sibling_F( &vis, "vis", owq ) != 0
	) {
		return -EINVAL ;
	}

	// calculation straight from datasheet: http://www.datanab.com/docs/sensors/1WTH_PRB_commDetails.pdf
	humidity_uncompensated = ( (vis/VAD) * 85.65 - nab.offset ) / nab.slope ;
	temperature_adjust = 1.0546 - ( 0.00216 * T ) ;
	if ( temperature_adjust == 0. ) {
		temperature_adjust = 1. ;
	}
	OWQ_F(owq) = humidity_uncompensated / temperature_adjust ;

	return 0 ;
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
static ZERO_OR_ERROR FS_Humid_1735(struct one_wire_query *owq)
{
	_FLOAT VAD = 0.;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F( &VAD, "VAD", owq ) ;

	OWQ_F(owq) = 38.92 * VAD - 41.98;
	return z_or_e ;
}

// Read current register
// turn on (temporary) A/D in scratchpad
static ZERO_OR_ERROR FS_Current(struct one_wire_query *owq)
{
	BYTE data[9];
	INT iad ;

	if ( FS_r_sibling_Y( &iad, "IAD", owq ) != 0 ) {
		return -EINVAL ;
	}

	// Actual units are volts-- need to know sense resistor for current
	RETURN_ERROR_IF_BAD( OW_r_page(data, 0, PN(owq)) ) ;

	LEVEL_DEBUG("DS2438 vis scratchpad " SNformat, SNvar(data));
	//F[0] = .0002441 * (_FLOAT) ((((int) data[6]) << 8) | data[5]);
	OWQ_F(owq) = .0002441 * UT_int16(&data[5]);

	return 0 ;
}

// Set the DataNAB humidity -- sets the vis converting and reads the constants
static ZERO_OR_ERROR FS_reset_datanab( struct one_wire_query * owq )
{
	if ( OWQ_Y(owq) == 1 ) {
		struct parsedname * pn = PN(owq) ;
		BYTE page6[8] ;
		struct s_datanab nab = {
			.slope = 0.031,
			.offset = 0.8 ,
		} ; // default values

		FS_w_sibling_U( 0, "offset", owq ) ; // set current offset to 0
		FS_w_sibling_Y( 1, "IAD", owq ) ; // turn on current sensor
		UT_delay(28) ; // wait 28 msec for 1st conversion

		// read slope and offset data stored in page 6 -- with some checks
		if ( GOOD( OW_r_page( page6, 6, pn ) ) && page6[0]==0xAA && page6[1]==0xAA ) {
			_FLOAT offset, slope ;
			slope = ( 256. * page6[4] + page6[5] ) / 100000. ;
			offset = ( 256. * page6[2] + page6[3] ) / 10000. ;
			if ( slope != 0. ) {
				nab.slope = slope ;
				nab.offset = offset ;
			}
		}
		// store slope and offset in permanent cache
		Cache_Add_SlaveSpecific((const void *) &nab, sizeof(struct s_datanab), SlaveSpecificTag(NAB), pn ) ;
	}
	return 0 ;
}

// status bit
static ZERO_OR_ERROR FS_r_status(struct one_wire_query *owq)
{
	BYTE page0[8];
	RETURN_ERROR_IF_BAD( OW_r_page(page0, 0, PN(owq)) );

	OWQ_Y(owq) = UT_getbit(page0, PN(owq)->selected_filetype->data.i);
	return 0;
}

static ZERO_OR_ERROR FS_w_status(struct one_wire_query *owq)
{
	BYTE page0[8];
	RETURN_ERROR_IF_BAD( OW_r_page(page0, 0, PN(owq)) );
	UT_setbit(page0, PN(owq)->selected_filetype->data.i, OWQ_Y(owq));
	RETURN_ERROR_IF_BAD( OW_w_page(page0, 0, PN(owq)) ) ;
	return 0;
}

static ZERO_OR_ERROR FS_r_Offset(struct one_wire_query *owq)
{
	RETURN_ERROR_IF_BAD( OW_r_int(&OWQ_I(owq), 0x0D, PN(owq)) ) ;

	OWQ_I(owq) >>= 3;
	return 0;
}

static ZERO_OR_ERROR FS_w_Offset(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int IAD_status ;
	int I = OWQ_I(owq);
	ZERO_OR_ERROR z_or_e ;

	if (I > 255 || I < -256) {
		return -EINVAL;
	}

	if ( BAD(FS_r_sibling_Y( &IAD_status, "IAD", owq )) ) {
		return -EINVAL ;
	}
	if ( IAD_status == 1 ) {
		// need to turn off
		if ( BAD(FS_w_sibling_Y( 0, "IAD", owq )) ) {
			return -EINVAL ;
		}
	}
	z_or_e = OW_w_offset(I << 3, pn) ;
	Cache_Del_Internal(SlaveSpecificTag(NAB), pn) ;
	if ( IAD_status == 1 ) {
		// need to turn back on
		if ( BAD(FS_w_sibling_Y( 1, "IAD", owq )) ) {
			return -EINVAL ;
		}
	}
	return z_or_e ;
}

/* set clock */
static ZERO_OR_ERROR FS_w_counter(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int page = DS2438_ADDRESS_TO_PAGE(pn->selected_filetype->data.s) ;
	int offset = DS2438_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s);
	BYTE data[8];

	RETURN_ERROR_IF_BAD( OW_r_page(data, page, pn) ) ;
	UT_uint32_to_bytes( OWQ_U(owq), &data[offset] );
	return GB_to_Z_OR_E( OW_w_page(data, page, pn) ) ;
}

/* read clock */
static ZERO_OR_ERROR FS_r_counter(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int page = DS2438_ADDRESS_TO_PAGE(pn->selected_filetype->data.s);
	int offset = DS2438_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s);
	BYTE data[8];

	RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
	OWQ_U(owq) = UT_int32(&data[offset]);
	return 0;
}

/*
 * Egil Kvaleberg's contribution
 *      The B1-R1-A barometer from Hobby-Boards has a 
 *      temperature compensated MPXA4115A absolute
 *      pressure sensor from Freescale wired to VAD via an INA126
 *      instrumentation amp with 10x gain and offset adjusted
 *      by a trimmer. The sensor sensitivity is specified as 46 mV/kPa,
 *      which is 21.737 mbar/V.
 *      The default offset is based on the standard calibration where
 *      2V is 948.2 mbar (28inHg) at sea level, which translates to
 *      904.7 mbar for 0V
 */
static ZERO_OR_ERROR FS_B1R1A_pressure(struct one_wire_query *owq)
{
	_FLOAT VAD;
	_FLOAT gain;
	_FLOAT offset;
	_FLOAT mbar;

	if (
		FS_r_sibling_F( &VAD, "VAD", owq )
	     || FS_r_sibling_F( &gain, "B1-R1-A/gain", owq )
	     || FS_r_sibling_F( &offset, "B1-R1-A/offset", owq )
	) {
		return -EINVAL ;
	}

	mbar = VAD * gain + offset;
	LEVEL_DEBUG("B1-R1-A Raw (mbar) = %g gain = %g ofs = %g", mbar, gain, offset);
	OWQ_F(owq) = mbar;

	return 0;
}

static ZERO_OR_ERROR FS_r_B1R1A_offset(struct one_wire_query *owq)
{
	int i = 0;

	/* Read page 3 byte 6&7 for barometer offset -- Egil Kvaleberg */
	RETURN_ERROR_IF_BAD( OW_r_int(&i, (3 << 3) + 6, PN(owq)) ) ;

	/* Offset in units of 1/20 millibars, default to 948.2 */
	if (i == 0) {
	    OWQ_F(owq) = 904.7;
	} else {
	    OWQ_F(owq) = i / 20.0;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_B1R1A_offset(struct one_wire_query *owq)
{
	_FLOAT offset = OWQ_F(owq);
	int i;
	if (offset < -0x7fff / 20.0 || offset > 0x7fff / 20.0) {
		return -EINVAL;
	}
	/* Offset in units of 1/20 millibars */
	i = lrint(offset * 20.0);
	/* Write page 3 byte 6&7 for B1-R1-A offset -- Egil Kvaleberg */
	return GB_to_Z_OR_E(OW_w_int(i, (3 << 3) + 6, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_B1R1A_gain(struct one_wire_query *owq)
{
	int i = 0;

	/* Read page 3 byte 4&5 for barometer gain -- Egil Kvaleberg */
	RETURN_ERROR_IF_BAD( OW_r_uint( (UINT*) &i, (3 << 3) + 4, PN(owq)) ) ;

	/* Gain in units of 1/1000 millibars/volt, default is 21.739 */
	if (i == 0) {
	    OWQ_F(owq) = 1000.0 / 46.0;
	} else {
	    OWQ_F(owq) = i / 1000.0;
	}

	return 0;
}

static ZERO_OR_ERROR FS_w_B1R1A_gain(struct one_wire_query *owq)
{
	_FLOAT gain = OWQ_F(owq);
	int i;
	if (gain < -0x7fff / 1000.0 || gain > 0x7fff / 1000.0) {
		return -EINVAL;
	}
	/* Gain in units of 1/1000 millibars/volt */
	i = lrint(gain * 1000.0);
	/* Write page 3 byte 4&5 for B1-R1-A gain -- Egil Kvaleberg */
	return GB_to_Z_OR_E(OW_w_int(i, (3 << 3) + 4, PN(owq))) ;
}

/*
 * Egil Kvaleberg's contribution
 *      The S3-R1-A solar radiation sensor from Hobby-Boards has a
 *      photodiode where the leakage current is read by the current
 *      sensor over a 390 ohm resistor. The reading is in microamps.
 *      The board is currently delivered with a SFH203P diode from Osram.
 *      The current at 1000 lx is specified as 9.5 uA, the dark current
 *      0.001 uA.
 *      Also, the actual mounting conditions will need to be
 *      compensated for, such as any integrating sphere and so on.
 *      Previously, the Clairex CLD140 was used, which is more sensitive.
 */
static ZERO_OR_ERROR FS_S3R1A_current(struct one_wire_query *owq)
{
	_FLOAT vis;

	if (
		FS_r_sibling_F( &vis, "vis", owq )
	) {
		return -EINVAL ;
	}

	/*
	 *  A negative current reading can happen, and
	 *  would be due to offset errors or noise.
	 */
	OWQ_F(owq) = vis * (1000000.0 / 390.0);

	return 0;
}

static ZERO_OR_ERROR FS_S3R1A_illuminance(struct one_wire_query *owq)
{
	_FLOAT current;
	_FLOAT gain;
	_FLOAT illuminance;

	if (
		FS_r_sibling_F( &current, "S3-R1-A/current", owq )
	     || FS_r_sibling_F( &gain, "S3-R1-A/gain", owq )
	) {
		return -EINVAL ;
	}
	/*
	 *  Negative current readings are eliminated to ensure positive
	 *  illuminance values. We
	 *  ensure they are non-zero to make it easier to deal with any
	 *  logaritms that may be applied.
	 *  Note that the chip used really is very crude: it has
	 *  a resolution of 0.24mV, corresponding to a dark current
	 *  resolution of 0.63 uA, corresponding to about 60 lx.
	 *  We use 1 lx as a representation of zero.
	 */
	illuminance = current * gain;
	if (illuminance < 1.0) illuminance = 1.0;
	OWQ_F(owq) = illuminance;

	return 0;
}

static ZERO_OR_ERROR FS_r_S3R1A_gain(struct one_wire_query *owq)
{
	unsigned u = 0;
	/* Read page 3 byte 2&3 for illuminance gain -- Egil Kvaleberg */
	RETURN_BAD_IF_BAD( OW_r_uint(&u, (3 << 3) + 2, PN(owq)) ) ;
	if (u == 0) {
		/* Default gain assumes SFH203P diode */
		OWQ_F(owq) = 1000.0 / 9.5; 
	} else {
		/* Gain stored in units of 1/10 lx/uA */
		OWQ_F(owq) = u / 10.0;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_S3R1A_gain(struct one_wire_query *owq)
{
	_FLOAT gain = OWQ_F(owq);
	unsigned u;
	if (gain < 0.0 || gain > 6553.5) {
		return -EINVAL;
	}
	/* Gain in units of 1/10 lx/uA */
	u = lrint(gain * 10.0);
	/* Write page 3 byte 2&3 for illuminance gain -- Egil Kvaleberg */
	return GB_to_Z_OR_E(OW_w_int(u, (3 << 3) + 2, PN(owq))) ;
}

/* DS2438 read page (8 bytes) */
/* p is 8 bytes long */
static GOOD_OR_BAD OW_r_page(BYTE * p, const int page, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE recall[] = { _1W_RECALL_SCRATCHPAD, page, };
	BYTE r[] = { _1W_READ_SCRATCHPAD, page, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(recall),
		TRXN_START,
		TRXN_WRITE2(r),
		TRXN_READ(data, 9),
		TRXN_CRC8(data, 9),
		TRXN_END,
	};

	// read to scratch, then in
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	// copy to buffer
	memcpy(p, data, 8);
	return gbGOOD;
}

/* write 8 bytes */
/* p is 8 bytes long */
static GOOD_OR_BAD OW_w_page(const BYTE * p, const int page, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE w[] = { _1W_WRITE_SCRATCHPAD, page, };
	BYTE r[] = { _1W_READ_SCRATCHPAD, page, };
	BYTE eeprom[] = { _1W_COPY_SCRATCHPAD, page, };
	struct transaction_log t[] = {
		TRXN_START,				// 0
		TRXN_WRITE2(w),
		TRXN_WRITE(p, 8),
		TRXN_START,
		TRXN_WRITE2(r),
		TRXN_READ(data, 9),
		TRXN_CRC8(data, 9),
		TRXN_START,
		TRXN_WRITE2(eeprom),
		TRXN_DELAY(10), // 10 msec
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_latesttemp(_FLOAT * T, const struct parsedname *pn)
{
	BYTE data[9];

	// read back registers
	RETURN_BAD_IF_BAD(OW_r_page(data, 0, pn)) ;

	//*T = ((int)((signed char)data[2])) + .00390625*data[1] ;
	T[0] = UT_int16(&data[1]) / 256.0;
	return gbGOOD;
}

static GOOD_OR_BAD OW_temp(_FLOAT * T, int simul_good, const struct parsedname *pn)
{
	UINT delay = 10 ;
	static BYTE t[] = { _1W_CONVERT_T, };
	struct transaction_log tconvert[] = {
		TRXN_START,
		TRXN_WRITE1(t),
		TRXN_DELAY(delay),
		TRXN_END,
	};
	// write conversion command
	if ( simul_good ) {
		RETURN_BAD_IF_BAD( FS_Test_Simultaneous( SlaveSpecificTag(S_T), delay, pn) ) ;
	} else {
		RETURN_BAD_IF_BAD(BUS_transaction(tconvert, pn)) ;
	}

	return OW_latesttemp( T, pn );
}

static GOOD_OR_BAD OW_set_AD( enum voltage_source src, const struct parsedname *pn)
{
	// src deserves some explanation:
	//   1 -- VDD (battery) measured
	//   0 -- VAD (other) measured
	BYTE data[1];
	BYTE r[] = { _1W_READ_SCRATCHPAD, 0, }; // page 0
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE2(r),
		TRXN_READ(data, 1),
		TRXN_END,
	};
	BYTE w[] = { _1W_WRITE_SCRATCHPAD, 0, }; // page 0
	struct transaction_log twrite[] = {
		TRXN_START,				// 0
		TRXN_WRITE2(w),
		TRXN_WRITE(data, 1),
		TRXN_END,
	};

	// read to scratch, then in
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;

	if ( UT_getbit( data, 3 ) == (BYTE) src ) {
		// correct setting already. Leave there
		return gbGOOD ;
	}

	UT_setbit( data, 3, (BYTE) src ) ;

	return BUS_transaction(twrite, pn) ;
}

static GOOD_OR_BAD OW_volts(_FLOAT * V, enum voltage_source src, const struct parsedname *pn)
{
	// src deserves some explanation:
	//   1 -- VDD (battery) measured
	//   0 -- VAD (other) measured
	BYTE data[9];
	static BYTE v[] = { _1W_CONVERT_V, };
	struct transaction_log tconvert[] = {
		TRXN_START,
		TRXN_WRITE1(v),
		TRXN_DELAY(10), // 10 ms
		TRXN_END,
	};

	// set voltage source command
	RETURN_BAD_IF_BAD( OW_set_AD( src, pn ) );

	// write conversion command
	RETURN_BAD_IF_BAD(BUS_transaction(tconvert, pn)) ;

	// read back registers
	RETURN_BAD_IF_BAD(OW_r_page(data, 0, pn));

	V[0] = .01 * (_FLOAT) UT_int16(&data[3]);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_offset(const int I, const struct parsedname *pn)
{
	BYTE data[8];
	int current_conversion_enabled;

	// set current readings off source command
	RETURN_BAD_IF_BAD(OW_r_page(data, 0, pn));
	current_conversion_enabled = UT_getbit(data, 0);
	if (current_conversion_enabled) {
		UT_setbit(data, 0, 0);	// AD bit in status register
		RETURN_BAD_IF_BAD(OW_w_page(data, 0, pn));
	}
	// read back registers
	RETURN_BAD_IF_BAD(OW_w_int(I, 0x0D, pn));

	if (current_conversion_enabled) {
		// if ( OW_r_page( data , 0 , pn ) ) return 1 ; /* Assume no change to these fields */
		UT_setbit(data, 0, 1);	// AD bit in status register
		RETURN_BAD_IF_BAD(OW_w_page(data, 0, pn));
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_r_int(int *I, const UINT address, const struct parsedname *pn)
{
	BYTE data[8];

	// read back registers
	RETURN_BAD_IF_BAD(OW_r_page(data, DS2438_ADDRESS_TO_PAGE(address), pn));
	*I = (int) UT_int16(&data[DS2438_ADDRESS_TO_OFFSET(address)]);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_int(const int I, const UINT address, const struct parsedname *pn)
{
	BYTE data[8];

	// write 16bit int
	RETURN_BAD_IF_BAD(OW_r_page(data, DS2438_ADDRESS_TO_PAGE(address), pn)) ;
	data[DS2438_ADDRESS_TO_OFFSET(address)] = BYTE_MASK(I);
	data[DS2438_ADDRESS_TO_OFFSET(address+1) & 0x07] = BYTE_MASK(I >> 8);
	return OW_w_page(data, DS2438_ADDRESS_TO_PAGE(address), pn);
}

static GOOD_OR_BAD OW_r_uint(UINT *U, const UINT address, const struct parsedname *pn)
{
	BYTE data[8];

	// read back registers
	RETURN_BAD_IF_BAD( OW_r_page(data, address >> 3, pn) ) ;
	*U = ((UINT) (data[(address + 1) & 0x07])) << 8 | data[address & 0x07];
	return gbGOOD;
}
