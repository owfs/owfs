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

/* ------- Prototypes ----------- */
static int FS_w_given_bus(struct one_wire_query * owq);
static int FS_w_local(struct one_wire_query * owq);
static int FS_w_simultaneous(struct one_wire_query * owq);
static int FS_write_single_lump(struct one_wire_query * owq) ;
static int FS_write_aggregate_lump(struct one_wire_query * owq) ;
static int FS_write_all_bits(struct one_wire_query * owq) ;
static int FS_write_a_bit(struct one_wire_query * owq) ;
static int FS_write_in_parts(struct one_wire_query * owq) ;
static int FS_write_a_part(struct one_wire_query * owq) ;
static int FS_write_mixed_part(struct one_wire_query * owq) ;

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
int FS_write(const char *path, const char *buf, const size_t size,
			 const off_t offset)
{
	OWQ_make( owq ) ;
	int write_return;

	LEVEL_CALL("WRITE path=%s size=%d offset=%d\n", SAFESTRING(path),
			   (int) size, (int) offset)

		/* if readonly exit */
		if (Global.readonly)
		return -EROFS;

	// parsable path?
	if (FS_OWQ_create(path, buf, size, offset, owq))
		return -ENOENT;

    write_return = FS_write_postparse(owq);
	FS_OWQ_destroy(owq);
    return write_return;					/* here's where the size is used! */
}

