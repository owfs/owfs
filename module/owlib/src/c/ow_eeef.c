/*
$Id$
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
READ_FUNCTION(FS_temperature);
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_type_number);
READ_FUNCTION(FS_localtype);
READ_FUNCTION(FS_r_temperature_offset);
WRITE_FUNCTION(FS_w_temperature_offset);
READ_FUNCTION(FS_r_UVI_offset);
WRITE_FUNCTION(FS_w_UVI_offset);
READ_FUNCTION(FS_r_in_case);
WRITE_FUNCTION(FS_w_in_case);
READ_FUNCTION(FS_UVI);
READ_FUNCTION(FS_UVI_valid);
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

READ_FUNCTION(FS_r_location);
WRITE_FUNCTION(FS_w_location);
WRITE_FUNCTION(FS_command);

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
#define _EEEF_READ_TEMPERATURE 0x21
#define _EEEF_SET_TEMPERATURE_OFFSET 0x22
#define _EEEF_READ_TEMPERATURE_OFFSET 0x23
#define _EEEF_READ_UVI 0x24
#define _EEEF_SET_UVI_OFFSET 0x25
#define _EEEF_READ_UVI_OFFSET 0x26
#define _EEEF_SET_IN_CASE 0x27
#define _EEEF_READ_IN_CASE 0x28

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
#define _EEEF_SET_LEAF 0x22
#define _EEEF_GET_LEAF 0x23
#define _EEEF_SET_LEAF_MAX 0x24
#define _EEEF_GET_LEAF_MAX 0x25
#define _EEEF_SET_LEAF_MIN 0x26
#define _EEEF_GET_LEAF_MIN 0x27
#define _EEEF_GET_LEAF_RAW 0x31

#define _EEEF_HUB_SET_CHANNELS 0x21
#define _EEEF_HUB_GET_CHANNELS_ACTIVE 0x22
#define _EEEF_HUB_GET_CHANNELS_SHORTED 0x23
#define _EEEF_HUB_SET_CONFIG 0x60
#define _EEEF_HUB_GET_CONFIG 0x61
#define _EEEF_HUB_SINGLE_CHANNEL_BIT 0x02
#define _EEEF_HUB_SET_CHANNEL_BIT 0x10
#define _EEEF_HUB_SET_CHANNEL_MASK 0x0F

struct location_pair {
	BYTE read ;
	BYTE write ;
	int size ;
	enum var_type { vt_bytes, vt_unsigned, vt_signed, } type ;
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


/* ------- Structures ----------- */
static struct filetype HobbyBoards_EE[] = {
	F_STANDARD_NO_TYPE,
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temperature, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"temperature_offset", PROPERTY_LENGTH_TEMPGAP, NON_AGGREGATE, ft_tempgap, fc_stable, FS_r_temperature_offset, FS_w_temperature_offset, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"version", 5, NON_AGGREGATE, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"type_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_type_number, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"type", PROPERTY_LENGTH_TYPE, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"UVI", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"UVI/UVI", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_UVI, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"UVI/valid", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_UVI_valid, NO_WRITE_FUNCTION, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"UVI/UVI_offset", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_UVI_offset, FS_w_UVI_offset, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
	{"UVI/in_case", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_in_case, FS_w_in_case, VISIBLE_EF_UVI, NO_FILETYPE_DATA, },
};

