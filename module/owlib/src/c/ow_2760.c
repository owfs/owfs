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

/* Changes
    7/2004 Extensive improvements based on input from Serg Oskin
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2760.h"
#include "ow_thermocouple.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_lock);
WRITE_FUNCTION(FS_w_lock);
READ_FUNCTION(FS_r_volt);
READ_FUNCTION(FS_r_temp);
READ_FUNCTION(FS_r_current);
READ_FUNCTION(FS_r_vis);
READ_FUNCTION(FS_r_vis_avg);
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_r_vh);
WRITE_FUNCTION(FS_w_vh);
READ_FUNCTION(FS_r_vis_off);
WRITE_FUNCTION(FS_w_vis_off);
READ_FUNCTION(FS_r_ah);
WRITE_FUNCTION(FS_w_ah);
READ_FUNCTION(FS_r_abias);
WRITE_FUNCTION(FS_w_abias);
READ_FUNCTION(FS_r_templim);
WRITE_FUNCTION(FS_w_templim);
READ_FUNCTION(FS_r_vbias);
WRITE_FUNCTION(FS_w_vbias);
READ_FUNCTION(FS_r_timer);
WRITE_FUNCTION(FS_w_timer);
READ_FUNCTION(FS_r_bit);
WRITE_FUNCTION(FS_w_bit);
WRITE_FUNCTION(FS_charge);
WRITE_FUNCTION(FS_refresh);

READ_FUNCTION(FS_WS603_temperature);
READ_FUNCTION(FS_WS603_r_data);
READ_FUNCTION(FS_WS603_r_param);
READ_FUNCTION(FS_WS603_wind_speed);
READ_FUNCTION(FS_WS603_wind_direction);
READ_FUNCTION(FS_WS603_r_led);
READ_FUNCTION(FS_WS603_light);
READ_FUNCTION(FS_WS603_volt);
READ_FUNCTION(FS_WS603_r_wind_calibration);
WRITE_FUNCTION(FS_WS603_w_wind_calibration);
READ_FUNCTION(FS_WS603_r_direction_calibration);
WRITE_FUNCTION(FS_WS603_w_direction_calibration);
READ_FUNCTION(FS_WS603_r_light_threshold);
WRITE_FUNCTION(FS_WS603_led_control);
WRITE_FUNCTION(FS_WS603_r_led_model);

READ_FUNCTION(FS_thermocouple);
READ_FUNCTION(FS_rangelow);
READ_FUNCTION(FS_rangehigh);
 /* ------- Structures ----------- */


#define F_thermocouple  \
	{"typeB"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeB/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_b}, } , \
	{"typeB/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_b}, } , \
	{"typeB/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_b}, } , \
	\
	{"typeE"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeE/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_e}, } , \
	{"typeE/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_e}, } , \
	{"typeE/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_e}, } , \
	\
	{"typeJ"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeJ/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_j}, } , \
	{"typeJ/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_j}, } , \
	{"typeJ/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_j}, } , \
	\
	{"typeK"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeK/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_k}, } , \
	{"typeK/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_k}, } , \
	{"typeK/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_k}, } , \
	\
	{"typeN"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeN/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_n}, } , \
	{"typeN/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_n}, } , \
	{"typeN/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_n}, } , \
	\
	{"typeR"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeR/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_r}, } , \
	{"typeR/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_r}, } , \
	{"typeR/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_r}, } , \
	\
	{"typeS"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeS/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_s}, } , \
	{"typeS/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_s}, } , \
	{"typeS/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_s}, } , \
	\
	{"typeT"            ,PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir  , fc_subdir,     NO_READ_FUNCTION, NO_WRITE_FUNCTION     , VISIBLE, NO_FILETYPE_DATA, } , \
	{"typeT/temperature",PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , VISIBLE, {.i=e_type_t}, } , \
	{"typeT/range_low"  ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_t}, } , \
	{"typeT/range_high" ,PROPERTY_LENGTH_TEMP, NON_AGGREGATE,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , VISIBLE, {.i=e_type_t}, } ,

#define _1W_DS27XX_PROTECT_REG 0x00

#define _1W_DS27XX_STATUS_REG 0x01
#define _1W_DS27XX_STATUS_REG_INIT 0x31

#define _1W_DS2770_TIMER 0x02

#define _1W_DS2760_SPECIAL_REG 0x08
#define _1W_DS2780_SPECIAL_REG 0x15

#define _1W_DS2780_AVERAGE_CURRENT 0x08
#define _1W_DS2756_AVERAGE_CURRENT 0x1A

#define _1W_DS27XX_VOLTAGE 0x0C
#define _1W_DS27XX_CURRENT 0x0E
#define _1W_DS27XX_ACCUMULATED 0x10

#define _1W_DS2770_CURRENT_OFFSET 0x32
#define _1W_DS2760_CURRENT_OFFSET 0x33
#define _1W_DS2780_CURRENT_OFFSET 0x61

#define _1W_DS2760_TEMPERATURE 0x18
#define _1W_DS2780_TEMPERATURE 0x0A

#define _1W_DS2780_PARAM_REG 0x60

struct LockPage {
	int pages;
	size_t reg;
	size_t size;
	off_t offset[3];
};

