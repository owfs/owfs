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

static int FS_input_yesno(int *result, const char *buf, const size_t size);
static int FS_input_integer(int *result, const char *buf,
							const size_t size);
static int FS_input_unsigned(UINT * result, const char *buf,
							 const size_t size);
static int FS_input_float(_FLOAT * result, const char *buf,
						  const size_t size);
static int FS_input_date(_DATE * result, const char *buf,
						 const size_t size);

static int FS_input_yesno_array(int *results, const char *buf,
								const size_t size,
								const struct parsedname *pn);
static int FS_input_unsigned_array(UINT * results, const char *buf,
								   const size_t size,
								   const struct parsedname *pn);
static int FS_input_integer_array(int *results, const char *buf,
								  const size_t size,
								  const struct parsedname *pn);
static int FS_input_float_array(_FLOAT * results, const char *buf,
								const size_t size,
								const struct parsedname *pn);
static int FS_input_date_array(_DATE * results, const char *buf,
							   const size_t size,
							   const struct parsedname *pn);

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
    print_owq(owq) ;
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
        char * buf = OWQ_buffer(owq) ;
        size_t size = OWQ_size(owq) ;
        off_t offset = OWQ_offset(owq) ;
        write_or_error = ServerWrite(buf, size, offset, pn);
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
    int ret = -EBADMSG;
//printf("FS_w_single\n");


	switch (pn->ft->format) {
	case ft_integer:
        ret = (pn->ft->write.i) (&OWQ_I(owq), pn);
        if (ret == 0)
            Cache_Add(&OWQ_I(owq), sizeof(int), pn);
		break;
	case ft_bitfield:
	case ft_unsigned:
        ret = (pn->ft->write.u) (&OWQ_U(owq), pn);
        if (ret == 0)
            Cache_Add(&OWQ_U(owq), sizeof(UINT), pn);
        break;
	case ft_tempgap:
	case ft_float:
	case ft_temperature:
        ret = (pn->ft->write.f) (&OWQ_F(owq), pn);
        if (ret == 0)
            Cache_Add(&OWQ_F(owq), sizeof(_FLOAT), pn);
        break;
	case ft_date:
        ret = (pn->ft->write.d) (&OWQ_D(owq), pn);
        if (ret == 0)
            Cache_Add(&OWQ_D(owq), sizeof(_DATE), pn);
        break;
	case ft_yesno:
        ret = (pn->ft->write.y) (&OWQ_Y(owq), pn);
        if (ret == 0)
            Cache_Add(&OWQ_Y(owq), sizeof(int), pn);
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
            ret = (pn->ft->write.a) (OWQ_buffer(owq), write_length, OWQ_offset(owq), pn);
            if ( (ret >= 0) && (write_length == file_length) ) {
                Cache_Add(OWQ_buffer(owq), write_length, pn);
            } else {
                Cache_Del(pn);
            }
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
            ret = (pn->ft->write.b) ((BYTE *) OWQ_buffer(owq), write_length, OWQ_offset(owq), pn);
            if ( (ret >= 0) && (write_length == file_length) ) {
                Cache_Add(OWQ_buffer(owq), write_length, pn);
            } else {
                Cache_Del(pn);
            }
        }
		break;
	case ft_directory:
	case ft_subdir:
		ret = -ENOSYS;
		break;
	default:					/* Unknown data type */
		ret = -EINVAL;
		break;
	}

	//printf("FS_w_single: return %d\n", ret);
	return ret;
}

