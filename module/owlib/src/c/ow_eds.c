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
#include "ow_eds.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_tag);
READ_FUNCTION(FS_r_type);
READ_FUNCTION(FS_r_float16);

READ_FUNCTION(FS_r_i8) ;
WRITE_FUNCTION(FS_w_i8) ;
READ_FUNCTION(FS_r_8) ;
WRITE_FUNCTION(FS_w_8) ;
READ_FUNCTION(FS_r_16) ;
WRITE_FUNCTION(FS_w_16) ;
READ_FUNCTION(FS_r_24) ;
WRITE_FUNCTION(FS_w_24) ;
READ_FUNCTION(FS_r_32) ;
WRITE_FUNCTION(FS_w_32) ;
READ_FUNCTION(FS_r_float8) ;
WRITE_FUNCTION(FS_w_float8) ;
READ_FUNCTION(FS_r_float24) ;
WRITE_FUNCTION(FS_w_float24) ;

WRITE_FUNCTION(FS_clear);

/* Extensive testing of the EDS0068 threchold writing shows a delay
 * is needed before a written value is readable */
#define _EDS_WRITE_DELAY_msec 700

#define _EDS_WRITE_SCRATCHPAD 0x0F
#define _EDS_READ_SCRATCHPAD 0xAA
#define _EDS_COPY_SCRATCHPAD 0x55
#define _EDS_READ_MEMMORY_NO_CRC 0xF0
#define _EDS_READ_MEMORY_WITH_CRC 0xA5
#define _EDS_CLEAR_ALARMS 0x33

#define _EDS_PAGES 3
#define _EDS_PAGESIZE 32

/* EDS0064 locations */
#define _EDS0064_Temp				0x22 // 16 bit float
#define _EDS0064_Humidity			0x24 // 16 bit float
#define _EDS0064_Dew				0x26 // 16 bit float
#define _EDS0064_Humidex			0x28 // 16 bit float
#define _EDS0064_Hindex				0x2A // 16 bit float
#define _EDS0064_mbar				0x2C // 24 bit float
#define _EDS0064_inHg				0x2F // 24 bit float
#define _EDS0064_Lux				0x32 // uint24
#define _EDS0064_Alarm_state		0x3A // uint16
#define _EDS0064_Seconds_counter	0x3C // uint32
#define _EDS0064_Conditional_search	0x40 // uint16
#define _EDS0064_Temp_hi			0x42 // int8
#define _EDS0064_Temp_lo			0x43 // int8
#define _EDS0064_Hum_hi				0x44 // int8
#define _EDS0064_Hum_lo				0x45 // int8
#define _EDS0064_Dew_hi				0x46 // int8
#define _EDS0064_Dew_lo				0x47 // int8
#define _EDS0064_Hdex_hi			0x48 // int8
#define _EDS0064_Hdex_lo			0x49 // int8
#define _EDS0064_HI_hi				0x4A // int8
#define _EDS0064_HI_lo				0x4B // int8
#define _EDS0064_mbar_hi			0x4C // 24 bit float
#define _EDS0064_mbar_lo			0x4F // 24 bit float
#define _EDS0064_inHg_hi			0x52 // 24 bit float
#define _EDS0064_inHg_lo			0x55 // 24 bit float
#define _EDS0064_Lux_hi				0x58 // uint24
#define _EDS0064_Lux_lo				0x5B // uint24
#define _EDS0064_Relay_function		0x5E // byte
#define _EDS0064_Relay_state		0x5F // byte

/* EDS0071 locations */
#define _EDS0071_Tag				0x00 // 30 chars
#define _EDS0071_ID					0x1E // uint16
#define _EDS0071_Temp				0x22 // 24bit float
#define _EDS0071_RTD				0x25 // 24bit float
#define _EDS0071_Conversion			0x28 // uint24
#define _EDS0071_Calibration		0x30 // uint32
//#define _EDS0071_Relay_state		0x35 // byte
#define _EDS0071_Alarm_state		0x36 // byte
#define _EDS0071_Conversion_counter	0x38 // uint32
#define _EDS0071_Seconds_counter	0x3C // uint32
#define _EDS0071_Conditional_search	0x40 // byte
#define _EDS0071_free_byte			0x41 // byte
#define _EDS0071_Temp_hi			0x42 // 24bit float
#define _EDS0071_Temp_lo			0x45 // 24bit float
#define _EDS0071_RTD_hi				0x48 // 24bit float
#define _EDS0071_RTD_lo				0x4B // 24bit float
#define _EDS0071_Calibration_key	0x5C // byte
#define _EDS0071_RTD_delay			0x5D // byte
#define _EDS0071_Relay_function		0x5E // byte
#define _EDS0071_Relay_state		0x5F // byte

