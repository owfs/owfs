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
#include "ow_specialcase.h"

/* ------- Prototypes ----------- */
static int FS_r_virtual(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_read_real(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_r_given_bus(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_r_local(struct one_wire_query *owq);
static int FS_read_from_parts(struct one_wire_query *owq);
static int FS_read_lump(struct one_wire_query *owq);
static int FS_structure(struct one_wire_query *owq);
static int FS_read_all_bits(struct one_wire_query *owq);
static int FS_read_one_bit(struct one_wire_query *owq);
static int FS_read_a_part(struct one_wire_query *owq);
static int FS_read_mixed_part(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_r_simultaneous(struct one_wire_query *owq) ;
static void adjust_file_size(struct one_wire_query *owq) ;


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

SIZE_OR_ERROR FS_read(const char *path, char *buf, const size_t size, const off_t offset)
{
	SIZE_OR_ERROR read_or_error;
	OWQ_allocate_struct_and_pointer(owq);

	LEVEL_CALL("path=%s size=%d offset=%d", SAFESTRING(path), (int) size, (int) offset);
	// Parseable path?
	if ( OWQ_create(path, owq) != 0 ) { // for read
		return -ENOENT;
	}
	OWQ_assign_read_buffer( buf, size, offset, owq) ;
	read_or_error = FS_read_postparse(owq);
	OWQ_destroy(owq);

	return read_or_error;
}

/* After parsing, but before sending to various devices. Will repeat 3 times if needed */
SIZE_OR_ERROR FS_read_postparse(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	SIZE_OR_ERROR read_or_error;

	// ServerRead jumps in here, perhaps with non-file entry
	if (pn->selected_device == NULL || pn->selected_filetype == NULL) {
		return -EISDIR;
	}

	/* Normal read. Try three times */
	LEVEL_DEBUG("%s", pn->path);
	STATLOCK;
	AVERAGE_IN(&read_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;

	/* First try */
	STAT_ADD1(read_tries[0]);

	read_or_error = (pn->type == ePN_real) ? FS_read_real(owq) : FS_r_virtual(owq);

	STATLOCK;
	if (read_or_error >= 0) {
		++read_success;			/* statistics */
		read_bytes += read_or_error;	/* statistics */
	}
	AVERAGE_OUT(&read_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;
	LEVEL_DEBUG("%s return %d", pn->path, read_or_error);
	return read_or_error;
}

/* Read real device (Non-virtual). Will repeat 3 times if needed */
static SIZE_OR_ERROR FS_read_real(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	SIZE_OR_ERROR read_or_error;

	/* First try */
	/* in and bus_nr already set */
	read_or_error = FS_read_distribute(owq);

	/* Second Try */
	/* if not a specified bus, relook for chip location */
	if (read_or_error < 0) {	//error
		STAT_ADD1(read_tries[1]);
		if (SpecifiedBus(pn)) {	// this bus or bust!
			if (TestConnection(pn)) {
				read_or_error = -ECONNABORTED;
			} else {
				read_or_error = FS_read_distribute(owq);	// 2nd try
				if (read_or_error < 0) {	// third try
					STAT_ADD1(read_tries[2]);
					read_or_error = FS_read_distribute(owq);
				}
			}
		} else if (BusIsServer(pn->selected_connection)) {
			int bus_nr = pn->selected_connection->index ; // current selected bus
			int busloc_or_error = ReCheckPresence(pn) ;
			// special handling or remote
			// only repeat if the bus number is wrong
			// because the remote does the rereads
			if ( bus_nr != busloc_or_error ) {
				if (busloc_or_error < 0) {
					read_or_error = -ENOENT;
				} else {
					read_or_error = FS_read_distribute(owq);
					if (read_or_error < 0) {	// third try
						STAT_ADD1(read_tries[2]);
						read_or_error = FS_read_distribute(owq);
					}
				}
			}
		} else {
			int busloc_or_error = ReCheckPresence(pn);	// search again
			if (busloc_or_error < 0) {
				read_or_error = -ENOENT;
			} else {
				read_or_error = FS_read_distribute(owq);
				if (read_or_error < 0) {	// third try
					STAT_ADD1(read_tries[2]);
					read_or_error = FS_read_distribute(owq);
				}
			}
		}
	}

	return read_or_error;
}

// This function probably needs to be modified a bit...
static SIZE_OR_ERROR FS_r_simultaneous(struct one_wire_query *owq)
{
	OWQ_allocate_struct_and_pointer(owq_given);

	if (SpecifiedBus(PN(owq))) {
		return FS_r_given_bus(owq);
	}

	memcpy(owq_given, owq, sizeof(struct one_wire_query));	// shallow copy

	// it's hard to know what we should return when reading /simultaneous/temperature
	SetKnownBus(0, PN(owq_given));
	return FS_r_given_bus(owq_given);
}


/* Note on return values */
/* functions return the actual number of bytes read, */
/* or a negative value if an error */
/* negative values are of the form -EINVAL, etc */
/* the negative of a system errno */

/* Note on size and offset: */
/* Buffer length (and requested data) is size bytes */
/* reading should start after offset bytes in original data */
/* only date, binary, and ascii data support offset in single data points */
/* only binary supports offset in array data */
/* size and offset are vetted against specification data size and calls */
/*   outside of this module will not have buffer overflows */
/* I.e. the rest of owlib can trust size and buffer to be legal */

/* After parsing, choose special read based on path type */
SIZE_OR_ERROR FS_read_distribute(struct one_wire_query *owq)
{
	SIZE_OR_ERROR read_or_error = 0;

	LEVEL_DEBUG("%s", OWQ_pn(owq).path);
	STATLOCK;
	AVERAGE_IN(&read_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;

	/* handle DeviceSimultaneous */
	if (PN(owq)->selected_device == DeviceSimultaneous) {
		read_or_error = FS_r_simultaneous(owq);
	} else {
		read_or_error = FS_r_given_bus(owq);
	}

	STATLOCK;
	if (read_or_error >= 0) {
		++read_success;			/* statistics */
		read_bytes += read_or_error;		/* statistics */
	}
	AVERAGE_OUT(&read_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;

	LEVEL_DEBUG("%s return %d", OWQ_pn(owq).path, read_or_error);
	//printf("FS_read_distribute: pid=%ld return %d\n", pthread_self(), read_or_error);
	return read_or_error;
}

// This function should return number of bytes read... not status.
static SIZE_OR_ERROR FS_r_given_bus(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int read_or_error = 0;

	LEVEL_DEBUG("structure contents before the read is performed");
	Debug_OWQ(owq);

	if (KnownBus(pn) && BusIsServer(pn->selected_connection)) {
		/* The bus is not local... use a network connection instead */
#if OW_MT
		LEVEL_DEBUG("pid=%ld call ServerRead", pthread_self());
#endif							/* OW_MT */
		// Read afar -- returns already formatted in buffer
		read_or_error = ServerRead(owq);
		LEVEL_DEBUG("back from server");
		//printf("FS_r_given_bus pid=%ld r=%d\n",pthread_self(), read_or_error);
	} else {
		STAT_ADD1(read_calls);	/* statistics */
		if (LockGet(pn) == 0) {
			read_or_error = FS_r_local(owq);	// this returns status
			LockRelease(pn);
			LEVEL_DEBUG("FS_r_local return=%d", read_or_error);
			if (read_or_error >= 0) {
				// local success -- now format in buffer
				read_or_error = OWQ_parse_output(owq);	// this returns nr. bytes
			}
		} else {
			LEVEL_DEBUG("Cannot lock bus to perform read") ;
			read_or_error = -EADDRINUSE;
		}
	}
	LEVEL_DEBUG("After read is performed (bytes or error %d)", read_or_error);
	Debug_OWQ(owq);
	return read_or_error;
}

// This function should return number of bytes read... not status.
// Works for all the fake directories, like statistics, interface, ...
// Doesn't need three-peat and bus was already set or not needed.
static int FS_r_virtual(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int read_status = 0;
	LEVEL_DEBUG("start");
	Debug_OWQ(owq);

	if (SpecifiedRemoteBus(pn)) {
		/* The bus is not local... use a network connection instead */
		// Read afar -- returns already formatted in buffer
		read_status = ServerRead(owq);
		LEVEL_DEBUG("back from server");
		Debug_OWQ(owq);
	} else {
		/* local bus -- any special locking needs? */
		STAT_ADD1(read_calls);	/* statistics */
		switch (pn->type) {
		case ePN_structure:
			read_status = FS_structure(owq);
			break;
		case ePN_interface:
			BUSLOCK(pn);
			read_status = FS_r_local(owq);	// this returns status
			BUSUNLOCK(pn);
			break;
		case ePN_statistics:
			// reading /statistics/read/tries.ALL
			// will cause a deadlock since it calls STAT_ADD1(read_array);
			// Should perhaps create a new mutex for this.
			// Comment out this STATLOCK until it's solved.
			// STATLOCK now done at time of actual read in ow_stats.c
			read_status = FS_r_local(owq);	// this returns status
			break;
		default:
			read_status = FS_r_local(owq);	// this returns status
		}
		if (read_status >= 0) {
			// local success -- now format in buffer
			read_status = OWQ_parse_output(owq);	// this returns nr. bytes
		}
	}
	LEVEL_DEBUG("return %d", read_status);
	return read_status;
}

size_t FileLength_vascii(struct one_wire_query * owq)
{
	size_t file_length = 0;

	// This is to avoid returning suglen on system/adapter/name.ALL etc with variable length
	if (PN(owq)->selected_filetype->ag != NON_AGGREGATE) {	/* array property */
		switch (PN(owq)->extension) {
		case EXTENSION_ALL:
			if (PN(owq)->selected_filetype->format == ft_bitfield) {
				return FileLength(PN(owq));
			}
			switch (PN(owq)->selected_filetype->ag->combined) {
			case ag_separate:	/* separate reads, artificially combined into a single array */
				// this is probably the only case needed
				file_length = FS_read_from_parts(owq);
				LEVEL_DEBUG("change1 file_length = %d", file_length);
				return file_length;
				break;
			case ag_mixed:		/* mixed mode, ALL read handled differently */
			case ag_aggregate:	/* natively an array */
				/* return ALL if required   (comma separated) */
			default:
				// don't think this will happen...
				file_length = FS_read_lump(owq);
				LEVEL_DEBUG("change2 file_length = %d", file_length);
				break;
			}
			break;
		default:
			// don't think this will happen...
			file_length = FileLength(PN(owq));
			LEVEL_DEBUG("change3 file_length = %d", file_length);
			break;
		}
	} else {
		// don't think this will happen...
		file_length = FileLength(PN(owq));
	}
	return file_length;
}

/* Adjusts size so that size+offset do not point past end of real file length*/
/* If offset is too large, size is set to 0 */
static void adjust_file_size(struct one_wire_query *owq)
{
    size_t file_length = 0;

    /* Adjust file length -- especially important for fuse which uses 4k buffers */
    /* First file filelength */
    if (PN(owq)->selected_filetype->format == ft_vascii) {
        file_length = FileLength_vascii(owq);
    } else {
        file_length = FullFileLength(PN(owq));
    }

    /* next adjust for offset */
    if ((unsigned long) OWQ_offset(owq) >= (unsigned long) file_length) {
        // This check is done in OWQ_parse_output() too, since it's always called when this function
        OWQ_size(owq) = 0 ;           // this is status ok... but 0 bytes were read...
    } else if ( OWQ_size(owq) + OWQ_offset(owq) > file_length ) {
        // Finally adjust buffer length
        OWQ_size(owq) = file_length - OWQ_offset(owq) ;
    }
    LEVEL_DEBUG("file_length=%lu offset=%lu size=%lu",
                (unsigned long) file_length, (unsigned long) OWQ_offset(owq), (unsigned long) OWQ_size(owq));
}

/* Real read -- called from read
Integrates with cache -- read not called if cached value already set
*/
static SIZE_OR_ERROR FS_r_local(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	
	/* Readable? */
	if (pn->selected_filetype->read == NO_READ_FUNCTION) {
		return -ENOTSUP;
	}
	
	// Check buffer length
	adjust_file_size(owq) ;
	if ( OWQ_size(owq) == 0 ) {
		// Mounting fuse with "direct_io" will cause a second read with offset
		// at end-of-file... Just return 0 if offset == size
		LEVEL_DEBUG("Call for read at very end of file") ;
		return 0 ;
	}
	
	if (pn->selected_filetype->change != fc_static && IsRealDir(pn)) {
		switch (pn->selected_connection->Adapter) {
			case adapter_fake:
				/* Special case for "fake" adapter */
				return FS_read_fake(owq);
			case adapter_tester:
				/* Special case for "tester" adapter */
				return FS_read_tester(owq);
			case adapter_mock:
				/* Special case for "mock" adapter */
				if (OWQ_Cache_Get(owq)) {	// non-zero means not found
					return FS_read_fake(owq);
				}
				return 0;
			default:
				break ;
		}
	}
	
	/* Array property? Read separately? Read together and manually separate? */
	if (pn->selected_filetype->ag != NON_AGGREGATE) {	/* array property */
		switch (pn->extension) {
			case EXTENSION_BYTE:
				return FS_read_lump(owq);
			case EXTENSION_ALL:
				if (OWQ_offset(owq) > 0) {
					return -1;		// no aggregates can be offset -- too confusing
				}
				if (pn->selected_filetype->format == ft_bitfield) {
					return FS_read_all_bits(owq);
				}
				switch (pn->selected_filetype->ag->combined) {
					case ag_separate:	/* separate reads, artificially combined into a single array */
						return FS_read_from_parts(owq);
					case ag_mixed:		/* mixed mode, ALL read handled differently */
					case ag_aggregate:	/* natively an array */
						/* return ALL if required   (comma separated) */
						return FS_read_lump(owq);
				}
			default:
				if (pn->selected_filetype->format == ft_bitfield) {
					return FS_read_one_bit(owq);
				}
				switch (pn->selected_filetype->ag->combined) {
					case ag_separate:	/* separate reads, artificially combined into a single array */
						return FS_read_lump(owq);
					case ag_mixed:		/* mixed mode, ALL read handled differently */
						return FS_read_mixed_part(owq);
					case ag_aggregate:	/* natively an array */
						return FS_read_a_part(owq);
				}
		}
	}
	
	/* Normal read. */
	return FS_read_lump(owq);
}

/* Structure file */
static int FS_structure(struct one_wire_query *owq)
{
	char ft_format_char[] = "  iufaabydytg";	/* return type */
	int output_length;
	OWQ_allocate_struct_and_pointer(owq_real);

	OWQ_create_shallow_single(owq_real, owq);	/* shallow copy */
	OWQ_pn(owq_real).type = ePN_real;	/* "real" type to get return length, rather than "structure" length */

	LEVEL_DEBUG("start");
	UCLIBCLOCK;
	output_length = snprintf(OWQ_buffer(owq),
							 OWQ_size(owq),
							 "%c,%.6d,%.6d,%.2s,%.6d,",
							 ft_format_char[OWQ_pn(owq_real).
											selected_filetype->format],
							 (OWQ_pn(owq_real).selected_filetype->
							  ag) ? OWQ_pn(owq_real).extension : 0,
							 (OWQ_pn(owq_real).selected_filetype->
							  ag) ? OWQ_pn(owq_real).selected_filetype->
							 ag->elements : 1,
							 (OWQ_pn(owq_real).selected_filetype->read ==
							  NO_READ_FUNCTION) ? ((OWQ_pn(owq_real).selected_filetype->write == NO_WRITE_FUNCTION) ? "oo" : "wo")
							 : ((OWQ_pn(owq_real).selected_filetype->write == NO_WRITE_FUNCTION) ? "ro" : "rw"), (int) FullFileLength(PN(owq_real))
		);
	UCLIBCUNLOCK;
	LEVEL_DEBUG("length=%d", output_length);

	if (output_length < 0) {
		return -EFAULT;
	}

	OWQ_length(owq) = output_length;

	return 0;
}

/* read without artificial separation or combination */
static int FS_read_lump(struct one_wire_query *owq)
{
	if (OWQ_Cache_Get(owq)) {	// non-zero means not found
		int read_error = SpecialCase_read(owq);
		if ( read_error == -ENOENT ) {
			read_error = (OWQ_pn(owq).selected_filetype->read) (owq);
		}
		if (read_error < 0) {
			return read_error;
		}
		OWQ_Cache_Add(owq);
	}
	return 0;
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->selected_filetype->ag->combined==ag_separate */
static int FS_read_from_parts(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t elements = pn->selected_filetype->ag->elements;
	size_t extension;
	OWQ_allocate_struct_and_pointer(owq_single);

	STAT_ADD1(read_array);		/* statistics */

	// Check that there is enough space for the combined message
	switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_binary:
			// sneaky! Move owq_single buffer location within owq buffer
			if ( OWQ_size(owq) < FullFileLength(pn) || OWQ_offset(owq) != 0 ) {
				return -EMSGSIZE ;
			}
			break;
		default:
			break;
	}

	/* shallow copy */
	// Use owq buffer and file length
	OWQ_create_temporary(owq_single, OWQ_buffer(owq), FileLength(pn), 0, pn);

	/* Loop through F_r_single, just to get data */
	for (extension = 0; extension < elements; ++extension) {
		int lump_read ;
		
		OWQ_pn(owq_single).extension = extension;
		lump_read = FS_read_lump(owq_single);
		if (lump_read < 0) {
			return lump_read;
		}
		
		// copy object (single to mixed array)
		memcpy(&OWQ_array(owq)[extension], &OWQ_val(owq_single), sizeof(union value_object));
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_binary:
			// sneaky! Move owq_single buffer location within owq buffer
			OWQ_buffer(owq_single) += OWQ_length(owq_single);
			break;
		default:
			break;
		}
	}

	return 0;
}

/* bitfield -- read the UINT and convert to explicit array */
static int FS_read_all_bits(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t elements = pn->selected_filetype->ag->elements;
	size_t extension;
	int lump_read;
	UINT U;
	OWQ_allocate_struct_and_pointer(owq_single);

	STAT_ADD1(read_array);		/* statistics */

	/* shallow copy */
	OWQ_create_shallow_single(owq_single, owq);

	/* read the UINT */
	lump_read = FS_read_lump(owq_single);
	if (lump_read < 0) {
		return lump_read;
	}
	U = OWQ_U(owq_single);

	/* Loop through F_r_single, just to get data */
	for (extension = 0; extension < elements; ++extension) {
		OWQ_array_Y(owq, extension) = UT_getbit((void *) (&U), extension);
	}

	return 0;
}

/* bitfield -- read the UINT and extract single value */
static int FS_read_one_bit(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int lump_read;
	UINT U;
	OWQ_allocate_struct_and_pointer(owq_single);

	STAT_ADD1(read_array);		/* statistics */

	/* shallow copy */
	OWQ_create_shallow_single(owq_single, owq);

	/* read the UINT */
	lump_read = FS_read_lump(owq_single);
	if (lump_read < 0) {
		return lump_read;
	}
	U = OWQ_U(owq_single);

	OWQ_Y(owq) = UT_getbit((void *) (&U), pn->extension);

	return 0;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->selected_filetype->ag->combined==ag_aggregate */
static int FS_read_a_part(struct one_wire_query *owq)
{
	size_t extension = OWQ_pn(owq).extension;
	int lump_read;
	OWQ_allocate_struct_and_pointer(owq_all);

	if (OWQ_create_shallow_aggregate(owq_all, owq)) {
		return -ENOMEM;
	}

	lump_read = FS_read_lump(owq_all);
	if (lump_read < 0) {
		OWQ_destroy_shallow_aggregate(owq_all);
		return lump_read;
	}

	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_vascii:
	case ft_ascii:
	case ft_binary:
		{
			size_t prior_length = 0;
			size_t extension_index;
			int return_status;
			for (extension_index = 0; extension_index < extension; ++extension_index) {
				prior_length += OWQ_array_length(owq_all, extension_index);
			}
			return_status = OWQ_parse_output_offset_and_size(&OWQ_buffer(owq_all)[prior_length], OWQ_array_length(owq_all, extension), owq);
			if (return_status < 0) {
				OWQ_destroy_shallow_aggregate(owq_all);
				return return_status;
			}
			OWQ_length(owq) = return_status;
			break;
		}
	default:
		memcpy(&OWQ_val(owq), &OWQ_array(owq_all)[extension], sizeof(union value_object));
		break;
	}

	OWQ_destroy_shallow_aggregate(owq_all);
	return 0;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->selected_filetype->ag->combined==ag_aggregate */
static int FS_read_mixed_part(struct one_wire_query *owq)
{
	size_t extension = OWQ_pn(owq).extension;
	OWQ_allocate_struct_and_pointer(owq_all);

	if (OWQ_create_shallow_aggregate(owq_all, owq)) {
		return -ENOMEM;
	}

	if (OWQ_Cache_Get(owq_all)) {	// no "all" cached
		OWQ_destroy_shallow_aggregate(owq_all);
		return FS_read_lump(owq);	// read individual value */
	}

	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_vascii:
	case ft_ascii:
	case ft_binary:
		{
			size_t prior_length = 0;
			size_t extension_index;
			int return_status;
			for (extension_index = 0; extension_index < extension; ++extension_index) {
				prior_length += OWQ_array_length(owq_all, extension_index);
			}
			return_status = OWQ_parse_output_offset_and_size(&OWQ_buffer(owq_all)[prior_length], OWQ_array_length(owq_all, extension), owq);
			if (return_status < 0) {
				OWQ_destroy_shallow_aggregate(owq_all);
				return return_status;
			}
			OWQ_length(owq) = return_status;
			break;
		}
	default:
		memcpy(&OWQ_val(owq), &OWQ_array(owq_all)[extension], sizeof(union value_object));
		break;
	}

	OWQ_Cache_Del(owq_all);
	OWQ_destroy_shallow_aggregate(owq_all);
	return 0;
}

/* Used in sibling reads
   Reads locally without locking the bus (since it's already locked)
*/
SIZE_OR_ERROR FS_read_local( struct one_wire_query *owq)
{
	return FS_r_local(owq);
}