/* return 0 if ok */
/* write aggregate all */
/* Unlike FS_w_aggregate, no need to read in values first, since all will be replaced */
static int FS_w_aggregate_all(struct one_wire_query * owq)
{
    char * buf = OWQ_buffer(owq) ;
    size_t size = OWQ_size(owq) ;
    off_t offset = OWQ_offset(owq) ;
    struct parsedname * pn = &OWQ_pn(owq) ;
    size_t elements = pn->ft->ag->elements;
	size_t ffl = FullFileLength(pn);
	int ret;

	if (offset)
		return -EADDRNOTAVAIL;

	switch (pn->ft->format) {
	case ft_integer:
		{
			int *i = (int *) calloc(elements, sizeof(int));
			if (i == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = FS_input_integer_array(i, buf, size, pn)) == 0) {
					ret = (pn->ft->write.i) (i, pn);
					if (ret == 0)
						Cache_Add(&i, elements * sizeof(int), pn);
				}
				free(i);
			}
		}
		break;
	case ft_unsigned:
		{
			UINT *u = (UINT *) calloc(elements, sizeof(UINT));
			if (u == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = FS_input_unsigned_array(u, buf, size, pn)) == 0) {
					ret = (pn->ft->write.u) (u, pn);
					if (ret == 0)
						Cache_Add(&u, elements * sizeof(UINT), pn);
				}
				free(u);
			}
		}
		break;
	case ft_tempgap:
	case ft_float:
	case ft_temperature:
		{
			_FLOAT *f = (_FLOAT *) calloc(elements, sizeof(_FLOAT));
			if (f == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = FS_input_float_array(f, buf, size, pn)) == 0) {
					size_t i;
					switch (pn->ft->format) {
					case ft_temperature:
						for (i = 0; i < elements; ++i)
							f[i] = fromTemperature(f[i], pn);
						break;
					case ft_tempgap:
						for (i = 0; i < elements; ++i)
							f[i] = fromTempGap(f[i], pn);
						// trivial fall through
					default:
						break;
					}
					ret = (pn->ft->write.f) (f, pn);
					if (ret == 0)
						Cache_Add(&f, elements * sizeof(_FLOAT), pn);
				}
				free(f);
			}
		}
		break;
	case ft_date:
		{
			_DATE *d = (_DATE *) calloc(elements, sizeof(_DATE));
			if (d == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = FS_input_date_array(d, buf, size, pn)) == 0) {
					ret = (pn->ft->write.d) (d, pn);
					if (ret == 0)
						Cache_Add(&d, elements * sizeof(_DATE), pn);
				}
				free(d);
			}
		}
		break;
	case ft_yesno:
		{
			int *y = (int *) calloc(elements, sizeof(int));
			if (y == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = FS_input_yesno_array(y, buf, size, pn)) == 0) {
					ret = (pn->ft->write.y) (y, pn);
					if (ret == 0)
						Cache_Add(&y, elements * sizeof(int), pn);
				}
				free(y);
			}
		}
		break;
	case ft_bitfield:
		{
			int *y = (int *) calloc(elements, sizeof(int));
			if (y == NULL) {
				ret = -ENOMEM;
			} else {
				int i;
				UINT U = 0;
				if ((ret = FS_input_yesno_array(y, buf, size, pn)) == 0) {
					for (i = pn->ft->ag->elements - 1; i >= 0; --i)
						U = (U << 1) | (y[i] & 0x01);
					ret = (pn->ft->write.u) (&U, pn);
					if (ret == 0)
						Cache_Add(&U, sizeof(UINT), pn);
				}
				free(y);
			}
		}
		break;
	case ft_vascii:
	case ft_ascii:
		{
			size_t s = ffl;
			if (offset > (off_t) s) {
				ret = -ERANGE;
			} else {
				s -= offset;
				if (s > size)
					s = size;
				ret = (pn->ft->write.a) (buf, s, offset, pn);
				if (ret >= 0) {
					if (s == ffl) {
						Cache_Add(&buf, s, pn);
					} else {
						Cache_Del(pn);
					}
				}
			}
		}
		break;
	case ft_binary:
		{
			size_t s = ffl;
			if (offset > (off_t) s) {
				ret = -ERANGE;
			} else {
				s -= offset;
				if (s > size)
					s = size;
				ret =
					(pn->ft->write.b) ((const BYTE *) buf, s, offset, pn);
				if (ret >= 0) {
					if (s == ffl) {
						Cache_Add(&buf, s, pn);
					} else {
						Cache_Del(pn);
					}
				}
			}
		}
		break;
	case ft_directory:
	case ft_subdir:
		ret = -ENOSYS;
		break;
	default:					/* Unknown data type */
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* Non-combined input  field, so treat  as several separate transactions */
/* return 0 if ok */
static int FS_w_separate_all(struct one_wire_query * owq)
{
    int extension ;
    struct one_wire_query struct_owq_single ;
    struct one_wire_query * owq_single = &struct_owq_single ;

	STAT_ADD1(write_array);		/* statistics */
	
    memcpy(owq_single, owq, sizeof(struct one_wire_query));	/* shallow copy */
    
    for (extension = 0; extension < OWQ_pn(owq).ft->ag->elements; ++extension) {
        int write_or_error ;
        OWQ_pn(owq_single).extension = extension ;
        memcpy( &OWQ_val(owq_single), &OWQ_array(owq)[extension], sizeof(union value_object) ) ;
        write_or_error = FS_w_single(owq_single) ;
        if ( write_or_error < 0 ) return write_or_error ;
    }
	return 0;
}

