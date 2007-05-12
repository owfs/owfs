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
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_temp);
READ_FUNCTION(FS_volts);
READ_FUNCTION(FS_Humid);
READ_FUNCTION(FS_Humid_1735);
READ_FUNCTION(FS_Humid_4000);
READ_FUNCTION(FS_Current);
READ_FUNCTION(FS_r_Ienable);
WRITE_FUNCTION(FS_w_Ienable);
READ_FUNCTION(FS_r_Offset);
WRITE_FUNCTION(FS_w_Offset);
READ_FUNCTION(FS_r_counter);
WRITE_FUNCTION(FS_w_counter);
READ_FUNCTION(FS_r_date);
WRITE_FUNCTION(FS_w_date);
READ_FUNCTION(FS_MStype);

/* ------- Structures ----------- */

struct aggregate A2437 = { 8, ag_numbers, ag_separate, };
struct filetype DS2437[] = {
	F_STANDARD,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"pages/page", 8, &A2437, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 8, &A2437, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 8, &A2437, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 8, &A2437, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:1},},  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:1},},  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:1},},  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_volts}, {o: NULL}, {i:1},},
  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:0},},  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:0},},  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:0},},  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_volts}, {o: NULL}, {i:0},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_temp}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_temp}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_temp}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_temp}, {o: NULL}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Current}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Current}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Current}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_Current}, {o: NULL}, {v:NULL},},
  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Ienable}, {o:FS_w_Ienable}, {v:NULL},},  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Ienable}, {o:FS_w_Ienable}, {v:NULL},},  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Ienable}, {o:FS_w_Ienable}, {v:NULL},},  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_Ienable}, {o: FS_w_Ienable}, {v:NULL},},
  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x08},},  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x08},},  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x08},},  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second, {o: FS_r_counter}, {o: FS_w_counter}, {s:0x08},},
  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {s:0x08},},  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {s:0x08},},  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {s:0x08},},  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second, {o: FS_r_date}, {o: FS_w_date}, {s:0x08},},
  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x10},},  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x10},},  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x10},},  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_counter}, {o: FS_w_counter}, {s:0x10},},
  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x10},},  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x10},},  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x10},},  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {o: FS_r_date}, {o: FS_w_date}, {s:0x10},},
  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x14},},  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x14},},  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x14},},  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_counter}, {o: FS_w_counter}, {s:0x14},},
  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x14},},  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x14},},  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x14},},  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {o: FS_r_date}, {o: FS_w_date}, {s:0x14},},
};

DeviceEntryExtended(1E, DS2437, DEV_temp | DEV_volt);


