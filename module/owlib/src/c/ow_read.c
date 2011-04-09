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
static SIZE_OR_ERROR FS_r_virtual(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_read_real(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_r_given_bus(struct one_wire_query *owq);
static SIZE_OR_ERROR FS_r_local(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_read_owq(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_structure(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_read_all_bits(struct one_wire_query *owq_byte);
static ZERO_OR_ERROR FS_read_a_bit( struct one_wire_query *owq_bit );
static SIZE_OR_ERROR FS_r_simultaneous(struct one_wire_query *owq) ;
static void adjust_file_size(struct one_wire_query *owq) ;
static SIZE_OR_ERROR FS_read_distribute(struct one_wire_query *owq) ;
static ZERO_OR_ERROR FS_read_all( struct one_wire_query *owq_all ); 
static ZERO_OR_ERROR FS_read_a_part( struct one_wire_query *owq_part );
static ZERO_OR_ERROR FS_read_in_parts( struct one_wire_query *owq_all );

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
	if ( BAD( OWQ_create(path, owq) ) ) { // for read
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
	if (pn->selected_device == NO_DEVICE || pn->selected_filetype == NO_FILETYPE) {
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
			if ( BAD(TestConnection(pn)) ) {
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
			INDEX_OR_ERROR busloc_or_error = ReCheckPresence(pn) ;
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
			INDEX_OR_ERROR busloc_or_error = ReCheckPresence(pn);	// search again
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
	// Device not locked.
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
static SIZE_OR_ERROR FS_read_distribute(struct one_wire_query *owq)
{
	// Device not locked
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
	// Device locking occurs here
	struct parsedname *pn = PN(owq);
	SIZE_OR_ERROR read_or_error = 0;

	LEVEL_DEBUG("About to read <%s> extension=%d size=%d offset=%d",SAFESTRING(pn->path),(int) pn->extension,(int) OWQ_size(owq),(int) OWQ_offset(owq));

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
		if (DeviceLockGet(pn) == 0) {
			read_or_error = FS_r_local(owq);	// this returns status
			DeviceLockRelease(pn);
			LEVEL_DEBUG("return=%d", read_or_error);
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
// Works for all the virtual directories, like statistics, interface, ...
// Doesn't need three-peat and bus was already set or not needed.
static SIZE_OR_ERROR FS_r_virtual(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	SIZE_OR_ERROR read_status = 0;

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

/* Adjusts size so that size+offset do not point past end of real file length*/
/* If offset is too large, size is set to 0 */
static void adjust_file_size(struct one_wire_query *owq)
{
	size_t file_length = 0;

	/* Adjust file length -- especially important for fuse which uses 4k buffers */
	/* First file filelength */
	file_length = FullFileLength(PN(owq));

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
	// Bus and device already locked
	struct parsedname *pn = PN(owq);
	struct filetype * ft = pn->selected_filetype ;
	
	/* Readable? */
	if (ft->read == NO_READ_FUNCTION) {
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

	// Special check for alias -- it's ok for fake and tester and mock as well
	if ( ft->read == FS_r_alias ) {
		return FS_read_owq(owq) ;
	}
	
	if (ft->change != fc_static && ft->format != ft_alias && IsRealDir(pn)) {
		switch (pn->selected_connection->Adapter) {
			case adapter_fake:
				/* Special case for "fake" adapter */
				return FS_read_fake(owq);
			case adapter_tester:
				/* Special case for "tester" adapter */
				return FS_read_tester(owq);
			case adapter_mock:
				/* Special case for "mock" adapter */
				if ( GOOD( OWQ_Cache_Get(owq)) ) {	// cached
					return 0;
				}
				return FS_read_fake(owq);
			default:
				break ;
		}
	}
	
	if ( ft->ag == NON_AGGREGATE ) {	/* non array property */
		return FS_read_owq(owq) ;
	}
	
	/* Array property 
	 * Read separately? 
	 * Read together and manually separate? */
	/* array */
	switch ( ft->ag->combined ) {
		case ag_aggregate:
			switch (pn->extension) {
				case EXTENSION_BYTE:
					LEVEL_DEBUG("Read an aggregate .BYTE %s",pn->path);
					return FS_read_owq(owq);
				case EXTENSION_ALL:
					LEVEL_DEBUG("Read an aggregate .ALL %s",pn->path);
					return FS_read_all(owq);
				default:
					LEVEL_DEBUG("Read an aggregate element %s",pn->path);
					return FS_read_a_part(owq);
			}
		case ag_mixed:
			switch (pn->extension) {
				case EXTENSION_BYTE:
					LEVEL_DEBUG("Read a mixed .BYTE %s",pn->path);
					OWQ_Cache_Del_parts(owq);
					return FS_read_owq(owq);
				case EXTENSION_ALL:
					LEVEL_DEBUG("Read a mixed .ALL %s",pn->path);
					OWQ_Cache_Del_parts(owq);
					return FS_read_all(owq);
				default:
					LEVEL_DEBUG("Read a mixed element %s",pn->path);
					OWQ_Cache_Del_ALL(owq);
					OWQ_Cache_Del_BYTE(owq);
					return FS_read_owq(owq);
			}
		case ag_separate:
			switch (pn->extension) {
				case EXTENSION_BYTE:
					LEVEL_DEBUG("Read a separate .BYTE %s",pn->path);
					return FS_read_all_bits(owq);
				case EXTENSION_ALL:
					LEVEL_DEBUG("Read a separate .ALL %s",pn->path);
					return FS_read_in_parts(owq);
				default:
					LEVEL_DEBUG("Read a separate element %s",pn->path);
					return FS_read_owq(owq);
			}
		default:
			return -ENOENT ;
	}
}

/* Structure file */
static ZERO_OR_ERROR FS_structure(struct one_wire_query *owq)
{
	char structure_text[PROPERTY_LENGTH_STRUCTURE+1] ;
	int output_length;
	struct parsedname * pn = PN(owq) ;
	struct filetype * ftype = pn->selected_filetype ;
	char format_char ;
	char change_char ;

	// TEMPORARY change type from structure->real to get real length rhather than structure length
	pn->type = ePN_real;	/* "real" type to get return length, rather than "structure" length */

	// Set property type
	switch ( ftype->format ) {
		case ft_directory:
		case ft_subdir:
			format_char = 'D' ;
			break ;
		case ft_integer:
			format_char = 'i' ;
			break ;
		case ft_unsigned:
			format_char = 'u' ;
			break ;
		case ft_float:
			format_char = 'f' ;
			break ;
		case ft_alias:
			format_char = 'l' ;
			break ;
		case ft_ascii:
		case ft_vascii:
			format_char = 'a' ;
			break ;
		case ft_binary:
			format_char = 'b' ;
			break ;
		case ft_yesno:
		case ft_bitfield:
			format_char = 'y' ;
			break ;
		case ft_date:
			format_char = 'd' ;
			break ;
		case ft_temperature:
			format_char = 't' ;
			break ;
		case ft_tempgap:
			format_char = 'g' ;
			break ;
		case ft_pressure:
			format_char = 'p' ;
			break ;
		case ft_unknown:
		default:
			format_char = '?' ;
			break ;
	}

	// Set changability
	switch ( ftype->change ) {
		case fc_static:
		case fc_subdir:
		case fc_directory:
			change_char = 'f' ;
			break ;
		case fc_stable:
		case fc_persistent:
			change_char = 's' ;
			break ;
		case fc_volatile:
		case fc_read_stable:
		case fc_simultaneous_temperature:
		case fc_simultaneous_voltage:
		case fc_uncached:
		case fc_statistic:
		case fc_presence:
		case fc_link:
		case fc_page:
			change_char = 'v' ;
			break ;
		case fc_second:
		default:
			change_char = 't' ;
			break ;
	}

	UCLIBCLOCK;
	output_length = snprintf(structure_text,
		 PROPERTY_LENGTH_STRUCTURE+1,
		 "%c,%.6d,%.6d,%.2s,%.6d,%c,",
		 format_char,
		 (ftype->ag) ? pn->extension : 0,
		 (ftype->ag) ? ftype->ag->elements : 1,
		 (ftype->read == NO_READ_FUNCTION) ? ((ftype->write == NO_WRITE_FUNCTION) ? "oo" : "wo")
		                                   : ((ftype->write == NO_WRITE_FUNCTION) ? "ro" : "rw"), 
		 (int) FullFileLength(PN(owq)),
		 change_char
		);
	UCLIBCUNLOCK;
	LEVEL_DEBUG("length=%d", output_length);

	// TEMPORARY restore type from real to structure
	pn->type = ePN_structure;

	if (output_length < 0) {
		LEVEL_DEBUG("Problem formatting structure string");
		return -EFAULT;
	}
	if ( output_length > PROPERTY_LENGTH_STRUCTURE ) {
		LEVEL_DEBUG("Structure string too long");
		return -EINVAL ;
	}

	return OWQ_format_output_offset_and_size(structure_text,output_length,owq);
}

/* read without artificial separation or combination */
// Handles: ALL
static ZERO_OR_ERROR FS_read_all( struct one_wire_query *owq_all)
{
	struct parsedname * pn = PN(owq_all) ;
	
	// bitfield, convert to .BYTE format and write ( and delete cache ) as BYTE.
	if ( pn->selected_filetype->format == ft_bitfield ) {
		struct one_wire_query * owq_byte = OWQ_create_separate( EXTENSION_BYTE, owq_all ) ;
		if ( owq_byte != NO_ONE_WIRE_QUERY ) {
			if ( FS_read_owq( owq_byte ) >= 0 ) {
				size_t elements = pn->selected_filetype->ag->elements;
				size_t extension;
			
				for ( extension=0 ; extension < elements ; ++extension ) {
					OWQ_array_Y(owq_all,extension) = UT_getbit( (BYTE *) &OWQ_U(owq_byte), extension ) ;
				}
				return 0 ;
			}
			OWQ_destroy( owq_byte ) ;
			return -EINVAL ;
		}
		return -ENOENT ;
	}
	return FS_read_owq( owq_all ) ;
}

/* read in native format */
/* ALL for aggregate
 * .n  for separate
 * BYTE bitfield
 * */
static ZERO_OR_ERROR FS_read_owq(struct one_wire_query *owq)
{
	// Bus and device already locked
	if ( BAD( OWQ_Cache_Get(owq)) ) {	// not found
		ZERO_OR_ERROR read_error = (OWQ_pn(owq).selected_filetype->read) (owq);
		LEVEL_DEBUG("Read %s Extension %d Gives result %d",PN(owq)->path,PN(owq)->extension,read_error);
		if (read_error < 0) {
			return read_error;
		}
		OWQ_Cache_Add(owq); // Only add good attempts
	}
	return 0;
}

/* Read each array element independently, but return as one long string */
// Handles: ALL
static ZERO_OR_ERROR FS_read_in_parts( struct one_wire_query *owq_all )
{
	struct parsedname *pn = PN(owq_all);
	struct filetype * ft = pn->selected_filetype ;
	struct one_wire_query * owq_part ;
	size_t elements = pn->selected_filetype->ag->elements;
	size_t extension;
	char * buffer_pointer = OWQ_buffer(owq_all) ;
	size_t buffer_left = OWQ_size(owq_all) ;
	
	// single for BYTE or iteration 
	owq_part = OWQ_create_separate( 0, owq_all ) ;
	if ( owq_part == NO_ONE_WIRE_QUERY ) {
		return -ENOENT ;
	}
	
	// bitfield
	if ( ft->format == ft_bitfield ) {
		OWQ_pn(owq_part).extension = EXTENSION_BYTE ;
		if ( FS_read_owq(owq_part) < 0 ) {
			OWQ_destroy( owq_part ) ;
			return -EINVAL ;
		}
		for (extension = 0; extension < elements; ++extension) {
			OWQ_array_Y(owq_all,extension) = UT_getbit( (BYTE *) &OWQ_U(owq_part), extension ) ;
		}
		OWQ_destroy( owq_part ) ;
		return 0 ;
	}

	if ( BAD( OWQ_allocate_read_buffer( owq_part )) ) {
		LEVEL_DEBUG("Can't allocate buffer space");
		OWQ_destroy( owq_part ) ;
		return -EMSGSIZE ;
	}

	/* Loop through get data */
	for (extension = 0; extension < elements; ++extension) {
		size_t part_length ;
		OWQ_pn(owq_part).extension = extension;
		if ( FS_read_owq(owq_part) < 0 ) {
			OWQ_destroy( owq_part ) ;
			return -EINVAL ;
		}
		
		// Check that there is enough space for the combined message
		switch ( ft->format ) {
			case ft_ascii:
			case ft_vascii:
			case ft_alias:
			case ft_binary:
				part_length = OWQ_length(owq_part) ;
				if ( buffer_left < part_length ) {
					OWQ_destroy( owq_part ) ;
					return -EMSGSIZE ;
				}
				memcpy( buffer_pointer, OWQ_buffer(owq_part), part_length ) ;
				OWQ_array_length(owq_all,extension) = part_length ;
				buffer_pointer += part_length ;
				buffer_left -= part_length ;
				break ;
			default:
				// copy object (single to mixed array)
				memcpy(&OWQ_array(owq_all)[extension], &OWQ_val(owq_part), sizeof(union value_object));
				break;
		}
	}

	OWQ_destroy( owq_part ) ;
	return 0;
}

/* read a BYTE using bits */
// Handles: BYTE
static ZERO_OR_ERROR FS_read_all_bits(struct one_wire_query *owq_byte)
{
	struct one_wire_query * owq_bit = OWQ_create_separate( 0, owq_byte ) ;
	struct parsedname *pn = PN(owq_byte);
	size_t elements = pn->selected_filetype->ag->elements;
	size_t extension;
	
	if ( owq_bit == NO_ONE_WIRE_QUERY ) {
		return -ENOENT ;
	}

	/* Loop through F_r_single, just to get data */
	for (extension = 0; extension < elements; ++extension) {
		OWQ_pn(owq_bit).extension = extension ;
		if ( FS_read_owq(owq_bit) < 0 ) {
			OWQ_destroy(owq_bit);
			return -EINVAL;
		}
		UT_setbit( (BYTE *) &OWQ_U(owq_byte), extension, OWQ_Y(owq_bit) ) ;
	}

	OWQ_destroy(owq_bit);
	return 0;
}

/* read a bit from BYTE */
static ZERO_OR_ERROR FS_read_a_bit( struct one_wire_query *owq_bit )
{
	// Bus and device already locked
	struct one_wire_query * owq_byte = OWQ_create_separate( EXTENSION_BYTE, owq_bit ) ;
	struct parsedname *pn = PN(owq_bit);
	ZERO_OR_ERROR z_or_e = -ENOENT ;
	
	if ( owq_byte == NO_ONE_WIRE_QUERY ) {
		return -ENOENT ;
	}

	/* read the UINT */
	if ( FS_read_owq(owq_byte) >= 0) {
		OWQ_Y(owq_bit) = UT_getbit((void *) &OWQ_U(owq_byte), pn->extension) ;
		z_or_e = 0 ;
	}

	OWQ_destroy(owq_byte);
	return z_or_e;
}

/* Read just one field of an aggregate property -- but a property that is handled as one big object */
// Handles .n
static ZERO_OR_ERROR FS_read_a_part( struct one_wire_query *owq_part )
{
	struct parsedname *pn = PN(owq_part);
	size_t extension = pn->extension;
	struct filetype * ft = pn->selected_filetype ;
	struct one_wire_query * owq_all ;
	
	// bitfield
	if ( ft->format == ft_bitfield ) {
		return FS_read_a_bit( owq_part ) ;
	}

	// non-bitfield 
	owq_all = OWQ_create_aggregate( owq_part ) ;
	if ( owq_all == NO_ONE_WIRE_QUERY ) {
		return -ENOENT ;
	}
	
	// First fill the whole array with current values
	if ( FS_read_owq( owq_all ) < 0 ) {
		OWQ_destroy( owq_all ) ;
		return -EINVAL ;
	}

	// Copy ascii/binary field
	switch (ft->format) {
	case ft_binary:
	case ft_ascii:
	case ft_vascii:
	case ft_alias:
		{
			size_t extension_index;
			char *buffer_pointer = OWQ_buffer(owq_all);

			// All prior elements
			for (extension_index = 0; extension_index < extension; ++extension) {
				// move past their buffer position
				buffer_pointer += OWQ_array_length(owq_all, extension_index);
			}

			// now move current element's buffer to location
			OWQ_length(owq_part) = OWQ_array_length(owq_all, extension) ;
			memmove( OWQ_buffer(owq_part), buffer_pointer, OWQ_length(owq_part));
			break;
		}
	default:
		// Copy value field
		memcpy( &OWQ_val(owq_part), &OWQ_array(owq_all)[pn->extension], sizeof(union value_object) );
		break;
	}

	OWQ_destroy(owq_all);
	return 0 ;
}

/* Used in sibling reads
   Reads locally without locking the bus (since it's already locked)
*/
SIZE_OR_ERROR FS_read_local( struct one_wire_query *owq)
{
	return FS_r_local(owq);
}