/* Combined field, so read all, change the relevant field, and write back */
/* return 0 if ok */
static int FS_w_aggregate(struct one_wire_query * owq)
{
    char * buf = OWQ_buffer(owq) ;
    size_t size = OWQ_size(owq) ;
    off_t offset = OWQ_offset(owq) ;
    struct parsedname * pn = &OWQ_pn(owq) ;
    size_t elements = pn->ft->ag->elements;
	int ret = 0;

	const size_t ffl = FullFileLength(pn);
	struct parsedname pn_all;

	memcpy(&pn_all, pn, sizeof(struct parsedname));	//shallow copy
	pn_all.extension = EXTENSION_ALL;		// to save full string only

	/* readable at all? cannot write a part if whole can't be read */
	if (pn->ft->read.v == NULL)
		return -EFAULT;

//printf("WRITE_SPLIT\n");

	switch (pn->ft->format) {
	case ft_yesno:
		if (offset) {
			ret = -EADDRNOTAVAIL;
		} else {
			int *y = (int *) calloc(elements, sizeof(int));
			if (y == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = (pn->ft->read.y) (y, pn)) >= 0) {
					if (FS_input_yesno(&y[pn->extension], buf, size)) {
						ret = -EBADMSG;
					} else {
						if ((ret = (pn->ft->write.y) (y, pn)) >= 0)
							Cache_Add(&y, elements * sizeof(int), pn);
					}
				}
				free(y);
			}
		}
		break;
	case ft_integer:
		if (offset) {
			ret = -EADDRNOTAVAIL;
		} else {
			int *i = (int *) calloc(elements, sizeof(int));
			if (i == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = (pn->ft->read.i) (i, pn)) >= 0) {
					if (FS_input_integer(&i[pn->extension], buf, size)) {
						ret = -EBADMSG;
					} else {
						if ((ret = (pn->ft->write.i) (i, pn)) >= 0)
							Cache_Add(i, elements * sizeof(int), pn);
					}
				}
				free(i);
			}
		}
		break;
	case ft_unsigned:
		if (offset) {
			ret = -EADDRNOTAVAIL;
		} else {
			UINT *u = (UINT *) calloc(elements, sizeof(UINT));
			if (u == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = (pn->ft->read.u) (u, pn)) >= 0) {
					if (FS_input_unsigned(&u[pn->extension], buf, size)) {
						ret = -EBADMSG;
					} else {
						if ((ret = (pn->ft->write.u) (u, pn)) >= 0)
							Cache_Add(u, elements * sizeof(UINT), pn);
					}
				}
				free(u);
			}
		}
		break;
	case ft_bitfield:
		if (offset) {
			ret = -EADDRNOTAVAIL;
		} else {
			UINT U;
			UINT y;
			if ((ret = (pn->ft->read.u) (&U, pn)) >= 0) {
				if (FS_input_unsigned(&y, buf, size)) {
					ret = -EBADMSG;
				} else {
					UT_setbit((void *) (&U), pn->extension, y);
					if ((ret = (pn->ft->write.u) (&U, pn) >= 0))
						Cache_Add(&U, sizeof(UINT), pn);
				}
			}
		}
		break;
	case ft_temperature:
	case ft_tempgap:
	case ft_float:
		if (offset) {
			ret = -EADDRNOTAVAIL;
		} else {
			_FLOAT *f = (_FLOAT *) calloc(elements, sizeof(_FLOAT));
			_FLOAT F;
			if (f == NULL) {
				ret = -ENOMEM;
			} else {
				if ((ret = (pn->ft->read.f) (f, pn)) >= 0) {
					if (FS_input_float(&F, buf, size)) {
						ret = -EBADMSG;
					} else {
						switch (pn->ft->format) {
						case ft_temperature:
							f[pn->extension] = fromTemperature(F, pn);
							break;
						case ft_tempgap:
							f[pn->extension] = fromTempGap(F, pn);
							break;
						default:
							f[pn->extension] = F;
						}
						if ((ret = (pn->ft->write.f) (f, pn)) >= 0)
							Cache_Add(f, elements * sizeof(_FLOAT), pn);
					}
				}
				free(f);
			}
		}
		break;
	case ft_date:{
			if (offset) {
				ret = -EADDRNOTAVAIL;
			} else {
				_DATE *d = (_DATE *) calloc(elements, sizeof(_DATE));
				if (d == NULL) {
					ret = -ENOMEM;
				} else {
					if ((ret = (pn->ft->read.d) (d, pn)) >= 0) {
						if (FS_input_date(&d[pn->extension], buf, size)) {
							ret = -EBADMSG;
						} else {
							if ((ret = (pn->ft->write.d) (d, pn)) >= 0)
								Cache_Add(&d, elements * sizeof(_DATE),
										  pn);
						}
					}
					free(d);
				}
			}
			break;
	case ft_binary:{
				BYTE *all;
				int suglen = pn->ft->suglen;
				size_t s = suglen;
				if (offset > suglen) {
					ret = -ERANGE;
				} else {
					s -= offset;
					if (s > size)
						s = size;
					if ((all = (BYTE *) malloc(ffl))) {;
						if ((ret =
							 (pn->ft->read.b) (all, ffl, (const off_t) 0,
											   pn)) == 0) {
							memcpy(&all[suglen * pn->extension + offset],
								   buf, s);
							if ((ret =
								 (pn->ft->write.b) (all, ffl,
													(const off_t) 0,
													pn)) >= 0)
								Cache_Add(all, ffl, pn);
						}
						free(all);
					} else {
						ret = -ENOMEM;
					}
				}
			}
			break;
	case ft_vascii:
	case ft_ascii:
			if (offset) {
				return -EADDRNOTAVAIL;
			} else {
				char *all;
				int suglen = pn->ft->suglen;
				size_t s = suglen;
				if (s > size)
					s = size;
				if ((all = (char *) malloc(ffl))) {
					if ((ret =
						 (pn->ft->read.a) (all, ffl, (const off_t) 0,
										   pn)) == 0) {
						memcpy(&all[(suglen + 1) * pn->extension], buf, s);
						if ((ret =
							 (pn->ft->write.a) (all, ffl, (const off_t) 0,
												pn)) >= 0)
							Cache_Add(all, ffl, pn);
					}
					free(all);
				} else
					ret = -ENOMEM;
			}
		}
		break;
	case ft_directory:
	case ft_subdir:
		ret = -ENOSYS;
	}

	return ret ? -EINVAL : 0;
}