struct aggregate A2438 = { 8, ag_numbers, ag_separate, };
struct filetype DS2438[] = {
	F_STANDARD,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"pages/page", 8, &A2438, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 8, &A2438, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 8, &A2438, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 8, &A2438, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:1},},  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:1},},  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:1},},  {"VDD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_volts}, {o: NULL}, {i:1},},
  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:0},},  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:0},},  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_volts}, {o:NO_WRITE_FUNCTION}, {i:0},},  {"VAD",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_volts}, {o: NULL}, {i:0},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_temp}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_temp}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_temp}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_temp}, {o: NULL}, {v:NULL},},
  {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_Humid}, {o: NULL}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Current}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Current}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Current}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_Current}, {o: NULL}, {v:NULL},},
  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Ienable}, {o:FS_w_Ienable}, {v:NULL},},  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Ienable}, {o:FS_w_Ienable}, {v:NULL},},  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Ienable}, {o:FS_w_Ienable}, {v:NULL},},  {"Ienable",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_Ienable}, {o: FS_w_Ienable}, {v:NULL},},
  {"offset",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Offset}, {o:FS_w_Offset}, {v:NULL},},  {"offset",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Offset}, {o:FS_w_Offset}, {v:NULL},},  {"offset",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_r_Offset}, {o:FS_w_Offset}, {v:NULL},},  {"offset",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_Offset}, {o: FS_w_Offset}, {v:NULL},},
  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x08},},  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x08},},  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x08},},  {"udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second, {o: FS_r_counter}, {o: FS_w_counter}, {s:0x08},},
  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {s:0x08},},  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {s:0x08},},  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {s:0x08},},  {"date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second, {o: FS_r_date}, {o: FS_w_date}, {s:0x08},},
  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"disconnect",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x10},},  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x10},},  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x10},},  {"disconnect/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_counter}, {o: FS_w_counter}, {s:0x10},},
  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x10},},  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x10},},  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x10},},  {"disconnect/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {o: FS_r_date}, {o: FS_w_date}, {s:0x10},},
  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"endcharge",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x14},},  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x14},},  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_counter}, {o:FS_w_counter}, {s:0x14},},  {"endcharge/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_counter}, {o: FS_w_counter}, {s:0x14},},
  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x14},},  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x14},},  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile,  {o:FS_r_date}, {o:FS_w_date}, {s:0x14},},  {"endcharge/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {o: FS_r_date}, {o: FS_w_date}, {s:0x14},},
  {"HTM1735",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HTM1735",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HTM1735",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HTM1735",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"HTM1735/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid_1735}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HTM1735/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid_1735}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HTM1735/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid_1735}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HTM1735/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_Humid_1735}, {o: NULL}, {v:NULL},},
  {"HIH4000",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HIH4000",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HIH4000",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HIH4000",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"HIH4000/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid_4000}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HIH4000/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid_4000}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HIH4000/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_Humid_4000}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"HIH4000/humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_Humid_4000}, {o: NULL}, {v:NULL},},
  {"MultiSensor",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"MultiSensor",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"MultiSensor",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"MultiSensor",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
  {"MultiSensor/type", 12, NULL, ft_vascii, fc_stable,  {o:FS_MStype}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"MultiSensor/type", 12, NULL, ft_vascii, fc_stable,  {o:FS_MStype}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"MultiSensor/type", 12, NULL, ft_vascii, fc_stable,  {o:FS_MStype}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"MultiSensor/type", 12, NULL, ft_vascii, fc_stable, {o: FS_MStype}, {o: NULL}, {v:NULL},},
};

DeviceEntryExtended(26, DS2438, DEV_temp | DEV_volt);

#define _1W_WRITE_SCRATCHPAD 0x4E
#define _1W_READ_SCRATCHPAD 0xBE
#define _1W_COPY_SCRATCHPAD 0x48
#define _1W_RECALL_SCRATCHPAD 0xB8
#define _1W_CONVERT_T 0x44
#define _1W_CONVERT_V 0xB4

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
static int FS_r_page(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[8];
	if (OW_r_page(data, pn->extension, pn))
		return -EINVAL;
    memcpy((BYTE *) OWQ_buffer(owq), &data[OWQ_offset(owq)], OWQ_size(owq));
    return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    if (OWQ_size(owq) != 8) {			/* partial page */
		BYTE data[8];
		if (OW_r_page(data, pn->extension, pn))
			return -EINVAL;
        memcpy(&data[OWQ_offset(owq)], (BYTE *) OWQ_buffer(owq), OWQ_size(owq));
		if (OW_w_page(data, pn->extension, pn))
			return -EFAULT;
	} else {					/* complete page */
        if (OW_w_page((BYTE *) OWQ_buffer(owq), pn->extension, pn))
			return -EFAULT;
	}
	return 0;
}

static int FS_MStype(struct one_wire_query * owq)
{
	BYTE data[8];
	ASCII *t;
    if (OW_r_page(data, 0, PN(owq)))
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
    return Fowq_output_offset_and_size_z(t, owq);
}

