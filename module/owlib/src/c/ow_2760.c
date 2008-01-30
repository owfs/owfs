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
READ_FUNCTION(FS_r_voltlim);
WRITE_FUNCTION(FS_w_voltlim);
READ_FUNCTION(FS_r_vbias);
WRITE_FUNCTION(FS_w_vbias);
READ_FUNCTION(FS_r_timer);
WRITE_FUNCTION(FS_w_timer);
READ_FUNCTION(FS_r_bit);
WRITE_FUNCTION(FS_w_bit);
WRITE_FUNCTION(FS_charge);
WRITE_FUNCTION(FS_refresh);

#if OW_THERMOCOUPLE
READ_FUNCTION(FS_thermocouple);
READ_FUNCTION(FS_rangelow);
READ_FUNCTION(FS_rangehigh);
 /* ------- Structures ----------- */


#define F_thermocouple  \
{"typeB"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeB/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_b}, } , \
{"typeB/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_b}, } , \
{"typeB/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_b}, } , \
    \
{"typeE"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeE/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_e}, } , \
{"typeE/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_e}, } , \
{"typeE/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_e}, } , \
    \
{"typeJ"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeJ/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_j}, } , \
{"typeJ/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_j}, } , \
{"typeJ/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_j}, } , \
    \
{"typeK"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeK/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_k}, } , \
{"typeK/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_k}, } , \
{"typeK/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_k}, } , \
    \
{"typeN"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeN/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_n}, } , \
{"typeN/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_n}, } , \
{"typeN/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_n}, } , \
    \
{"typeR"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeR/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_r}, } , \
{"typeR/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_r}, } , \
{"typeR/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_r}, } , \
    \
{"typeS"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeS/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_s}, } , \
{"typeS/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_s}, } , \
{"typeS/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_s}, } , \
    \
{"typeT"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION     , {v:NULL}, } , \
{"typeT/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile,   FS_thermocouple, NO_WRITE_FUNCTION  , {i:e_type_t}, } , \
{"typeT/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangelow, NO_WRITE_FUNCTION     , {i:e_type_t}, } , \
{"typeT/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  ,   FS_rangehigh, NO_WRITE_FUNCTION     , {i:e_type_t}, } ,

#else							/* OW_THERMOCOUPLE */
#define F_thermocouple
#endif							/* OW_THERMOCOUPLE */

struct LockPage {
	int pages;
	size_t reg;
	size_t size;
	off_t offset[3];
};

#define Pages2720   2
#define Pages2751   2
#define Pages2755   3
#define Pages2760	2
#define Pages2770	3
#define Pages2780	2

#define Size2720	4
#define Size2751       16
#define Size2755       32
#define Size2760       16
#define Size2770       16
#define Size2780       16

struct LockPage P2720 = { Pages2720, 0x07, Size2720, {0x20, 0x30, 0x00,}, };
struct LockPage P2751 = { Pages2751, 0x07, Size2751, {0x20, 0x30, 0x00,}, };
struct LockPage P2755 = { Pages2755, 0x07, Size2755, {0x20, 0x40, 0x60,}, };
struct LockPage P2760 = { Pages2760, 0x07, Size2760, {0x20, 0x30, 0x00,}, };
struct LockPage P2770 = { Pages2770, 0x07, Size2770, {0x20, 0x30, 0x40,}, };
struct LockPage P2780 = { Pages2780, 0x1F, Size2780, {0x20, 0x60, 0x00,}, };

struct aggregate L2720 = { Pages2720, ag_numbers, ag_separate };
struct aggregate L2751 = { Pages2751, ag_numbers, ag_separate };
struct aggregate L2755 = { Pages2755, ag_numbers, ag_separate };
struct aggregate L2760 = { Pages2760, ag_numbers, ag_separate };
struct aggregate L2770 = { Pages2770, ag_numbers, ag_separate };
struct aggregate L2780 = { Pages2780, ag_numbers, ag_separate };

struct filetype DS2720[] = {
	F_STANDARD,
  {"lock", PROPERTY_LENGTH_YESNO, &L2720, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2720},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2720, &L2720, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2720},},

  {"cc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x03},},
  {"ce", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x01},},
  {"dc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x02},},
  {"de", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x00},},
  {"doc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x04},},
  {"ot", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x08 << 8) | 0x00},},
  {"ov", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x07},},
  {"psf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x08 << 8) | 0x07},},
  {"uv", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x06},},
};

