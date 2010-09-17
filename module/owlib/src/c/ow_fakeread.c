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
static ZERO_OR_ERROR FS_read_fake_single(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_read_fake_array(struct one_wire_query *owq);

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

#define Random (((_FLOAT)rand())/RAND_MAX)
#define Random_y (rand()&0x01)
#define Random_t (Random*100)
#define Random_d (time(NULL)*(1-.1*Random))
#define Random_i (rand()&0xFF)
#define Random_u (rand()&0xFF)
#define Random_b (rand()&0xFF)
#define Random_a ('a'+(rand() % (26)))
#define Random_f (100.*Random)

ZERO_OR_ERROR FS_read_fake(struct one_wire_query *owq)
{
	switch (OWQ_pn(owq).extension) {
	case EXTENSION_ALL:
		if (OWQ_offset(owq)) {
			return 0;
		}
		if (OWQ_size(owq) < FullFileLength(PN(owq))) {
			return -ERANGE;
		}
		return FS_read_fake_array(owq);
	case EXTENSION_BYTE:		/* bitfield */
	default:
		return FS_read_fake_single(owq);
	}
}

static ZERO_OR_ERROR FS_read_fake_single(struct one_wire_query *owq)
{
	enum { type_a, type_b } format_type = type_a;	// assume ascii

	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_integer:
		OWQ_I(owq) = Random_i;
		break;
	case ft_yesno:
		OWQ_Y(owq) = Random_y;
		break;
	case ft_bitfield:
		if (OWQ_pn(owq).extension == EXTENSION_BYTE) {
			OWQ_U(owq) = Random_u;
		} else {
			OWQ_Y(owq) = Random_y;
		}
		break;
	case ft_unsigned:
		OWQ_U(owq) = Random_u;
		break;
	case ft_temperature:
		{
			_FLOAT low  = OWQ_pn(owq).selected_connection->master.fake.templow ;
			_FLOAT high = OWQ_pn(owq).selected_connection->master.fake.temphigh ;
			OWQ_F(owq) = low + (high-low)*Random_f/100.;
		}
		break;
	case ft_tempgap:
	case ft_pressure:
	case ft_float:
		OWQ_F(owq) = Random_f;
		break;
	case ft_date:
		OWQ_D(owq) = Random_d;
		break;
	case ft_binary:
		format_type = type_b;	// binary
		// fall through
	case ft_vascii:
	case ft_alias:
	case ft_ascii:
		{
			size_t i;
			size_t length = FileLength(PN(owq));
			ASCII random_chars[length];
			for (i = 0; i < length; ++i) {
				random_chars[i] = (format_type == type_a) ? Random_a : Random_b;
			}
			return OWQ_format_output_offset_and_size(random_chars, length, owq);
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
static ZERO_OR_ERROR FS_read_fake_array(struct one_wire_query *owq)
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
		if (FS_read_fake_single(owq_single)) {
			OWQ_destroy(owq_single) ;
			return -EINVAL;
		}
		memcpy(&OWQ_array(owq)[extension], &OWQ_val(owq_single), sizeof(union value_object));
		OWQ_destroy(owq_single) ;
	}
	return 0;
}
