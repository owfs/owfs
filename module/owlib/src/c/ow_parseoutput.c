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
static int Fowq_output_integer(struct one_wire_query *owq);
static int Fowq_output_unsigned(struct one_wire_query *owq);
static int Fowq_output_float(struct one_wire_query *owq);
static int Fowq_output_date(struct one_wire_query *owq);
static int Fowq_output_yesno(struct one_wire_query *owq);
static int Fowq_output_ascii(struct one_wire_query *owq);
static int Fowq_output_array_with_commas(struct one_wire_query *owq);
static int Fowq_output_array_no_commas(struct one_wire_query *owq);
static int Fowq_output_ascii_array(struct one_wire_query *owq);

/*
Change in strategy 6/2006:
Now use CheckPresence as primary method of finding correct bus
Can break down cases into:
1. bad ParsedName -- no read possible
2. structure -- read from 1st bus (local)
3. specified bus (picked up in ParsedName) -- use that
4. statistics, settings, Simultaneous, Thermostat -- use first or specified
5. real -- use caced, if error, delete cache entry and try twice more.
*/

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

size_t FileLength_vascii(struct one_wire_query *owq);

int FS_output_owq(struct one_wire_query *owq)
{
	// have to check if offset is beyond the filesize.
	if (OWQ_offset(owq)) {
		size_t file_length = 0;
		if (PN(owq)->selected_filetype->format == ft_vascii) {
			file_length = FileLength_vascii(owq);
		} else {
			file_length = FileLength(PN(owq));
		}
		LEVEL_DEBUG("FS_r_local: file_length=%lu offset=%lu size=%lu",
					(unsigned long) file_length, (unsigned long) OWQ_offset(owq), (unsigned long) OWQ_size(owq));
		if ((unsigned long) OWQ_offset(owq) >= (unsigned long) file_length) {
			return 0;			// This is data-length
		}
	}

	/* Special case, structure is always ascii */
	if (OWQ_pn(owq).type == ePN_structure) {
		return Fowq_output_ascii(owq);
	}

	switch (OWQ_pn(owq).extension) {
	case EXTENSION_BYTE:
		return Fowq_output_unsigned(owq);
	case EXTENSION_ALL:
		switch (OWQ_pn(owq).selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
			return Fowq_output_ascii_array(owq);
		case ft_binary:
			return Fowq_output_array_no_commas(owq);
		default:
			return Fowq_output_array_with_commas(owq);
		}
	default:
		switch (OWQ_pn(owq).selected_filetype->format) {
		case ft_integer:
			return Fowq_output_integer(owq);
		case ft_yesno:
		case ft_bitfield:
			return Fowq_output_yesno(owq);
		case ft_unsigned:
			return Fowq_output_unsigned(owq);
		case ft_pressure:
		case ft_temperature:
		case ft_tempgap:
		case ft_float:
			return Fowq_output_float(owq);
		case ft_date:
			return Fowq_output_date(owq);
		case ft_vascii:
		case ft_ascii:
		case ft_binary:
			return Fowq_output_ascii(owq);
		case ft_directory:
		case ft_subdir:
			return -ENOENT;
		}
	}
	return -EINVAL;				// should never be reached if all the cases are truly covered
}

static int Fowq_output_integer(struct one_wire_query *owq)
{
	/* should only need suglen+1, but uClibc's snprintf()
	   seem to trash 'len' if not increased */
	int len;
	char c[PROPERTY_LENGTH_INTEGER + 2];

	UCLIBCLOCK;
	len = snprintf(c, PROPERTY_LENGTH_INTEGER + 1, "%*d", PROPERTY_LENGTH_INTEGER, OWQ_I(owq));
	UCLIBCUNLOCK;
	if ((len < 0) || ((size_t) len > PROPERTY_LENGTH_INTEGER)) {
		return -EMSGSIZE;
	}
	return Fowq_output_offset_and_size(c, PROPERTY_LENGTH_INTEGER, owq);
}

static int Fowq_output_unsigned(struct one_wire_query *owq)
{
	/* should only need suglen+1, but uClibc's snprintf()
	   seem to trash 'len' if not increased */
	int len;
	char c[PROPERTY_LENGTH_UNSIGNED + 2];

	UCLIBCLOCK;
	len = snprintf(c, PROPERTY_LENGTH_UNSIGNED + 1, "%*u", PROPERTY_LENGTH_UNSIGNED, OWQ_U(owq));
	UCLIBCUNLOCK;
	if ((len < 0) || ((size_t) len > PROPERTY_LENGTH_UNSIGNED)) {
		return -EMSGSIZE;
	}
	return Fowq_output_offset_and_size(c, PROPERTY_LENGTH_UNSIGNED, owq);
}

