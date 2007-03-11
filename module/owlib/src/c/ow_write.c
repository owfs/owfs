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
static int FS_w_aggregate_all(struct one_wire_query * owq);
static int FS_w_separate_all(struct one_wire_query * owq);
static int FS_w_aggregate(struct one_wire_query * owq);
static int FS_w_single(struct one_wire_query * owq);

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
	struct one_wire_query struct_owq;
    struct one_wire_query * owq = &struct_owq ;
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
    struct parsedname * pn = &OWQ_pn(owq) ;

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
    if (SpecifiedBus(&OWQ_pn(owq))) {
		return FS_w_given_bus(owq);
	} else {
        struct one_wire_query struct_owq_given ;
        struct one_wire_query * owq_given = &struct_owq_given ;
		int bus_number;

		memcpy(owq_given, owq, sizeof(struct one_wire_query));	// shallow copy
		for (bus_number = 0; bus_number < indevices; ++bus_number) {
            SetKnownBus(bus_number, &OWQ_pn(owq_given));
			FS_w_given_bus(owq_given);
		}
		return 0;
	}
}

/* return 0 if ok, else negative */
static int FS_w_given_bus(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
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
	int write_or_error;
    struct parsedname * pn = &OWQ_pn(owq) ;
	//printf("FS_w_local\n");

	/* Writable? */
	if ((pn->ft->write.v) == NULL)
		return -ENOTSUP;

	/* Special case for "fake" adapter */
	if (pn->in->Adapter == adapter_fake)
		return 0;

	/* Array properties? Write all together if aggregate */
	if (pn->ft->ag) {
		switch (pn->ft->ag->combined) {
		case ag_aggregate:
			/* agregate property -- need to read all and replace a single value, then write all */
			if (pn->extension > EXTENSION_ALL)
				return FS_w_aggregate(owq);
			/* fallthrough for extension==-1 or -2 */
		case ag_mixed:
			if (pn->extension == EXTENSION_ALL)
				return FS_w_aggregate_all(owq);
			/* Does the right thing, aggregate write for ALL and individual for splits */
			break;				/* continue for bitfield */
		case ag_separate:
			/* write all of them, but one at a time */
			if (pn->extension == EXTENSION_ALL)
				return FS_w_separate_all(owq);
			break;				/* fall through for individual writes */
		}
	}

	/* write individual entry */
	write_or_error = FS_w_single(owq);
	if (write_or_error < 0)
        LEVEL_DATA("Write error on %s (size=%d)\n", pn->path, (int) OWQ_size(owq));
	return write_or_error;
}

/* return 0 if ok */
/* write a single element */
/* either no array, or a separate-type array */
static int FS_w_single(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    int write_status = -EBADMSG;
//printf("FS_w_single\n");


	switch (pn->ft->format) {
	case ft_integer:
        write_status = (pn->ft->write.i) (&OWQ_I(owq), pn);
		break;
	case ft_bitfield:
	case ft_unsigned:
        write_status = (pn->ft->write.u) (&OWQ_U(owq), pn);
        break;
	case ft_tempgap:
	case ft_float:
	case ft_temperature:
        write_status = (pn->ft->write.f) (&OWQ_F(owq), pn);
        break;
	case ft_date:
        write_status = (pn->ft->write.d) (&OWQ_D(owq), pn);
        break;
	case ft_yesno:
        write_status = (pn->ft->write.y) (&OWQ_Y(owq), pn);
        break;
	case ft_vascii:
	case ft_ascii:
		{
            ssize_t file_length = OWQ_FileLength(owq) ;
            ssize_t write_length = OWQ_size(owq) ;
            if ( OWQ_offset(owq) > file_length ) return -ERANGE ;
            if ( write_length + OWQ_offset(owq) > file_length ) {
                write_length = file_length - OWQ_offset(owq) ;
            }
            write_status = (pn->ft->write.a) (OWQ_buffer(owq), write_length, OWQ_offset(owq), pn);
            OWQ_mem(owq) = OWQ_buffer(owq) ;
            OWQ_length(owq) = write_status ;
		}
		break;
	case ft_binary:
		{
            ssize_t file_length = OWQ_FileLength(owq) ;
            ssize_t write_length = OWQ_size(owq) ;
            if ( OWQ_offset(owq) > file_length ) return -ERANGE ;
            if ( write_length + OWQ_offset(owq) > file_length ) {
                write_length = file_length - OWQ_offset(owq) ;
            }
            write_status = (pn->ft->write.b) ((BYTE *) OWQ_buffer(owq), write_length, OWQ_offset(owq), pn);
            OWQ_mem(owq) = OWQ_buffer(owq) ;
            OWQ_length(owq) = write_status ;
        }
		break;
	case ft_directory:
	case ft_subdir:
        write_status = -ENOSYS;
		break;
	default:					/* Unknown data type */
        write_status = -EINVAL;
		break;
	}

    if ( write_status < 0 || OWQ_Cache_Add(owq) ) {
        OWQ_Cache_Del(owq) ;
    }
    return write_status;
}