/* return 0 if ok */
static int FS_input_yesno(int *result, const char *buf, const size_t size)
{
//printf("yesno size=%d, buf=%s\n",size,buf);
	const char *b;
	size_t s;
	for (s = size, b = buf; s > 0; --s, ++b) {
		if (b[0] == ' ')
			continue;
		if (s > 2) {
			if (strncasecmp("yes", b, 3) == 0)
				goto yes;
			if (strncasecmp("off", b, 3) == 0)
				goto no;
		}
		if (s > 1) {
			if (strncasecmp("on", b, 2) == 0)
				goto yes;
			if (strncasecmp("no", b, 2) == 0)
				goto no;
		}
		if (s > 0) {
			if (b[0] == '1')
				goto yes;
			if (b[0] == '0')
				goto no;
		}
		break;
	}
	return 1;
  yes:result[0] = 1;
	return 0;
  no:result[0] = 0;
	return 0;

}

/* return 0 if ok */
static int FS_input_integer(int *result, const char *buf,
							const size_t size)
{
	char cp[size + 1];
	char *end;

	memcpy(cp, buf, size);
	cp[size] = '\0';
	errno = 0;
	*result = strtol(cp, &end, 10);
	return end == cp || errno;
}

/* return 0 if ok */
static int FS_input_unsigned(UINT * result, const char *buf,
							 const size_t size)
{
	char cp[size + 1];
	char *end;

	memcpy(cp, buf, size);
	cp[size] = '\0';
	errno = 0;
	*result = strtoul(cp, &end, 10);
//printf("UI str=%s, val=%u\n",cp,*result) ;
	return end == cp || errno;
}

/* return 0 if ok */
static int FS_input_float(_FLOAT * result, const char *buf,
						  const size_t size)
{
	char cp[size + 1];
	char *end;

	memcpy(cp, buf, size);
	cp[size] = '\0';
	errno = 0;
	*result = strtod(cp, &end);
	return end == cp || errno;
}