DeviceEntry(31, DS2720);

struct filetype DS2740[] = {
	F_STANDARD,
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_pio, FS_w_pio, {u:(0x08 << 8) | 0x06},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"vis_B", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {f:1.5625E-6},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"smod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x06},},
};

DeviceEntry(36, DS2740);

struct filetype DS2751[] = {
	F_STANDARD,
  {"amphours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_ah, FS_w_ah, {v:NULL},},
  {"current", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_current, NO_WRITE_FUNCTION, {v:NULL},},
  {"currentbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_abias, FS_w_abias, {v:NULL},},
  {"lock", PROPERTY_LENGTH_YESNO, &L2751, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2751},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2751, &L2751, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2751},},
  {"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, {u:(0x08 << 8) | 0x06},},
  {"sensed", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x08 << 8) | 0x06},},
  {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, {v:NULL},},
  {"vbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, {v:NULL},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"volt", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, {s:0x0C},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"defaultpmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x31 << 8) | 0x05},},
  {"pmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x05},},
  {"por", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x08 << 8) | 0x00},},
  {"uven", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x03},},

	F_thermocouple
};

DeviceEntry(51, DS2751);

struct filetype DS2755[] = {
	F_STANDARD,
  {"lock", PROPERTY_LENGTH_YESNO, &L2755, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2751},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2755, &L2755, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2755},},
  {"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, {u:(0x08 << 8) | 0x06},},
  {"sensed", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x08 << 8) | 0x06},},
  {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, {v:NULL},},
  {"vbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, {v:NULL},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"vis_avg", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis_avg, NO_WRITE_FUNCTION, {v:NULL},},
  {"volt", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, {s:0x0C},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"alarm_set", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"alarm_set/volthigh", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_voltlim, FS_w_voltlim, {u:0x80},},
  {"alarm_set/voltlow", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_voltlim, FS_w_voltlim, {u:0x82},},
  {"alarm_set/temphigh", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_templim, FS_w_templim, {u:0x84},},
  {"alarm_set/templow", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_templim, FS_w_templim, {u:0x85},},

  {"defaultpmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x31 << 8) | 0x05},},
  {"pie1", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x07},},
  {"pie0", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x06},},
  {"pmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x05},},
  {"rnaop", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x04},},
  {"uven", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x03},},
  {"ios", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x02},},
  {"uben", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x01},},
  {"ovd", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x00},},
  {"por", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x08 << 8) | 0x00},},

	F_thermocouple
};

DeviceEntryExtended(35, DS2755, DEV_alarm);

struct filetype DS2760[] = {
	F_STANDARD,
  {"amphours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_ah, FS_w_ah, {v:NULL},},
  {"current", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_current, NO_WRITE_FUNCTION, {v:NULL},},
  {"currentbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_abias, FS_w_abias, {v:NULL},},
  {"lock", PROPERTY_LENGTH_YESNO, &L2760, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2760},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2760, &L2760, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2760},},
  {"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, {u:(0x08 << 8) | 0x06},},
  {"sensed", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x08 << 8) | 0x06},},
  {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, {s:0x18},},
  {"vbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, {v:NULL},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"volt", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, {s:0x0C},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"cc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x03},},
  {"ce", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x01},},
  {"coc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x05},},
  {"defaultpmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x31 << 8) | 0x05},},
  {"defaultswen", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x31 << 8) | 0x03},},
  {"dc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x02},},
  {"de", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x00},},
  {"doc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x04},},
  {"mstr", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x08 << 8) | 0x05},},
  {"ov", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x07},},
  {"ps", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x08 << 8) | 0x07},},
  {"pmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x05},},
  {"swen", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x01 << 8) | 0x03},},
  {"uv", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x00 << 8) | 0x06},},

	F_thermocouple
};