static enum e_visibility VISIBLE_EDS0064( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EDS0065( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EDS0066( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EDS0067( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EDS0068( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EDS0071( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_EDS0072( const struct parsedname * pn ) ;

#define _EDS_TAG_LENGTH 30
#define _EDS_TYPE_LENGTH 7

// struct bitfield { "alias_link", number_of_bits, shift_left, }
static struct bitfield eds0064_cond_temp_hi   = { "EDS0064/set_alarm/alarm_function", 1, 0, } ;
static struct bitfield eds0064_cond_temp_lo   = { "EDS0064/set_alarm/alarm_function", 1, 1, } ;
static struct bitfield eds0064_cond_hum_hi    = { "EDS0064/set_alarm/alarm_function", 1, 2, } ;
static struct bitfield eds0064_cond_hum_lo    = { "EDS0064/set_alarm/alarm_function", 1, 3, } ;
static struct bitfield eds0064_cond_dp_hi     = { "EDS0064/set_alarm/alarm_function", 1, 4, } ;
static struct bitfield eds0064_cond_dp_lo     = { "EDS0064/set_alarm/alarm_function", 1, 5, } ;
static struct bitfield eds0064_cond_hdex_hi   = { "EDS0064/set_alarm/alarm_function", 1, 6, } ;
static struct bitfield eds0064_cond_hdex_lo   = { "EDS0064/set_alarm/alarm_function", 1, 7, } ;
static struct bitfield eds0064_cond_hindex_hi = { "EDS0064/set_alarm/alarm_function", 1, 8, } ;
static struct bitfield eds0064_cond_hindex_lo = { "EDS0064/set_alarm/alarm_function", 1, 9, } ;
static struct bitfield eds0064_cond_mbar_hi   = { "EDS0064/set_alarm/alarm_function", 1, 10, } ;
static struct bitfield eds0064_cond_mbar_lo   = { "EDS0064/set_alarm/alarm_function", 1, 11, } ;
static struct bitfield eds0064_cond_inHg_hi   = { "EDS0064/set_alarm/alarm_function", 1, 12, } ;
static struct bitfield eds0064_cond_inHg_lo   = { "EDS0064/set_alarm/alarm_function", 1, 13, } ;
static struct bitfield eds0064_cond_lux_hi    = { "EDS0064/set_alarm/alarm_function", 1, 14, } ;
static struct bitfield eds0064_cond_lux_lo    = { "EDS0064/set_alarm/alarm_function", 1, 15, } ;
static struct bitfield eds0064_temp_hi   = { "EDS0064/alarm/state", 1, 0, } ;
static struct bitfield eds0064_temp_lo   = { "EDS0064/alarm/state", 1, 1, } ;
static struct bitfield eds0064_hum_hi    = { "EDS0064/alarm/state", 1, 2, } ;
static struct bitfield eds0064_hum_lo    = { "EDS0064/alarm/state", 1, 3, } ;
static struct bitfield eds0064_dp_hi     = { "EDS0064/alarm/state", 1, 4, } ;
static struct bitfield eds0064_dp_lo     = { "EDS0064/alarm/state", 1, 5, } ;
static struct bitfield eds0064_hdex_hi   = { "EDS0064/alarm/state", 1, 6, } ;
static struct bitfield eds0064_hdex_lo   = { "EDS0064/alarm/state", 1, 7, } ;
static struct bitfield eds0064_hindex_hi = { "EDS0064/alarm/state", 1, 8, } ;
static struct bitfield eds0064_hindex_lo = { "EDS0064/alarm/state", 1, 9, } ;
static struct bitfield eds0064_mbar_hi   = { "EDS0064/alarm/state", 1, 10, } ;
static struct bitfield eds0064_mbar_lo   = { "EDS0064/alarm/state", 1, 11, } ;
static struct bitfield eds0064_inHg_hi   = { "EDS0064/alarm/state", 1, 12, } ;
static struct bitfield eds0064_inHg_lo   = { "EDS0064/alarm/state", 1, 13, } ;
static struct bitfield eds0064_lux_hi    = { "EDS0064/alarm/state", 1, 14, } ;
static struct bitfield eds0064_lux_lo    = { "EDS0064/alarm/state", 1, 15, } ;
static struct bitfield eds0064_relay_state  = { "EDS0064/relay_state", 1, 0, } ;
static struct bitfield eds0064_led_state  = { "EDS0064/relay_state", 1, 1, } ;
static struct bitfield eds0064_led_function  = { "EDS0064/relay_function", 2, 2, } ;

// struct bitfield { "alias_link", number_of_bits, shift_left, }
static struct bitfield eds0071_cond_temp_hi = { "EDS0071/set_alarm/alarm_function", 1, 0, } ;
static struct bitfield eds0071_cond_temp_lo = { "EDS0071/set_alarm/alarm_function", 1, 1, } ;
static struct bitfield eds0071_cond_RTD_hi  = { "EDS0071/set_alarm/alarm_function", 1, 2, } ;
static struct bitfield eds0071_cond_RTD_lo  = { "EDS0071/set_alarm/alarm_function", 1, 3, } ;
static struct bitfield eds0071_temp_hi = { "EDS0071/alarm/state", 1, 0, } ;
static struct bitfield eds0071_temp_lo = { "EDS0071/alarm/state", 1, 1, } ;
static struct bitfield eds0071_RTD_hi  = { "EDS0071/alarm/state", 1, 2, } ;
static struct bitfield eds0071_RTD_lo  = { "EDS0071/alarm/state", 1, 3, } ;
static struct bitfield eds0071_relay_state  = { "EDS0071/relay_state", 1, 0, } ;
static struct bitfield eds0071_led_state  = { "EDS0071/relay_state", 1, 1, } ;
static struct bitfield eds0071_relay_function  = { "EDS0071/relay_function", 2, 0, } ;
static struct bitfield eds0071_led_function  = { "EDS0071/relay_function", 2, 2, } ;

/* ------- Structures ----------- */

static struct aggregate AEDS = { _EDS_PAGES, ag_numbers, ag_separate, };
static struct filetype EDS[] = {
	F_STANDARD,
	{"memory", _EDS_PAGES * _EDS_PAGESIZE, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", _EDS_PAGESIZE, &AEDS, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },

	{"tag", _EDS_TAG_LENGTH, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_tag, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"device_type", _EDS_TYPE_LENGTH, NON_AGGREGATE, ft_ascii, fc_link, FS_r_type, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"device_id", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_16, NO_WRITE_FUNCTION, VISIBLE, {u: _EDS0071_ID,}, },

	/* EDS 0064 */
	{"EDS0064", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0064, {u: _EDS0064_Temp,}, },

	{"EDS0064/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0064, {u: _EDS0064_Seconds_counter,}, },
	{"EDS0064/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_function,}, },
	{"EDS0064/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_state,}, },

	{"EDS0064/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0064, {v: &eds0064_cond_temp_hi,}, },
	{"EDS0064/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0064, {v: &eds0064_cond_temp_lo,}, },
	{"EDS0064/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, INVISIBLE, {u: _EDS0064_Conditional_search,}, },

	{"EDS0064/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/threshold/temp_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0064, {u: _EDS0064_Temp_hi,}, },
	{"EDS0064/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0064, {u: _EDS0064_Temp_lo,}, },

	{"EDS0064/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0064_Alarm_state,}, },
	{"EDS0064/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0064, {v: &eds0064_temp_hi,}, },
	{"EDS0064/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0064, {v: &eds0064_temp_lo,}, },

	{"EDS0064/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0064, {v: &eds0064_relay_state,}, },
	{"EDS0064/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0064, {v: &eds0064_led_function,}, },

	{"EDS0064/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0064, NO_FILETYPE_DATA, },
	{"EDS0064/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0064, {v: &eds0064_led_state,}, },
	{"EDS0064/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0064, {v: &eds0064_led_function,}, },

	/* EDS 0065 */
	{"EDS0065", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {u: _EDS0064_Temp,}, },
	{"EDS0065/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {u: _EDS0064_Humidity,}, },
	{"EDS0065/dew_point", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {u: _EDS0064_Dew,}, },
	{"EDS0065/humidex", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {u: _EDS0064_Humidex,}, },
	{"EDS0065/heat_index", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {u: _EDS0064_Hindex,}, },
	{"EDS0065/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_function,}, },
	{"EDS0065/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_state,}, },

	{"EDS0065/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {u: _EDS0064_Seconds_counter,}, },

	{"EDS0065/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_temp_hi,}, },
	{"EDS0065/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_temp_lo,}, },
	{"EDS0065/set_alarm/humidity_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_hum_hi,}, },
	{"EDS0065/set_alarm/humidity_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_hum_lo,}, },
	{"EDS0065/set_alarm/humidex_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_hdex_hi,}, },
	{"EDS0065/set_alarm/humidex_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_hdex_lo,}, },
	{"EDS0065/set_alarm/heat_index_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_hindex_hi,}, },
	{"EDS0065/set_alarm/heat_index_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_hindex_lo,}, },
	{"EDS0065/set_alarm/dew_point_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_dp_hi,}, },
	{"EDS0065/set_alarm/dew_point_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_cond_dp_lo,}, },
	{"EDS0065/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, INVISIBLE, {u: _EDS0064_Conditional_search,}, },

	{"EDS0065/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/threshold/temp_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_Temp_hi,}, },
	{"EDS0065/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_Temp_lo,}, },
	{"EDS0065/threshold/humidity_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_i8, FS_w_i8, VISIBLE_EDS0065, {u: _EDS0064_Hum_hi,}, },
	{"EDS0065/threshold/humidity_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_i8, FS_w_i8, VISIBLE_EDS0065, {u: _EDS0064_Hum_lo,}, },
	{"EDS0065/threshold/dew_point_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_Dew_hi,}, },
	{"EDS0065/threshold/dew_point_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_Dew_lo,}, },
	{"EDS0065/threshold/heat_index_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_HI_hi,}, },
	{"EDS0065/threshold/heat_index_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_HI_lo,}, },
	{"EDS0065/threshold/humidex_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_Hdex_hi,}, },
	{"EDS0065/threshold/humidex_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0065, {u: _EDS0064_Hdex_lo,}, },

	{"EDS0065/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0064_Alarm_state,}, },
	{"EDS0065/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0071_temp_hi,}, },
	{"EDS0065/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_temp_lo,}, },
	{"EDS0065/alarm/humidity_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_hum_hi,}, },
	{"EDS0065/alarm/humidity_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_hum_lo,}, },
	{"EDS0065/alarm/humidex_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_hdex_hi,}, },
	{"EDS0065/alarm/humidex_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_hdex_lo,}, },
	{"EDS0065/alarm/heat_index_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_hindex_hi,}, },
	{"EDS0065/alarm/heat_index_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_hindex_lo,}, },
	{"EDS0065/alarm/dew_point_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_dp_hi,}, },
	{"EDS0065/alarm/dew_point_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0065, {v: &eds0064_dp_lo,}, },

	{"EDS0065/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_relay_state,}, },
	{"EDS0065/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_led_function,}, },

	{"EDS0065/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0065, NO_FILETYPE_DATA, },
	{"EDS0065/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_led_state,}, },
	{"EDS0065/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0065, {v: &eds0064_led_function,}, },

	/* EDS 0066 */
	{"EDS0066", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {u: _EDS0064_Temp,}, },
	{"EDS0066/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {u: _EDS0064_mbar,}, },
	{"EDS0066/inHg", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {u: _EDS0064_inHg,}, },
	{"EDS0066/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_function,}, },
	{"EDS0066/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_state,}, },

	{"EDS0066/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {u: _EDS0064_Seconds_counter,}, },

	{"EDS0066/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_cond_temp_hi,}, },
	{"EDS0066/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_cond_temp_lo,}, },
	{"EDS0066/set_alarm/pressure_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_cond_mbar_hi,}, },
	{"EDS0066/set_alarm/pressure_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_cond_mbar_lo,}, },
	{"EDS0066/set_alarm/inHg_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_cond_inHg_hi,}, },
	{"EDS0066/set_alarm/inHg_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_cond_inHg_lo,}, },
	{"EDS0066/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, INVISIBLE, {u: _EDS0064_Conditional_search,}, },

	{"EDS0066/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/threshold/temp_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0066, {u: _EDS0064_Temp_hi,}, },
	{"EDS0066/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0066, {u: _EDS0064_Temp_lo,}, },
	{"EDS0066/threshold/pressure_hi" , PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0066, {u: _EDS0064_mbar_hi,}, },
	{"EDS0066/threshold/pressure_low", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0066, {u: _EDS0064_mbar_lo,}, },
	{"EDS0066/threshold/inHg_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0066, {u: _EDS0064_inHg_hi,}, },
	{"EDS0066/threshold/inHg_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0066, {u: _EDS0064_inHg_lo,}, },

	{"EDS0066/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0064_Alarm_state,}, },
	{"EDS0066/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {v: &eds0064_temp_hi,}, },
	{"EDS0066/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {v: &eds0064_temp_lo,}, },
	{"EDS0066/alarm/pressure_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {v: &eds0064_mbar_hi,}, },
	{"EDS0066/alarm/pressure_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {v: &eds0064_mbar_lo,}, },
	{"EDS0066/alarm/inHg_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {v: &eds0064_inHg_hi,}, },
	{"EDS0066/alarm/inHg_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0066, {v: &eds0064_inHg_lo,}, },

	{"EDS0066/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_relay_state,}, },
	{"EDS0066/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_led_function,}, },

	{"EDS0066/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0066, NO_FILETYPE_DATA, },
	{"EDS0066/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_led_state,}, },
	{"EDS0066/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0066, {v: &eds0064_led_function,}, },

	/* EDS 0067 */
	{"EDS0067", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {u: _EDS0064_Temp,}, },
	{"EDS0067/light", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_24, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {u: _EDS0064_Lux,}, },
	{"EDS0067/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_function,}, },
	{"EDS0067/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_state,}, },

	{"EDS0067/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {u: _EDS0064_Seconds_counter,}, },

	{"EDS0067/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_cond_temp_hi,}, },
	{"EDS0067/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_cond_temp_lo,}, },
	{"EDS0067/set_alarm/light_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_cond_lux_hi,}, },
	{"EDS0067/set_alarm/light_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_cond_lux_lo,}, },
	{"EDS0067/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, INVISIBLE, {u: _EDS0064_Conditional_search,}, },

	{"EDS0067/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/threshold/temp_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0067, {u: _EDS0064_Temp_hi,}, },
	{"EDS0067/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0067, {u: _EDS0064_Temp_lo,}, },
	{"EDS0067/threshold/light_hi ", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_24, FS_w_24, VISIBLE_EDS0067, {u: _EDS0064_Lux_hi,}, },
	{"EDS0067/threshold/light_low", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_24, FS_w_24, VISIBLE_EDS0067, {u: _EDS0064_Lux_lo,}, },

	{"EDS0067/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0064_Alarm_state,}, },
	{"EDS0067/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {v: &eds0064_temp_hi,}, },
	{"EDS0067/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {v: &eds0064_temp_lo,}, },
	{"EDS0067/alarm/light_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {v: &eds0064_lux_hi,}, },
	{"EDS0067/alarm/light_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0067, {v: &eds0064_lux_lo,}, },

	{"EDS0067/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_relay_state,}, },
	{"EDS0067/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_led_function,}, },

	{"EDS0067/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0067, NO_FILETYPE_DATA, },
	{"EDS0067/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_led_state,}, },
	{"EDS0067/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0067, {v: &eds0064_led_function,}, },

	/* EDS 0068 */
	{"EDS0068", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Temp,}, },
	{"EDS0068/humidity", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Humidity,}, },
	{"EDS0068/dew_point", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Dew,}, },
	{"EDS0068/humidex", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Humidex,}, },
	{"EDS0068/heat_index", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float16, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Hindex,}, },
	{"EDS0068/pressure", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_mbar,}, },
	{"EDS0068/inHg", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_inHg,}, },
	{"EDS0068/light", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_24, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Lux,}, },
	{"EDS0068/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_function,}, },
	{"EDS0068/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0064_Relay_state,}, },

	{"EDS0068/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {u: _EDS0064_Seconds_counter,}, },

	{"EDS0068/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_temp_hi,}, },
	{"EDS0068/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_temp_lo,}, },
	{"EDS0068/set_alarm/humidity_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_hum_hi,}, },
	{"EDS0068/set_alarm/humidity_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_hum_lo,}, },
	{"EDS0068/set_alarm/humidex_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_hdex_hi,}, },
	{"EDS0068/set_alarm/humidex_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_hdex_lo,}, },
	{"EDS0068/set_alarm/heat_index_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_hindex_hi,}, },
	{"EDS0068/set_alarm/heat_index_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_hindex_lo,}, },
	{"EDS0068/set_alarm/dew_point_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_dp_hi,}, },
	{"EDS0068/set_alarm/dew_point_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_dp_lo,}, },
	{"EDS0068/set_alarm/pressure_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_mbar_hi,}, },
	{"EDS0068/set_alarm/pressure_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_mbar_lo,}, },
	{"EDS0068/set_alarm/inHg_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_inHg_hi,}, },
	{"EDS0068/set_alarm/inHg_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_inHg_lo,}, },
	{"EDS0068/set_alarm/light_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_lux_hi,}, },
	{"EDS0068/set_alarm/light_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_cond_lux_lo,}, },
	{"EDS0068/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, INVISIBLE, {u: _EDS0064_Conditional_search,}, },

	{"EDS0068/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/threshold/temp_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Temp_hi,}, },
	{"EDS0068/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Temp_lo,}, },
	{"EDS0068/threshold/humidity_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Hum_hi,}, },
	{"EDS0068/threshold/humidity_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Hum_lo,}, },
	{"EDS0068/threshold/dew_point_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Dew_hi,}, },
	{"EDS0068/threshold/dew_point_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Dew_lo,}, },
	{"EDS0068/threshold/heat_index_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_HI_hi,}, },
	{"EDS0068/threshold/heat_index_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_HI_lo,}, },
	{"EDS0068/threshold/humidex_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Hdex_hi,}, },
	{"EDS0068/threshold/humidex_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float8, FS_w_float8, VISIBLE_EDS0068, {u: _EDS0064_Hdex_lo,}, },
	{"EDS0068/threshold/pressure_hi" , PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0068, {u: _EDS0064_mbar_hi,}, },
	{"EDS0068/threshold/pressure_low", PROPERTY_LENGTH_PRESSURE, NON_AGGREGATE, ft_pressure, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0068, {u: _EDS0064_mbar_lo,}, },
	{"EDS0068/threshold/inHg_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0068, {u: _EDS0064_inHg_hi,}, },
	{"EDS0068/threshold/inHg_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0068, {u: _EDS0064_inHg_lo,}, },
	{"EDS0068/threshold/light_hi" , PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_24, FS_w_24, VISIBLE_EDS0068, {u: _EDS0064_Lux_hi,}, },
	{"EDS0068/threshold/light_low", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_24, FS_w_24, VISIBLE_EDS0068, {u: _EDS0064_Lux_lo,}, },

	{"EDS0068/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0064_Alarm_state,}, },
	{"EDS0068/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0071_temp_hi,}, },
	{"EDS0068/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_temp_lo,}, },
	{"EDS0068/alarm/humidity_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_hum_hi,}, },
	{"EDS0068/alarm/humidity_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_hum_lo,}, },
	{"EDS0068/alarm/humidex_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_hdex_hi,}, },
	{"EDS0068/alarm/humidex_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_hdex_lo,}, },
	{"EDS0068/alarm/heat_index_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_hindex_hi,}, },
	{"EDS0068/alarm/heat_index_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_hindex_lo,}, },
	{"EDS0068/alarm/dew_point_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_dp_hi,}, },
	{"EDS0068/alarm/dew_point_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_dp_lo,}, },
	{"EDS0068/alarm/pressure_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_mbar_hi,}, },
	{"EDS0068/alarm/pressure_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_mbar_lo,}, },
	{"EDS0068/alarm/inHg_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_inHg_hi,}, },
	{"EDS0068/alarm/inHg_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_inHg_lo,}, },
	{"EDS0068/alarm/light_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_lux_hi,}, },
	{"EDS0068/alarm/light_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0068, {v: &eds0064_lux_lo,}, },

	{"EDS0068/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_relay_state,}, },
	{"EDS0068/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_led_function,}, },

	{"EDS0068/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0068, NO_FILETYPE_DATA, },
	{"EDS0068/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_led_state,}, },
	{"EDS0068/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0068, {v: &eds0064_led_function,}, },

	/* EDS0071 */
	{"EDS0071", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Temp,}, },
	{"EDS0071/resistance", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_RTD,}, },
	{"EDS0071/raw", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_24, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Conversion,}, },
	{"EDS0071/user_byte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_EDS0071, {u: _EDS0071_free_byte,}, },
	{"EDS0071/delay", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_EDS0071, {u: _EDS0071_RTD_delay,}, },
	{"EDS0071/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Relay_function,}, },
	{"EDS0071/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Relay_state,}, },

	{"EDS0071/calibration", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/calibration/constant", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Calibration,}, },
	{"EDS0071/calibration/key", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_8, FS_w_8, VISIBLE_EDS0071, {u: _EDS0071_Calibration_key,}, },

	{"EDS0071/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Seconds_counter,}, },
	{"EDS0071/counter/samples", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Conversion_counter,}, },

	{"EDS0071/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/set_alarm/temp_hi", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_cond_temp_hi,}, },
	{"EDS0071/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_cond_temp_lo,}, },
	{"EDS0071/set_alarm/resistance__hi", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_cond_RTD_hi,}, },
	{"EDS0071/set_alarm/resistance_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_cond_RTD_lo,}, },
	{"EDS0071/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Conditional_search,}, },

	{"EDS0071/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/threshold/temp_hi", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0071, {u: _EDS0071_Temp_hi,}, },
	{"EDS0071/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0071, {u: _EDS0071_Temp_lo,}, },
	{"EDS0071/threshold/resistance_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0071, {u: _EDS0071_RTD_hi,}, },
	{"EDS0071/threshold/resistance_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0071, {u: _EDS0071_RTD_lo,}, },

	{"EDS0071/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Alarm_state,}, },
	{"EDS0071/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {v: &eds0071_temp_hi,}, },
	{"EDS0071/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {v: &eds0071_temp_lo,}, },
	{"EDS0071/alarm/resistance_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {v: &eds0071_RTD_hi,}, },
	{"EDS0071/alarm/resistance_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {v: &eds0071_RTD_lo,}, },

	{"EDS0071/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0071/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_relay_state,}, },
	{"EDS0071/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_led_function,}, },

	{"EDS0071/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0071/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_led_state,}, },
	{"EDS0071/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0071, {v: &eds0071_led_function,}, },

	/* EDS0072 */
	{"EDS0072", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {u: _EDS0071_Temp,}, },
	{"EDS0072/resistance", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_float24, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {u: _EDS0071_RTD,}, },
	{"EDS0072/raw", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_24, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Conversion,}, },
	{"EDS0072/user_byte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_EDS0072, {u: _EDS0071_free_byte,}, },
	{"EDS0072/delay", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_EDS0072, {u: _EDS0071_RTD_delay,}, },
	{"EDS0072/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Relay_function,}, },
	{"EDS0072/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Relay_state,}, },

	{"EDS0072/calibration", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/calibration/constant", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {u: _EDS0071_Calibration,}, },
	{"EDS0072/calibration/key", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_8, FS_w_8, VISIBLE_EDS0072, {u: _EDS0071_Calibration_key,}, },

	{"EDS0072/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {u: _EDS0071_Seconds_counter,}, },
	{"EDS0072/counter/samples", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {u: _EDS0071_Conversion_counter,}, },

	{"EDS0072/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_cond_temp_hi,}, },
	{"EDS0072/set_alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_cond_temp_hi,}, },
	{"EDS0072/set_alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_cond_temp_lo,}, },
	{"EDS0072/set_alarm/RTD_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_cond_RTD_hi,}, },
	{"EDS0072/set_alarm/RTD_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_cond_RTD_lo,}, },
	{"EDS0072/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Conditional_search,}, },

	{"EDS0072/threshold", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA, },
	{"EDS0072/threshold/temp_hi" , PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0072, {u: _EDS0071_Temp_hi,}, },
	{"EDS0072/threshold/temp_low", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0072, {u: _EDS0071_Temp_lo,}, },
	{"EDS0072/threshold/resistance_hi" , PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0072, {u: _EDS0071_RTD_hi,}, },
	{"EDS0072/threshold/resistance_low", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_float24, FS_w_float24, VISIBLE_EDS0072, {u: _EDS0071_RTD_lo,}, },

	{"EDS0072/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/alarm/clear", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clear, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Alarm_state,}, },
	{"EDS0072/alarm/temp_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {v: &eds0071_temp_hi,}, },
	{"EDS0072/alarm/temp_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {v: &eds0071_temp_lo,}, },
	{"EDS0072/alarm/RTD_hi" , PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {v: &eds0071_RTD_hi,}, },
	{"EDS0072/alarm/RTD_low", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_EDS0072, {v: &eds0071_RTD_lo,}, },

	{"EDS0072/relay", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/relay/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_relay_state,}, },
	{"EDS0072/relay/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_led_function,}, },

	{"EDS0072/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0072, NO_FILETYPE_DATA, },
	{"EDS0072/LED/state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_led_state,}, },
	{"EDS0072/LED/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_r_bitfield, FS_w_bitfield, VISIBLE_EDS0072, {v: &eds0071_led_function,}, },
};

DeviceEntryExtended(7E, EDS, DEV_temp | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* ------- Functions ------------ */
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_w_mem_crc(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_r_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname * pn);
static int VISIBLE_EDS( const struct parsedname * pn ) ;
static GOOD_OR_BAD OW_clear( struct parsedname * pn) ;

/* finds the visibility value (0x0071 ...) either cached, or computed via the device_id (then cached) */
static int VISIBLE_EDS( const struct parsedname * pn )
{
	int device_id = -1 ;
	
	LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
	if ( BAD( GetVisibilityCache( &device_id, pn ) ) ) {
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		if ( owq != NULL) {
			UINT U_device_id ;
			if ( FS_r_sibling_U( &U_device_id, "device_id", owq ) == 0 ) {
				device_id = U_device_id ;
				SetVisibilityCache( device_id, pn ) ;
			}
			OWQ_destroy(owq) ;
		}
	}
	return device_id ;
}

static enum e_visibility VISIBLE_EDS0064( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0064:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EDS0065( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0065:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EDS0066( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0066:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EDS0067( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0067:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EDS0068( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0068:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EDS0071( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0071:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_EDS0072( const struct parsedname * pn )
{
	switch ( VISIBLE_EDS(pn) ) {
		case 0x0072:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static ZERO_OR_ERROR FS_r_tag(struct one_wire_query *owq)
{
	return FS_r_mem(owq) ;
}

static ZERO_OR_ERROR FS_clear(struct one_wire_query *owq)
{
	if ( OWQ_Y(owq) != 0 ) {
		RETURN_ERROR_IF_BAD( OW_clear( PN(owq) ) ) ;
	}
	return 0 ;
}

static ZERO_OR_ERROR FS_r_type(struct one_wire_query *owq)
{
	UINT id ;
	ASCII typ[_EDS_TYPE_LENGTH+1] ;

	RETURN_ERROR_IF_BAD( FS_r_sibling_U( &id, "device_id", owq ) ) ;

	UCLIBCLOCK ;
	snprintf(typ,sizeof(typ),"EDS%.4X",id);
	UCLIBCUNLOCK ;

	return OWQ_format_output_offset_and_size_z(typ,owq);
}

/* 8 bit float */
static ZERO_OR_ERROR FS_r_float8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_F(owq) = (_FLOAT) ((int8_t) (data[0] )) ;
	return 0 ;
}

/* 16 bit float in DS18B20 format */
static ZERO_OR_ERROR FS_r_float16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_F(owq) = (_FLOAT) ((int16_t) ((data[1] << 8) | data[0] )) * .0625 ;
	return 0 ;
}

/* 24 bit float with resolution 1/2048th  */
static ZERO_OR_ERROR FS_r_float24(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 24/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	// use 32 bit with 0 for lowest byte
	OWQ_F(owq) = (_FLOAT) ((int32_t) ((data[2] << 24) | (data[1] << 16) | ( data[0] << 8 ) )) / ( 2048.*256.) ;
	return 0 ;
}

/* write a 24 bit value from a register stored in filetype.data */
/* write as a signed float with resolution 1/2048th */
static ZERO_OR_ERROR FS_w_float24(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 24/8 ;
	BYTE data[bytes] ;
	int32_t big = 256. * 2048. * OWQ_F(owq) ;

	data[0] = (big>>8) & 0xFF ;
	data[1] = (big>>16) & 0xFF ;
	data[2] = (big>>24) & 0xFF ;
	
	return GB_to_Z_OR_E( OW_w_mem( data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_F0)) ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	return COMMON_offset_process( FS_r_mem, owq, OWQ_pn(owq).extension*_EDS_PAGESIZE) ;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	size_t size = OWQ_size(owq) ;
	off_t start = OWQ_offset(owq) ;
	BYTE * position = (BYTE *)OWQ_buffer(owq) ;
	size_t write_size = _EDS_PAGESIZE - (start % _EDS_PAGESIZE) ;
	
	while ( size > 0 ) {
		if ( write_size > size ) {
			write_size = size ;
		}
		RETURN_ERROR_IF_BAD( OW_w_mem(position, write_size, start, PN(owq)) ) ;
		position += write_size ;
		start += write_size ;
		size -= write_size ;
		write_size = _EDS_PAGESIZE ;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*_EDS_PAGESIZE) ;
}

/* read a signed 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_r_i8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_I(owq) = (int8_t) data[0] ;
	return 0 ;
}

/* write a signed 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_w_i8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	data[0] = BYTE_MASK( OWQ_I(owq) ) ;
	return GB_to_Z_OR_E(OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_r_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = data[0] ;
	return 0 ;
}

/* write an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_w_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	data[0] = BYTE_MASK( OWQ_U(owq) ) ;
	return GB_to_Z_OR_E(OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = UT_int16(data) ;

	return 0 ;
}

/* write a 24 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_24(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 24/8 ;
	BYTE data[bytes] ;

	UT_uint16_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 24 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_24(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 24/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = UT_int24(data) ;
	return 0 ;
}

/* write a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;

	UT_uint24_to_bytes( OWQ_U(owq), data ) ;

	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 32/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = UT_int32(data) ;
	return 0 ;
}

/* write an 8 bit value from a register stored in filetype.data */
/* write as a signed float*/
static ZERO_OR_ERROR FS_w_float8(struct one_wire_query *owq)
{
	int8_t val = OWQ_F(owq) ;

	OWQ_I(owq) = val ;
	return FS_w_8(owq) ;
}

static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[3 + 1 + _EDS_PAGESIZE + 2] = { _EDS_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(p, 3 + size),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_READ( &p[1], 3 + size),
		TRXN_COMPARE(&p[4], data, size),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_DELAY(_EDS_WRITE_DELAY_msec),
		TRXN_END,
	};

	// use different scheme for write that goes to page end (use CRC)
	if ( ( (offset + size) & 0x1F) == 0 ) {	/* to end of page */
		return OW_w_mem_crc( data, size, offset, pn ) ;
	}

	/* Copy to scratchpad */
	memcpy(&p[3], data, size);
	RETURN_BAD_IF_BAD(BUS_transaction(twrite, pn)) ;

	/* Re-read scratchpad and compare */
	/* Note: location of data has now shifted down a byte for E/S register */
	p[0] = _EDS_READ_SCRATCHPAD;
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;

	/* write Scratchpad to SRAM */
	p[0] = _EDS_COPY_SCRATCHPAD;
	return BUS_transaction(tcopy, pn) ;
}

// write to end of page (size is appropriate)
static GOOD_OR_BAD OW_w_mem_crc(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[3 + 1 + _EDS_PAGESIZE + 2] = { _EDS_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3 + size, 0),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 3 + size),
		TRXN_COMPARE(&p[4], data, size),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_DELAY(_EDS_WRITE_DELAY_msec),
		TRXN_END,
	};

	/* Copy to scratchpad -- use CRC16 */
	memcpy(&p[3], data, size);
	RETURN_BAD_IF_BAD(BUS_transaction(twrite, pn)) ;

	/* Re-read scratchpad and compare */
	/* Note: location of data has now shifted down a byte for E/S register */
	p[0] = _EDS_READ_SCRATCHPAD;
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;

	/* write Scratchpad to SRAM */
	p[0] = _EDS_COPY_SCRATCHPAD;
	return BUS_transaction(tcopy, pn) ;
}

/* Read a few bytes within a page */
static GOOD_OR_BAD OW_r_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	BYTE p[3] = { _EDS_READ_MEMMORY_NO_CRC, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ( data, size),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

/* Clear alarms */
static GOOD_OR_BAD OW_clear( struct parsedname * pn)
{
	BYTE p[1] = { _EDS_CLEAR_ALARMS };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}