#define Pages2720	2
#define Pages2751	2
#define Pages2755	3
#define Pages2760	2
#define Pages2770	3
#define Pages2780	2

#define Size2720	4
#define Size2751	16
#define Size2755	32
#define Size2760	16
#define Size2770	16
#define Size2780	16

static struct LockPage P2720 = { Pages2720, 0x07, Size2720, {0x20, 0x30, 0x00,}, };
static struct LockPage P2751 = { Pages2751, 0x07, Size2751, {0x20, 0x30, 0x00,}, };
static struct LockPage P2755 = { Pages2755, 0x07, Size2755, {0x20, 0x40, 0x60,}, };
static struct LockPage P2760 = { Pages2760, 0x07, Size2760, {0x20, 0x30, 0x00,}, };
static struct LockPage P2770 = { Pages2770, 0x07, Size2770, {0x20, 0x30, 0x40,}, };
static struct LockPage P2780 = { Pages2780, 0x1F, Size2780, {0x20, 0x60, 0x00,}, };

static struct aggregate L2720 = { Pages2720, ag_numbers, ag_separate };
static struct aggregate L2751 = { Pages2751, ag_numbers, ag_separate };
static struct aggregate L2755 = { Pages2755, ag_numbers, ag_separate };
static struct aggregate L2760 = { Pages2760, ag_numbers, ag_separate };
static struct aggregate L2770 = { Pages2770, ag_numbers, ag_separate };
static struct aggregate L2780 = { Pages2780, ag_numbers, ag_separate };

static struct filetype DS2720[] = {
	F_STANDARD,
	{"memory", 256, NULL, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2720, &L2720, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2720}, },

	{"lock", PROPERTY_LENGTH_YESNO, &L2720, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2720}, },
	{"cc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 3}, },
	{"ce", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 1}, },
	{"dc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 2}, },
	{"de", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 0}, },
	{"doc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 4}, },
	{"ot", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 0}, },
	{"ov", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 7}, },
	{"psf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 7}, },
	{"uv", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 6}, },
};

DeviceEntry(31, DS2720, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS2740[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },

	{"PIO", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_pio, FS_w_pio, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vis_B", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, {.f=1.5625E-6}, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"smod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 6}, },
};

DeviceEntry(36, DS2740, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS2751[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2751, &L2751, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2751}, },

	{"amphours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_ah, FS_w_ah, VISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"currentbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_abias, FS_w_abias, VISIBLE, NO_FILETYPE_DATA, },
	{"lock", PROPERTY_LENGTH_YESNO, &L2751, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2751}, },
	{"PIO", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"sensed", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"defaultpmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG_INIT << 8) | 5}, },
	{"pmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 5}, },
	{"por", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 0}, },
	{"uven", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 3}, },
	F_thermocouple
};

DeviceEntry(51, DS2751, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS2755[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2755, &L2755, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2755}, },

	{"lock", PROPERTY_LENGTH_YESNO, &L2755, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2751}, },
	{"PIO", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"sensed", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vis_avg", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis_avg, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"alarm_set", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_float, fc_volatile, FS_r_templim, FS_w_templim, VISIBLE, {.u=0x85}, },

	{"defaultpmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG_INIT << 8) | 5}, },
	{"pie1", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 7}, },
	{"pie0", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 6}, },
	{"pmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 5}, },
	{"rnaop", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 4}, },
	{"uven", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 3}, },
	{"ios", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 2}, },
	{"uben", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 1}, },
	{"ovd", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 0}, },
	{"por", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 0}, },
	F_thermocouple
};

DeviceEntryExtended(35, DS2755, DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

struct aggregate Aled_control = { 4, ag_numbers, ag_aggregate, };
static struct filetype DS2760[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2760, &L2760, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2760}, },

	{"amphours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_ah, FS_w_ah, VISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"currentbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_abias, FS_w_abias, VISIBLE, NO_FILETYPE_DATA, },
	{"lock", PROPERTY_LENGTH_YESNO, &L2760, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2760}, },
	{"PIO", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"sensed", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 6}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"cc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 3}, },
	{"ce", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 1}, },
	{"coc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 5}, },
	{"defaultpmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG_INIT << 8) | 5}, },
	{"defaultswen", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG_INIT << 8) | 3}, },
	{"dc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 2}, },
	{"de", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 0}, },
	{"doc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 4}, },
	{"mstr", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 5}, },
	{"ov", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 7}, },
	{"ps", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2760_SPECIAL_REG << 8) | 7}, },
	{"pmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 5}, },
	{"swen", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 3}, },
	{"uv", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_PROTECT_REG << 8) | 6}, },
	F_thermocouple

	{"WS603", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/calibration", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/calibration/wind_speed", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_r_wind_calibration, FS_WS603_w_wind_calibration, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/calibration/direction", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_r_direction_calibration, FS_WS603_w_direction_calibration, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_link, FS_WS603_temperature, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/data_string", 5, NON_AGGREGATE, ft_binary, fc_volatile, FS_WS603_r_data, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"WS603/param_string", 5, NON_AGGREGATE, ft_binary, fc_stable, FS_WS603_r_param, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"WS603/wind_speed", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_WS603_wind_speed, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/direction", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_wind_direction, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/LED", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/LED/status", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_r_led, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/LED/control", PROPERTY_LENGTH_UNSIGNED, &Aled_control, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_WS603_led_control, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/LED/model", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_r_led_model, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/light", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/light/intensity", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_light, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/light/threshold", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_r_light_threshold, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"WS603/volt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_WS603_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

};

