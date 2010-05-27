/*
$Id$
    OW -- One-Wire filesystem

    LICENSE (As of version 2.5p4 2-Oct-2006)
    owlib: GPL v2
    owfs, owhttpd, owftpd, owserver: GPL v2
    owshell(owdir owread owwrite owpresent): GPL v2
    owcapi (libowcapi): GPL v2
    owperl: GPL v2
    owtcl: LGPL v2
    owphp: GPL v2
    owpython: GPL v2
    owsim.tcl: GPL v2
    where GPL v2 is the "Gnu General License version 2"
    and "LGPL v2" is the "Lesser Gnu General License version 2"

    Written 2003 Paul H Alfille
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/

#ifndef OW_DEVICE_H				/* tedious wrapper */
#define OW_DEVICE_H

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"
#include "ow_parsedname.h"

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Predeclare struct filetype */
struct filetype;

/* -------------------------------- */
/* Devices -- types of 1-wire chips */
/*
device structure corresponds to 1-wire device
also to virtual devices, like statistical groupings
and interfaces (LINK, DS2408, ... )

devices have a list or properties that appear as
files under the device directory, they correspond
to device features (memory, name, temperature) and
bound the allowable files in a device directory
*/

	/* supports RESUME command */
#define DEV_resume  0x0001
	/* can trigger an alarm */
#define DEV_alarm   0x0002
	/* support OVERDRIVE */
#define DEV_ovdr    0x0004
	/* responds to simultaneous temperature convert 0x44 */
#define DEV_temp    0x8000
	/* responds to simultaneous voltage convert 0x3C */
#define DEV_volt    0x4000
	/* supports CHAIN command */
#define DEV_chain   0x2000

#include "ow_generic_read.h"
#include "ow_generic_write.h"

struct device {
	const char *family_code;
	char *readable_name;
	uint32_t flags;
	int count_of_filetypes;
	struct filetype *filetype_array;
	struct generic_read * g_read ;
	struct generic_write * g_write ;
};

#define DeviceHeader( chip )    extern struct device d_##chip

/* Entries for struct device */
/* Cannot set the 3rd element (number of filetypes) at compile time because
   filetype arrays aren;t defined at this point */
#define COUNT_OF_FILETYPES(filetype_array) ((int)(sizeof(filetype_array)/sizeof(struct filetype)))

#define DeviceEntryExtended( code , chip , flags, gread, gwrite )  struct device d_##chip = {#code,#chip,flags,COUNT_OF_FILETYPES(chip),chip,gread,gwrite}

#define DeviceEntry( code , chip, gread, gwrite )  DeviceEntryExtended( code, chip, 0, gread, gwrite )

/* Device tree for matching names */
/* Bianry tree implementation */
/* A separate root for each device type: real, statistics, settings, system, structure */
extern void *Tree[ePN_max_type];

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */
struct device_opaque {
	struct device *key;
	void *other;
};

/* Must be sorted for bsearch */
//extern struct device * Devices[] ;
//extern size_t nDevices ;
extern struct device NoDevice;
extern struct device AnyDevice; // returned from remote DIR listing.
extern struct device *DeviceSimultaneous;
extern struct device *DeviceThermostat;

/* ---- end device --------------------- */
#endif							/* OW_DEVICE_H */
