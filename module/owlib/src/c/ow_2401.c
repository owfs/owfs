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

/* DS2401 Simple ID device */
/* Also needed for all others */

/* ------- Prototypes ----------- */

	/* All from ow_xxxx.c file */

#include <config.h>
#include "owfs_config.h"
#include "ow_2401.h"

/* ------- Structures ----------- */

static struct filetype DS2401[] = {
	F_STANDARD,
};

DeviceEntry(01, DS2401, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS1420[] = {
	F_STANDARD,
};

DeviceEntry(81, DS1420, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* ------- Functions ------------ */