DeviceEntry(30, DS2760, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS2770[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2770, &L2770, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2770}, },

	{"amphours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_ah, FS_w_ah, VISIBLE, NO_FILETYPE_DATA, },
	{"current", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_current, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"currentbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_link, FS_r_abias, FS_w_abias, VISIBLE, NO_FILETYPE_DATA, },
	{"lock", PROPERTY_LENGTH_YESNO, &L2770, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2770}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"charge", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_charge, VISIBLE, NO_FILETYPE_DATA, },
	{"cini", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 1}, },
	{"cstat1", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 7}, },
	{"cstat0", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 6}, },
	{"ctype", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 0}, },
	{"defaultpmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG_INIT << 8) | 5}, },
	{"pmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 5}, },
	{"refresh", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_refresh, VISIBLE, NO_FILETYPE_DATA, },
	{"timer", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_timer, FS_w_timer, VISIBLE, NO_FILETYPE_DATA, },
	F_thermocouple
};

DeviceEntry(2E, DS2770, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* DS2780 also includees the DS2775 DS2776 DS2784 */
static struct filetype DS2780[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2780, &L2780, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2780}, },

	{"lock", PROPERTY_LENGTH_YESNO, &L2780, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2780}, },
	{"PIO", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, VISIBLE, {.u=(_1W_DS2780_SPECIAL_REG << 8) | 0}, },
	{"sensed", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS2780_SPECIAL_REG << 8) | 0}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vis_avg", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis_avg, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"chgtf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 7}, },
	{"aef", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 6}, },
	{"sef", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 5}, },
	{"learnf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 4}, },
	{"uvf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 2}, },
	{"porf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 1}, },
	{"nben", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2780_PARAM_REG << 8) | 7}, },
	{"uven", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2780_PARAM_REG << 8) | 6}, },
	{"pmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2780_PARAM_REG << 8) | 5}, },
	{"rnaop", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2780_PARAM_REG << 8) | 4}, },
	{"dc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2780_PARAM_REG << 8) | 3}, },
	F_thermocouple
};

DeviceEntry(32, DS2780, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS2781[] = {
	F_STANDARD,
	{"memory", 256, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", Size2780, &L2780, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, {.v=&P2780}, },

	{"lock", PROPERTY_LENGTH_YESNO, &L2780, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, VISIBLE, {.v=&P2780}, },
	{"PIO", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, VISIBLE, {.u=(_1W_DS2780_SPECIAL_REG << 8) | 0}, },
	{"sensed", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, VISIBLE, {.u=(_1W_DS2780_SPECIAL_REG << 8) | 0}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vbias", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, VISIBLE, NO_FILETYPE_DATA, },
	{"vis", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"vis_offset", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis_off, FS_w_vis_off, VISIBLE, NO_FILETYPE_DATA, },
	{"vis_avg", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vis_avg, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"volthours", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_vh, FS_w_vh, VISIBLE, NO_FILETYPE_DATA, },

	{"aef", PROPERTY_LENGTH_YESNO,NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 4}, },
	{"pmod", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit,VISIBLE,  {.u=(_1W_DS2780_PARAM_REG << 8) | 5}, },
	{"porf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 1}, },
	{"sef", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 5}, },
	{"uven", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS2780_PARAM_REG << 8) | 6}, },
	{"uvf", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, VISIBLE, {.u=(_1W_DS27XX_STATUS_REG << 8) | 2}, },
	F_thermocouple
};