/* return 0 if ok */
static int FS_input_date(_DATE * result, const char *buf,
						 const size_t size)
{
	struct tm tm;
	if (size < 2 || buf[0] == '\0' || buf[0] == '\n') {
		*result = time(NULL);
	} else if (strptime(buf, "%a %b %d %T %Y", &tm) == NULL
			   && strptime(buf, "%b %d %T %Y", &tm) == NULL
			   && strptime(buf, "%c", &tm) == NULL
			   && strptime(buf, "%D %T", &tm) == NULL) {
		return -EINVAL;
	} else {
		*result = mktime(&tm);
	}
	return 0;
}

/* returns 0 if ok */
static int FS_input_yesno_array(int *results, const char *buf,
								const size_t size,
								const struct parsedname *pn)
{
	int i;
	int last = pn->ft->ag->elements - 1;
	const char *first;
	const char *end = buf + size - 1;
	const char *next = buf;
	for (i = 0; i <= last; ++i) {
		if (next <= end) {
			first = next;
			if ((next =
				 memchr(first, ',', (size_t) (first - end + 1))) == NULL)
				next = end;
			if (FS_input_yesno
				(&results[i], first, (const size_t) (next - first)))
				results[i] = 0;
			++next;				/* past comma */
		} else {				/* assume "no" for absent values */
			results[i] = 0;
		}
	}
	return 0;
}

/* returns number of valid integers, or negative for error */
static int FS_input_integer_array(int *results, const char *buf,
								  const size_t size,
								  const struct parsedname *pn)
{
	int i;
	int last = pn->ft->ag->elements - 1;
	const char *first;
	const char *end = buf + size - 1;
	const char *next = buf;
	for (i = 0; i <= last; ++i) {
		if (next <= end) {
			first = next;
			if ((next =
				 memchr(first, ',', (size_t) (first - end + 1))) == NULL)
				next = end;
			if (FS_input_integer
				(&results[i], first, (const size_t) (next - first)))
				results[i] = 0;
			++next;				/* past comma */
		} else {				/* assume 0 for absent values */
			results[i] = 0;
		}
	}
	return 0;
}

/* returns 0, or negative for error */
static int FS_input_unsigned_array(UINT * results, const char *buf,
								   const size_t size,
								   const struct parsedname *pn)
{
	int i;
	int last = pn->ft->ag->elements - 1;
	const char *first;
	const char *end = buf + size - 1;
	const char *next = buf;
	for (i = 0; i <= last; ++i) {
		if (next <= end) {
			first = next;
			if ((next =
				 memchr(first, ',', (size_t) (first - end + 1))) == NULL)
				next = end;
			if (FS_input_unsigned
				(&results[i], first, (const size_t) (next - first)))
				results[i] = 0;
			++next;				/* past comma */
		} else {				/* assume 0 for absent values */
			results[i] = 0;
		}
	}
	return 0;
}

/* returns 0, or negative for error */
static int FS_input_float_array(_FLOAT * results, const char *buf,
								const size_t size,
								const struct parsedname *pn)
{
	int i;
	int last = pn->ft->ag->elements - 1;
	const char *first;
	const char *end = buf + size - 1;
	const char *next = buf;
	for (i = 0; i <= last; ++i) {
		if (next <= end) {
			first = next;
			if ((next =
				 memchr(first, ',', (size_t) (first - end + 1))) == NULL)
				next = end;
			if (FS_input_float
				(&results[i], first, (const size_t) (next - first)))
				results[i] = 0.;
			++next;				/* past comma */
		} else {				/* assume 0. for absent values */
			results[i] = 0.;
		}
	}
	return 0;
}

/* returns 0, or negative for error */
static int FS_input_date_array(_DATE * results, const char *buf,
							   const size_t size,
							   const struct parsedname *pn)
{
	int i;
	int last = pn->ft->ag->elements - 1;
	const char *first;
	const char *end = buf + size - 1;
	const char *next = buf;
	_DATE now = time(NULL);
	for (i = 0; i <= last; ++i) {
		if (next <= end) {
			first = next;
			if ((next =
				 memchr(first, ',', (size_t) (first - end + 1))) == NULL)
				next = end;
			if (FS_input_date
				(&results[i], first, (const size_t) (next - first)))
				results[i] = now;
			++next;				/* past comma */
		} else {				/* assume now for absent values */
			results[i] = now;
		}
	}
	return 0;
}
