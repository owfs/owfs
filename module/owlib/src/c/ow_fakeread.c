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
static int FS_read_fake_single(struct one_wire_query *owq);
static int FS_read_fake_array(struct one_wire_query *owq);

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

int FS_read_fake(struct one_wire_query *owq)
{
	switch (OWQ_pn(owq).extension) {
	case EXTENSION_ALL:
		if (OWQ_offset(owq))
			return 0;
		if (OWQ_size(owq) < FullFileLength(PN(owq)))
			return -ERANGE;
		return FS_read_fake_array(owq);
	case EXTENSION_BYTE:		/* bitfield */
	default:
		return FS_read_fake_single(owq);
	}
}

static int FS_read_fake_single(struct one_wire_query *owq)
{
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
	case ft_tempgap:
	case ft_float:
		OWQ_F(owq) = Random_f;
		break;
	case ft_date:
		OWQ_D(owq) = Random_d;
		break;
	case ft_vascii:
	case ft_ascii:
		{
			size_t i;
			OWQ_length(owq) = OWQ_size(owq);
			for (i = 0; i < OWQ_size(owq); ++i) {
				OWQ_buffer(owq)[i] = Random_a;
			}
		}
		break;
	case ft_binary:
		{
			size_t i;
			OWQ_length(owq) = OWQ_size(owq);
			for (i = 0; i < OWQ_size(owq); ++i) {
				OWQ_buffer(owq)[i] = Random_b;
			}
		}
		break;
	case ft_directory:
	case ft_subdir:
		return -ENOENT;
	}
	return 0;					// put data as string into buffer and return length
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==EXTENSION_ALL and pn->selected_filetype->ag->combined==ag_separate */
static int FS_read_fake_array(struct one_wire_query *owq)
{
	size_t elements = OWQ_pn(owq).selected_filetype->ag->elements;
	size_t extension;
	size_t entry_length = FileLength(PN(owq));
	OWQ_allocate_struct_and_pointer(owq_single);

	OWQ_create_shallow_single(owq_single, owq);

	for (extension = 0; extension < elements; ++extension) {
		OWQ_pn(owq_single).extension = extension;
		switch (OWQ_pn(owq).selected_filetype->format) {
		case ft_integer:
		case ft_yesno:
		case ft_bitfield:
		case ft_unsigned:
		case ft_temperature:
		case ft_tempgap:
		case ft_float:
		case ft_date:
			break;
		case ft_vascii:
		case ft_ascii:
		case ft_binary:
			OWQ_size(owq_single) = entry_length;
			OWQ_buffer(owq_single) = &OWQ_buffer(owq)[extension * entry_length];
			break;
		case ft_directory:
		case ft_subdir:
			return -ENOENT;
		}
		if (FS_read_fake_single(owq_single))
			return -EINVAL;
		memcpy(&OWQ_array(owq)[extension], &OWQ_val(owq_single), sizeof(union value_object));
	}
	return 0;
}