DeviceEntry(3D, DS2781, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* DS2784 - specific */
#define _1W_WRITE_CHALLENGE 0x0C
#define _1W_COMPUTE_MAC_WITHOUT 0x36
#define _1W_COMPUTE_MAC_WITH 0x35
#define _1W_COPY_DATA 0x48
#define _1W_CLEAR_SECRET 0x5A
#define _1W_COMPUTE_NEXT_SECRET_WITHOUT 0x30
#define _1W_COMPUTE_NEXT_SECRET_WITH 0x33
#define _1W_LOCK_SECRET 0x60
#define _1W_REFRESH 0x63
#define _1W_READ_DATA 0x69
#define _1W_LOCK 0x6A
#define _1W_WRITE_DATA 0x6C
#define _1W_SET_OVERDRIVE 0x8B
#define _1W_CLEAR_OVERDRIVE 0x8D
#define _1W_START_CHARGE 0xB5
#define _1W_RECALL_DATA 0xB8
#define _1W_STOP_CHARGE 0xBE
#define _1W_RESET 0xC4

#define _WS603_COMMAND_ADDRESS 0x80
#define _WS603_RESPONSE_ADDRESS 0x88

#define _WS603_READ_DATA 0xA1
#define _WS603_READ_PARAMETER 0xA0
#define _WS603_LED_CONTROL 0xA2
#define _WS603_WIND_SPEED_CALIBRATION 0xA3
#define _WS603_WIND_DIRECTION_CALIBRATION 0xA4

#define WS603_ENDBYTE(b) ( ( ( (b) & 0x0F ) *16 ) + 0x0E )

/* ------- Functions ------------ */

/* DS2406 */
static GOOD_OR_BAD OW_r_sram(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_sram(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_mem(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_recall_eeprom(const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_copy_eeprom(const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_lock(const struct parsedname *pn);
static GOOD_OR_BAD OW_r_int(int *I, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_int(const int *I, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_int8(int *I, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_int8(const int *I, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_cmd(const BYTE cmd, const struct parsedname *pn);
static struct LockPage * Lockpage( const struct parsedname * pn ) ;

static char WS603_CHECKSUM( BYTE * byt, int length ) ;
static GOOD_OR_BAD WS603_Send( BYTE * byt, int length, struct parsedname * pn ) ;
static GOOD_OR_BAD WS603_Get( BYTE cmd, BYTE * byt, int length, struct parsedname * pn ) ;
static GOOD_OR_BAD WS603_Transaction( BYTE * cmd, int length, BYTE * resp, struct parsedname * pn ) ;
static GOOD_OR_BAD WS603_Get_Readings( BYTE * data, struct parsedname * pn ) ;
static GOOD_OR_BAD WS603_Get_Parameters( BYTE * data, struct parsedname * pn ) ;

/* 2406 memory read */
static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	/* read is not a "paged" endeavor, the CRC comes after a full read */
	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E(OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq))) ;
}

/* 2406 memory write */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
	return GB_to_Z_OR_E(OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) +
			((struct LockPage *) OWQ_pn(owq).selected_filetype->data.v)->offset[OWQ_pn(owq).extension], PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_mem
		((BYTE *) OWQ_buffer(owq), OWQ_size(owq),
			OWQ_offset(owq) + ((struct LockPage *) OWQ_pn(owq).selected_filetype->data.v)->offset[OWQ_pn(owq).extension], PN(owq))) ;
}

/* 2406 memory write */
static ZERO_OR_ERROR FS_r_lock(struct one_wire_query *owq)
{
	BYTE data;
	RETURN_ERROR_IF_BAD( OW_r_mem(&data, 1, ((struct LockPage *) OWQ_pn(owq).selected_filetype->data.v)->reg, PN(owq)) ) ;
	OWQ_Y(owq) = UT_getbit(&data, OWQ_pn(owq).extension);
	return 0;
}

static ZERO_OR_ERROR FS_w_lock(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_lock(PN(owq))) ;
}

/* Note, it's EPROM -- write once */
static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	/* write is "byte at a time" -- not paged */
	return GB_to_Z_OR_E(OW_w_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_vis(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	_FLOAT f = 0.;
	RETURN_ERROR_IF_BAD( OW_r_int(&I, _1W_DS27XX_CURRENT, pn) );
	switch (pn->sn[0]) {
	case 0x36:					//DS2740
		f = 6.25E-6;
		if (pn->selected_filetype->data.v) {
			f = pn->selected_filetype->data.f;	// for DS2740BU
		}
		break;
	case 0x51:					//DS2751
	case 0x35:					//DS2755
	case 0x30:					//DS2760
		f = 15.625E-6 / 8;		// Jan Bertelsen's correction
		break;
	case 0x28:					//DS2770
		f = 1.56E-6;
		break;
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		f = 1.5625E-6;
		break;
	}
	OWQ_F(owq) = f * I;
	return 0;
}

// Volt-hours
static ZERO_OR_ERROR FS_r_vis_avg(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	GOOD_OR_BAD ret = gbBAD;
	switch (pn->sn[0]) {
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		ret = OW_r_int(&I, _1W_DS2780_AVERAGE_CURRENT, pn);
		if(GOOD(ret))
			OWQ_F(owq) = .0000015625 * I;
		break;
	case 0x35:					//DS2755
		ret = OW_r_int(&I, _1W_DS2756_AVERAGE_CURRENT, pn);
		if(GOOD(ret))
			OWQ_F(owq) = .000001953 * I;
		break;
	}
	return GB_to_Z_OR_E(ret);
}

// Volt-hours
static ZERO_OR_ERROR FS_r_vh(struct one_wire_query *owq)
{
	int I;
	RETURN_ERROR_IF_BAD( OW_r_int(&I, _1W_DS27XX_ACCUMULATED, PN(owq)) ) ;
	OWQ_F(owq) = .00000625 * I;
	return 0;
}

// Volt-hours
static ZERO_OR_ERROR FS_w_vh(struct one_wire_query *owq)
{
	int I = OWQ_F(owq) / .00000625;
	return GB_to_Z_OR_E(OW_w_int(&I, _1W_DS27XX_ACCUMULATED, PN(owq)));
}

// temperature-limits
static ZERO_OR_ERROR FS_r_templim(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	RETURN_ERROR_IF_BAD( OW_r_int8(&I, pn->selected_filetype->data.u, pn) ) ;
	OWQ_F(owq) = I;
	return 0;
}

// Temperature-limits
static ZERO_OR_ERROR FS_w_templim(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I = OWQ_F(owq);
	if (I < -128) {
		return -ERANGE;
	}
	if (I > 127) {
		return -ERANGE;
	}
	return GB_to_Z_OR_E( OW_w_int8(&I, pn->selected_filetype->data.u, pn) );
}

// timer
static ZERO_OR_ERROR FS_r_timer(struct one_wire_query *owq)
{
	int I;
	RETURN_ERROR_IF_BAD( OW_r_int(&I, _1W_DS2770_TIMER, PN(owq)) ) ;
	OWQ_F(owq) = .015625 * I;
	return 0;
}

// timer
static ZERO_OR_ERROR FS_w_timer(struct one_wire_query *owq)
{
	int I = OWQ_F(owq) / .015625;
	return GB_to_Z_OR_E( OW_w_int(&I, _1W_DS2770_TIMER, PN(owq)) );
}

// Amp-hours -- using 25mOhm internal resistor
static ZERO_OR_ERROR FS_r_ah(struct one_wire_query *owq)
{
	if (FS_r_vh(owq)) {
		return -EINVAL;
	}
	OWQ_F(owq) = OWQ_F(owq) / .025;
	return 0;
}

// Amp-hours -- using 25mOhm internal resistor
static ZERO_OR_ERROR FS_w_ah(struct one_wire_query *owq)
{
	OWQ_F(owq) = OWQ_F(owq) * .025;
	return FS_w_vh(owq);
}

// current offset
static ZERO_OR_ERROR FS_r_vis_off(struct one_wire_query *owq)
{
	int I;
	RETURN_ERROR_IF_BAD( OW_r_int8(&I, 0x7B, PN(owq)) ); 
	OWQ_F(owq) = 1.56E-6 * I;
	return 0;
}

// current offset
static ZERO_OR_ERROR FS_w_vis_off(struct one_wire_query *owq)
{
	int I = OWQ_F(owq) / 1.56E-6;
	if (I < -128) {
		return -ERANGE;
	}
	if (I > 127) {
		return -ERANGE;
	}
	return GB_to_Z_OR_E( OW_w_int8(&I, 0x7B, PN(owq)) );
}

// Current bias -- using 25mOhm internal resistor
static ZERO_OR_ERROR FS_r_abias(struct one_wire_query *owq)
{
	_FLOAT vbias = 0. ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F( &vbias, "vbias", owq ) ;

	OWQ_F(owq) = vbias / .025;
	return z_or_e ;
}

// Current bias -- using 25mOhm internal resistor
static ZERO_OR_ERROR FS_w_abias(struct one_wire_query *owq)
{
	return FS_w_sibling_F( OWQ_F(owq) * .025, "vbias", owq ) ;
}

// Read current using internal 25mOhm resistor and Vis
static ZERO_OR_ERROR FS_r_current(struct one_wire_query *owq)
{
	_FLOAT vis = 0.;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_F( &vis, "vis", owq ) ;

	OWQ_F(owq) = vis / .025;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_r_vbias(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	switch (pn->sn[0]) {
	case 0x51:					//DS2751
	case 0x30:					//DS2760
		RETURN_ERROR_IF_BAD( OW_r_int8(&I, _1W_DS2760_CURRENT_OFFSET, pn) ) ;
		OWQ_F(owq) = 15.625E-6 * I;
		break;
	case 0x35:					//DS2755
		RETURN_ERROR_IF_BAD( OW_r_int8(&I, _1W_DS2760_CURRENT_OFFSET, pn) ) ;
		OWQ_F(owq) = 1.95E-6 * I;
		break;
	case 0x28:					//DS2770
		// really a 2byte value!
		RETURN_ERROR_IF_BAD( OW_r_int(&I, _1W_DS2770_CURRENT_OFFSET, pn) ) ;
		OWQ_F(owq) = 1.5625E-6 * I;
		break;
	case 0x32:					//DS2780
	case 0x3D:					//DS2780
		RETURN_ERROR_IF_BAD( OW_r_int8(&I, _1W_DS2780_CURRENT_OFFSET, pn) ) ;
		OWQ_F(owq) = 1.5625E-6 * I;
		break;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_vbias(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	GOOD_OR_BAD ret = 0;				// assign unnecessarily to avoid compiler warning
	switch (pn->sn[0]) {
	case 0x51:					//DS2751
	case 0x30:					//DS2760
		I = OWQ_F(owq) / 15.625E-6;
		if (I < -128) {
			return -ERANGE;
		}
		if (I > 127) {
			return -ERANGE;
		}
		ret = OW_w_int8(&I, _1W_DS2760_CURRENT_OFFSET, pn);
		break;
	case 0x35:					//DS2755
		I = OWQ_F(owq) / 1.95E-6;
		if (I < -128) {
			return -ERANGE;
		}
		if (I > 127) {
			return -ERANGE;
		}
		ret = OW_w_int8(&I, _1W_DS2760_CURRENT_OFFSET, pn);
		break;
	case 0x28:					//DS2770
		I = OWQ_F(owq) / 1.5625E-6;
		if (I < -32768) {
			return -ERANGE;
		}
		if (I > 32767) {
			return -ERANGE;
		}
		// really 2 bytes
		ret = OW_w_int(&I, _1W_DS2770_CURRENT_OFFSET, pn);
		break;
	case 0x32:					//DS2780
		I = OWQ_F(owq) / 1.5625E-6;
		if (I < -128) {
			return -ERANGE;
		}
		if (I > 127) {
			return -ERANGE;
		}
		ret = OW_w_int8(&I, _1W_DS2780_CURRENT_OFFSET, pn);
		break;
	}
	return GB_to_Z_OR_E( ret );
}

static ZERO_OR_ERROR FS_r_volt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	RETURN_ERROR_IF_BAD( OW_r_int(&I, _1W_DS27XX_VOLTAGE, pn) ) ;
	switch (pn->sn[0]) {
	case 0x3D:					//DS2781
		OWQ_F(owq) = ((int)(I >>5)) * .00976;
		break;
	default:
		OWQ_F(owq) = ((int)(I >>5)) * .00488;
		break;
	}
	return 0;
}

static ZERO_OR_ERROR FS_r_temp(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	size_t off;
	switch (pn->sn[0]) {
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		off = _1W_DS2780_TEMPERATURE;
		break;
	default:
		off = _1W_DS2760_TEMPERATURE;
	}
	RETURN_ERROR_IF_BAD( OW_r_int(&I, off, pn) ) ;
	
	OWQ_F(owq) = ( (int) (I >> 5) ) * .125;
	return 0;
}

static ZERO_OR_ERROR FS_r_bit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int bit = BYTE_MASK(pn->selected_filetype->data.u);
	size_t location = pn->selected_filetype->data.u >> 8;
	BYTE c[1];
	RETURN_ERROR_IF_BAD( OW_r_mem(c, 1, location, pn) ) ;
	OWQ_Y(owq) = UT_getbit(c, bit);
	return 0;
}

static ZERO_OR_ERROR FS_w_bit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int bit = BYTE_MASK(pn->selected_filetype->data.u);
	size_t location = pn->selected_filetype->data.u >> 8;
	BYTE c[1];
	RETURN_ERROR_IF_BAD( OW_r_mem(c, 1, location, pn) ) ;
	UT_setbit(c, bit, OWQ_Y(owq) != 0);
	return GB_to_Z_OR_E( OW_w_mem(c, 1, location, pn) );
}

/* switch PIO sensed*/
static ZERO_OR_ERROR FS_r_pio(struct one_wire_query *owq)
{
	if (FS_r_bit(owq)) {
		return -EINVAL;
	}
	OWQ_Y(owq) = !OWQ_Y(owq);
	return 0;
}

static ZERO_OR_ERROR FS_charge(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_cmd(OWQ_Y(owq) ? _1W_START_CHARGE : _1W_STOP_CHARGE, PN(owq))) ;
}

static ZERO_OR_ERROR FS_refresh(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_cmd(_1W_REFRESH, PN(owq))) ;
}

/* write PIO -- bit 6 */
static ZERO_OR_ERROR FS_w_pio(struct one_wire_query *owq)
{
	OWQ_Y(owq) = !OWQ_Y(owq);
	return FS_w_bit(owq);
}

static ZERO_OR_ERROR FS_WS603_temperature(struct one_wire_query *owq)
{
	return FS_r_sibling_F( &OWQ_F(owq), "temperature", owq ) ;
}

static ZERO_OR_ERROR FS_WS603_r_data( struct one_wire_query *owq)
{
	BYTE data[5] ;

	RETURN_ERROR_IF_BAD( WS603_Get_Readings( data, PN(owq) ) ) ;
	return OWQ_format_output_offset_and_size( (char *) data, 5, owq);
}

static ZERO_OR_ERROR FS_WS603_r_param( struct one_wire_query *owq)
{
	BYTE data[5] ;

	RETURN_ERROR_IF_BAD( WS603_Get_Parameters( data, PN(owq) ) ) ;
	return OWQ_format_output_offset_and_size( (char *) data, 5, owq);
}

static ZERO_OR_ERROR FS_WS603_wind_speed( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;
	UINT wind_cal ;

	if ( FS_r_sibling_binary( data, &length, "WS603/data_string", owq ) ) {
		return -EINVAL ;
	}

	if ( FS_r_sibling_U( &wind_cal, "WS603/calibration/wind_speed", owq ) ) {
		wind_cal = 100 ;
	} else if ( wind_cal == 0 || wind_cal >199 ) {
		wind_cal = 100 ;
	}

	OWQ_F(owq) = data[0]*2.453*1.069*1000*wind_cal/(3600.*100.) ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_wind_direction( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/data_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[1] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_r_led( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/data_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[2] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_light( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/data_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[3] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_volt( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/data_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[4] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_r_led_model( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/param_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[0] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_r_wind_calibration( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/param_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[1] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_w_wind_calibration( struct one_wire_query *owq)
{
	BYTE data[4] = {
		_WS603_WIND_SPEED_CALIBRATION,
		OWQ_U(owq) & 0xFF,
		0x00,
		0x00, } ;
	return GB_to_Z_OR_E( WS603_Send( data, 4, PN(owq) ) ) ;
}

static ZERO_OR_ERROR FS_WS603_r_direction_calibration( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/param_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[2] ;
	return 0 ;
}

static ZERO_OR_ERROR FS_WS603_w_direction_calibration( struct one_wire_query *owq)
{
	BYTE data[4] = {
		_WS603_WIND_DIRECTION_CALIBRATION,
		OWQ_U(owq) & 0xFF,
		0x00,
		0x00, } ;
	return GB_to_Z_OR_E( WS603_Send( data, 4, PN(owq) ) ) ;
}

static ZERO_OR_ERROR FS_WS603_led_control( struct one_wire_query *owq)
{
	BYTE data[7] = {
		_WS603_LED_CONTROL,
		OWQ_array_U(owq, 0) & 0xFF,
		OWQ_array_U(owq, 1) & 0xFF,
		OWQ_array_U(owq, 2) & 0xFF,
		OWQ_array_U(owq, 3) & 0xFF,
		0x00,
		0x00, } ;
	return GB_to_Z_OR_E( WS603_Send( data, 7, PN(owq) ) ) ;
}

static ZERO_OR_ERROR FS_WS603_r_light_threshold( struct one_wire_query *owq)
{
	size_t length = 5 ;
	BYTE data[length] ;

	if ( FS_r_sibling_binary( data, &length, "WS603/param_string", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = data[3] ;
	return 0 ;
}

static struct LockPage * Lockpage( const struct parsedname * pn )
{
	switch( pn->sn[0])
	{
		case 0x31:					//DS2720
			return &P2720 ;
		case 0x36:					//DS2740
			return NULL ;
		case 0x51:					//DS2751
			return &P2751 ;
		case 0x35:					//DS2755
			return &P2755 ;
		case 0x30:					//DS2760
			return &P2760 ;
		case 0x28:					//DS2770
			return &P2770 ;
		case 0x32:					//DS2780
		case 0x3D:					//DS2781
			return &P2780 ;
		default:
			return NULL ;
	}
}

// This routine tests to see if the memory range is eeprom and calls recall_eeprom if it is
// otherwise a good return
static GOOD_OR_BAD OW_recall_eeprom( const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_RECALL_DATA, 0x00, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_END,
	};
	int eeprom_range ;
	struct LockPage * lp = Lockpage(pn) ;
	
	if ( lp == NULL ) {
		LEVEL_DEBUG("No eeprom information for this device.") ;
		return gbGOOD ;
	}
	for ( eeprom_range = 0 ; eeprom_range < lp->pages ; ++eeprom_range ) {
		off_t range_offset = lp->offset[eeprom_range] ;
		if ( (off_t)(offset+size) <= range_offset ) {
			// before this range
			continue ;
		}
		if ( offset >= (off_t)(range_offset + lp->size) ) {
			// after this range
			continue ;
		}
		p[1] = BYTE_MASK(range_offset) ;
		return BUS_transaction(t,pn) ;
	}
	return gbGOOD ;
}

/* just read the sram -- eeprom may need to be recalled if you want it */
static GOOD_OR_BAD OW_r_sram(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_READ_DATA, BYTE_MASK(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_READ(data, size),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	RETURN_BAD_IF_BAD( OW_recall_eeprom(size, offset, pn) ) ;
	return OW_r_sram(data, size, offset, pn) ;
}

/* Special processing for eeprom -- page is the address of a 16 byte page (e.g. 0x20) */
static GOOD_OR_BAD OW_copy_eeprom(const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_COPY_DATA, 0x00, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_END,
	};
	int eeprom_range ;
	struct LockPage * lp = Lockpage(pn) ;
	
	if ( lp == NULL ) {
		LEVEL_DEBUG("No eeprom information for this device.") ;
		return gbGOOD ;
	}
	for ( eeprom_range = 0 ; eeprom_range < lp->pages ; ++eeprom_range ) {
		off_t range_offset = lp->offset[eeprom_range] ;
		if ( (off_t) (offset+size) <= range_offset ) {
			// before this range
			continue ;
		}
		if ( offset >= (off_t)(range_offset + lp->size) ) {
			// after this range
			continue ;
		}
		p[1] = BYTE_MASK(range_offset) ;
		return BUS_transaction(t,pn) ;
	}
	return gbGOOD ;
}

static GOOD_OR_BAD OW_w_sram(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_WRITE_DATA, BYTE_MASK(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_WRITE(data, size),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_cmd(const BYTE cmd, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&cmd),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_w_mem(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	RETURN_BAD_IF_BAD( OW_recall_eeprom(size, offset, pn) ) ;
	RETURN_BAD_IF_BAD( OW_w_sram(data, size, offset, pn) ) ;
	return OW_copy_eeprom(size, offset, pn) ;
}

static GOOD_OR_BAD OW_r_int(int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[2];
	RETURN_BAD_IF_BAD( OW_r_sram(i, 2, offset, pn) );
	// Data in the DS27xx is stored MSB/LSB -- different from DS2438 for example
	// Thanks to Jan Bertelsen for finding this!
	I[0] = (((int) ((int8_t) i[0]) << 8) + ((uint8_t) i[1]));
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_int(const int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[2] = { BYTE_MASK(I[0] >> 8), BYTE_MASK(I[0]), };
	return OW_w_sram(i, 2, offset, pn);
}

static GOOD_OR_BAD OW_r_int8(int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[1];
	
	RETURN_BAD_IF_BAD( OW_r_sram(i, 1, offset, pn) );
	//I[0] = ((int)((int8_t)i[0]) ) ;
	I[0] = UT_int8(i);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_int8(const int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[1] = { BYTE_MASK(I[0]), };
	return OW_w_sram(i, 1, offset, pn);
}

static GOOD_OR_BAD OW_lock(const struct parsedname *pn)
{
	BYTE lock[] = { _1W_WRITE_DATA, 0x07, 0x40, _1W_LOCK, 0x00 };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE(lock, 5),
		TRXN_END,
	};

	UT_setbit(&lock[2], pn->extension, 1);
	lock[4] = ((struct LockPage *) pn->selected_filetype->data.v)->offset[pn->extension];

	return BUS_transaction(t, pn);
}

static ZERO_OR_ERROR FS_rangelow(struct one_wire_query *owq)
{
	OWQ_F(owq) = Thermocouple_range_low(OWQ_pn(owq).selected_filetype->data.i);
	return 0;
}

static ZERO_OR_ERROR FS_rangehigh(struct one_wire_query *owq)
{
	OWQ_F(owq) = Thermocouple_range_high(OWQ_pn(owq).selected_filetype->data.i);
	return 0;
}

static ZERO_OR_ERROR FS_thermocouple(struct one_wire_query *owq)
{
	_FLOAT T_coldjunction, vis, mV;

	/* Get measured voltage */
	if ( FS_r_sibling_F( &vis, "vis", owq )  != 0 ) {
		return -EINVAL ;
	}
	mV = vis * 1000;	/* convert Volts to mVolts */

	/* Get cold junction temperature */
	if ( FS_r_sibling_F( &T_coldjunction, "temperature", owq ) != 0 ) {
		return -EINVAL ;
	}

	OWQ_F(owq) = ThermocoupleTemperature(mV, T_coldjunction, OWQ_pn(owq).selected_filetype->data.i);

	return 0;
}

// Based on Les Arbes notes http://www.lesarbresdesign.info/automation/ws603-weather-instrument
static GOOD_OR_BAD WS603_Send( BYTE * byt, int length, struct parsedname * pn )
{
	int repeat ;
	BYTE resp[length] ;

	// Finish setting up string
	byt[length-2] = WS603_CHECKSUM( byt, length-2 ) ;
	byt[length-1] = WS603_ENDBYTE( byt[0] ) ;

	// Send the command and check that it was received
	for ( repeat = 0 ; repeat < 20;  ++repeat ) {
		if ( GOOD(OW_w_sram( byt, length, _WS603_COMMAND_ADDRESS, pn ) )
		&& ( GOOD( OW_r_sram( resp, length, _WS603_COMMAND_ADDRESS, pn ) ) )
		) {
			if ( memcmp( byt, resp, length ) == 0 ) {
				return gbGOOD ;
			}
		}
		UT_delay(20) ;
	}
	LEVEL_DEBUG("Cannot send command to WS603") ;
	return gbBAD ;
}

// Based on Les Arbes notes http://www.lesarbresdesign.info/automation/ws603-weather-instrument
static GOOD_OR_BAD WS603_Get( BYTE cmd, BYTE * byt, int length, struct parsedname * pn )
{
	int repeat ;

	// Send the command and check that it was received
	for ( repeat = 0 ; repeat < 21;  ++repeat ) {
		UT_delay(20) ;
		if ( GOOD( OW_r_sram( (BYTE *)byt, length, _WS603_RESPONSE_ADDRESS, pn ) ) ) {
			if ( ( byt[0] == cmd ) && ( byt[length-1] == WS603_CHECKSUM( byt, length-1 ) ) ) {
				return gbGOOD ;
			}
		}
	}
	LEVEL_DEBUG("Cannot read response to WS603") ;
	return gbBAD ;
}

static GOOD_OR_BAD WS603_Transaction( BYTE * cmd, int length, BYTE * resp, struct parsedname * pn )
{
	RETURN_BAD_IF_BAD( WS603_Send( cmd, length, pn ) ) ;
	return WS603_Get( cmd[0], resp, 7, pn ) ;
}

static char WS603_CHECKSUM( BYTE * byt, int length )
{
	int l ;
	BYTE sum = 0 ;
	for ( l=0 ; l < length ; ++l ) {
		sum += byt[l] ;
	}
	return sum & 0xFF ;
}

// return 5 bytes of data
static GOOD_OR_BAD WS603_Get_Readings( BYTE * data, struct parsedname * pn )
{
	BYTE resp[7] ;
	BYTE cmd[3] = { _WS603_READ_DATA, 0x00, 0x00, } ;

	RETURN_BAD_IF_BAD( WS603_Transaction( cmd, 3, resp, pn ) ) ;
	memcpy( data, &resp[1], 5 ) ;
	return gbGOOD ;
}

// return 5 bytes of data
static GOOD_OR_BAD WS603_Get_Parameters( BYTE * data, struct parsedname * pn )
{
	BYTE resp[7] ;
	BYTE cmd[3] = { _WS603_READ_PARAMETER, 0x00, 0x00, } ;

	RETURN_BAD_IF_BAD( WS603_Transaction( cmd, 3, resp, pn ) ) ;
	memcpy( data, &resp[1], 5 ) ;
	return gbGOOD ;
}