static int FS_temp(struct one_wire_query * owq)
{
    if (OW_temp(&OWQ_F(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_volts(struct one_wire_query * owq)
{
	/* data=1 VDD data=0 VAD */
    if (OW_volts(&OWQ_F(owq), OWQ_pn(owq).ft->data.i, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_Humid(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq);
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
    OWQ_F(owq) = (VAD / VDD - (0.8 / VDD)) / (.0062 * (1.0546 - .00216 * T));

	return 0;
}

static int FS_Humid_4000(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq);
    _FLOAT T, VAD, VDD;
	if (OW_volts(&VDD, 1, pn) || OW_volts(&VAD, 0, pn) || OW_temp(&T, pn))
		return -EINVAL;
    OWQ_F(owq) =
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
static int FS_Humid_1735(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq);
    _FLOAT VAD;
	if (OW_volts(&VAD, 0, pn))
		return -EINVAL;
    OWQ_F(owq) = 38.92 * VAD - 41.98;

	return 0;
}

static int FS_Current(struct one_wire_query * owq)
{
    if (OW_current(&OWQ_F(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_Ienable(struct one_wire_query * owq)
{
    if (OW_r_Ienable(&OWQ_U(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_Ienable(struct one_wire_query * owq)
{
    if (OWQ_U(owq) > 3)
		return -EINVAL;
    if (OW_w_Ienable(OWQ_U(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_Offset(struct one_wire_query * owq)
{
    if (OW_r_int(&OWQ_I(owq), 0x0D, PN(owq)))
		return -EINVAL;			/* page1 byte 5 */
    OWQ_I(owq) >>= 3;
	return 0;
}

static int FS_w_Offset(struct one_wire_query * owq)
{
    int I = OWQ_I(owq);
	if (I > 255 || I < -256)
		return -EINVAL;
    if (OW_w_offset(I << 3, PN(owq)))
		return -EINVAL;
	return 0;
}

/* set clock */
static int FS_w_counter(struct one_wire_query * owq)
{
    _DATE d = (_DATE) OWQ_U(owq);
    OWQ_D(owq) = d ;
    return FS_w_date(owq);
}

/* set clock */
static int FS_w_date(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
	int page = ((uint32_t) (pn->ft->data.s)) >> 3;
	int offset = ((uint32_t) (pn->ft->data.s)) & 0x07;
	BYTE data[8];
	if (OW_r_page(data, page, pn))
		return -EINVAL;
    UT_fromDate(OWQ_D(owq), &data[offset]);
	if (OW_w_page(data, page, pn))
		return -EINVAL;
	return 0;
}

/* read clock */
int FS_r_counter(struct one_wire_query * owq)
{
	_DATE d;
	int ret = FS_r_date(owq);
    d = OWQ_D(owq) ;
    OWQ_U(owq) = (UINT) d;
	return ret;
}

/* read clock */
int FS_r_date(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int page = ((uint32_t) (pn->ft->data.s)) >> 3;
	int offset = ((uint32_t) (pn->ft->data.s)) & 0x07;
	BYTE data[8];
	if (OW_r_page(data, page, pn))
		return -EINVAL;
    OWQ_D(owq) = UT_toDate(&data[offset]);
	return 0;
}

/* DS2438 fancy battery */
static int OW_r_page(BYTE * p, const int page, const struct parsedname *pn)
{
	BYTE data[9];
	BYTE recall[] = { _1W_RECALL_SCRATCHPAD, page, };
	BYTE r[] = { _1W_READ_SCRATCHPAD, page, };
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
	BYTE w[] = { _1W_WRITE_SCRATCHPAD, page, };
	BYTE r[] = { _1W_READ_SCRATCHPAD, page, };
	BYTE eeprom[] = { _1W_COPY_SCRATCHPAD, page, };
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
	static BYTE t[] = { _1W_CONVERT_T, };
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
	static BYTE v[] = { _1W_CONVERT_V, };
	static BYTE w[] = { _1W_WRITE_SCRATCHPAD, 0x00, };
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
	BYTE r[] = { _1W_READ_SCRATCHPAD, 0x00, };
	BYTE w[] = { _1W_WRITE_SCRATCHPAD, 0x00, };
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
