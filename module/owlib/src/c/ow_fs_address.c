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

#include <config.h>
#include "owfs_config.h"
#include "ow_standard.h"

/* ------- Prototypes ------------ */

/* ------- Functions ------------ */

ZERO_OR_ERROR FS_address(struct one_wire_query *owq)
{
	ASCII ad[SERIAL_NUMBER_SIZE*2];
	struct parsedname *pn = PN(owq);
	bytes2string(ad, pn->sn, SERIAL_NUMBER_SIZE);
	return OWQ_format_output_offset_and_size(ad, SERIAL_NUMBER_SIZE*2, owq);
}

ZERO_OR_ERROR FS_r_address(struct one_wire_query *owq)
{
	int sn_index, ad_index;
	ASCII ad[SERIAL_NUMBER_SIZE*2];
	struct parsedname *pn = PN(owq);
	for (sn_index = SERIAL_NUMBER_SIZE-1, ad_index = 0; sn_index >= 0; --sn_index, ad_index += 2) {
		num2string(&ad[ad_index], pn->sn[sn_index]);
	}
	return OWQ_format_output_offset_and_size(ad, SERIAL_NUMBER_SIZE*2, owq);
}