DeviceEntry(EE, HobbyBoards_EE, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate AMOIST = { 4, ag_numbers, ag_aggregate, };
static struct aggregate AHUB = { 4, ag_numbers, ag_aggregate, };
static struct filetype HobbyBoards_EF[] = {
	F_STANDARD_NO_TYPE,
	{"version", 5, NON_AGGREGATE, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"type_number", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_type_number, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
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
	{"humidity/location", 21, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_location, FS_w_location, VISIBLE_EF_HUMIDITY, NO_FILETYPE_DATA, } ,
	{"humidity/config", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_HUMIDITY, {v:&lp_config, }, } ,
	{"humidity/reboot", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_command, VISIBLE_EF_HUMIDITY, {c:_EEEF_REBOOT,}, } ,
	{"humidity/reset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_command, VISIBLE_EF_HUMIDITY, {c:_EEEF_RESET_TO_FACTORY_DEFAULTS,}, } ,
	
	{"barometer", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, NO_FILETYPE_DATA, },
	{"barometer/polling_frequency", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_polling, }, } ,
	{"barometer/polling_available", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, NO_WRITE_FUNCTION, VISIBLE_EF_BAROMETER, {v:&lp_available_polling, }, } ,
	{"barometer/location", 21, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_location, FS_w_location, VISIBLE_EF_BAROMETER, NO_FILETYPE_DATA, } ,
	{"barometer/config", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_variable, FS_w_variable, VISIBLE_EF_BAROMETER, {v:&lp_config, }, } ,
	{"barometer/reboot", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_command, VISIBLE_EF_BAROMETER, {c:_EEEF_REBOOT,}, } ,
	{"barometer/reset", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_command, VISIBLE_EF_BAROMETER, {c:_EEEF_RESET_TO_FACTORY_DEFAULTS,}, } ,
	
	{"hub", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EF_HUB, NO_FILETYPE_DATA, },
	{"hub/config", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_hub_config, FS_w_hub_config, INVISIBLE, NO_FILETYPE_DATA, },
	{"hub/branch", PROPERTY_LENGTH_BITFIELD, &AHUB, ft_bitfield, fc_stable, FS_r_channels, FS_w_channels, VISIBLE_EF_HUB, NO_FILETYPE_DATA, },
	{"hub/short", PROPERTY_LENGTH_BITFIELD, &AHUB, ft_bitfield, fc_stable, FS_short, NO_WRITE_FUNCTION, VISIBLE_EF_HUB, NO_FILETYPE_DATA, },
};

DeviceEntry(EF, HobbyBoards_EF, NO_GENERIC_READ, NO_GENERIC_WRITE);


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
static GOOD_OR_BAD OW_version(BYTE * major, BYTE * minor, struct parsedname * pn) ;
static GOOD_OR_BAD OW_type(BYTE * type_number, struct parsedname * pn) ;

static GOOD_OR_BAD OW_UVI(_FLOAT * UVI, struct parsedname * pn) ;
static GOOD_OR_BAD OW_r_UVI_offset(_FLOAT * UVI, struct parsedname * pn) ;

static GOOD_OR_BAD OW_temperature(_FLOAT * T, struct parsedname * pn) ;
static GOOD_OR_BAD OW_r_temperature_offset(_FLOAT * T, struct parsedname * pn) ;

static GOOD_OR_BAD OW_read(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn) ;
static GOOD_OR_BAD OW_write(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn);

static GOOD_OR_BAD OW_r_wetness( UINT *wetness, struct parsedname * pn);

static GOOD_OR_BAD OW_command( BYTE cmd, struct parsedname * pn);

static GOOD_OR_BAD OW_r_doubles( BYTE command, UINT * dubs, int elements, struct parsedname * pn ) ;
static GOOD_OR_BAD OW_w_doubles( BYTE command, UINT * dubs, int elements, struct parsedname * pn ) ;

// returns major/minor as 2 hex bytes (ascii)
static ZERO_OR_ERROR FS_version(struct one_wire_query *owq)
{
    char v[6];
    BYTE major, minor ;

    RETURN_ERROR_IF_BAD(OW_version(&major,&minor,PN(owq))) ;

    UCLIBCLOCK;
    snprintf(v,6,"%.2X.%.2X",major,minor);
    UCLIBCUNLOCK;

    return OWQ_format_output_offset_and_size(v, 5, owq);
}

static ZERO_OR_ERROR FS_r_location(struct one_wire_query *owq)
{
    char loc[21];

	memset(loc,0,21);
    RETURN_ERROR_IF_BAD(OW_read(_EEEF_GET_LOCATION, (BYTE *)loc,20,PN(owq))) ;

    return OWQ_format_output_offset_and_size_z(loc, owq);
}

static ZERO_OR_ERROR FS_w_location(struct one_wire_query *owq)
{
    char loc[21];
    size_t len = OWQ_length(owq) ;
    
    if ( len > 20 ) {
		len = 20 ;
	}

	memset(loc,0,21);
	memcpy( loc, OWQ_buffer(owq), len ) ;
    return GB_to_Z_OR_E( OW_write( _EEEF_SET_LOCATION, (BYTE *)loc, 21, PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_type_number(struct one_wire_query *owq)
{
    BYTE type_number ;

    RETURN_ERROR_IF_BAD(OW_type(&type_number,PN(owq))) ;

    OWQ_U(owq) = type_number ;

    return 0;
}

static ZERO_OR_ERROR FS_command( struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE cmd = pn->selected_filetype->data.c ;
	if ( OWQ_Y(owq) ) {
		return GB_to_Z_OR_E( OW_command( cmd, pn ) ) ;
	}
	return 0 ;
}

static ZERO_OR_ERROR FS_temperature(struct one_wire_query *owq)
{
    _FLOAT T ;

    RETURN_ERROR_IF_BAD(OW_temperature(&T,PN(owq))) ;

    OWQ_F(owq) = T ;

    return 0;
}

static ZERO_OR_ERROR FS_UVI(struct one_wire_query *owq)
{
    _FLOAT UVI ;

    RETURN_ERROR_IF_BAD( OW_UVI(&UVI,PN(owq))) ;

    OWQ_F(owq) = UVI ;

    return 0;
}

static ZERO_OR_ERROR FS_r_UVI_offset(struct one_wire_query *owq)
{
    _FLOAT UVI ;

    RETURN_ERROR_IF_BAD(OW_r_UVI_offset(&UVI,PN(owq))) ;

    OWQ_F(owq) = UVI ;

    return 0;
}

static ZERO_OR_ERROR FS_r_temperature_offset(struct one_wire_query *owq)
{
    _FLOAT T ;

    RETURN_BAD_IF_BAD(OW_r_temperature_offset(&T,PN(owq))) ;

    OWQ_F(owq) = T ;

    return 0;
}

static ZERO_OR_ERROR FS_w_temperature_offset(struct one_wire_query *owq)
{
    BYTE c ;

#ifdef HAVE_LRINT
    c = lrint(2.0*OWQ_F(owq));    // round off
#else
    c = 2.0*OWQ_F(owq);
#endif

    RETURN_ERROR_IF_BAD(OW_write(_EEEF_SET_TEMPERATURE_OFFSET, &c, 1, PN(owq))) ;

    return 0;
}

static ZERO_OR_ERROR FS_w_UVI_offset(struct one_wire_query *owq)
{
    BYTE c ;

#ifdef HAVE_LRINT
    c = lrint(10.0*OWQ_F(owq));    // round off
#else
    c = 10.0*OWQ_F(owq)+.49;
#endif

    return GB_to_Z_OR_E(OW_write(_EEEF_SET_UVI_OFFSET, &c, 1, PN(owq))) ;
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

static ZERO_OR_ERROR FS_UVI_valid(struct one_wire_query *owq)
{
    UINT type_number = 0 ;
    ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &type_number, "type_number", owq ) ;

    switch (type_number) {
        case 0x01:
            OWQ_Y(owq) = 1 ;
            break ;
        default:
            OWQ_Y(owq) = 0 ;
            break ;
    }
    return z_or_e ;
}

static ZERO_OR_ERROR FS_r_in_case(struct one_wire_query *owq)
{
    BYTE in_case ;
    
	RETURN_ERROR_IF_BAD( OW_read(_EEEF_READ_IN_CASE,&in_case,1,PN(owq))) ;

    switch(in_case) {
        case 0x00:
            OWQ_Y(owq) = 0 ;
            break ;
        case 0xFF:
            OWQ_Y(owq) = 1 ;
            break ;
        default:
            return -EINVAL ;
    }
    return 0 ;
}

static ZERO_OR_ERROR FS_w_in_case(struct one_wire_query *owq)
{
    BYTE in_case = OWQ_Y(owq) ? 0xFF : 0x00 ;
    return GB_to_Z_OR_E( OW_write(_EEEF_SET_IN_CASE, &in_case, 1, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_sensor(struct one_wire_query *owq)
{
	UINT w[4] ;
	
	RETURN_ERROR_IF_BAD( OW_r_wetness( w, PN(owq) ) ) ;
	
	OWQ_array_I(owq, 0) = w[0] ;
	OWQ_array_I(owq, 1) = w[1] ;
	OWQ_array_I(owq, 2) = w[2] ;
	OWQ_array_I(owq, 3) = w[3] ;

	return 0 ;
}

// bitfield for each channel
// 0 = watermark moisture sensor
// 1 = leaf wetness sensor
static ZERO_OR_ERROR FS_r_moist(struct one_wire_query *owq)
{
    BYTE moist ;
    
    if ( BAD( OW_read( _EEEF_GET_LEAF, &moist, 1, PN(owq) ) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = (~moist) & 0x0F ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_moist(struct one_wire_query *owq)
{
    BYTE moist = (~OWQ_U(owq)) & 0x0F ;
    return GB_to_Z_OR_E( OW_write(_EEEF_SET_LEAF, &moist, 1, PN(owq))) ;
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
	BYTE command ;
	
	switch( (enum e_cal_type) PN(owq)->selected_filetype->data.i ) {
		case cal_min:
			command = _EEEF_GET_LEAF_MIN ;
			break ;
		case cal_max:
			command = _EEEF_GET_LEAF_MAX ;
			break ;
		default:
			return -EINVAL ;
	}
			
    return GB_to_Z_OR_E( OW_r_doubles( command, &OWQ_U(owq), 1, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_variable(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	struct filetype * ft = pn->selected_filetype ;
	struct location_pair * lp = ( struct location_pair *) ft->data.v ;
	BYTE cmd = lp->read ;
	int size = lp->size ;
	BYTE data[size] ;
	enum var_type vt = lp->type ;
	
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
		case vt_signed:
			switch ( size ) {
				case 1:
					OWQ_U(owq) = UT_int8( data ) ;
					break ;
				case 2:
					OWQ_U(owq) = UT_int16( data ) ;
					break ;
				case 4:
					OWQ_U(owq) = UT_int32( data ) ;
					break ;
				default:
					return -EINVAL ;
			}
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
	BYTE data[size] ;
	enum var_type vt = lp->type ;
	
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
	}

	// write data
	return GB_to_Z_OR_E( OW_write( cmd, data, size, pn ) ) ;
}

static ZERO_OR_ERROR FS_w_cal(struct one_wire_query *owq)
{
	BYTE command ;
	
	switch( (enum e_cal_type) PN(owq)->selected_filetype->data.i ) {
		case cal_min:
			command = _EEEF_SET_LEAF_MIN ;
			break ;
		case cal_max:
			command = _EEEF_SET_LEAF_MAX ;
			break ;
		default:
			return -EINVAL ;
	}
			
    return GB_to_Z_OR_E( OW_w_doubles( command, &OWQ_U(owq), 1, PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_raw(struct one_wire_query *owq)
{
	UINT raw[4] ;

	if ( BAD( OW_r_doubles( _EEEF_GET_LEAF_RAW, raw, 4, PN(owq) ) ) ) {
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

static GOOD_OR_BAD OW_version(BYTE * major, BYTE * minor, struct parsedname * pn)
{
    BYTE response[2] ;
    
	RETURN_BAD_IF_BAD(OW_read(_EEEF_READ_VERSION, response, 2, pn)) ;

    *minor = response[0] ;
    *major = response[1] ;
    return gbGOOD ;
}

static GOOD_OR_BAD OW_type(BYTE * type_number, struct parsedname * pn)
{
    
	RETURN_BAD_IF_BAD(OW_read(_EEEF_READ_TYPE, type_number, 1, pn)) ;
    return gbGOOD ;
}

static GOOD_OR_BAD OW_UVI(_FLOAT * UVI, struct parsedname * pn)
{
    BYTE u[1] ;
    
	RETURN_BAD_IF_BAD( OW_read(_EEEF_READ_UVI, u, 1, pn) ) ;
	
    if (u[0]==0xFF) {
        return gbBAD;
    }
    UVI[0] = 0.1 * ((_FLOAT) u[0]) ;
    return gbGOOD ;
}

static GOOD_OR_BAD OW_r_UVI_offset(_FLOAT * UVI, struct parsedname * pn)
{
    BYTE u[1] ;
    
	RETURN_BAD_IF_BAD( OW_read(_EEEF_READ_UVI_OFFSET, u, 1, pn) );
	
    if (u[0]==0xFF) {
        return gbBAD;
    }
    UVI[0] = 0.1 * ((_FLOAT) ((signed char) u[0])) ;
    return gbGOOD ;
}

static GOOD_OR_BAD OW_r_temperature_offset(_FLOAT * T, struct parsedname * pn)
{
    BYTE t[1] ;
    
	RETURN_BAD_IF_BAD(OW_read(_EEEF_READ_TEMPERATURE_OFFSET, t, 1, pn)) ;

    T[0] = 0.5 * ((_FLOAT) ((signed char)t[0])) ;
    return gbGOOD ;
}

static GOOD_OR_BAD OW_temperature(_FLOAT * temperature, struct parsedname * pn)
{
    BYTE t[2] ;
    
	RETURN_BAD_IF_BAD(OW_read(_EEEF_READ_TEMPERATURE, t, 2, pn)) ;

    temperature[0] = (_FLOAT) 0.5 * UT_int16(t) ;
    return gbGOOD ;
}

static GOOD_OR_BAD OW_read(BYTE command, BYTE * bytes, size_t size, struct parsedname * pn)
{
    BYTE c[] = { command,} ;
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE1(c),
        TRXN_READ(bytes,size),
        TRXN_END,
    };
    return BUS_transaction(t, pn) ;
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

static GOOD_OR_BAD OW_command( BYTE cmd, struct parsedname * pn)
{
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE1(&cmd),
        TRXN_END,
    };
    return  BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_r_wetness( UINT *wetness, struct parsedname * pn)
{
    BYTE w[4] ;
    
	RETURN_BAD_IF_BAD(OW_read(_EEEF_READ_SENSOR, w, 4, pn)) ;

    wetness[0] = (uint8_t) w[0] ;
    wetness[1] = (uint8_t) w[1] ;
    wetness[2] = (uint8_t) w[2] ;
    wetness[3] = (uint8_t) w[3] ;

    return gbGOOD ;
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