/* return size if ok, else negative */
int FS_write_postparse(struct one_wire_query * owq)
{
    ssize_t input_or_error;
    ssize_t write_or_error;
    struct parsedname * pn = PN(owq) ;

	if (Global.readonly)
		return -EROFS;			// read-only invokation
    if (IsDir(pn))
		return -EISDIR;			// not a file
	if (pn->in == NULL)
		return -ENODEV;			// no busses

	STATLOCK;
	AVERAGE_IN(&write_avg)
		AVERAGE_IN(&all_avg)
		++ write_calls;			/* statistics */
	STATUNLOCK;
    
    input_or_error = FS_input_owq(owq) ;
    Debug_OWQ(owq) ;
    if ( input_or_error < 0 ) return input_or_error ;

	switch (pn->type) {
	case pn_structure:
	case pn_statistics:
	case pn_system:
        write_or_error = -ENOTSUP;
		break;
	case pn_settings:
        write_or_error = FS_w_given_bus(owq);
		break;
	default:					// pn_real
//printf("FS_write_postparse: pid=%ld call FS_w_given_bus size=%ld\n", pthread_self(), size);

		/* handle DeviceSimultaneous */
		if (pn->dev == DeviceSimultaneous) {
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
            if (write_or_error < 0) { // second look -- initial write gave an error
				STAT_ADD1(write_tries[1]);
				if (Global.opt == opt_server) {	// called from owserver
					Cache_Del_Device(pn);
				} else if (SpecifiedBus(pn)) {
					write_or_error = TestConnection(pn) ? -ECONNABORTED : FS_w_given_bus(owq);
                    if ( write_or_error < 0 ) { // third try
                        STAT_ADD1(write_tries[2]);
                        write_or_error = TestConnection(pn) ? -ECONNABORTED : FS_w_given_bus(owq);
                    }
                } else {
                    int busloc_or_error;
                    UnsetKnownBus(pn) ; // eliminate cached location
                    busloc_or_error = CheckPresence(pn) ;
                    if ( busloc_or_error < 0 ) {
                        write_or_error = -ENOENT ;
                    } else {
                        write_or_error = FS_w_given_bus(owq);
                        if ( write_or_error < 0 ) { // third try
                            STAT_ADD1(write_tries[2]);
                            write_or_error = TestConnection(pn) ? -ECONNABORTED : FS_w_given_bus(owq);
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
        write_or_error = OWQ_size(owq);				/* here's where the size is used! */
	}
	AVERAGE_OUT(&write_avg)
		AVERAGE_OUT(&all_avg)
		STATUNLOCK;

    return write_or_error;
}

/* This function is only used by "Simultaneous" */
/* It certainly could use pthreads, but might be overkill */
static int FS_w_simultaneous(struct one_wire_query * owq)
{
    if (SpecifiedBus(PN(owq))) {
		return FS_w_given_bus(owq);
	} else {
		OWQ_make( owq_given ) ;
		int bus_number;

		memcpy(owq_given, owq, sizeof(struct one_wire_query));	// shallow copy
		for (bus_number = 0; bus_number < indevices; ++bus_number) {
            SetKnownBus(bus_number, PN(owq_given));
			FS_w_given_bus(owq_given);
		}
		return 0;
	}
}

/* return 0 if ok, else negative */
static int FS_w_given_bus(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    ssize_t write_or_error;

	if (TestConnection(pn)) {
		write_or_error = -ECONNABORTED;
	} else if (KnownBus(pn) && BusIsServer(pn->in)) {
        write_or_error = ServerWrite(owq);
    } else {
        write_or_error = LockGet(pn) ;
        if (write_or_error == 0) {
            write_or_error = FS_w_local(owq);
            LockRelease(pn);
        }
	}
	return write_or_error;
}

/* return 0 if ok */
static int FS_w_local(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
	//printf("FS_w_local\n");

	/* Writable? */
	if ((pn->ft->write.v) == NULL)
		return -ENOTSUP;

	/* Special case for "fake" adapter */
    if (pn->in->Adapter == adapter_fake && IsRealDir(pn) )
		return 0;

	/* Array properties? Write all together if aggregate */
	if (pn->ft->ag) {
        switch (pn->extension) {
            case EXTENSION_BYTE:
                return FS_write_single_lump(owq) ;
            case EXTENSION_ALL:
                if ( pn->ft->format == ft_bitfield ) return FS_write_all_bits(owq) ;
                switch (pn->ft->ag->combined) {
                    case ag_aggregate:
                    case ag_mixed:
                        return FS_write_aggregate_lump(owq) ;
                    case ag_separate:
                        return FS_write_in_parts(owq) ;
                }
            default:
                if ( pn->ft->format == ft_bitfield ) return FS_write_a_bit(owq) ;
                switch (pn->ft->ag->combined) {
                    case ag_aggregate:
                        return FS_write_a_part(owq) ;
                    case ag_mixed:
                        return FS_write_mixed_part(owq) ;
                    case ag_separate:
                        return FS_write_single_lump(owq) ;
                }
        }
    }

    return FS_write_single_lump(owq) ;
}

static int FS_write_single_lump(struct one_wire_query * owq)
{
	OWQ_make( owq_copy ) ;

    int write_error ;

    OWQ_create_shallow_single( owq_copy, owq ) ;

    write_error = (OWQ_pn(owq).ft->write.o)(owq_copy) ;

    if ( write_error < 0 ) return write_error ;

    OWQ_Cache_Add(owq) ;

    return 0 ;
}

static int FS_write_a_part(struct one_wire_query * owq)
{
	OWQ_make( owq_all ) ;
    struct parsedname * pn = PN(owq) ;

    size_t extension = pn->extension ;

    int write_error ;

    if ( OWQ_create_shallow_aggregate( owq_all, owq ) < 0 ) return -ENOMEM ;

    if ( OWQ_Cache_Get(owq_all) ) {
        write_error = (pn->ft->write.o)(owq_all) ;
        if ( write_error < 0 ) {
            OWQ_destroy_shallow_aggregate(owq_all) ;
            return write_error ;
        }
    }

    switch( pn->ft->format ) {
        case ft_binary:
        case ft_ascii:
        case ft_vascii:
        {
            size_t extension_index ;
            size_t elements = pn->ft->ag->elements;
            char * buffer_pointer = OWQ_buffer(owq_all) ;
            char * entry_pointer ;
            char * target_pointer ;
            
            for ( extension_index = 0 ; extension_index < extension ; ++extension ) {
                buffer_pointer += OWQ_array_length(owq_all,extension_index) ;
            }

            entry_pointer = buffer_pointer ;

            target_pointer = buffer_pointer + OWQ_length(owq) ;
            buffer_pointer = buffer_pointer + OWQ_array_length(owq_all,extension) ;

            for ( extension_index = extension+1 ; extension_index < elements ; ++extension ) {
                size_t this_length = OWQ_array_length(owq_all,extension_index) ;
                memmove( target_pointer, buffer_pointer, this_length ) ;
                buffer_pointer += this_length ;
                buffer_pointer += this_length ;
            }
            
            memmove( entry_pointer, OWQ_buffer(owq), OWQ_length(owq) ) ;
            break ;
        }
        default:
            break ;
    }
    memcpy( &OWQ_array(owq_all)[pn->extension], &OWQ_val(owq), sizeof( union value_object ) ) ;

    write_error = FS_write_aggregate_lump(owq_all) ;

    OWQ_destroy_shallow_aggregate(owq_all) ;

    return write_error ;
}

static int FS_write_aggregate_lump(struct one_wire_query * owq)
{
	OWQ_make( owq_copy ) ;

    int write_error ;

    OWQ_create_shallow_aggregate( owq_copy, owq ) ;

    write_error = (OWQ_pn(owq).ft->write.o)(owq_copy) ;

    OWQ_destroy_shallow_aggregate(owq_copy) ;

    if ( write_error < 0 ) return write_error ;

    OWQ_Cache_Add(owq) ;

    return 0 ;
}

static int FS_write_mixed_part(struct one_wire_query * owq)
{
	OWQ_make( owq_copy ) ;

    int write_error = FS_write_single_lump(owq);

    OWQ_create_shallow_aggregate( owq_copy, owq ) ;

    OWQ_pn(owq_copy).extension = EXTENSION_ALL ;
    OWQ_Cache_Del(owq_copy) ;

    return write_error ;
}

static int FS_write_in_parts(struct one_wire_query * owq)
{
	OWQ_make( owq_single ) ;
	struct parsedname * pn = PN(owq) ;
    size_t elements = pn->ft->ag->elements;

    char * buffer_pointer ;
    size_t extension ;

    OWQ_create_shallow_single( owq_single, owq ) ;

    switch (pn->ft->format ) {
        case ft_ascii:
        case ft_vascii:
        case ft_binary:
            buffer_pointer = OWQ_buffer(owq) ;
            break ;
        default:
            buffer_pointer = NULL ;
            break ;
    }
    
    for ( extension = 0 ; extension < elements ; ++extension ) {
        int single_write ;
        memcpy( &OWQ_val(owq_single), &OWQ_array(owq)[extension], sizeof( union value_object ) ) ;
        OWQ_pn(owq_single).extension = extension ;
        OWQ_size(owq_single) = FileLength(PN(owq_single)) ;
        OWQ_offset(owq_single) = 0 ;
        if ( buffer_pointer ) OWQ_buffer(owq_single) = buffer_pointer ;

        single_write = FS_write_single_lump(owq_single) ;

        if ( single_write < 0 ) return single_write ;
        
        if ( buffer_pointer ) buffer_pointer += OWQ_array_length(owq,extension) ;
    }

    return 0 ;
}

static int FS_write_all_bits(struct one_wire_query * owq)
{
	OWQ_make( owq_single ) ;
    struct parsedname * pn = PN(owq) ;
    size_t elements = pn->ft->ag->elements;

    size_t extension ;
    UINT U = 0 ;

    OWQ_create_shallow_single( owq_single, owq ) ;

    for ( extension = 0 ; extension < elements ; ++extension ) {
        UT_setbit( (void *) (&U), extension, OWQ_array_Y(owq,extension) ) ;
    }

    OWQ_U(owq_single) = U ;
    OWQ_pn(owq_single).extension = EXTENSION_BYTE ;
    
    return FS_write_single_lump(owq_single) ;
}

static int FS_write_a_bit(struct one_wire_query * owq)
{
	OWQ_make( owq_single ) ;
    struct parsedname * pn = PN(owq) ;

    OWQ_create_shallow_single( owq_single, owq ) ;

    OWQ_pn(owq_single).extension = EXTENSION_BYTE ;
    
    if ( OWQ_Cache_Get(owq_single) ) {
        int read_error = (pn->ft->read.o)(owq_single) ;
        if ( read_error < 0 ) return read_error ;
    }

    UT_setbit( (void *) (&OWQ_U(owq_single)), pn->extension, OWQ_Y(owq) ) ;

    return FS_write_single_lump(owq_single) ;
}
