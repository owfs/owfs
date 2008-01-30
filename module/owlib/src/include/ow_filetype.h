/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions

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
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

#ifndef OW_FILETYPE_H			/* tedious wrapper */
#define OW_FILETYPE_H

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Filetypes -- directory entries for each 1-wire chip found */
/*
Filetype is the most elaborate of the internal structures, though
simple in concept.

Actually a little misnamed. Each filetype corresponds to a device
property, and to a file in the file system (though there are Filetype
entries for some directory elements too)

Filetypes belong to a particular device. (i.e. each device has it's list
of filetypes) and have a name and pointers to processing functions. Filetypes
also have a data format (integer, ascii,...) and a data length, and an indication
of whether the property is static, changes only on command, or is volatile.

Some properties occur are arrays (pages of memory, logs of temperature
values). The "aggregate" structure holds to allowable size, and the method
of access. -- Aggregate properties are either accessed all at once, then
split, or accessed individually. The choice depends on the device hardware.
There is even a further wrinkle: mixed. In cases where the handling can be either,
mixed causes separate handling of individual items are querried, and combined
if ALL are requested. This is useful for the DS2450 quad A/D where volt and PIO functions
step on each other, but the conversion time for individual is rather costly.
 */

enum ag_index { ag_numbers, ag_letters, };
enum ag_combined { ag_separate, ag_aggregate, ag_mixed, };
/* aggregate defines array properties */
struct aggregate {
	int elements;				/* Maximum number of elements */
	enum ag_index letters;		/* name them with letters or numbers */
	enum ag_combined combined;	/* Combined bitmaps properties, or separately addressed */
};

	/* property format, controls web display */
/* Some explanation of ft_format:
     Each file type is either a device (physical 1-wire chip or virtual statistics container).
     or a file (property).
     The devices act as directories of properties.
     The files are either properties of the device, or sometimes special directories themselves.
     If properties, they can be integer, text, etc or special directory types.
     There is also the directory type, ft_directory reflects a branch type, which restarts the parsing process.
*/
enum ft_format {
	ft_directory,
	ft_subdir,
	ft_integer,
	ft_unsigned,
	ft_float,
	ft_ascii,
	ft_vascii,					// variable length ascii -- must be read and measured.
	ft_binary,
	ft_yesno,
	ft_date,
	ft_bitfield,
	ft_temperature,
	ft_tempgap,
};
	/* property changability. Static unchanged, Stable we change, Volatile changes */
enum fc_change { fc_local, fc_static, fc_stable, fc_Astable, fc_volatile,
	fc_Avolatile, fc_second, fc_statistic, fc_persistent, fc_directory,
	fc_presence,
};

/* Predeclare one_wire_query */
struct one_wire_query;

#define PROPERTY_LENGTH_INTEGER   12
#define PROPERTY_LENGTH_UNSIGNED  12
#define PROPERTY_LENGTH_BITFIELD  12
#define PROPERTY_LENGTH_FLOAT     12
#define PROPERTY_LENGTH_TEMP      12
#define PROPERTY_LENGTH_TEMPGAP   12
#define PROPERTY_LENGTH_DATE      24
#define PROPERTY_LENGTH_YESNO      1
#define PROPERTY_LENGTH_STRUCTURE 30
#define PROPERTY_LENGTH_DIRECTORY  8
#define PROPERTY_LENGTH_SUBDIR     0

#define NO_READ_FUNCTION NULL
#define NO_WRITE_FUNCTION NULL
/* filetype gives -file types- for chip properties */
struct filetype {
	char *name;
	int suglen;					// length of field
	struct aggregate *ag;		// struct pointer for aggregate
	enum ft_format format;		// type of data
	enum fc_change change;		// volatility
	int (*read) (struct one_wire_query *);	// read callback function
	int (*write) (struct one_wire_query *);	// write callback function
	union {
		void *v;
		int i;
		UINT u;
		_FLOAT f;
		size_t s;
		BYTE c;
	} data;						// extra data pointer (used for separating similar but differently name functions)
};

/* --------- end Filetype -------------------- */

#endif							/* OW_FILETYPE_H */