/* return 0 if ok */
/* write aggregate all */
/* Unlike FS_w_aggregate, no need to read in values first, since all will be replaced */
static int FS_w_aggregate_all(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    size_t elements = pn->ft->ag->elements;
	int write_status ;

    if (OWQ_offset(owq))
		return -EADDRNOTAVAIL;

	switch (pn->ft->format) {
        case ft_integer:
        {
            int *i = calloc(elements, sizeof(int));
            size_t extension ;
            if (i == NULL) return-ENOMEM;
            for ( extension = 0 ; extension < elements ; ++extension ) i[extension] = OWQ_array_I(owq,extension) ;
            free(i);
            write_status = (pn->ft->write.i) (i, pn) ;
            break;
        }
        case ft_unsigned:
        {
            UINT *u = calloc(elements, sizeof(UINT));
            size_t extension ;
            if (u == NULL) return-ENOMEM;
            for ( extension = 0 ; extension < elements ; ++extension ) u[extension] = OWQ_array_U(owq,extension) ;
            free(u);
            write_status = (pn->ft->write.u) (u, pn) ;
            break;
        }
        case ft_tempgap:
        case ft_float:
        case ft_temperature:
        {
            _FLOAT *f = calloc(elements, sizeof(_FLOAT));
            size_t extension ;
            if (f == NULL) return-ENOMEM;
            for ( extension = 0 ; extension < elements ; ++extension ) f[extension] = OWQ_array_F(owq,extension) ;
            free(f);
            write_status = (pn->ft->write.f) (f, pn) ;
            break;
        }
        case ft_date:
        {
            _DATE *d = calloc(elements, sizeof(_DATE));
            size_t extension ;
            if (d == NULL) return-ENOMEM;
            for ( extension = 0 ; extension < elements ; ++extension ) d[extension] = OWQ_array_D(owq,extension) ;
            free(d);
            write_status = (pn->ft->write.d) (d, pn) ;
            break;
        }
        case ft_bitfield:
        case ft_yesno:
        {
            int *y = calloc(elements, sizeof(int));
            size_t extension ;
            if (y == NULL) return-ENOMEM;
            for ( extension = 0 ; extension < elements ; ++extension ) y[extension] = OWQ_array_Y(owq,extension) ;
            free(y);
            write_status = (pn->ft->write.y) (y, pn) ;
            break;
        }
        case ft_vascii:
        case ft_ascii:
            write_status = (pn->ft->write.a) (OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), pn) ;
            break;
        case ft_binary:
            write_status = (pn->ft->write.b) ((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), pn) ;
            break;
        case ft_directory:
        case ft_subdir:
            return -ENOSYS;
            break;
        default:					/* Unknown data type */
            return -EINVAL;
            break;
	}
    
    if ( write_status < 0 || OWQ_Cache_Add(owq) ) {
        OWQ_Cache_Del(owq) ;
    }

	return write_status;
}

/* Non-combined input  field, so treat  as several separate transactions */
/* return 0 if ok */
static int FS_w_separate_all(struct one_wire_query * owq)
{
    int extension ;
    struct one_wire_query struct_owq_single ;
    struct one_wire_query * owq_single = &struct_owq_single ;

	STAT_ADD1(write_array);		/* statistics */
	
    OWQ_create_shallow_single( owq_single, owq) ;
    
    for (extension = 0; extension < OWQ_pn(owq).ft->ag->elements; ++extension) {
        int write_or_error ;
        OWQ_pn(owq_single).extension = extension ;
        memcpy( &OWQ_val(owq_single), &OWQ_array(owq)[extension], sizeof(union value_object) ) ;
        write_or_error = FS_w_single(owq_single) ;
        if ( write_or_error < 0 ) {
            OWQ_destroy_shallow_single( owq_single ) ;
            return write_or_error ;
        }
    }
    OWQ_destroy_shallow_single( owq_single ) ;
    return 0;
}

/* Combined field, so read all, change the relevant field, and write back */
/* return 0 if ok */
static int FS_w_aggregate(struct one_wire_query * owq)
{
    struct one_wire_query struct_owq_all ;
    struct one_wire_query * owq_all = &struct_owq_all ;
    
    int return_status = 0;
    size_t extension = OWQ_pn(owq).extension ;

    if ( OWQ_create_shallow_aggregate( owq_all, owq ) ) return -ENOMEM ;

    return_status = FS_r_aggregate_all(owq_all) ;

    if ( return_status == 0 ) {
        switch (OWQ_pn(owq).ft->format) {
            case ft_integer:
            case ft_unsigned:
            case ft_float:
            case ft_temperature:
            case ft_tempgap:
            case ft_date:
            case ft_yesno:
            case ft_bitfield:
                memcpy( &OWQ_array(owq_all)[extension], &OWQ_val(owq),  sizeof(union value_object)) ;
                break ;
            case ft_vascii:
            case ft_ascii:
            case ft_binary:
            {
                ssize_t copy_length = OWQ_FileLength(owq) - OWQ_offset(owq) ;
                if ( copy_length < 0 ) {
                    return_status = -ERANGE ;
                } else {
                    if ( copy_length > (ssize_t) OWQ_size(owq) ) copy_length = OWQ_size(owq) ;
                    memcpy( &(OWQ_array_mem(owq_all,extension)[OWQ_offset(owq)]), OWQ_buffer(owq), copy_length ) ;
                }
            }
            case ft_directory:
            case ft_subdir:
                return_status = -ENOSYS;
            default:
                return_status = -ENOENT;
        }
        return_status = FS_w_aggregate_all(owq_all) ;
        if ( return_status == 0 ) {
            OWQ_Cache_Add(owq_all) ;
        }else{
            OWQ_Cache_Del(owq_all) ;
        }
    }

    OWQ_destroy_shallow_aggregate(owq_all) ;
    
    return return_status ;
}
