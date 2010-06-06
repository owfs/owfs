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
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_specialcase.h"

/* ------- Prototypes ----------- */
static ZERO_OR_ERROR FS_w_given_bus(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_w_local(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_w_simultaneous(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_single_lump(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_aggregate_lump(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_all_bits(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_a_bit(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_in_parts(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_a_part(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_write_mixed_part(struct one_wire_query *owq);
static void FS_write_add_to_cache( struct one_wire_query *owq);

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

/* Note on return values: */
/* Top level FS_write will return size if ok, else a negative number */
/* Each lower level function called will return 0 if ok, else non-zero */

/* Note on size and offset: */
/* Buffer length (and requested data) is size bytes */
/* writing should start after offset bytes in original data */
/* only binary, and ascii data support offset in single data points */
/* only binary supports offset in array data */
/* size and offset are vetted against specification data size and calls */
/*   outside of this module will not have buffer overflows */
/* I.e. the rest of owlib can trust size and buffer to be legal */

/* Format of input,
        Depends on "filetype"
        type     function    format                         Handled as
        integer  strol      decimal integer                 integer array
        unsigned strou      decimal integer                 unsigned array
        bitfield strou      decimal integer                 unsigned array
        yesno    strcmp     "0" "1" "yes" "no" "on" "off"   unsigned array
        float    strod      decimal floating point          double array
        date     strptime   "Jan 01, 1901", etc             date array
        ascii    strcpy     string without "," or null      comma-separated-strings
        binary   memcpy     fixed length binary string      binary "string"
*/


/* return size if ok, else negative */
ZERO_OR_ERROR FS_write(const char *path, const char *buf, const size_t size, const off_t offset)
{
	ZERO_OR_ERROR write_return;
	OWQ_allocate_struct_and_pointer(owq);

	LEVEL_CALL("path=%s size=%d offset=%d", SAFESTRING(path), (int) size, (int) offset);

	// parsable path?
	if ( OWQ_create(path, owq) != 0 ) { // for write
		return -ENOENT;
	}
	OWQ_assign_write_buffer(buf, size, offset, owq) ;
	write_return = FS_write_postparse(owq);
	OWQ_destroy(owq);
	return write_return;		/* here's where the size is used! */
}

/* return size if ok, else negative */
ZERO_OR_ERROR FS_write_postparse(struct one_wire_query *owq)
{
	ZERO_OR_ERROR input_or_error;
	ZERO_OR_ERROR write_or_error;
	struct parsedname *pn = PN(owq);

	if (Globals.readonly) {
		LEVEL_DEBUG("Attempt to write but readonly set on command line.");
		return -EROFS;			// read-only invokation
	}

	if (IsDir(pn)) {
		LEVEL_DEBUG("Attempt to write to a directory.");
		return -EISDIR;			// not a file
	}

	if (pn->selected_connection == NULL) {
		LEVEL_DEBUG("Attempt to write but no 1-wire bus master.");
		return -ENODEV;			// no buses
	}

	STATLOCK;
	AVERAGE_IN(&write_avg);
	AVERAGE_IN(&all_avg);
	++write_calls;				/* statistics */
	STATUNLOCK;

	input_or_error = OWQ_parse_input(owq);
	Debug_OWQ(owq);
	if (input_or_error < 0) {
		LEVEL_DEBUG("Error interpretting input value.") ;
		return input_or_error;
	}
	switch (pn->type) {
	case ePN_structure:
	case ePN_statistics:
		LEVEL_DEBUG("Cannomt write in this type of directory.") ;
		write_or_error = -ENOTSUP;
		break;
	case ePN_system:
	case ePN_settings:
		write_or_error = FS_w_given_bus(owq);
		break;
	default:					// ePN_real

		/* handle DeviceSimultaneous */
		if (pn->selected_device == DeviceSimultaneous) {
			/* writing to /simultaneous/temperature will write to ALL
			 * available bus.?/simultaneous/temperature
			 * not just /simultaneous/temperature
			 */
			write_or_error = FS_w_simultaneous(owq);
		} else {
			/* First try */
			/* in and bus_nr already set */
			STAT_ADD1(write_tries[0]);
			write_or_error = FS_w_given_bus(owq);

			/* Second Try */
			/* if not a specified bus, relook for chip location */
			if (write_or_error < 0) {	// second look -- initial write gave an error
				STAT_ADD1(write_tries[1]);
				if (SpecifiedBus(pn)) {
					write_or_error = BAD(TestConnection(pn)) ? -ECONNABORTED : FS_w_given_bus(owq);
					if (write_or_error < 0) {	// third try
						STAT_ADD1(write_tries[2]);
						write_or_error = BAD(TestConnection(pn)) ? -ECONNABORTED : FS_w_given_bus(owq);
					}
				} else if (BusIsServer(pn->selected_connection)) {
					int bus_nr = pn->selected_connection->index ; // current selected bus
					INDEX_OR_ERROR busloc_or_error = ReCheckPresence(pn) ;
					// special handling or remote
					// only repeat if the bus number is wrong
					// because the remote does the rewrites
					if ( bus_nr != busloc_or_error ) {
						if (busloc_or_error < 0) {
							write_or_error = -ENOENT;
						} else {
							write_or_error = FS_w_given_bus(owq);
							if (write_or_error < 0) {	// third try
								STAT_ADD1(write_tries[2]);
								write_or_error = BAD(TestConnection(pn)) ? -ECONNABORTED : FS_w_given_bus(owq);
							}
						}
					}				
				} else {
					INDEX_OR_ERROR busloc_or_error = ReCheckPresence(pn);
					if (busloc_or_error < 0) {
						write_or_error = -ENOENT;
					} else {
						write_or_error = FS_w_given_bus(owq);
						if (write_or_error < 0) {	// third try
							STAT_ADD1(write_tries[2]);
							write_or_error = BAD(TestConnection(pn)) ? -ECONNABORTED : FS_w_given_bus(owq);
						}
					}
				}
			}
		}
	}

	STATLOCK;
	if (write_or_error == 0) {
		++write_success;		/* statistics */
		write_bytes += OWQ_size(owq);	/* statistics */
		write_or_error = OWQ_size(owq);	/* here's where the size is used! */
	}
	AVERAGE_OUT(&write_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;

	if ( write_or_error == 0 ) {
		LEVEL_DEBUG("Successful write to %s",pn->path) ;
	} else {
		LEVEL_DEBUG("Error writing to %s",pn->path) ;
	}
	return write_or_error;
}

#if OW_MT
struct simultaneous_struct {
	struct connection_in *in;
	const struct one_wire_query *owq ;
};

static void * Simultaneous_write(void * v)
{
	struct simultaneous_struct *ss = (struct simultaneous_struct *) v;
	struct simultaneous_struct ss_next = { ss->in->next, ss->owq };
	pthread_t thread;
	int threadbad = 1;
	OWQ_allocate_struct_and_pointer(owq_copy);
	
	
	threadbad = (ss->in->next == NULL)
	|| pthread_create(&thread, NULL, Simultaneous_write, (void *) (&ss_next));
	
	memcpy(owq_copy, ss->owq, sizeof(struct one_wire_query));	// shallow copy
	
	SetKnownBus(ss->in->index, PN(owq_copy));
	
	FS_w_given_bus(owq_copy);

	if (threadbad == 0) {		/* was a thread created? */
		void *v_ignore;
		pthread_join(thread, &v_ignore) ;
	}
	return NULL ;
}
#endif /* OW_MT */

/* This function is only used by "Simultaneous" */
static ZERO_OR_ERROR FS_w_simultaneous(struct one_wire_query *owq)
{
	if (SpecifiedBus(PN(owq))) {
		return FS_w_given_bus(owq);
	} else if (Inbound_Control.head) {
#if OW_MT
		struct simultaneous_struct ss = { Inbound_Control.head, owq };
		Simultaneous_write( (void *) (&ss) ) ;
#else /* OW_MT */
		struct connection_in * in;
		OWQ_allocate_struct_and_pointer(owq_given);
		
		memcpy(owq_given, owq, sizeof(struct one_wire_query));	// shallow copy
		
		for( in=Inbound_Control.head; in ; in=in->next ) {
			SetKnownBus(in->index, PN(owq_given));
			FS_w_given_bus(owq_given);
		}
#endif /* OW_MT */
	}
	return 0;
}

/* return 0 if ok, else negative */
static ZERO_OR_ERROR FS_w_given_bus(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	ZERO_OR_ERROR write_or_error;

	if ( BAD(TestConnection(pn)) ) {
		write_or_error = -ECONNABORTED;
	} else if (KnownBus(pn) && BusIsServer(pn->selected_connection)) {
		write_or_error = ServerWrite(owq);
	} else if (OWQ_pn(owq).type == ePN_real) {
		write_or_error = DeviceLockGet(pn);
		if (write_or_error == 0) {
			write_or_error = FS_w_local(owq);
			DeviceLockRelease(pn);
		} else {
			LEVEL_DEBUG("Cannot lock device for writing") ;
		}
	} else if (OWQ_pn(owq).type == ePN_interface) {
		BUSLOCK(pn);
		write_or_error = FS_w_local(owq);
		BUSUNLOCK(pn);
	} else {
		write_or_error = FS_w_local(owq);
	}
	return write_or_error;
}

/* return 0 if ok */
static ZERO_OR_ERROR FS_w_local(struct one_wire_query *owq)
{
	// Device already locked
	struct parsedname *pn = PN(owq);
	//printf("FS_w_local\n");

	/* Special case for "fake" adapter */
	if ( IsRealDir(pn) ) {
		switch (get_busmode(pn->selected_connection)) {
			case bus_mock:
				// Mock -- write even "unwritable" to the cache for testing
				FS_write_add_to_cache(owq);
				// fall through
			case bus_fake:
			case bus_tester:
				return (pn->selected_filetype->write == NO_WRITE_FUNCTION) ? -ENOTSUP : 0 ;
			default:
				break ;
		}
	}

	/* Writable? */
	if (pn->selected_filetype->write == NO_WRITE_FUNCTION) {
		return -ENOTSUP;
	}

	/* Array properties? Write all together if aggregate */
	if (pn->selected_filetype->ag != NON_AGGREGATE) {
		switch (pn->extension) {
		case EXTENSION_BYTE:
			LEVEL_DEBUG("Writing collectively as a bitfield");
			return FS_write_single_lump(owq);
		case EXTENSION_ALL:
			if (pn->selected_filetype->format == ft_bitfield) {
				return FS_write_all_bits(owq);
			}
			switch (pn->selected_filetype->ag->combined) {
			case ag_aggregate:
			case ag_mixed:
				LEVEL_DEBUG("Writing collectively");
				return FS_write_aggregate_lump(owq);
			case ag_separate:
				LEVEL_DEBUG("Writing separately");
				return FS_write_in_parts(owq);
			}
		default:
			// Just write one field of an array
			if (pn->selected_filetype->format == ft_bitfield) {
				LEVEL_DEBUG("Writing a bit in a bitfield");
				return FS_write_a_bit(owq);
			}
			switch (pn->selected_filetype->ag->combined) {
			case ag_aggregate:
				// need to read it all and overwrite just a part
				return FS_write_a_part(owq);
			case ag_mixed:
				return FS_write_mixed_part(owq);
			case ag_separate:
				return FS_write_single_lump(owq);
			}
		}
	}
	return FS_write_single_lump(owq);
}

static ZERO_OR_ERROR FS_write_single_lump(struct one_wire_query *owq)
{
	ZERO_OR_ERROR write_error;
	OWQ_allocate_struct_and_pointer(owq_copy);

	OWQ_create_shallow_single(owq_copy, owq);

	write_error = SpecialCase_write(owq_copy);
	if ( write_error == -ENOENT) {
		write_error = (OWQ_pn(owq).selected_filetype->write) (owq_copy);
	}

	if (write_error != 0) {
		return write_error;
	}

	FS_write_add_to_cache(owq);

	return 0;
}

/* Write just one field of an aggregate property -- but a property that is handled as one big object */
static ZERO_OR_ERROR FS_write_a_part(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t extension = pn->extension;
	ZERO_OR_ERROR write_error;
	OWQ_allocate_struct_and_pointer(owq_all);

	if (OWQ_create_shallow_aggregate(owq_all, owq) < 0) {
		return -ENOMEM;
	}

	// First fill the whole array with current values
	if (OWQ_Cache_Get(owq_all)) {
		ZERO_OR_ERROR read_error = (pn->selected_filetype->read) (owq_all);
		if (read_error != 0) {
			OWQ_destroy_shallow_aggregate(owq_all);
			return read_error;
		}
	}

	// Copy ascii/binary field
	switch (pn->selected_filetype->format) {
	case ft_binary:
	case ft_ascii:
	case ft_vascii:
	case ft_alias:
		{
			size_t extension_index;
			size_t elements = pn->selected_filetype->ag->elements;
			char *buffer_pointer = OWQ_buffer(owq_all);
			char *entry_pointer;
			char *target_pointer;

			for (extension_index = 0; extension_index < extension; ++extension) {
				buffer_pointer += OWQ_array_length(owq_all, extension_index);
			}

			entry_pointer = buffer_pointer;

			target_pointer = buffer_pointer + OWQ_length(owq);
			buffer_pointer = buffer_pointer + OWQ_array_length(owq_all, extension);

			for (extension_index = extension + 1; extension_index < elements; ++extension) {
				size_t this_length = OWQ_array_length(owq_all, extension_index);
				memmove(target_pointer, buffer_pointer, this_length);
				buffer_pointer += this_length;
				buffer_pointer += this_length;
			}

			memmove(entry_pointer, OWQ_buffer(owq), OWQ_length(owq));
			break;
		}
	default:
		break;
	}
	// Copy value field
	memcpy(&OWQ_array(owq_all)[pn->extension], &OWQ_val(owq), sizeof(union value_object));

	// Write whole thing out
	write_error = FS_write_aggregate_lump(owq_all);

	OWQ_destroy_shallow_aggregate(owq_all);

	return write_error;
}

// Write a whole aggregate array (treated as a single large value )
static ZERO_OR_ERROR FS_write_aggregate_lump(struct one_wire_query *owq)
{
	ZERO_OR_ERROR write_error;
	OWQ_allocate_struct_and_pointer(owq_copy);

	OWQ_create_shallow_aggregate(owq_copy, owq);

	write_error = SpecialCase_write(owq_copy);
	if ( write_error == -ENOENT) {
		write_error = (OWQ_pn(owq).selected_filetype->write) (owq_copy);
	}

	OWQ_destroy_shallow_aggregate(owq_copy);

	if (write_error != 0) {
		return write_error;
	}

	FS_write_add_to_cache(owq);

	return 0;
}

static ZERO_OR_ERROR FS_write_mixed_part(struct one_wire_query *owq)
{
	ZERO_OR_ERROR write_error = FS_write_single_lump(owq);
	OWQ_allocate_struct_and_pointer(owq_copy);

	OWQ_create_shallow_aggregate(owq_copy, owq);

	OWQ_pn(owq_copy).extension = EXTENSION_ALL;
	OWQ_Cache_Del(owq_copy);

	OWQ_destroy_shallow_aggregate(owq_copy);

	return write_error;
}

static ZERO_OR_ERROR FS_write_in_parts(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t elements = pn->selected_filetype->ag->elements;
	char *buffer_pointer;
	size_t extension;
	OWQ_allocate_struct_and_pointer(owq_single);

	OWQ_create_shallow_single(owq_single, owq);

	switch (pn->selected_filetype->format) {
	case ft_ascii:
	case ft_vascii:
	case ft_alias:
	case ft_binary:
		buffer_pointer = OWQ_buffer(owq);
		break;
	default:
		buffer_pointer = NULL;
		break;
	}

	for (extension = 0; extension < elements; ++extension) {
		ZERO_OR_ERROR single_write;
		memcpy(&OWQ_val(owq_single), &OWQ_array(owq)[extension], sizeof(union value_object));
		OWQ_pn(owq_single).extension = extension;
		OWQ_size(owq_single) = FileLength(PN(owq_single));
		OWQ_offset(owq_single) = 0;
		if ( buffer_pointer != NULL ) {
			OWQ_buffer(owq_single) = buffer_pointer;
		}
		//_print_owq(owq_single) ; // debug
		single_write = FS_write_single_lump(owq_single);

		if (single_write != 0) {
			return single_write;
		}

		if (buffer_pointer) {
			buffer_pointer += OWQ_array_length(owq, extension);
		}
	}

	return 0;
}

static ZERO_OR_ERROR FS_write_all_bits(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t elements = pn->selected_filetype->ag->elements;
	size_t extension;
	UINT U = 0;
	OWQ_allocate_struct_and_pointer(owq_single);

	OWQ_create_shallow_single(owq_single, owq);

	for (extension = 0; extension < elements; ++extension) {
		UT_setbit((void *) (&U), extension, OWQ_array_Y(owq, extension));
	}

	OWQ_U(owq_single) = U;
	OWQ_pn(owq_single).extension = EXTENSION_BYTE;

	return FS_write_single_lump(owq_single);
}

/* Writing a single bit -- need to read in whole byte and change bit */
static ZERO_OR_ERROR FS_write_a_bit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	OWQ_allocate_struct_and_pointer(owq_single);

	OWQ_create_shallow_single(owq_single, owq);

	OWQ_pn(owq_single).extension = EXTENSION_BYTE;

	if (OWQ_Cache_Get(owq_single)) {
		ZERO_OR_ERROR read_error = (pn->selected_filetype->read) (owq_single);
		if (read_error < 0) {
			return read_error;
		}
	}

	UT_setbit((void *) (&OWQ_U(owq_single)), pn->extension, OWQ_Y(owq));

	return FS_write_single_lump(owq_single);
}

// Used for sibling write -- bus already locked, and it's local
ZERO_OR_ERROR FS_write_local(struct one_wire_query *owq)
{
	return FS_w_local(owq);
}

// special case for fc_read_stable that actually doesn't add a written value, since the chip can modify it on writing
// i.e. The chip can decide the written value is out of range and use a valid entry instead.
static void FS_write_add_to_cache( struct one_wire_query *owq)
{
	if ( OWQ_pn(owq).selected_filetype->change == fc_read_stable ) {
		OWQ_Cache_Del(owq) ;
	} else {
		OWQ_Cache_Add(owq) ;
	}
}
