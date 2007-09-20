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
static int FS_r_given_bus(struct one_wire_query * owq);
static int FS_r_local(struct one_wire_query * owq);
static int FS_read_from_parts(struct one_wire_query * owq);
static int FS_read_lump(struct one_wire_query * owq);
static int FS_structure(struct one_wire_query * owq) ;
static int FS_read_all_bits(struct one_wire_query * owq) ;
static int FS_read_one_bit(struct one_wire_query * owq) ;
static int FS_read_a_part(struct one_wire_query * owq) ;
static int FS_read_mixed_part(struct one_wire_query * owq) ;


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

int FS_read(const char *path, char *buf, const size_t size,
            const off_t offset)
{
	OWQ_allocate_struct_and_pointer( owq ) ;
	int read_or_error ;

	LEVEL_CALL("READ path=%s size=%d offset=%d\n", SAFESTRING(path),
		   (int) size, (int) offset) ;
	// Parseable path?
	if (FS_OWQ_create(path,buf,size,offset,owq))
		return -ENOENT;

	read_or_error = FS_read_postparse(owq);
	FS_OWQ_destroy(owq);
    
	return read_or_error;
}

/* After parsing, but before sending to various devices. Will repeat 3 times if needed */
int FS_read_postparse(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int read_or_error ;

	// ServerRead jumps in here, perhaps with non-file entry
	if (pn->dev == NULL || pn->selected_filetype == NULL)
		return -EISDIR;

	/* Normal read. Try three times */
	LEVEL_DEBUG("READ_POSTPARSE %s\n", pn->path);
	STATLOCK;
	AVERAGE_IN(&read_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;

	/* First try */
	/* in and bus_nr already set */
	STAT_ADD1(read_tries[0]);
	read_or_error = FS_read_distribute(owq);

	/* Second Try */
	/* if not a specified bus, relook for chip location */
	if (read_or_error < 0) { //error
		STAT_ADD1(read_tries[1]);
		if (Global.opt == opt_server) { // called from owserver
			// Only one try (repeated remotely)
			Cache_Del_Device(pn);
		} else if (SpecifiedBus(pn)) { // this bus or bust!
			if ( TestConnection(pn) ) {
				read_or_error = -ECONNABORTED ;
			} else {
				read_or_error = FS_read_distribute(owq) ; // 2nd try
				if ( read_or_error < 0 ) { // third try
					STAT_ADD1(read_tries[2]);
					read_or_error = FS_read_distribute(owq);
				}
			}
		} else {
			int busloc_or_error ;
			UnsetKnownBus(pn) ; // eliminate cached presence
			busloc_or_error = CheckPresence(pn) ; // search again
			if ( busloc_or_error < 0 ) {
				read_or_error = -ENOENT ;
			} else {
				read_or_error = FS_read_distribute(owq);
				if ( read_or_error < 0 ) { // third try
					STAT_ADD1(read_tries[2]);
					read_or_error = FS_read_distribute(owq);
				}
			}
		}
	}

	STATLOCK;
	if (read_or_error >= 0) {
		++read_success;         /* statistics */
		read_bytes += read_or_error;        /* statistics */
	}
	AVERAGE_OUT(&read_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;
	LEVEL_DEBUG("READ_POSTPARSE %s return %d\n", pn->path, read_or_error);
	return read_or_error;
}

// This function probably need to be modified a bit...
static int FS_r_simultaneous(struct one_wire_query * owq)
{
	if (SpecifiedBus(PN(owq))) {
		return FS_r_given_bus(owq);
	} else {
		OWQ_allocate_struct_and_pointer(owq_given);

		memcpy(owq_given, owq, sizeof(struct one_wire_query));	// shallow copy

		// it's hard to know what we should return when reading /simultaneous/temperature
		SetKnownBus(0, PN(owq_given));
		return FS_r_given_bus(owq_given);
	}
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
int FS_read_distribute(struct one_wire_query * owq)
{
	int r = 0;

	LEVEL_DEBUG("READ_POSTPOSTPARSE %s\n", OWQ_pn(owq).path);
	STATLOCK;
	AVERAGE_IN(&read_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;

	switch (OWQ_pn(owq).type) {
        case pn_structure:
		/* Get structure data from local memory */
		//printf("FS_read_distribute: pid=%ld call fs_structure\n", pthread_self());
		r = FS_structure(owq);
		break;
        default:
		/* handle DeviceSimultaneous */
		if (PN(owq)->dev == DeviceSimultaneous) {
			r = FS_r_simultaneous(owq);
		} else {
			r = FS_r_given_bus(owq);
		}
		break;
	}
	STATLOCK;
	if (r >= 0) {
		++read_success;         /* statistics */
		read_bytes += r;        /* statistics */
	}
	AVERAGE_OUT(&read_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;
	
	LEVEL_DEBUG("READ_POSTPOSTPARSE: %s return %d\n", OWQ_pn(owq).path, r);
	//printf("FS_read_distribute: pid=%ld return %d\n", pthread_self(), r);
	return r;
}

// This function should return number of bytes read... not status.
static int FS_r_given_bus(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	int read_status = 0;
	//printf("FS_r_given_bus\n");
	LEVEL_DEBUG("FS_r_given_bus\n");
	Debug_OWQ(owq) ;
	
	if (!KnownBus(pn)) {
		if(pn->type == pn_settings) {
			/* I think we have to do something here... */
			LEVEL_DEBUG("FS_r_given_bus: ERROR bus is not set and type=pn_settings!\n");
		} else if(pn->type == pn_system) {
			/* I think we have to do something here... 
			 * reading /system/adapter/name.0 works
			 * reading /bus.0/system/adapter/name.0 works
			 * writing /bus.0/system/adapter/overdrive.0 crash owfs
			 */
			LEVEL_DEBUG("FS_r_given_bus: ERROR bus is not set and type=pn_system!\n");
		} else {
			LEVEL_DEBUG("FS_r_given_bus: ERROR bus is not set! type=%d\n", pn->type);
		}
	}
	if (KnownBus(pn) && BusIsServer(pn->in)) {
		/* The bus is not local... use a network connection instead */
#if OW_MT
		LEVEL_DEBUG("FS_r_given_bus pid=%ld call ServerRead\n",
			    pthread_self());
#endif /* OW_MT */
		// Read afar -- returns already formatted in buffer
		read_status = ServerRead(owq);
		LEVEL_DEBUG("FS_r_given_bus -- back from server\n");
		Debug_OWQ(owq) ;
		//printf("FS_r_given_bus pid=%ld r=%d\n",pthread_self(), r);
	} else {
		STAT_ADD1(read_calls);  /* statistics */
		if (LockGet(pn) == 0) {
			read_status = FS_r_local(owq);  // this returns status
			LEVEL_DEBUG("FS_r_given_bus FS_r_local return=%d\n", read_status);
			if ( read_status >= 0 ) {
				// local success -- now format in buffer
				read_status = FS_output_owq(owq) ; // this returns nr. bytes
			}
			LockRelease(pn);
		} else {
			read_status = -EADDRINUSE ;
		}
	}
	LEVEL_DEBUG("FS_r_given_bus return %d\n", read_status);
	return read_status ;
}

size_t FileLength_vascii(struct one_wire_query * owq)
{
  size_t file_length = 0;

  // This is to avoid returning suglen on system/adapter/name.ALL etc with variable length
  if (PN(owq)->selected_filetype->ag) {           /* array property */
    switch (PN(owq)->extension) {
    case EXTENSION_ALL:
      if ( PN(owq)->selected_filetype->format == ft_bitfield ) return FileLength(PN(owq));
      switch (PN(owq)->selected_filetype->ag->combined) {
      case ag_separate:   /* separate reads, artificially combined into a single array */
	// this is probably the only case needed
	file_length = FS_read_from_parts(owq);
	LEVEL_DEBUG("FS_r_local: change1 file_length = %d\n", file_length);
	return file_length;
	break;
      case ag_mixed:      /* mixed mode, ALL read handled differently */
      case ag_aggregate:  /* natively an array */
	/* return ALL if required   (comma separated) */
      default:
	// don't think this will happen...
	file_length = FS_read_lump(owq);
	LEVEL_DEBUG("FS_r_local: change2 file_length = %d\n", file_length);
	break;
      }
      break;
    default:
	// don't think this will happen...
      file_length = FileLength(PN(owq));
      LEVEL_DEBUG("FS_r_local: change3 file_length = %d\n", file_length);
      break;
    }
  } else {
    // don't think this will happen...
    file_length = FileLength(PN(owq));
  }
  return file_length;
}

/* Real read -- called from read
   Integrates with cache -- read not called if cached value already set
*/
static int FS_r_local(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    
    /* Readable? */
    if ( pn->selected_filetype->read == NO_READ_FUNCTION )
        return -ENOTSUP;

    /* Mounting fuse with "direct_io" will cause a second read with offset
     * at end-of-file... Just return 0 if offset == size */
    // This check is done in FS_output_owq() too, since it's always called when this function returns 0
    if(OWQ_offset(owq)) {
      size_t file_length = 0;
      if( PN(owq)->selected_filetype->format == ft_vascii) {
	file_length = FileLength_vascii(owq);
      } else {
	file_length = FileLength(PN(owq));
      }
      LEVEL_DEBUG("FS_r_local: file_length=%lu offset=%lu size=%lu\n",
		  (unsigned long)file_length, (unsigned long)OWQ_offset(owq), (unsigned long)OWQ_size(owq));
      if ((unsigned long)OWQ_offset(owq) >= (unsigned long)file_length) {
        return 0;  // this is status ok... but 0 bytes were read...
      }
    }

    /* Special case for "fake" adapter */
    if (pn->in->Adapter == adapter_fake && pn->selected_filetype->change != fc_static && IsRealDir(pn))
        return FS_read_fake(owq);

    /* Special case for "tester" adapter */
    if (pn->in->Adapter == adapter_tester && pn->selected_filetype->change != fc_static)
        return FS_read_tester(owq);

    /* Array property? Read separately? Read together and manually separate? */
    if (pn->selected_filetype->ag) {           /* array property */
        switch (pn->extension) {
            case EXTENSION_BYTE:
                return FS_read_lump(owq);
            case EXTENSION_ALL:
                if ( OWQ_offset(owq) > 0 ) return 0 ; // no aggregates can be offset -- too confusing
                if ( pn->selected_filetype->format == ft_bitfield ) return FS_read_all_bits(owq) ;
                switch (pn->selected_filetype->ag->combined) {
                    case ag_separate:   /* separate reads, artificially combined into a single array */
                        return FS_read_from_parts(owq);
                    case ag_mixed:      /* mixed mode, ALL read handled differently */
                    case ag_aggregate:  /* natively an array */
                        /* return ALL if required   (comma separated) */
                        return FS_read_lump(owq);
                }
            default:
                if ( pn->selected_filetype->format == ft_bitfield ) return FS_read_one_bit(owq) ;
                switch (pn->selected_filetype->ag->combined) {
                    case ag_separate:   /* separate reads, artificially combined into a single array */
                        return FS_read_lump(owq);
                    case ag_mixed:      /* mixed mode, ALL read handled differently */
                        return FS_read_mixed_part(owq) ;
                    case ag_aggregate:  /* natively an array */
                        return FS_read_a_part(owq) ;
                }
        }
    }

    /* Normal read. */
    return FS_read_lump(owq);
}

/* Structure file */
static int FS_structure(struct one_wire_query * owq)
{
    char ft_format_char[] = "  iufaabydytg";    /* return type */
    char output_string[PROPERTY_LENGTH_STRUCTURE+1] ;
    int output_length;
    int print_status ;
    OWQ_allocate_struct_and_pointer( owq_copy ) ;
    OWQ_create_shallow_single( owq_copy, owq ); /* shallow copy */
    OWQ_pn(owq_copy).type = pn_real;            /* "real" type to get return length, rather than "structure" length */

    UCLIBCLOCK;
    output_length = snprintf(output_string,
                   PROPERTY_LENGTH_STRUCTURE,
                   "%c,%.6d,%.6d,%.2s,%.6d,",
                   ft_format_char[OWQ_pn(owq_copy).selected_filetype->format],
                   (OWQ_pn(owq_copy).selected_filetype->ag) ? OWQ_pn(owq_copy).extension : 0,
                   (OWQ_pn(owq_copy).selected_filetype->ag) ? OWQ_pn(owq_copy).selected_filetype->ag->elements : 1,
                   (OWQ_pn(owq_copy).selected_filetype->read == NO_READ_FUNCTION) ?
                     ((OWQ_pn(owq_copy).selected_filetype->write == NO_WRITE_FUNCTION) ? "oo" : "wo") :
                     ((OWQ_pn(owq_copy).selected_filetype->write == NO_WRITE_FUNCTION) ? "ro" : "rw") ,
                   (int) FullFileLength(PN(owq_copy))
                  );
    UCLIBCUNLOCK;

    if ( output_length <0 ) return -EFAULT ;

    /* store value object in owq_copy --
       if structure of an array, the pointer could get overwritten and cause a seg fault
       on owq_destroy
    */
    memcpy( &OWQ_val(owq_copy), &OWQ_val(owq), sizeof ( union value_object ) ) ;
    
    print_status = Fowq_output_offset_and_size( output_string, output_length, owq ) ;
    
    /* restore */
    memcpy( &OWQ_val(owq), &OWQ_val(owq_copy), sizeof ( union value_object ) ) ;

    return print_status ;
}

/* read without artificial separation or combination */
static int FS_read_lump(struct one_wire_query * owq)
{
    if ( OWQ_Cache_Get(owq) ) { // non-zero means not found
        int read_error = (OWQ_pn(owq).selected_filetype->read)(owq) ;
        if ( read_error < 0 ) return read_error ;
        OWQ_Cache_Add(owq) ;
    }
    return 0 ;
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->selected_filetype->ag->combined==ag_separate */
static int FS_read_from_parts(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    OWQ_allocate_struct_and_pointer( owq_single ) ;

    size_t elements = pn->selected_filetype->ag->elements ;
    size_t extension ;

    STAT_ADD1(read_array);      /* statistics */

    /* shallow copy */
    OWQ_create_temporary( owq_single, OWQ_buffer(owq), FileLength(pn), 0, pn ) ;

    /* Loop through F_r_single, just to get data */
    for (extension = 0; extension < elements; ++extension) {
        OWQ_pn(owq_single).extension = extension ;
        if ( OWQ_Cache_Get(owq_single) ) { // non-zero if not there
            int single_or_error = (pn->selected_filetype->read)(owq_single) ;
            if ( single_or_error < 0 ) return single_or_error ;
            OWQ_Cache_Add(owq_single) ;
        }
        //printf("FS_read_from_parts extension=%d, length=%d, <%.*s> %p\n",(int)extension,(int)OWQ_length(owq_single),(int)OWQ_length(owq_single),OWQ_buffer(owq_single),OWQ_buffer(owq_single)) ;
        memcpy( &OWQ_array(owq)[extension], &OWQ_val(owq_single), sizeof(union value_object ) ) ;
        switch ( pn->selected_filetype->format ) {
            case ft_ascii:
            case ft_vascii:
            case ft_binary:
                OWQ_buffer(owq_single) += OWQ_length(owq_single) ;
                break ;
            default:
                break ;
        }
    }

    return 0 ;
}

/* bitfield -- read the UINT and convert to explicit array */
static int FS_read_all_bits(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	OWQ_allocate_struct_and_pointer( owq_single ) ;

	size_t elements = pn->selected_filetype->ag->elements ;
	size_t extension ;
	int lump_read ;
	UINT U ;

	STAT_ADD1(read_array);      /* statistics */

	/* shallow copy */
	OWQ_create_shallow_single( owq_single, owq ) ;

	/* read the UINT */
	lump_read = FS_read_lump(owq_single) ;
	if ( lump_read < 0 ) return lump_read ;
	U = OWQ_U(owq_single) ;

	/* Loop through F_r_single, just to get data */
	for (extension = 0; extension < elements; ++extension) {
		OWQ_array_Y(owq,extension) = UT_getbit( (void *)(&U), extension ) ;
	}

	return 0 ;
}

/* bitfield -- read the UINT and extract single value */
static int FS_read_one_bit(struct one_wire_query * owq)
{
	struct parsedname * pn = PN(owq) ;
	OWQ_allocate_struct_and_pointer( owq_single ) ;

	int lump_read ;
	UINT U ;

	STAT_ADD1(read_array);      /* statistics */

	/* shallow copy */
	OWQ_create_shallow_single( owq_single, owq ) ;

	/* read the UINT */
	lump_read = FS_read_lump(owq_single) ;
	if ( lump_read < 0 ) return lump_read ;
	U = OWQ_U(owq_single) ;

	OWQ_Y(owq) = UT_getbit( (void *) (&U), pn->extension ) ;

	return 0 ;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->selected_filetype->ag->combined==ag_aggregate */
static int FS_read_a_part(struct one_wire_query * owq)
{
	OWQ_allocate_struct_and_pointer( owq_all ) ;
    
	size_t extension = OWQ_pn(owq).extension ;
	int lump_read ;

	if ( OWQ_create_shallow_aggregate( owq_all, owq ) ) return -ENOMEM ;

	lump_read = FS_read_lump(owq_all) ;
	if ( lump_read < 0 ) {
		OWQ_destroy_shallow_aggregate(owq_all) ;
		return lump_read ;
	}

	switch (OWQ_pn(owq).selected_filetype->format) {
        case ft_vascii:
        case ft_ascii:
        case ft_binary:
	  {
		size_t prior_length = 0 ;
		size_t extension_index ;
		int return_status ;
		for ( extension_index = 0 ; extension_index < extension ; ++extension_index ) {
			prior_length += OWQ_array_length(owq_all,extension_index) ;
		}
		return_status = Fowq_output_offset_and_size( &OWQ_buffer(owq_all)[prior_length], OWQ_array_length(owq_all,extension), owq ) ;
		if ( return_status < 0 ) {
			OWQ_destroy_shallow_aggregate(owq_all) ;
			return return_status ;
		}
		OWQ_length(owq) = return_status ;
		break ;
	  }
        default:
		memcpy( &OWQ_val(owq), &OWQ_array(owq_all)[extension], sizeof(union value_object)) ;
		break ;
	}

	OWQ_destroy_shallow_aggregate(owq_all) ;
	return 0 ;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->selected_filetype->ag->combined==ag_aggregate */
static int FS_read_mixed_part(struct one_wire_query * owq)
{
	OWQ_allocate_struct_and_pointer( owq_all ) ;
    
	size_t extension = OWQ_pn(owq).extension ;

	if ( OWQ_create_shallow_aggregate( owq_all, owq ) ) return -ENOMEM ;
    
	if ( OWQ_Cache_Get(owq_all) ) { // no "all" cached
		OWQ_destroy_shallow_aggregate(owq_all) ;
		return FS_read_lump(owq) ; // read individual value */
	}

	switch (OWQ_pn(owq).selected_filetype->format) {
        case ft_vascii:
        case ft_ascii:
        case ft_binary:
        {
		size_t prior_length = 0 ;
		size_t extension_index ;
		int return_status ;
		for ( extension_index = 0 ; extension_index < extension ; ++extension_index ) {
			prior_length += OWQ_array_length(owq_all,extension_index) ;
		}
		return_status = Fowq_output_offset_and_size( &OWQ_buffer(owq_all)[prior_length], OWQ_array_length(owq_all,extension), owq ) ;
		if ( return_status < 0 ) {
			OWQ_destroy_shallow_aggregate(owq_all) ;
			return return_status ;
		}
		OWQ_length(owq) = return_status ;
		break ;
        }
        default:
		memcpy( &OWQ_val(owq), &OWQ_array(owq_all)[extension], sizeof(union value_object)) ;
		break ;
	}

	OWQ_Cache_Del(owq_all) ;
	OWQ_destroy_shallow_aggregate(owq_all) ;
	return 0 ;
}