static int Fowq_output_float(struct one_wire_query *owq)
{
	/* should only need suglen+1, but uClibc's snprintf()
	   seem to trash 'len' if not increased */
	int len;
	char c[PROPERTY_LENGTH_FLOAT + 2];
	_FLOAT F;

	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_pressure:
		F = Pressure(OWQ_F(owq), PN(owq));
		break;
	case ft_temperature:
		F = Temperature(OWQ_F(owq), PN(owq));
		break;
	case ft_tempgap:
		F = TemperatureGap(OWQ_F(owq), PN(owq));
		break;
	default:
		F = OWQ_F(owq);
		break;
	}

	UCLIBCLOCK;
	len = snprintf(c, PROPERTY_LENGTH_FLOAT + 1, "%*G", PROPERTY_LENGTH_FLOAT, F);
	UCLIBCUNLOCK;
	if ((len < 0) || ((size_t) len > PROPERTY_LENGTH_FLOAT)) {
		return -EMSGSIZE;
	}
	return Fowq_output_offset_and_size(c, PROPERTY_LENGTH_FLOAT, owq);
}

static int Fowq_output_date(struct one_wire_query *owq)
{
	char c[PROPERTY_LENGTH_DATE + 2];
	if (OWQ_size(owq) < PROPERTY_LENGTH_DATE) {
		return -EMSGSIZE;
	}
	ctime_r(&OWQ_D(owq), c);
	return Fowq_output_offset_and_size(c, PROPERTY_LENGTH_DATE, owq);
}

static int Fowq_output_yesno(struct one_wire_query *owq)
{
	if (OWQ_size(owq) < PROPERTY_LENGTH_YESNO) {
		return -EMSGSIZE;
	}
	OWQ_buffer(owq)[0] = ((OWQ_Y(owq) & 0x1) == 0) ? '0' : '1';
	return PROPERTY_LENGTH_YESNO;
}

int Fowq_output_offset_and_size_z(char *string, struct one_wire_query *owq)
{
	return Fowq_output_offset_and_size(string, strlen(string), owq);
}

/* Put a string ionto the OWQ structure and return the length
   check lengths and offsets as part of the process */
int Fowq_output_offset_and_size(char *string, size_t length, struct one_wire_query *owq)
{
	size_t copy_length = length;
	off_t offset = OWQ_offset(owq);
	Debug_Bytes("Fowq_output_offset_and_size", (BYTE *) string, length);

	/* offset is after the length of the string -- return 0 since
	   some conditions a read after the end is done automatically */
	if (offset > (off_t) length) {
		return 0;
	}

	/* correct length for offset (cannot be negative because of previous check) */
	copy_length -= offset;

	/* correct length for buffer space */
	if (copy_length > OWQ_size(owq)) {
		copy_length = OWQ_size(owq);
	}

	/* and copy */
	memcpy(OWQ_buffer(owq), &string[offset], copy_length);
	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_vascii:
	case ft_ascii:
	case ft_binary:
		OWQ_length(owq) = copy_length;
	default:
		break;
	}
	return copy_length;
}

static int Fowq_output_ascii(struct one_wire_query *owq)
{
	Debug_OWQ(owq);
	return OWQ_length(owq);
}

static int Fowq_output_array_with_commas(struct one_wire_query *owq)
{
	struct one_wire_query owq_single;
	size_t extension;
	int len;
	size_t used_size = 0;
	size_t remaining_size = OWQ_size(owq);
	size_t elements = OWQ_pn(owq).selected_filetype->ag->elements;

	// loop though all array elements
	for (extension = 0; extension < elements; ++extension) {
		//printf("Fowq_output_array_with_commas element=%d, size_used=%d, remaining=%d\n",(int)extension,(int)used_size,(int)remaining_size) ;
		// Prepare a copy of owq that only points to a single element
		memcpy(&owq_single, owq, sizeof(owq_single));
		OWQ_pn(&owq_single).extension = extension;
		memcpy(&OWQ_val(&owq_single), &OWQ_array(owq)[extension], sizeof(union value_object));
		// add the comma first (if not the first element and enough room)
		if (used_size > 0) {
			if (remaining_size == 0) {
				return -EFAULT;
			}
			OWQ_buffer(owq)[used_size] = ',';
			++used_size;
			--remaining_size;
		}
		// Now process the single element
		OWQ_buffer(&owq_single) = &OWQ_buffer(owq)[used_size];
		OWQ_size(&owq_single) = remaining_size;
		len = FS_output_owq(&owq_single);
		// any error aborts
		if (len < 0) {
			return len;
		}
		remaining_size -= len;
		used_size += len;
	}
	return used_size;
}

static int Fowq_output_ascii_array(struct one_wire_query *owq)
{
	size_t extension;
	size_t elements = OWQ_pn(owq).selected_filetype->ag->elements;
	size_t total_length = 0;
	size_t current_offset = OWQ_array_length(owq, 0);

	for (extension = 0; extension < elements; ++extension) {
		total_length += OWQ_array_length(owq, extension);
	}

	for (extension = 1; extension < elements; ++extension) {
		memmove(&OWQ_buffer(owq)[current_offset + 1], &OWQ_buffer(owq)[current_offset], total_length - current_offset);
		OWQ_buffer(owq)[current_offset] = ',';
		++total_length;
		current_offset += 1 + OWQ_array_length(owq, extension);
	}

	return total_length;
}

static int Fowq_output_array_no_commas(struct one_wire_query *owq)
{
	size_t extension;
	size_t total_length = 0;
	size_t elements = OWQ_pn(owq).selected_filetype->ag->elements;

	for (extension = 0; extension < elements; ++extension) {
		total_length += OWQ_array_length(owq, extension);
	}
	return total_length;
}