DeviceEntry(30, DS2760);

struct filetype DS2770[] = {
	F_STANDARD,
  {"amphours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_ah, FS_w_ah, {v:NULL},},
  {"current", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_current, NO_WRITE_FUNCTION, {v:NULL},},
  {"currentbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_abias, FS_w_abias, {v:NULL},},
  {"lock", PROPERTY_LENGTH_YESNO, &L2770, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2770},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2770, &L2770, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2770},},
  {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, {v:NULL},},
  {"vbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, {v:NULL},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"volt", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, {s:0x0C},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"charge", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_charge, {v:NULL},},
  {"cini", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x01},},
  {"cstat1", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x07},},
  {"cstat0", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x06},},
  {"ctype", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x00},},
  {"defaultpmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x31 << 8) | 0x05},},
  {"pmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x05},},
  {"refresh", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_refresh, {v:NULL},},
  {"timer", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_timer, FS_w_timer, {v:NULL},},

	F_thermocouple
};

DeviceEntry(2E, DS2770);

struct filetype DS2780[] = {
	F_STANDARD,
  {"lock", PROPERTY_LENGTH_YESNO, &L2780, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2780},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2780, &L2780, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2780},},
  {"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, {u:(0x15 << 8) | 0x00},},
  {"sensed", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x15 << 8) | 0x00},},
  {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, {v:NULL},},
  {"vbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, {v:NULL},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"vis_avg", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis_avg, NO_WRITE_FUNCTION, {v:NULL},},
  {"volt", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, {s:0x0C},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"chgtf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x07},},
  {"aef", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x06},},
  {"sef", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x05},},
  {"learnf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x04},},
  {"uvf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x02},},
  {"porf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x01},},
  {"nben", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x07},},
  {"uven", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x06},},
  {"pmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x05},},
  {"rnaop", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x04},},
  {"dc", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x03},},

	F_thermocouple
};

DeviceEntry(32, DS2780);

struct filetype DS2781[] = {
	F_STANDARD,
  {"lock", PROPERTY_LENGTH_YESNO, &L2780, ft_yesno, fc_stable, FS_r_lock, FS_w_lock, {v:&P2780},},
  {"memory", 256, NULL, ft_binary, fc_volatile, FS_r_mem, FS_w_mem, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", Size2780, &L2780, ft_binary, fc_volatile, FS_r_page, FS_w_page, {v:&P2780},},
  {"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_w_pio, {u:(0x15 << 8) | 0x00},},
  {"sensed", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, NO_WRITE_FUNCTION, {u:(0x15 << 8) | 0x00},},
  {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_r_temp, NO_WRITE_FUNCTION, {v:NULL},},
  {"vbias", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_vbias, FS_w_vbias, {v:NULL},},
  {"vis", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis, NO_WRITE_FUNCTION, {v:NULL},},
  {"vis_offset", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis_off, FS_w_vis_off, {v:NULL},},
  {"vis_avg", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vis_avg, NO_WRITE_FUNCTION, {v:NULL},},
  {"volt", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_volt, NO_WRITE_FUNCTION, {s:0x0C},},
  {"volthours", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_r_vh, FS_w_vh, {v:NULL},},

  {"aef", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x06},},
  {"chgtf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x07},},
  {"learnf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x04},},
  {"pmod", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x05},},
  {"porf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x01},},
  {"sef", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x05},},
  {"uven", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x60 << 8) | 0x06},},
  {"uvf", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_bit, FS_w_bit, {u:(0x01 << 8) | 0x02},},

	F_thermocouple
};

DeviceEntry(3D, DS2781);

