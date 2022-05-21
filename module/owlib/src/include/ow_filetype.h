/*
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

enum ag_combined { 
	ag_separate, 
	ag_aggregate, 
	ag_mixed, 
	ag_sparse,
	};

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
     There is also the directory type, ft_directory reflects a branch type, 
     * which restarts the parsing process.
*/
enum ft_format {
	ft_unknown,
	ft_directory,
	ft_subdir,
	ft_integer,
	ft_unsigned,
	ft_float,
	ft_alias,        // A human readable name in lue of numeric ID
	ft_ascii,
	ft_vascii,					// variable length ascii -- must be read and measured.
	ft_binary,
	ft_yesno,
	ft_date,
	ft_bitfield,
	ft_temperature,
	ft_tempgap,
	ft_pressure,
};

	/* property changability. Static unchanged, Stable we change, Volatile changes */
enum fc_change {
	fc_static,       // doesn't change (e.g. chip property)
	fc_stable,       // only changes if we write to the chip
	fc_read_stable,  // stable after a read, not a write
	fc_volatile,     // changes on it's own (e.g. external pin voltage)
	fc_simultaneous_temperature, // volatile with a twist
	fc_simultaneous_voltage, // volatile with a twist
	fc_uncached,     // Don't cache (because the read and write interpretation are different)
	fc_second,       // timer (changes every second)
	fc_statistic,    // internally held statistic
	fc_persistent,   // internal cumulative counter
	fc_directory,    // directory listing
	fc_presence,     // chip <-> bus pairing
	fc_link,         // a link to another property -- cache that one instead
	fc_page,	 // cache a page of memory
	fc_subdir,       // really a NOP, but makes code clearer
};

/* Predeclare one_wire_query */
struct one_wire_query;
/* Predeclare parsedname */
struct parsedname;

#define PROPERTY_LENGTH_INTEGER   12
#define PROPERTY_LENGTH_UNSIGNED  12
#define PROPERTY_LENGTH_BITFIELD  12
#define PROPERTY_LENGTH_FLOAT     12
#define PROPERTY_LENGTH_PRESSURE  12
#define PROPERTY_LENGTH_TEMP      12
#define PROPERTY_LENGTH_TEMPGAP   12
#define PROPERTY_LENGTH_DATE      24
#define PROPERTY_LENGTH_YESNO      1
#define PROPERTY_LENGTH_STRUCTURE 32
#define PROPERTY_LENGTH_DIRECTORY  8
#define PROPERTY_LENGTH_SUBDIR     0
#define PROPERTY_LENGTH_ALIAS    256
#define PROPERTY_LENGTH_ADDRESS   16
#define PROPERTY_LENGTH_TYPE      32

#define NON_AGGREGATE	NULL

#define NO_FILETYPE_DATA {.v=NULL}

#define NO_READ_FUNCTION NULL
#define NO_WRITE_FUNCTION NULL

#include "ow_visibility.h"

/* filetype gives -file types- for chip properties */
struct filetype {
	char *name;
	int suglen;					// length of field
	struct aggregate *ag;		// struct pointer for aggregate
	enum ft_format format;		// type of data
	enum fc_change change;		// volatility
	ZERO_OR_ERROR (*read) (struct one_wire_query *);	// read callback function
	ZERO_OR_ERROR (*write) (struct one_wire_query *);	// write callback function
	enum e_visibility (*visible) (const struct parsedname *);	// Show in a directory listing?
	union {
		void * v;
		int    i;
		UINT   u;
		_FLOAT f;
		size_t s;
		BYTE   c;
		ASCII *a;
	} data;						// extra data pointer (used for separating similar but differently named functions)
};
#define NO_FILETYPE_DATA {.v=NULL}

#include "ow_bitfield.h" // read/write bit fields

/* --------- end Filetype -------------------- */

#endif							/* OW_FILETYPE_H */
