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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* Length of a property element */
/* based on property type in most cases, except ascii and binary, which are explicitly sized */
size_t FileLength(const struct parsedname *pn)
{
	if (pn->type == ePN_structure) {
		return PROPERTY_LENGTH_STRUCTURE;	/* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */
	}
	/* directory ? */
	if (IsDir(pn)) {
		return PROPERTY_LENGTH_DIRECTORY;
	}
	
	switch (pn->selected_filetype->format) {
	case ft_yesno:
		return PROPERTY_LENGTH_YESNO;
	case ft_integer:
		return PROPERTY_LENGTH_INTEGER;
	case ft_unsigned:
		return PROPERTY_LENGTH_UNSIGNED;
	case ft_float:
		return PROPERTY_LENGTH_FLOAT;
	case ft_pressure:
		return PROPERTY_LENGTH_PRESSURE;
	case ft_temperature:
		return PROPERTY_LENGTH_TEMP;
	case ft_tempgap:
		return PROPERTY_LENGTH_TEMPGAP;
	case ft_date:
		return PROPERTY_LENGTH_DATE;
	case ft_bitfield:
		return (pn->extension == EXTENSION_BYTE) ? PROPERTY_LENGTH_UNSIGNED : PROPERTY_LENGTH_YESNO;
	case ft_vascii:			// not used anymore here... 
	case ft_alias:
	case ft_ascii:
	case ft_binary:
	default:
		return pn->selected_filetype->suglen;
	}
}

/* Length of file based on filetype and extension */
size_t FullFileLength(const struct parsedname * pn)
{
	size_t entry_length = FileLength(pn);
	if (pn->type == ePN_structure) {
		return entry_length;
	} else if (pn->extension != EXTENSION_ALL) {
		return entry_length;
	} else {
		size_t elements = pn->selected_filetype->ag->elements;
		if (pn->selected_filetype->format == ft_binary) {
			return entry_length * elements;
		} else {				// add room for commas
			return (entry_length + 1) * elements - 1;
		}
	}
}
