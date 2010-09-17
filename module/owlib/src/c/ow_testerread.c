/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
$ID: $
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Prototypes ----------- */
static ZERO_OR_ERROR FS_read_tester_single(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_read_tester_array(struct one_wire_query *owq);

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

ZERO_OR_ERROR FS_read_tester(struct one_wire_query *owq)
{
	switch (OWQ_pn(owq).extension) {
	case EXTENSION_ALL:		/* array */
		if (OWQ_offset(owq)) {
			return 0;
		}
		if (OWQ_size(owq) < FullFileLength(PN(owq))) {
			return -ERANGE;
		}
		return FS_read_tester_array(owq);
	case EXTENSION_BYTE:		/* bitfield */
	default:
		return FS_read_tester_single(owq);
	}
}

static ZERO_OR_ERROR FS_read_tester_single(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int tester_bus = (pn->sn[2] << 8) + pn->sn[1];
	int device = (pn->sn[6] << 8) + pn->sn[5];
	int family_code = pn->sn[0];
	int calculated_value = family_code + tester_bus + device + pn->extension;

	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_integer:
		OWQ_I(owq) = calculated_value;
		break;
	case ft_yesno:
		OWQ_Y(owq) = calculated_value & 0x1;
		break;
	case ft_bitfield:
		if (OWQ_pn(owq).extension == EXTENSION_BYTE) {
			OWQ_U(owq) = calculated_value;
		} else {
			OWQ_Y(owq) = calculated_value & 0x1;
		}
		break;
	case ft_unsigned:
		OWQ_U(owq) = calculated_value;
		break;
	case ft_pressure:
	case ft_temperature:
	case ft_tempgap:
	case ft_float:
		OWQ_F(owq) = (_FLOAT) calculated_value *0.1;;
		break;
	case ft_date:
		OWQ_D(owq) = 1174622400;
		break;
	case ft_alias:
	case ft_vascii:
	case ft_ascii:
		{
			ASCII address[16];
			size_t length_left = OWQ_size(owq);
			size_t buffer_index;
			size_t length = FileLength(PN(owq));
			ASCII return_chars[length];

			bytes2string(address, pn->sn, SERIAL_NUMBER_SIZE);
			OWQ_length(owq) = OWQ_size(owq);
			for (buffer_index = 0; buffer_index < length; buffer_index += sizeof(address)) {
				size_t copy_length = length_left;
				if (copy_length > sizeof(address)) {
					copy_length = sizeof(address);
				}
				memcpy(&return_chars[buffer_index], address, copy_length);
				length_left -= copy_length;
			}
			return OWQ_format_output_offset_and_size(return_chars, length, owq);
		}
	case ft_binary:
		{
			size_t length_left = OWQ_size(owq);
			size_t buffer_index;
			size_t length = FileLength(PN(owq));
			ASCII return_chars[length];

			OWQ_length(owq) = OWQ_size(owq);
			for (buffer_index = 0; buffer_index < length; buffer_index += SERIAL_NUMBER_SIZE) {
				size_t copy_length = length_left;
				if (copy_length > SERIAL_NUMBER_SIZE) {
					copy_length = SERIAL_NUMBER_SIZE;
				}
				memcpy(&return_chars[buffer_index], pn->sn, copy_length);
				length_left -= copy_length;
			}
			return OWQ_format_output_offset_and_size(return_chars, length, owq);
		}
	case ft_directory:
	case ft_subdir:
	case ft_unknown:
		return -ENOENT;
	}
	return 0;					// put data as string into buffer and return length
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==EXTENSION_ALL and pn->selected_filetype->ag->combined==ag_separate */
static ZERO_OR_ERROR FS_read_tester_array(struct one_wire_query *owq)
{
	size_t elements = OWQ_pn(owq).selected_filetype->ag->elements;
	size_t extension;
	size_t entry_length = FileLength(PN(owq));

	for (extension = 0; extension < elements; ++extension) {
		struct one_wire_query * owq_single = OWQ_create_separate( extension, owq ) ;
		if ( owq_single == NO_ONE_WIRE_QUERY ) {
			return -ENOMEM ;
		}
		switch (OWQ_pn(owq).selected_filetype->format) {
		case ft_integer:
		case ft_yesno:
		case ft_bitfield:
		case ft_unsigned:
		case ft_pressure:
		case ft_temperature:
		case ft_tempgap:
		case ft_float:
		case ft_date:
			break;
		case ft_vascii:
		case ft_alias:
		case ft_ascii:
		case ft_binary:
			OWQ_assign_read_buffer(&OWQ_buffer(owq)[extension * entry_length],entry_length,0,owq_single) ;
			break;
		case ft_directory:
		case ft_subdir:
		case ft_unknown:
			OWQ_destroy(owq_single) ;
			return -ENOENT;
		}
		if (FS_read_tester_single(owq_single)) {
			OWQ_destroy(owq_single) ;
			return -EINVAL;
		}
		memcpy(&OWQ_array(owq)[extension], &OWQ_val(owq_single), sizeof(union value_object));
		OWQ_destroy(owq_single) ;
	}
	return 0;
}