#define _1W_READ_DATA 0x69
#define _1W_WRITE_DATA 0x6C
#define _1W_COPY_DATA 0x48
#define _1W_RECALL_DATA 0xB8
#define _1W_LOCK 0x6A
/* DS2784 - specific */
#define _1W_WRITE_CHALLENGE 0x0C
#define _1W_COMPUTE_MAC_WITHOUT 0x36
#define _1W_COMPUTE_MAC_WITH 0x35
#define _1W_CLEAR_SECRET 0x5A
#define _1W_COMPUTE_NEXT_SECRET_WITHOUT 0x30
#define _1W_COMPUTE_NEXT_SECRET_WITH 0x33
#define _1W_LOCK_SECRET 0x60
#define _1W_SET_OVERDRIVE 0x8B
#define _1W_CLEAR_OVERDRIVE 0x8D
#define _1W_RESET 0xC4

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_sram(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static int OW_w_sram(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static int OW_w_mem(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static int OW_r_eeprom(const size_t page, const size_t size, const off_t offset, const struct parsedname *pn);
static int OW_w_eeprom(const size_t page, const size_t size, const off_t offset, const struct parsedname *pn);
static int OW_lock(const struct parsedname *pn);
static int OW_r_int(int *I, off_t offset, const struct parsedname *pn);
static int OW_w_int(const int *I, off_t offset, const struct parsedname *pn);
static int OW_r_int8(int *I, off_t offset, const struct parsedname *pn);
static int OW_w_int8(const int *I, off_t offset, const struct parsedname *pn);
static int OW_cmd(const BYTE cmd, const struct parsedname *pn);


/* 2406 memory read */
static int FS_r_mem(struct one_wire_query *owq)
{
	/* read is not a "paged" endeavor, the CRC comes after a full read */
	if (OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return OWQ_size(owq);
}

/* 2406 memory write */
static int FS_r_page(struct one_wire_query *owq)
{
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
	if (OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) +
				 ((struct LockPage *) OWQ_pn(owq).selected_filetype->data.v)->offset[OWQ_pn(owq).extension], PN(owq)))
		return -EINVAL;
	return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query *owq)
{
	if (OW_w_mem
		((BYTE *) OWQ_buffer(owq), OWQ_size(owq),
		 OWQ_offset(owq) + ((struct LockPage *) OWQ_pn(owq).selected_filetype->data.v)->offset[OWQ_pn(owq).extension], PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2406 memory write */
static int FS_r_lock(struct one_wire_query *owq)
{
	BYTE data;
	if (OW_r_mem(&data, 1, ((struct LockPage *) OWQ_pn(owq).selected_filetype->data.v)->reg, PN(owq)))
		return -EINVAL;
	OWQ_Y(owq) = UT_getbit(&data, OWQ_pn(owq).extension);
	return 0;
}

static int FS_w_lock(struct one_wire_query *owq)
{
	if (OW_lock(PN(owq)))
		return -EINVAL;
	return 0;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(struct one_wire_query *owq)
{
	/* write is "byte at a time" -- not paged */
	if (OW_w_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_vis(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	_FLOAT f = 0.;
	if (OW_r_int(&I, 0x0E, pn))
		return -EINVAL;
	switch (pn->sn[0]) {
	case 0x36:					//DS2740
		f = 6.25E-6;
		if (pn->selected_filetype->data.v)
			f = pn->selected_filetype->data.f;	// for DS2740BU
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
static int FS_r_vis_avg(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	int ret = 1;
	switch (pn->sn[0]) {
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		ret = OW_r_int(&I, 0x08, pn);
		OWQ_F(owq) = .0000015625 * I;
		break;
	case 0x35:					//DS2755
		ret = OW_r_int(&I, 0x1A, pn);
		OWQ_F(owq) = .000001953 * I;
		break;
	}
	return ret ? -EINVAL : 0;
}

// Volt-hours
static int FS_r_vh(struct one_wire_query *owq)
{
	int I;
	if (OW_r_int(&I, 0x10, PN(owq)))
		return -EINVAL;
	OWQ_F(owq) = .00000625 * I;
	return 0;
}

// Volt-hours
static int FS_w_vh(struct one_wire_query *owq)
{
	int I = OWQ_F(owq) / .00000625;
	return OW_w_int(&I, 0x10, PN(owq)) ? -EINVAL : 0;
}

// Volt-limits
static int FS_r_voltlim(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	if (OW_r_int(&I, pn->selected_filetype->data.u, pn))
		return -EINVAL;
	OWQ_F(owq) = .00000625 * I;
	return 0;
}

// Volt-limits
static int FS_w_voltlim(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I = OWQ_F(owq) / .00000625;
	return OW_w_int(&I, pn->selected_filetype->data.u, pn) ? -EINVAL : 0;
}

// Volt-limits
static int FS_r_templim(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	if (OW_r_int8(&I, pn->selected_filetype->data.u, pn))
		return -EINVAL;
	OWQ_F(owq) = I;
	return 0;
}

// Volt-limits
static int FS_w_templim(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I = OWQ_F(owq);
	if (I < -128)
		return -ERANGE;
	if (I > 127)
		return -ERANGE;
	return OW_w_int8(&I, pn->selected_filetype->data.u, pn) ? -EINVAL : 0;
}

// timer
static int FS_r_timer(struct one_wire_query *owq)
{
	int I;
	if (OW_r_int(&I, 0x02, PN(owq)))
		return -EINVAL;
	OWQ_F(owq) = .015625 * I;
	return 0;
}

// timer
static int FS_w_timer(struct one_wire_query *owq)
{
	int I = OWQ_F(owq) / .015625;
	return OW_w_int(&I, 0x02, PN(owq)) ? -EINVAL : 0;
}

// Amp-hours -- using 25mOhm internal resistor
static int FS_r_ah(struct one_wire_query *owq)
{
	if (FS_r_vh(owq))
		return -EINVAL;
	OWQ_F(owq) = OWQ_F(owq) / .025;
	return 0;
}

// Amp-hours -- using 25mOhm internal resistor
static int FS_w_ah(struct one_wire_query *owq)
{
	OWQ_F(owq) = OWQ_F(owq) * .025;
	return FS_w_vh(owq);
}

// current offset
static int FS_r_vis_off(struct one_wire_query *owq)
{
	int I;
	if (OW_r_int8(&I, 0x7B, PN(owq)))
		return -EINVAL;
	OWQ_F(owq) = 1.56E-6 * I;
	return 0;
}

// current offset
static int FS_w_vis_off(struct one_wire_query *owq)
{
	int I = OWQ_F(owq) / 1.56E-6;
	if (I < -128)
		return -ERANGE;
	if (I > 127)
		return -ERANGE;
	return OW_w_int8(&I, 0x7B, PN(owq)) ? -EINVAL : 0;
}

// Current bias -- using 25mOhm internal resistor
static int FS_r_abias(struct one_wire_query *owq)
{
	OWQ_allocate_struct_and_pointer(owq_sibling);
	OWQ_create_shallow_single(owq_sibling, owq);

	if (FS_read_sibling("vbias", owq_sibling))
		return -EINVAL;

	OWQ_F(owq) = OWQ_F(owq_sibling) / .025;
	return 0;
}

// Current bias -- using 25mOhm internal resistor
static int FS_w_abias(struct one_wire_query *owq)
{
	OWQ_allocate_struct_and_pointer(owq_sibling);
	OWQ_create_shallow_single(owq_sibling, owq);

	OWQ_F(owq_sibling) = OWQ_F(owq) * .025;

	return FS_write_sibling("vbias", owq_sibling) ? -EINVAL : 0;
}

// Read current using internal 25mOhm resistor and Vis
static int FS_r_current(struct one_wire_query *owq)
{
	OWQ_allocate_struct_and_pointer(owq_sibling);

	OWQ_create_shallow_single(owq_sibling, owq);

	if (FS_read_sibling("vis", owq_sibling))
		return -EINVAL;

	OWQ_F(owq) = OWQ_F(owq_sibling) / .025;
	return 0;
}

static int FS_r_vbias(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	switch (pn->sn[0]) {
	case 0x51:					//DS2751
	case 0x30:					//DS2760
		if (OW_r_int8(&I, 0x33, pn))
			return -EINVAL;
		OWQ_F(owq) = 15.625E-6 * I;
		break;
	case 0x35:					//DS2755
		if (OW_r_int8(&I, 0x33, pn))
			return -EINVAL;
		OWQ_F(owq) = 1.95E-6 * I;
		break;
	case 0x28:					//DS2770
		// really a 2byte value!
		if (OW_r_int(&I, 0x32, pn))
			return -EINVAL;
		OWQ_F(owq) = 1.5625E-6 * I;
		break;
	case 0x32:					//DS2780
	case 0x3D:					//DS2780
		if (OW_r_int8(&I, 0x61, pn))
			return -EINVAL;
		OWQ_F(owq) = 1.5625E-6 * I;
		break;
	}
	return 0;
}

static int FS_w_vbias(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	int ret = 0;				// assign unnecessarily to avoid compiler warning
	switch (pn->sn[0]) {
	case 0x51:					//DS2751
	case 0x30:					//DS2760
		I = OWQ_F(owq) / 15.625E-6;
		if (I < -128)
			return -ERANGE;
		if (I > 127)
			return -ERANGE;
		ret = OW_w_int8(&I, 0x33, pn);
		break;
	case 0x35:					//DS2755
		I = OWQ_F(owq) / 1.95E-6;
		if (I < -128)
			return -ERANGE;
		if (I > 127)
			return -ERANGE;
		ret = OW_w_int8(&I, 0x33, pn);
		break;
	case 0x28:					//DS2770
		I = OWQ_F(owq) / 1.5625E-6;
		if (I < -32768)
			return -ERANGE;
		if (I > 32767)
			return -ERANGE;
		// really 2 bytes
		ret = OW_w_int(&I, 0x32, pn);
		break;
	case 0x32:					//DS2780
		I = OWQ_F(owq) / 1.5625E-6;
		if (I < -128)
			return -ERANGE;
		if (I > 127)
			return -ERANGE;
		ret = OW_w_int8(&I, 0x61, pn);
		break;
	}
	return ret ? -EINVAL : 0;
}

static int FS_r_volt(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	if (OW_r_int(&I, 0x0C, pn))
		return -EINVAL;
	switch (pn->sn[0]) {
	case 0x3D:					//DS2781
		OWQ_F(owq) = (I / 32) * .00976;
		break;
	default:
		OWQ_F(owq) = (I / 32) * .00488;
		break;
	}
	return 0;
}

static int FS_r_temp(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int I;
	size_t off;
	switch (pn->sn[0]) {
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		off = 0x0A;
		break;
	default:
		off = 0x18;
	}
	if (OW_r_int(&I, off, pn))
		return -EINVAL;
	OWQ_F(owq) = (I / 32) * .125;
	return 0;
}

static int FS_r_bit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int bit = BYTE_MASK(pn->selected_filetype->data.u);
	size_t location = pn->selected_filetype->data.u >> 8;
	BYTE c[1];
	if (OW_r_mem(c, 1, location, pn))
		return -EINVAL;
	OWQ_Y(owq) = UT_getbit(c, bit);
	return 0;
}

static int FS_w_bit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int bit = BYTE_MASK(pn->selected_filetype->data.u);
	size_t location = pn->selected_filetype->data.u >> 8;
	BYTE c[1];
	if (OW_r_mem(c, 1, location, pn))
		return -EINVAL;
	UT_setbit(c, bit, OWQ_Y(owq) != 0);
	return OW_w_mem(c, 1, location, pn) ? -EINVAL : 0;
}

/* switch PIO sensed*/
static int FS_r_pio(struct one_wire_query *owq)
{
	if (FS_r_bit(owq))
		return -EINVAL;
	OWQ_Y(owq) = !OWQ_Y(owq);
	return 0;
}

static int FS_charge(struct one_wire_query *owq)
{
	if (OW_cmd(OWQ_Y(owq) ? 0xB5 : 0xBE, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_refresh(struct one_wire_query *owq)
{
	if (OW_cmd(0x63, PN(owq)))
		return -EINVAL;
	return 0;
}

/* write PIO -- bit 6 */
static int FS_w_pio(struct one_wire_query *owq)
{
	OWQ_Y(owq) = !OWQ_Y(owq);
	return FS_w_bit(owq);
}

static int OW_r_eeprom(const size_t page, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_RECALL_DATA, BYTE_MASK(page), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_END,
	};
	if (offset > (off_t) page + 16)
		return 0;
	if (offset + size <= page)
		return 0;
	return BUS_transaction(t, pn);
}

/* just read the sram -- eeprom may need to be recalled if you want it */
static int OW_r_sram(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
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

static int OW_r_mem(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	return OW_r_eeprom(0x20, size, offset, pn)
		|| OW_r_eeprom(0x30, size, offset, pn)
		|| OW_r_sram(data, size, offset, pn);
}

/* Special processing for eeprom -- page is the address of a 16 byte page (e.g. 0x20) */
static int OW_w_eeprom(const size_t page, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_COPY_DATA, BYTE_MASK(page), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_END,
	};
	if (offset > (off_t) page + 16)
		return 0;
	if (offset + size <= page)
		return 0;
	return BUS_transaction(t, pn);
}

static int OW_w_sram(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { _1W_WRITE_DATA, BYTE_MASK(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_READ(data, size),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_cmd(const BYTE cmd, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(&cmd),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_w_mem(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	return OW_r_eeprom(0x20, size, offset, pn)
		|| OW_r_eeprom(0x30, size, offset, pn)
		|| OW_w_sram(data, size, offset, pn)
		|| OW_w_eeprom(0x20, size, offset, pn)
		|| OW_w_eeprom(0x30, size, offset, pn);
}

static int OW_r_int(int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[2];
	int ret = OW_r_sram(i, 2, offset, pn);
	if (ret)
		return ret;
	// Data in the DS27xx is stored MSB/LSB -- different from DS2438 for example
	// Thanks to Jan Bertelsen for finding this!
	I[0] = (((int) ((int8_t) i[0]) << 8) + ((uint8_t) i[1]));
	return 0;
}

static int OW_w_int(const int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[2] = { BYTE_MASK(I[0] >> 8), BYTE_MASK(I[0]), };
	return OW_w_sram(i, 2, offset, pn);
}

static int OW_r_int8(int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[1];
	int ret = OW_r_sram(i, 1, offset, pn);
	if (ret)
		return ret;
	//I[0] = ((int)((int8_t)i[0]) ) ;
	I[0] = UT_int8(i);
	return 0;
}

static int OW_w_int8(const int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[1] = { BYTE_MASK(I[0]), };
	return OW_w_sram(i, 1, offset, pn);
}

static int OW_lock(const struct parsedname *pn)
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

#if OW_THERMOCOUPLE
static int FS_rangelow(struct one_wire_query *owq)
{
	OWQ_F(owq) = Thermocouple_range_low(OWQ_pn(owq).selected_filetype->data.i);
	return 0;
}

static int FS_rangehigh(struct one_wire_query *owq)
{
	OWQ_F(owq) = Thermocouple_range_high(OWQ_pn(owq).selected_filetype->data.i);
	return 0;
}

static int FS_thermocouple(struct one_wire_query *owq)
{
	_FLOAT T_coldjunction, mV;
	OWQ_allocate_struct_and_pointer(owq_sibling);

	OWQ_create_shallow_single(owq_sibling, owq);

	/* Get measured voltage */
	if (FS_read_sibling("vis", owq_sibling))
		return -EINVAL;
	mV = OWQ_F(owq_sibling) * 1000;	/* convert Volts to mVolts */

	/* Get cold junction temperature */
	if (FS_read_sibling("temperature", owq_sibling))
		return -EINVAL;
	T_coldjunction = OWQ_F(owq_sibling);

	OWQ_F(owq) = ThermocoupleTemperature(mV, T_coldjunction, OWQ_pn(owq).selected_filetype->data.i);

	return 0;
}
#endif							/* OW_THERMOCOUPLE */
