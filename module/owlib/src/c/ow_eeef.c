/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_eeef.h"
#include <math.h>

/* ------- Prototypes ----------- */

/* Hobby boards UVI and colleagues */
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_localtype);
READ_FUNCTION(FS_r_sensor);
READ_FUNCTION(FS_r_moist);
WRITE_FUNCTION(FS_w_moist);
READ_FUNCTION(FS_r_leaf);
WRITE_FUNCTION(FS_w_leaf);
READ_FUNCTION(FS_r_cal);
WRITE_FUNCTION(FS_w_cal);
READ_FUNCTION(FS_r_raw);
READ_FUNCTION(FS_r_hub_config);
WRITE_FUNCTION(FS_w_hub_config);
READ_FUNCTION(FS_r_channels);
WRITE_FUNCTION(FS_w_channels);
READ_FUNCTION(FS_short);

READ_FUNCTION(FS_r_variable);
WRITE_FUNCTION(FS_w_variable);

static enum e_visibility VISIBLE_EF_UVI( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EF_MOISTURE( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EF_HUB( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EF_BAROMETER( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EF_HUMIDITY( const struct parsedname * pn ) ;

enum e_EF_type {
	eft_UVI = 1,
	eft_MOI = 2,
	eft_LOG = 3,
	eft_SNF = 4,
	eft_HUB = 5,
	eft_BAR = 6,
	eft_HUM = 7,
} ;

enum e_cal_type {
	cal_min ,
	cal_max ,
} ;

#define _EEEF_BLANK	0x00

#define _EEEF_READ_VERSION 0x11
#define _EEEF_READ_TYPE 0x12
#define _EEEF_GET_TEMPERATURE 0x21
#define _EEEF_SET_TEMPERATURE_OFFSET 0x22
#define _EEEF_GET_TEMPERATURE_OFFSET 0x23
#define _EEEF_GET_UVI 0x24
#define _EEEF_SET_UVI_OFFSET 0x25
#define _EEEF_GET_UVI_OFFSET 0x26
#define _EEEF_SET_IN_CASE 0x27
#define _EEEF_GET_IN_CASE 0x28

#define _EEEF_GET_POLLING_FREQUENCY 0x14
#define _EEEF_SET_POLLING_FREQUENCY 0x94
#define _EEEF_GET_AVAILABLE_POLLING_FREQUENCIES 0x15

#define _EEEF_GET_LOCATION 0x16
#define _EEEF_SET_LOCATION 0x96

#define _EEEF_GET_CONFIG 0x61
#define _EEEF_SET_CONFIG 0xE1

#define _EEEF_REBOOT 0xF1
#define _EEEF_RESET_TO_FACTORY_DEFAULTS 0xF7

#define _EEEF_READ_SENSOR 0x21
#define _EEEF_SET_LEAF_OLD 0x22
#define _EEEF_SET_LEAF_NEW 0xA2
#define _EEEF_GET_LEAF_OLD 0x23
#define _EEEF_GET_LEAF_NEW 0x22
#define _EEEF_SET_LEAF_MAX_OLD 0x24
#define _EEEF_SET_LEAF_MAX_NEW 0xA3
#define _EEEF_GET_LEAF_MAX_OLD 0x25
#define _EEEF_GET_LEAF_MAX_NEW 0x23
#define _EEEF_SET_LEAF_MIN_OLD 0x26
#define _EEEF_SET_LEAF_MIN_NEW 0xA4
#define _EEEF_GET_LEAF_MIN_OLD 0x27
#define _EEEF_GET_LEAF_MIN_NEW 0x24
#define _EEEF_GET_LEAF_RAW_OLD 0x31
#define _EEEF_GET_LEAF_RAW_NEW 0x31

#define _EEEF_HUB_SET_CHANNELS 0x21
#define _EEEF_HUB_GET_CHANNELS_ACTIVE 0x22
#define _EEEF_HUB_GET_CHANNELS_SHORTED 0x23
#define _EEEF_HUB_SET_CONFIG 0x60
#define _EEEF_HUB_GET_CONFIG 0x61
#define _EEEF_HUB_SINGLE_CHANNEL_BIT 0x02
#define _EEEF_HUB_SET_CHANNEL_BIT 0x10
#define _EEEF_HUB_SET_CHANNEL_MASK 0x0F

#define _EEEF_HB_TEMPERATURE_C 0x40
#define _EEEF_GET_KPA 0x22

#define _EEEF_GET_PRESSURE_ALTITUDE 0x24
#define _EEEF_SET_ALTITUDE 0xA6
#define _EEEF_GET_ALTITUDE 0x26

#define _EEEF_GET_TC_HUMIDITY 0x21
#define _EEEF_GET_RAW_HUMIDITY 0x22
#define _EEEF_GET_TC_HUMIDITY_OFFSET 0x24
#define _EEEF_SET_TC_HUMIDITY_OFFSET 0xA4

struct location_pair {
	BYTE read ;
	BYTE write ;
	int size ;
	enum var_type {
		vt_unsigned, 
		vt_signed,
		vt_location,
		vt_command,
		vt_ee_temperature,
		vt_temperature,
		vt_pressure,
		vt_humidity,
		vt_uvi,
		vt_incase,
	} type ;
} ;

struct location_pair lp_type_number = {
	_EEEF_READ_TYPE,
	_EEEF_BLANK,
	1,
	vt_unsigned,
} ;
struct location_pair lp_version_number = {
	_EEEF_READ_VERSION,
	_EEEF_BLANK,
	2,
	vt_unsigned,
} ;
struct location_pair lp_config = {
	_EEEF_GET_CONFIG,
	_EEEF_SET_CONFIG,
	1,
	vt_unsigned,
} ;
struct location_pair lp_polling = {
	_EEEF_GET_POLLING_FREQUENCY,
	_EEEF_SET_POLLING_FREQUENCY,
	1,
	vt_unsigned,
} ;
struct location_pair lp_available_polling = {
	_EEEF_GET_AVAILABLE_POLLING_FREQUENCIES,
	_EEEF_BLANK,
	1,
	vt_unsigned,
} ;
struct location_pair lp_factory = {
	_EEEF_BLANK,
	_EEEF_RESET_TO_FACTORY_DEFAULTS,
	0,
	vt_command,
} ;
struct location_pair lp_reboot = {
	_EEEF_BLANK,
	_EEEF_REBOOT,
	0,
	vt_command,
} ;
struct location_pair lp_hb_temperature = {
	_EEEF_HB_TEMPERATURE_C,
	_EEEF_BLANK,
	2,
	vt_temperature,
} ;
struct location_pair lp_ee_temperature = {
	_EEEF_GET_TEMPERATURE,
	_EEEF_BLANK,
	2,
	vt_ee_temperature,
} ;
struct location_pair lp_ee_temperature_offset = {
	_EEEF_GET_TEMPERATURE_OFFSET,
	_EEEF_SET_TEMPERATURE_OFFSET,
	2,
	vt_ee_temperature,
} ;
struct location_pair lp_uvi = {
	_EEEF_GET_UVI,
	_EEEF_BLANK,
	1,
	vt_uvi,
} ;
struct location_pair lp_uvi_offset = {
	_EEEF_GET_UVI_OFFSET,
	_EEEF_SET_UVI_OFFSET,
	1,
	vt_uvi,
} ;
struct location_pair lp_incase = {
	_EEEF_GET_IN_CASE,
	_EEEF_SET_IN_CASE,
	1,
	vt_incase,
} ;
struct location_pair lp_pressure = {
	_EEEF_GET_KPA,
	_EEEF_BLANK,
	2,
	vt_pressure,
} ;
struct location_pair lp_pressurealtitude = {
	_EEEF_GET_PRESSURE_ALTITUDE,
	_EEEF_BLANK,
	2,
	vt_signed,
} ;
struct location_pair lp_altitude = {
	_EEEF_GET_ALTITUDE,
	_EEEF_SET_ALTITUDE,
	2,
	vt_signed,
} ;

struct location_pair lp_raw_humidity = {
	_EEEF_GET_RAW_HUMIDITY,
	_EEEF_BLANK,
	2,
	vt_humidity,
} ;
struct location_pair lp_tc_humidity = {
	_EEEF_GET_TC_HUMIDITY,
	_EEEF_BLANK,
	2,
	vt_humidity,
} ;
struct location_pair lp_tc_humidity_offset = {
	_EEEF_GET_TC_HUMIDITY_OFFSET,
	_EEEF_SET_TC_HUMIDITY_OFFSET,
	2,
	vt_humidity,
} ;

#define _EEEF_LOCATION_LENGTH 21
struct location_pair lp_location = {
	_EEEF_GET_LOCATION,
	_EEEF_SET_LOCATION,
	_EEEF_LOCATION_LENGTH,
	vt_location,
} ;

#define _EEEF_version_length  7

/* ------- Structures ----------- */
static struct filetype HobbyBoards_EE[] = {
	F_STANDARD_NO_TYPE,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, {v:&lp_ee_temperature,}, },
	{"temperature_offset", PROPERTY_LENGTH_TEMPGAP, NON_AGGREGATE, ft_tempgap, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_UVI, {v:&lp_ee_temperature_offset}, },
	{"version", _EEEF_version_length, NON_AGGREGATE, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"version_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, INVISIBLE, {v:&lp_version_number}, },
	{"type_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE, {v:&lp_type_number}, },
	{"type", PROPERTY_LENGTH_TYPE, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"UVI", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"UVI/UVI", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, {v:&lp_uvi}, },
	{"UVI/UVI_offset", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_UVI, {v:&lp_uvi_offset}, },
	{"UVI/in_case", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_UVI, {v:&lp_incase}, },
};

DeviceEntry(EE, HobbyBoards_EE, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate AMOIST = { 4, ag_numbers, ag_aggregate, };
static struct aggregate AHUB = { 4, ag_numbers, ag_aggregate, };
static struct filetype HobbyBoards_EF[] = {
	F_STANDARD_NO_TYPE,
	{"version", _EEEF_version_length, NON_AGGREGATE, ft_ascii, fc_link, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"version_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, INVISIBLE, {v:&lp_version_number}, },
	{"type_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE, {v:&lp_type_number}, },
	{"type", PROPERTY_LENGTH_TYPE, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	
	{"moisture", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_MOISTURE, NO_FILETYPE_DATA, },
	{"moisture/sensor", PROPERTY_LENGTH_INTEGER, &AMOIST, ft_integer, fc_volatile, FS_r_sensor, NO_WRITE_FUNCTION, VISIBLE_EF_MOISTURE, NO_FILETYPE_DATA, },
	{"moisture/is_moisture", PROPERTY_LENGTH_BITFIELD, &AMOIST, ft_bitfield, fc_stable, FS_r_moist, FS_w_moist, VISIBLE_EF_MOISTURE, NO_FILETYPE_DATA, },
	{"moisture/is_leaf", PROPERTY_LENGTH_BITFIELD, &AMOIST, ft_bitfield, fc_link, FS_r_leaf, FS_w_leaf, VISIBLE_EF_MOISTURE, NO_FILETYPE_DATA, },
	{"moisture/calibrate", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_MOISTURE, NO_FILETYPE_DATA, },
	{"moisture/calibrate/min", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_cal, FS_w_cal, VISIBLE_EF_MOISTURE, {i:cal_min,}, },
	{"moisture/calibrate/max", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_cal, FS_w_cal, VISIBLE_EF_MOISTURE, {i:cal_max,}, },
	{"moisture/calibrate/raw", PROPERTY_LENGTH_UNSIGNED, &AMOIST, ft_unsigned, fc_volatile, FS_r_raw, NO_WRITE_FUNCTION, VISIBLE_EF_MOISTURE, NO_FILETYPE_DATA, },
	
	{"humidity", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_HUMIDITY, NO_FILETYPE_DATA, },
	{"humidity/polling_frequency", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_polling, }, } ,
	{"humidity/polling_available", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_HUMIDITY, {v:&lp_available_polling, }, } ,
	{"humidity/location", _EEEF_LOCATION_LENGTH, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_location,}, } ,
	{"humidity/config", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_config, }, } ,
	{"humidity/reboot", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_reboot,}, } ,
	{"humidity/reset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_factory,}, } ,
	{"humidity/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_HUMIDITY, {v:&lp_hb_temperature,}, } ,
	{"humidity/humidity_raw", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_HUMIDITY, {v:&lp_raw_humidity,}, } ,
	{"humidity/humidity_corrected", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_HUMIDITY, {v:&lp_tc_humidity,}, } ,
	{"humidity/humidity_offset", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_variable, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_tc_humidity_offset,}, } ,
	
	{"barometer", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, NO_FILETYPE_DATA, },
	{"barometer/polling_frequency", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_polling, }, } ,
	{"barometer/polling_available", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, {v:&lp_available_polling, }, } ,
	{"barometer/location", _EEEF_LOCATION_LENGTH, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_location,}, } ,
	{"barometer/config", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_config, }, } ,
	{"barometer/reboot", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_reboot,}, } ,
	{"barometer/reset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_factory,}, } ,
	{"barometer/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, {v:&lp_hb_temperature,}, } ,
	{"barometer/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, {v:&lp_pressure,}, } ,
	{"barometer/pressure_altitude", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, {v:&lp_pressurealtitude,}, } ,
	{"barometer/altitude", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_variable, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_altitude,}, } ,
	
	{"hub", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_HUB, NO_FILETYPE_DATA, },
	{"hub/config", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_hub_config, FS_w_hub_config, INVISIBLE, NO_FILETYPE_DATA, },
	{"hub/branch", PROPERTY_LENGTH_BITFIELD, &AHUB, ft_bitfield, fc_stable, FS_r_channels, FS_w_channels, VISIBLE_EF_HUB, NO_FILETYPE_DATA, },
	{"hub/short", PROPERTY_LENGTH_BITFIELD, &AHUB, ft_bitfield, fc_stable, FS_short, NO_WRITE_FUNCTION, VISIBLE_EF_HUB, NO_FILETYPE_DATA, },
};

DeviceEntry(EF, HobbyBoards_EF, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* Internal properties */
Make_SlaveSpecificTag(VER, fc_stable);	// version
#define EFversion(maj,min) (maj*256+min)

static enum e_EF_type VISIBLE_EF( const struct parsedname * pn ) ;

/* finds the visibility value (0x0071 ...) either cached, or computed via the device_id (then cached) */
static enum e_EF_type VISIBLE_EF( const struct parsedname * pn )
{
	int eft = -1 ;
	
	LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
	if ( BAD( GetVisibilityCache( &eft, pn ) ) ) {
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		if ( owq != NULL) {
			UINT U_eft ;
			if ( FS_r_sibling_U( &U_eft, "type_number", owq ) == 0 ) {
				eft = U_eft ;
				SetVisibilityCache( eft, pn ) ;
			}
			OWQ_destroy(owq) ;
		}
	}
	return (enum e_EF_type) eft ;
}

static enum e_visibility VISIBLE_EF_UVI( const struct parsedname * pn )
{
	switch ( VISIBLE_EF(pn) ) {
		case eft_UVI:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EF_MOISTURE( const struct parsedname * pn )
{
	switch ( VISIBLE_EF(pn) ) {
		case eft_MOI:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EF_BAROMETER( const struct parsedname * pn )
{
	switch ( VISIBLE_EF(pn) ) {
		case eft_BAR:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EF_HUMIDITY( const struct parsedname * pn )
{
	switch ( VISIBLE_EF(pn) ) {
		case eft_HUM:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EF_HUB( const struct parsedname * pn )
{
	switch ( VISIBLE_EF(pn) ) {
		case eft_HUB:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

/* ------- Functions ------------ */

/* prototypes */
static GOOD_OR_BAD OW_read(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn) ;
static GOOD_OR_BAD OW_write(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn);

static GOOD_OR_BAD OW_r_doubles( BYTE command, UINT * dubs, int elements, struct parsedname * pn ) ;
static GOOD_OR_BAD OW_w_doubles( BYTE command, UINT * dubs, int elements, struct parsedname * pn ) ;

#ifdef HAVE_LRINT
    #define Roundoff(x) (UINT) lrint(x)
#else
    #define Roundoff(x) (UINT) (x+.49)
#endif

// returns major/minor as 2 hex bytes (ascii)
static ZERO_OR_ERROR FS_version(struct one_wire_query *owq)
{
    char v[_EEEF_version_length];
    BYTE major, minor ;
    int ret_size ;
    UINT version_number ;
    if ( FS_r_sibling_U( &version_number, "version_number", owq ) != 0 ) {
        return -EINVAL ;
    }

	minor = version_number & 0xFF ;
	major = (version_number>>8) & 0xFF ;

    UCLIBCLOCK;
    ret_size = snprintf(v,_EEEF_version_length,"%u.%u",major,minor);
    UCLIBCUNLOCK;

    return OWQ_format_output_offset_and_size(v, ret_size, owq);
}

static ZERO_OR_ERROR FS_localtype(struct one_wire_query *owq)
{
    UINT type_number ;
    if ( FS_r_sibling_U( &type_number, "type_number", owq ) != 0 ) {
        return -EINVAL ;
    }

    switch ((enum e_EF_type) type_number) {
        case eft_UVI:
            return OWQ_format_output_offset_and_size_z("HB_UVI_METER", owq) ;
        case eft_MOI:
            return OWQ_format_output_offset_and_size_z("HB_MOISTURE_METER", owq) ;
        case eft_LOG:
            return OWQ_format_output_offset_and_size_z("HB_MOISTURE_METER_DATALOGGER", owq) ;
        case eft_SNF:
            return OWQ_format_output_offset_and_size_z("HB_SNIFFER", owq) ;
        case eft_HUB:
            return OWQ_format_output_offset_and_size_z("HB_HUB", owq) ;
        default:
            return FS_type(owq);
    }
}

static ZERO_OR_ERROR FS_r_sensor(struct one_wire_query *owq)
{
	BYTE w[4] ;
	
	RETURN_ERROR_IF_BAD( OW_read( _EEEF_READ_SENSOR, w, 4, PN(owq) ) ) ;

	OWQ_array_I(owq, 0) = (uint8_t) w[0] ;
	OWQ_array_I(owq, 1) = (uint8_t) w[1] ;
	OWQ_array_I(owq, 2) = (uint8_t) w[2] ;
	OWQ_array_I(owq, 3) = (uint8_t) w[3] ;

	return 0 ;
}

// bitfield for each channel
// 0 = watermark moisture sensor
// 1 = leaf wetness sensor
static ZERO_OR_ERROR FS_r_moist(struct one_wire_query *owq)
{
    BYTE moist ;
    BYTE cmd ;
    UINT version ;
    struct parsedname * pn = PN(owq) ;
    
    if ( BAD( Cache_Get_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn) ) ) {
		if ( FS_r_sibling_U( &version, "version_number", owq ) != 0 ) {
			return -EINVAL ;
		}
		Cache_Add_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn) ;
	}

    cmd = ( version>=EFversion(1,80) ) ? _EEEF_GET_LEAF_NEW : _EEEF_GET_LEAF_OLD ;
    
    if ( BAD( OW_read( cmd, &moist, 1, pn ) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = (~moist) & 0x0F ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_moist(struct one_wire_query *owq)
{
    BYTE moist = (~OWQ_U(owq)) & 0x0F ;
    BYTE cmd ;
    UINT version ;
    struct parsedname * pn = PN(owq) ;
    
    if ( BAD( Cache_Get_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn) ) ) {
		if ( FS_r_sibling_U( &version, "version_number", owq ) != 0 ) {
			return -EINVAL ;
		}
		Cache_Add_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn);
	}

    cmd = ( version>=EFversion(1,80) ) ? _EEEF_SET_LEAF_NEW : _EEEF_SET_LEAF_OLD ;
    
    return GB_to_Z_OR_E( OW_write( cmd, &moist, 1, pn)) ;
}

static ZERO_OR_ERROR FS_r_leaf(struct one_wire_query *owq)
{
    UINT moist ;
    
    if ( FS_r_sibling_U( &moist, "moisture/is_moisture.BYTE", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = (~moist) & 0x0F ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_leaf(struct one_wire_query *owq)
{
    UINT moist = (~OWQ_U(owq)) & 0x0F  ;
    
    return FS_w_sibling_U( moist, "moisture/is_moisture.BYTE", owq ) ;
}

static ZERO_OR_ERROR FS_r_cal(struct one_wire_query *owq)
{
    BYTE cmd ;
    UINT version ;
    struct parsedname * pn = PN(owq) ;
    
    if ( BAD( Cache_Get_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn) ) ) {
		if ( FS_r_sibling_U( &version, "version_number", owq ) != 0 ) {
			return -EINVAL ;
		}
		Cache_Add_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn);
	}
	
	switch( (enum e_cal_type) PN(owq)->selected_filetype->data.i ) {
		case cal_min:
			cmd = ( version>=EFversion(1,80) ) ? _EEEF_GET_LEAF_MIN_NEW : _EEEF_GET_LEAF_MIN_OLD ;
			break ;
		case cal_max:
			cmd = ( version>=EFversion(1,80) ) ? _EEEF_GET_LEAF_MAX_NEW : _EEEF_GET_LEAF_MAX_OLD ;
			break ;
		default:
			return -EINVAL ;
	}
			
    return GB_to_Z_OR_E( OW_r_doubles( cmd, &OWQ_U(owq), 1, pn)) ;
}

static ZERO_OR_ERROR FS_r_variable(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	struct filetype * ft = pn->selected_filetype ;
	struct location_pair * lp = ( struct location_pair *) ft->data.v ;
	BYTE cmd = lp->read ;
	int size = lp->size ;
	BYTE data[size+1] ;
	enum var_type vt = lp->type ;
	
	// clear buffer
	memset( data, 0, size+1 ) ;
	
	// valid command?
	if ( cmd == _EEEF_BLANK ) {
		return -EINVAL ;
	}
	
	// read data
	RETURN_ERROR_IF_BAD( OW_read( cmd, data, size, pn ) ) ;

	switch( vt ) {
		case vt_unsigned:
			switch ( size ) {
				case 1:
					OWQ_U(owq) = UT_uint8( data ) ;
					break ;
				case 2:
					OWQ_U(owq) = UT_uint16( data ) ;
					break ;
				case 4:
					OWQ_U(owq) = UT_uint32( data ) ;
					break ;
				default:
					return -EINVAL ;
			}
			break ;
		case vt_signed:
			switch ( size ) {
				case 1:
					OWQ_I(owq) = UT_int8( data ) ;
					break ;
				case 2:
					OWQ_I(owq) = UT_int16( data ) ;
					break ;
				case 4:
					OWQ_I(owq) = UT_int32( data ) ;
					break ;
				default:
					return -EINVAL ;
			}
			break ;
		case vt_location:
			return OWQ_format_output_offset_and_size_z( (ASCII*)data, owq);
		case vt_humidity:
			// to percent
			OWQ_F(owq) = ( (_FLOAT) (UT_int16(data)) ) * 0.01 ;
			break ;
		case vt_temperature:
			OWQ_F(owq) = ( (_FLOAT) (UT_int8(data)) ) * 0.1 ;
			break ;
		case vt_uvi:
			if ( data[0] == 0xFF ) {
				// error flag
				return -EINVAL ;
			}
			OWQ_F(owq) = ( (_FLOAT) (UT_int16(data)) ) * 0.1 ;
			break ;
		case vt_ee_temperature:
			OWQ_F(owq) = ( (_FLOAT) (UT_int16(data)) ) * 0.5 ;
			break ;
		case vt_pressure:
			// kPa to mbar
			OWQ_F(owq) = ( (_FLOAT) (UT_int16(data)) ) * 10.0 ;
			break ;
		case vt_incase:
			OWQ_Y(owq) = (data[0]!=0x00) ;
			break ;
		case vt_command:
		default:
			break ;
	}

	return 0 ;
}

static ZERO_OR_ERROR FS_w_variable(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	struct filetype * ft = pn->selected_filetype ;
	struct location_pair * lp = ( struct location_pair *) ft->data.v ;
	BYTE cmd = lp->read ;
	int size = lp->size ;
	BYTE data[size+1] ;
	enum var_type vt = lp->type ;
	
	// clear buffer
	memset( data, 0, size+1 );

	// valid command?
	if ( cmd == _EEEF_BLANK ) {
		return -EINVAL ;
	}
	

	switch( vt ) {
		case vt_signed:
		case vt_unsigned:
			switch ( size ) {
				case 1:
					UT_uint8_to_bytes( OWQ_U(owq), data ) ;
					break ;
				case 2:
					UT_uint16_to_bytes( OWQ_U(owq), data ) ;
					break ;
				case 4:
					UT_uint32_to_bytes( OWQ_U(owq), data ) ;
					break ;
				default:
					return -EINVAL ;
			}
			break ;
		case vt_location:
		{	
			int len = size ;
			if ( len > _EEEF_LOCATION_LENGTH-1 ) {
				len = _EEEF_LOCATION_LENGTH-1 ;
			}
			memcpy( data, OWQ_buffer(owq), len ) ;
		}
			break ;
		case vt_command:
			if ( ! OWQ_Y(owq) ) {
				return 0 ;
			}
			break ;	
		case vt_uvi:
			UT_uint8_to_bytes( Roundoff(OWQ_F(owq) * 10.0), data ) ;
			break ;
		case vt_humidity:
			UT_uint16_to_bytes( Roundoff(OWQ_F(owq) * 100.0), data ) ;
			break ;
		case vt_ee_temperature:
			UT_uint16_to_bytes( Roundoff(OWQ_F(owq) * 2.), data ) ;
			break ;
		case vt_incase:
			data[0] = OWQ_Y(owq) ? 0xFF : 0x00 ;
			break ;
		case vt_temperature:
		case vt_pressure:
			return -EINVAL ;
	}

	// write data
	return GB_to_Z_OR_E( OW_write( cmd, data, size, pn ) ) ;
}

static ZERO_OR_ERROR FS_w_cal(struct one_wire_query *owq)
{
    BYTE cmd ;
    UINT version ;
    struct parsedname * pn = PN(owq) ;
    
    if ( BAD( Cache_Get_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn) ) ) {
		if ( FS_r_sibling_U( &version, "version_number", owq ) != 0 ) {
			return -EINVAL ;
		}
		Cache_Add_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn);
	}
	
	switch( (enum e_cal_type) PN(owq)->selected_filetype->data.i ) {
		case cal_min:
			cmd = ( version>=EFversion(1,80) ) ? _EEEF_SET_LEAF_MIN_NEW : _EEEF_SET_LEAF_MIN_OLD ;
			break ;
		case cal_max:
			cmd = ( version>=EFversion(1,80) ) ? _EEEF_SET_LEAF_MAX_NEW : _EEEF_SET_LEAF_MAX_OLD ;
			break ;
		default:
			return -EINVAL ;
	}
			
    return GB_to_Z_OR_E( OW_w_doubles( cmd, &OWQ_U(owq), 1, pn)) ;
}

static ZERO_OR_ERROR FS_r_raw(struct one_wire_query *owq)
{
	UINT raw[4] ;
    BYTE cmd ;
    UINT version ;
    struct parsedname * pn = PN(owq) ;
    
    if ( BAD( Cache_Get_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn) ) ) {
		if ( FS_r_sibling_U( &version, "version_number", owq ) != 0 ) {
			return -EINVAL ;
		}
		Cache_Add_SlaveSpecific( &version, sizeof(UINT), SlaveSpecificTag(VER), pn);
	}

    cmd = ( version>=EFversion(1,80) ) ? _EEEF_GET_LEAF_RAW_NEW : _EEEF_GET_LEAF_RAW_OLD ;

	if ( BAD( OW_r_doubles( cmd, raw, 4, pn ) ) ) {
		return -EINVAL ;
	}
	
	OWQ_array_U(owq,0) = raw[0] ;
	OWQ_array_U(owq,1) = raw[1] ;
	OWQ_array_U(owq,2) = raw[2] ;
	OWQ_array_U(owq,3) = raw[3] ;
			
    return 0 ;
}

static ZERO_OR_ERROR FS_r_hub_config(struct one_wire_query *owq)
{
    BYTE config ;
    
    if ( BAD( OW_read( _EEEF_HUB_GET_CONFIG, &config, 1, PN(owq) ) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = config ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_hub_config(struct one_wire_query *owq)
{
    BYTE config = OWQ_U(owq) ;

    return GB_to_Z_OR_E( OW_write(_EEEF_HUB_SET_CONFIG, &config, 1, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_channels(struct one_wire_query *owq)
{
    BYTE channels ;
    
    if ( BAD( OW_read( _EEEF_HUB_GET_CHANNELS_ACTIVE, &channels, 1, PN(owq) ) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = channels ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_channels(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
    BYTE channels = OWQ_U(owq) & _EEEF_HUB_SET_CHANNEL_MASK ;
    UINT config ;
    
    // Take out of single channel mode if needed
    // Only if more than one channel selected 
    switch ( channels & 0x0F ) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x04:
		case 0x08:
			// single channel (or none), leave single channels mode alone
			break ;
		default:
			if ( FS_r_sibling_U( &config, "hub/config", owq ) != 0 ) {
				LEVEL_DEBUG("Could not read Hub configuration");
				return -EINVAL ;
			}
			if ( config & _EEEF_HUB_SINGLE_CHANNEL_BIT ) {
				// need to clean single channel
				if ( FS_w_sibling_U( config & ~_EEEF_HUB_SINGLE_CHANNEL_BIT, "hub/config", owq ) != 0 ) {
					LEVEL_DEBUG("Could not write Hub configuration");
					return -EINVAL ;
				}
			}
			break ;
	}

	// Clear directory cache
	Cache_Del_Dir( pn ) ;

	// Set channels
	channels |= _EEEF_HUB_SET_CHANNEL_BIT ;
    RETURN_ERROR_IF_BAD( OW_write(_EEEF_HUB_SET_CHANNELS, &channels, 1, pn) ) ;
    
	// Clear channels
	channels = ~channels ;
    RETURN_ERROR_IF_BAD( OW_write(_EEEF_HUB_SET_CHANNELS, &channels, 1, pn) ) ;

    return 0 ;
}

static ZERO_OR_ERROR FS_short(struct one_wire_query *owq)
{
    BYTE channels ;
    
    if ( BAD( OW_read( _EEEF_HUB_GET_CHANNELS_SHORTED, &channels, 1, PN(owq) ) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = channels ;
	return 0 ;
}

/* Make a change to handle bus master not ready */
static GOOD_OR_BAD OW_read(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn)
{
	int retries ;
    BYTE c[] = { command,} ;
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE1(c),
        TRXN_READ(bytes,size),
        TRXN_END,
    };
    for ( retries = 0 ; retries < 10 ; ++ retries ) {
		RETURN_BAD_IF_BAD( BUS_transaction(t, pn) ) ;
		if ( bytes[0] != 0xFF ) {
			break ;
		}
		LEVEL_DEBUG("Slave "SNformat" apparently not ready to read %d bytes",SNvar(pn->sn),size) ;
	}
	// Maybe that's a real value?
	return gbGOOD ;
}

static GOOD_OR_BAD OW_write(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn)
{
    BYTE c[] = { command, } ;
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE1(c),
        TRXN_WRITE(bytes,size),
        TRXN_END,
    };
    return  BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_r_doubles( BYTE command, UINT * dubs, int elements, struct parsedname * pn ) 
{
	size_t size = 2 * elements ;
	BYTE data[size] ;
	int counter ;
	
	RETURN_BAD_IF_BAD( OW_read( command, data, size, pn ) ) ;
	
	for ( counter = 0 ; counter < elements ; ++counter ) {
		dubs[counter] = data[2*counter] + 256 * data[2*counter+1] ;
	}
	return gbGOOD ;
}

static GOOD_OR_BAD OW_w_doubles( BYTE command, UINT * dubs, int elements, struct parsedname * pn ) 
{
	size_t size = 2 * elements ;
	BYTE data[size] ;
	int counter ;
	
	for ( counter = 0 ; counter < elements ; ++counter ) {
		data[2*counter] = dubs[counter] & 0xFF ;
		data[2*counter+1] = (dubs[counter]>>8) & 0xFF ;
	}

	return OW_write( command, data, size, pn ) ;
}
