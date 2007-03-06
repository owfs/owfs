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
static int FS_r_separate_all(struct one_wire_query * owq);
static int FS_r_aggregate(struct one_wire_query * owq);
static int FS_r_single(struct one_wire_query * owq);
static int FS_r_aggregate_all(struct one_wire_query * owq);
static int FS_structure(struct one_wire_query * owq) ;

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
	//struct parsedname pn;
    struct one_wire_query owq ;
    
	int r;

	LEVEL_CALL("READ path=%s size=%d offset=%d\n", SAFESTRING(path),
        (int) size, (int) offset) ;
    // Parseable path?
    //if (FS_ParsedName(path, &pn))
    if (FS_OWQ_create(path,buf,size,offset,&owq))
        return -ENOENT;

	//printf("FS_read: KnownBus=%c pn->bus_nr=%d\n", KnownBus(&pn)?'Y':'N', pn.bus_nr);
	//printf("FS_read: pn->path=%s pn->path_busless=%s\n", pn.path, pn.path_busless);
	//printf("FS_read: pid=%ld call postparse size=%ld pn->type=%d\n", pthread_self(), size, pn.type);
    //r = FS_read_postparse(buf, size, offset, &pn);
#if 0
    Debug_Bytes("Real buffer returned",buf,r) ;
    owq_reply = FS_input_owq(&owq) ;
    printf("OWQ INPUT PARSE = %d\n",owq_reply) ;
    owq_reply = FS_output_owq(&owq) ;
    printf("OWQ OUTPUT PARSE = %d\n",owq_reply) ;
    print_owq(&owq) ;
#endif
    r = FS_read_postparse(&owq);
    //FS_ParsedName_destroy(&pn);
    FS_OWQ_destroy(&owq);
	return r;
}

/* After parsing, but before sending to various devices. Will repeat 3 times if needed */
int FS_read_postparse(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    ssize_t read_or_error ;

	// ServerRead jumps in here, perhaps with non-file entry
	if (pn->dev == NULL || pn->ft == NULL)
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
    read_or_error = FS_read_postpostparse(owq);

	/* Second Try */
	/* if not a specified bus, relook for chip location */
    if (read_or_error < 0) { //error
        STAT_ADD1(read_tries[1]);
		if (Global.opt == opt_server) {	// called from owserver
            // Only one try (repeated remotely)
			Cache_Del_Device(pn);
        } else if (SpecifiedBus(pn)) { // this bus or bust!
			read_or_error = TestConnection(pn) ? -ECONNABORTED :
				FS_read_postpostparse(owq);
            if ( read_or_error < 0 ) { // third try
                STAT_ADD1(read_tries[2]);
                read_or_error = FS_read_postpostparse(owq);
            }
        } else {
            int busloc_or_error ;
            UnsetKnownBus(pn) ; // eliminate cached presence
            busloc_or_error = CheckPresence(pn) ; // search again
            if ( busloc_or_error < 0 ) {
                read_or_error = -ENOENT ;
            } else {
                read_or_error = FS_read_postpostparse(owq);
                if ( read_or_error < 0 ) { // third try
                    STAT_ADD1(read_tries[2]);
                    read_or_error = FS_read_postpostparse(owq);

                }
            }
		}
    }

    STATLOCK;
    if (read_or_error >= 0) {
		++read_success;			/* statistics */
        read_bytes += read_or_error;		/* statistics */
    }
	AVERAGE_OUT(&read_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;
    LEVEL_DEBUG("READ_POSTPARSE %s return %d\n", pn->path, read_or_error);
    return read_or_error;
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
int FS_read_postpostparse(struct one_wire_query * owq)
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
		//printf("FS_read_postpostparse: pid=%ld call fs_structure\n", pthread_self());
        r = FS_structure(owq);
		break;
	default:
        r = FS_r_given_bus(owq);
		break;
	}
	STATLOCK;
	if (r >= 0) {
		++read_success;			/* statistics */
		read_bytes += r;		/* statistics */
	}
	AVERAGE_OUT(&read_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;

    LEVEL_DEBUG("READ_POSTPOSTPARSE: %s return %d\n", OWQ_pn(owq).path, r);
//printf("FS_read_postpostparse: pid=%ld return %d\n", pthread_self(), r);
	return r;
}

static int FS_r_given_bus(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    int r = 0;
	//printf("FS_r_given_bus\n");
	LEVEL_DEBUG("FS_r_given_bus\n");
    print_owq(owq) ;

	if (!KnownBus(pn)) {
		LEVEL_DEBUG("FS_r_given_bus: ERROR bus is not set!\n");
	}
    if (KnownBus(pn) && BusIsServer(pn->in)) {
        /* The bus is not local... use a network connection instead */
#if OW_MT
		LEVEL_DEBUG("FS_r_given_bus pid=%ld call ServerRead\n",
					pthread_self());
#endif /* OW_MT */
		//printf("FS_r_given_bus pid=%ld call ServerRead\n", pthread_self());
		r = ServerRead(OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), pn);
		//printf("FS_r_given_bus pid=%ld r=%d\n",pthread_self(), r);
	} else {
        STAT_ADD1(read_calls);	/* statistics */
		if ((r = LockGet(pn)) == 0) {
            r = FS_r_local(owq);
			//printf("FS_r_given_bus FS_r_local ret=%d\n", r);
			LockRelease(pn);
		}
	}
    return r;
}

/* Real read -- called from read
   Integrates with cache -- read not called if cached value already set
*/
static int FS_r_local(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
	
    /* Readable? */
	if ((pn->ft->read.v) == NULL)
		return -ENOTSUP;

    /* Special case for "fake" adapter */
    if (pn->in->Adapter == adapter_fake && pn->ft->change != fc_static)
        return FS_read_fake(owq);

    /* Special case for "tester" adapter */
    if (pn->in->Adapter == adapter_tester && pn->ft->change != fc_static)
        return FS_read_tester(owq);

	/* Array property? Read separately? Read together and manually separate? */
	if (pn->ft->ag) {			/* array property */
		if (pn->extension == -1) {
			switch (pn->ft->ag->combined) {
			case ag_separate:	/* separate reads, artificially combined into a single array */
				return FS_r_separate_all(owq);
			case ag_mixed:		/* mixed mode, ALL read handled differently */
			case ag_aggregate:	/* natively an array */
				/* return ALL if required   (comma separated) */
				return FS_r_aggregate_all(owq);
			}
		} else if (pn->extension > -1
				   && pn->ft->ag->combined == ag_aggregate) {
			/* split apart if a single item requested */
			return FS_r_aggregate(owq);
		}
	}

	/* Normal read. */
    return FS_r_single(owq);
}

/* Structure file */
static int FS_structure(struct one_wire_query * owq)
{
	char ft_format_char[] = "  iufaabydytg";	/* return type */
	/* 
	   enum ft_format {
	   ft_directory,
	   ft_subdir,
	   ft_integer,
	   ft_unsigned,
	   ft_float,
	   ft_ascii,
	   ft_vascii, // variable length ascii -- must be read and measured.
	   ft_binary,
	   ft_yesno,
	   ft_date,
	   ft_bitfield,
	   ft_temperature,
	   ft_tempgap,
	   } ;
	 */
	int len;
    struct one_wire_query owq_copy ;

	size_t file_length = OWQ_FullFileLength(owq);
    if (OWQ_offset(owq) > (off_t) file_length)
		return -ERANGE;

	memcpy(&owq_copy, owq, sizeof(struct one_wire_query));	/* shallow copy */
    OWQ_pn(&owq_copy).type = pn_real;			/* "real" type to get return length, rather than "structure" length */

	UCLIBCLOCK;
    len = snprintf(OWQ_buffer(owq),
                   OWQ_size(owq),
				   "%c,%.6d,%.6d,%.2s,%.6d,",
                   ft_format_char[OWQ_pn(owq).ft->format],
                   (OWQ_pn(owq).ft->ag) ? OWQ_pn(owq).extension : 0,
                   (OWQ_pn(owq).ft->ag) ? OWQ_pn(owq).ft->ag->elements : 1,
                   (OWQ_pn(owq).ft->read.v) ?
				   ((OWQ_pn(owq).ft->write.v) ? "rw" : "ro") :
                           ((OWQ_pn(owq).ft->write.v) ? "wo" : "oo"),
				   (int) OWQ_FullFileLength(owq)
		);
	UCLIBCUNLOCK;

    if ( len <0 ) return -EFAULT ;
    return Fowq_output_offset_and_size( OWQ_buffer(owq), len, owq ) ;
}

/* read without artificial separation or combination */
static int FS_r_single(struct one_wire_query * owq)
{
	size_t file_length ;
    struct parsedname * pn = &OWQ_pn(owq) ;
    enum ft_format format = pn->ft->format;

//printf("FS_r_single pid=%ld path=%s size=%d, offset=%d, extension=%d adapter=%d\n",pthread_self(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;

	LEVEL_CALL("FS_r_single: format=%d file_length=%d offset=%d\n",
               (int) pn->ft->format, (int) OWQ_size(owq), (int) OWQ_offset(owq))

    /* Mounting fuse with "direct_io" will cause a second read with offset
        * at end-of-file... Just return 0 if offset == size */
            file_length = OWQ_FileLength(owq);
    if (OWQ_offset(owq) > (off_t) file_length)
		return -ERANGE;
    if (OWQ_offset(owq) == (off_t) file_length)
		return 0;

	/* Special for *.BYTE -- treat as a single value */
	if (format == ft_bitfield && pn->extension == -2)
		format = ft_unsigned;

	switch (format) {
	case ft_integer:{
			int i;
			if (Cache_Get_Strict(&i, sizeof(int), pn)) {
                int ret ;
                if ((ret = (pn->ft->read.i) (&i, pn)) < 0)
					return ret;
				Cache_Add(&i, sizeof(int), pn);
			}
			LEVEL_DEBUG("FS_r_single: (integer) %d\n", i);
            OWQ_I(owq) = i ;
			break;
		}
	case ft_bitfield:{
			UINT u;
			if (Cache_Get_Strict(&u, sizeof(UINT), pn)) {
                int ret ;
                if ((ret = (pn->ft->read.u) (&u, pn)) < 0)
					return ret;
				Cache_Add(&u, sizeof(UINT), pn);
			}
			LEVEL_DEBUG("FS_r_single: (bitfield) %u\n", u);
            OWQ_Y(owq) = UT_getbit((void *) (&u), pn->extension) ;
			break;
		}
	case ft_unsigned:{
			UINT u;
			if (Cache_Get_Strict(&u, sizeof(UINT), pn)) {
                int ret ;
                if ((ret = (pn->ft->read.u) (&u, pn)) < 0)
					return ret;
				Cache_Add(&u, sizeof(UINT), pn);
			}
			LEVEL_DEBUG("FS_r_single: (unsigned) %u\n", u);
            OWQ_U(owq) = u ;
            break;
		}
	case ft_float:
	case ft_temperature:
	case ft_tempgap:{
			_FLOAT f;
			if (Cache_Get_Strict(&f, sizeof(_FLOAT), pn)) {
                int ret ;
                if ((ret = (pn->ft->read.f) (&f, pn)) < 0)
					return ret;
				Cache_Add(&f, sizeof(_FLOAT), pn);
			}
			LEVEL_DEBUG("FS_r_single: (float) %G\n", f);
            OWQ_F(owq) = f ;
            break;
		}
	case ft_date:{
			_DATE d;
			if (Cache_Get_Strict(&d, sizeof(_DATE), pn)) {
                int ret ;
                if ((ret = (pn->ft->read.d) (&d, pn)) < 0)
					return ret;
				Cache_Add(&d, sizeof(_DATE), pn);
			}
			LEVEL_DEBUG("FS_r_single: (date) %lu\n",
						(unsigned long int) d);
            OWQ_D(owq) = d ;
            break;
		}
	case ft_yesno:{
			int y;
			if (Cache_Get_Strict(&y, sizeof(int), pn)) {
                int ret ;
                if ((ret = (pn->ft->read.y) (&y, pn)) < 0)
					return ret;
				Cache_Add(&y, sizeof(int), pn);
			}
			LEVEL_DEBUG("FS_r_single: (yesno) %d\n", y);
            OWQ_Y(owq) = y ;
            break;
		}
	case ft_vascii:
	case ft_ascii:
	case ft_binary:{
        ssize_t allowed_size = file_length - OWQ_offset(owq);
        if (allowed_size > (ssize_t) OWQ_size(owq) ) {
            allowed_size = OWQ_size(owq) ;
        }
        if (Cache_Get_Strict(OWQ_buffer(owq), file_length, pn)) {
            int ret ;
            ret = (format == ft_binary) ?
                  (pn->ft->read.b) ((BYTE *) OWQ_buffer(owq), allowed_size, OWQ_offset(owq),pn) :
                  (pn->ft->read.a) ((ASCII *) OWQ_buffer(owq), allowed_size, OWQ_offset(owq),pn) ;
            if (ret == pn->ft->suglen) {
                Cache_Add(OWQ_buffer(owq), ret, pn);
            } else if (ret >= 0) {
                Cache_Del(pn);
            }
            return ret;
        }
        return file_length ;
    }
	case ft_directory:
	case ft_subdir:
		return -ENOSYS;
	default:
		return -ENOENT;
	}
    return FS_output_owq(owq) ; // put data as string into buffer and return length
}

/* read an aggregation (returns an array from a single read) */
static int FS_r_aggregate_all(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    size_t elements = pn->ft->ag->elements;
	int ret = 0;
	size_t file_length = 0;

    
	/* Mounting fuse with "direct_io" will cause a second read with offset
	 * at end-of-file... Just return 0 if offset == size */
    file_length = OWQ_FullFileLength(owq);
    if (OWQ_offset(owq) > (off_t) file_length)
		return -ERANGE;
    if (OWQ_offset(owq) == (off_t) file_length)
		return 0;

	switch (pn->ft->format) {
	case ft_integer:
		{
			int *i = (int *) calloc(elements, sizeof(int));
            if (i == NULL) {
				ret = -ENOMEM;
                break ;
            }
			if (Cache_Get_Strict(i, elements * sizeof(int), pn)) {
				if ((ret = (pn->ft->read.i) (i, pn)) >= 0)
					Cache_Add(i, elements * sizeof(int), pn);
			}
            if (ret >= 0) {
                size_t transfer_index ;
                for (transfer_index = 0 ; transfer_index < elements ; ++transfer_index ) OWQ_array_I(owq,transfer_index) = i[transfer_index] ;
            }
            free(i);
			break;
		}
	case ft_unsigned:
		{
			UINT *u = (UINT *) calloc(elements, sizeof(UINT));

            if (u == NULL){
                ret = -ENOMEM;
                break ;
            }
			if (Cache_Get_Strict(u, elements * sizeof(UINT), pn)) {
				if ((ret = (pn->ft->read.u) (u, pn)) >= 0)
					Cache_Add(u, elements * sizeof(UINT), pn);
			}
            if (ret >= 0) {
                size_t transfer_index ;
                for (transfer_index = 0 ; transfer_index < elements ; ++transfer_index ) OWQ_array_U(owq,transfer_index) = u[transfer_index] ;
            }
            free(u);
			break;
		}
	case ft_temperature:
	case ft_tempgap:
	case ft_float:
		{
			_FLOAT *f = (_FLOAT *) calloc(elements, sizeof(_FLOAT));
            if (f == NULL){
                ret = -ENOMEM;
                break ;
            }
            if (Cache_Get_Strict(f, elements * sizeof(_FLOAT), pn)) {
				if ((ret = (pn->ft->read.f) (f, pn)) >= 0)
					Cache_Add(f, elements * sizeof(_FLOAT), pn);
			}
            if (ret >= 0) {
                size_t transfer_index ;
                for (transfer_index = 0 ; transfer_index < elements ; ++transfer_index ) OWQ_array_F(owq,transfer_index) = f[transfer_index] ;
            }
            free(f);
			break;
		}
	case ft_date:
		{
			_DATE *d = (_DATE *) calloc(elements, sizeof(_DATE));
            if (d == NULL){
                ret = -ENOMEM;
                break ;
            }
            ret = (pn->ft->read.d) (d, pn);
			if (Cache_Get_Strict(d, elements * sizeof(_DATE), pn)) {
				if ((ret = (pn->ft->read.d) (d, pn)) >= 0)
					Cache_Add(d, elements * sizeof(_DATE), pn);
			}
            if (ret >= 0) {
                size_t transfer_index ;
                for (transfer_index = 0 ; transfer_index < elements ; ++transfer_index ) OWQ_array_D(owq,transfer_index) = d[transfer_index] ;
            }
            free(d);
			break;
		}
	case ft_yesno:
		{
			int *y = (int *) calloc(elements, sizeof(int));
            if (y == NULL){
                ret = -ENOMEM;
                break ;
            }
            if (Cache_Get_Strict(y, elements * sizeof(int), pn)) {
				if ((ret = (pn->ft->read.y) (y, pn)) >= 0)
					Cache_Add(y, elements * sizeof(int), pn);
			}
            if (ret >= 0) {
                size_t transfer_index ;
                for (transfer_index = 0 ; transfer_index < elements ; ++transfer_index ) OWQ_array_Y(owq,transfer_index) = y[transfer_index] ;
            }
            free(y);
			break;
		}
	case ft_bitfield:
		{
			UINT u;
			ret = (pn->ft->read.u) (&u, pn);
			if (Cache_Get_Strict(&u, sizeof(UINT), pn)) {
				if ((ret = (pn->ft->read.u) (&u, pn)) >= 0)
					Cache_Add(&u, sizeof(UINT), pn);
			}
            if (ret >= 0) {
                size_t transfer_index ;
                for (transfer_index = 0 ; transfer_index < elements ; ++transfer_index ) OWQ_array_Y(owq,transfer_index) = UT_getbit((void *)(&u),transfer_index) ;
            }
            break;
		}
	case ft_vascii:
	case ft_ascii:{
        ssize_t allowed_size = file_length - OWQ_offset(owq);
        if (allowed_size > (ssize_t) OWQ_size(owq))
            allowed_size = OWQ_size(owq);
        return (pn->ft->read.a) ((ASCII *) OWQ_buffer(owq), allowed_size, OWQ_offset(owq), pn);
		}
	case ft_binary:{
        ssize_t allowed_size = file_length - OWQ_offset(owq);
        if (allowed_size > (ssize_t) OWQ_size(owq))
            allowed_size = OWQ_size(owq);
        return (pn->ft->read.b) ((BYTE *) OWQ_buffer(owq), allowed_size, OWQ_offset(owq), pn);
		}
	case ft_directory:
	case ft_subdir:
		return -ENOSYS;
	default:
		return -ENOENT;
	}

    if ( ret >= 0 ) {
        ret = FS_output_owq(owq) ; // put data as string into buffer and return length
    }
	return ret;
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->ft->ag->combined==ag_separate */
static int FS_r_separate_all(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    struct one_wire_query struct_owq_single ;
    struct one_wire_query * owq_single = &struct_owq_single ;
    size_t file_length  = OWQ_FullFileLength(owq);
    size_t entry_length  = OWQ_FileLength(owq);;
    size_t elements = pn->ft->ag->elements ;
    size_t extension ;
    int output_or_error ;
    
    BYTE * memory_buffer = NULL ; // used for ascii and binary only

	STAT_ADD1(read_array);		/* statistics */

    if (OWQ_offset(owq) > (off_t) file_length)
		return -ERANGE;
    if (OWQ_offset(owq) == (off_t) file_length)
		return 0;

	/* shallow copy */
	memcpy(owq_single, owq, sizeof(struct one_wire_query));

    /* set up a memory buffer space for ascii or binary data, temporarily */
    switch ( pn->ft->format ) {
        case ft_ascii:
        case ft_vascii:
        case ft_binary:
            memory_buffer = malloc( file_length ) ;
            if ( memory_buffer == NULL ) return -ENOMEM ;
            break ;
        default:
            break ;
    }
    
    /* Loop through F_r_single, just to get data */
	for (extension = 0; extension < elements; ++extension) {
        OWQ_pn(owq_single).extension = extension ;
        int ret = FS_r_single(owq_single) ;
        if ( ret < 0 ) {
            if ( memory_buffer != NULL ) free( memory_buffer) ;
            return ret ;
        }
        switch ( pn->ft->format ) {
            case ft_ascii:
            case ft_vascii:
            case ft_binary:
                OWQ_mem(owq_single) = &memory_buffer[extension*entry_length] ;
                memcpy( OWQ_mem(owq_single), OWQ_buffer(owq_single), ret ) ;
                OWQ_length(owq_single) = ret ;
                break ;
            default:
                break ;
        }

        memmove( &(OWQ_array(owq)[extension]), &OWQ_val(owq_single), sizeof(union value_object) ) ;
	}
    
    output_or_error = FS_output_owq(owq) ; // put data as string into buffer and return length
    if ( memory_buffer != NULL ) free( memory_buffer) ;
    return output_or_error;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->ft->ag->combined==ag_aggregate */
static int FS_r_aggregate(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    size_t elements = pn->ft->ag->elements;
	int ret = 0;
	off_t file_length = 0;
    size_t extension = pn->extension ;

	/* Mounting fuse with "direct_io" will cause a second read with offset
	 * at end-of-file... Just return 0 if offset == size */
	file_length = OWQ_FileLength(owq);
    if (OWQ_offset(owq) > file_length)
		return -ERANGE;
    if (OWQ_offset(owq) == file_length)
		return 0;

	switch (pn->ft->format) {
	case ft_integer:
		{
			int *i = (int *) calloc(elements, sizeof(int));
			if (i == NULL)
				return -ENOMEM;
			if (Cache_Get_Strict(i, elements * sizeof(int), pn)) {
				if ((ret = (pn->ft->read.i) (i, pn)) >= 0)
					Cache_Add(i, elements * sizeof(int), pn);
			}
			if (ret >= 0)
                OWQ_I(owq) = i[extension] ;
			free(i);
			break;
		}
	case ft_unsigned:
		{
			UINT *u = (UINT *) calloc(elements, sizeof(UINT));
			if (u == NULL)
				return -ENOMEM;
			if (Cache_Get_Strict(u, elements * sizeof(UINT), pn)) {
				if ((ret = (pn->ft->read.u) (u, pn)) >= 0)
					Cache_Add(u, elements * sizeof(UINT), pn);
			}
			if (ret >= 0)
                OWQ_U(owq) = u[extension] ;
            free(u);
			break;
		}
	case ft_float:
	case ft_temperature:
	case ft_tempgap:
		{
			_FLOAT *f = (_FLOAT *) calloc(elements, sizeof(_FLOAT));
			if (f == NULL)
				return -ENOMEM;
			if (Cache_Get_Strict(f, elements * sizeof(_FLOAT), pn)) {
				if ((ret = (pn->ft->read.f) (f, pn)) >= 0)
					Cache_Add(f, elements * sizeof(_FLOAT), pn);
			}
            if (ret >= 0)
                OWQ_F(owq) = f[extension] ;
            free(f);
			break;
		}
	case ft_date:
		{
			_DATE *d = (_DATE *) calloc(elements, sizeof(_DATE));
			if (d == NULL)
				return -ENOMEM;
			if (Cache_Get_Strict(d, elements * sizeof(_DATE), pn)) {
				if ((ret = (pn->ft->read.d) (d, pn)) >= 0)
					Cache_Add(d, elements * sizeof(_DATE), pn);
			}
			if (ret >= 0)
                OWQ_D(owq) = d[extension] ;
            free(d);
			break;
		}
	case ft_yesno:
		{
			int *y = (int *) calloc(elements, sizeof(int));
			if (y == NULL)
				return -ENOMEM;
			if (Cache_Get_Strict(y, elements * sizeof(int), pn)) {
				if ((ret = (pn->ft->read.y) (y, pn)) >= 0)
					Cache_Add(y, elements * sizeof(int), pn);
			}
			if (ret >= 0)
                OWQ_Y(owq) = y[extension] ;
            free(y);
			break;
		}
	case ft_bitfield:
		{
			UINT u;
			ret = (pn->ft->read.u) (&u, pn);
			if (Cache_Get_Strict(&u, sizeof(UINT), pn)) {
				if ((ret = (pn->ft->read.u) (&u, pn)) >= 0)
					Cache_Add(&u, sizeof(UINT), pn);
			}
			if (ret >= 0) 
                OWQ_Y(owq) = UT_getbit((void *) (&u), extension) ;
			break;
		}
	case ft_vascii:
	case ft_ascii:
    case ft_binary:
    {
        size_t full_file_length = OWQ_FullFileLength(owq) ;
        BYTE * memory_buffer = malloc( full_file_length ) ;
        if ( memory_buffer == NULL ) return -ENOMEM ;
        ret = ( OWQ_pn(owq).ft->format == ft_binary ) ?
                (pn->ft->read.b) (memory_buffer, full_file_length, 0, pn) :
                (pn->ft->read.a) ((ASCII *)memory_buffer, full_file_length, 0, pn) ;
        if ( ret < 0 ) {
            free(memory_buffer) ;
            return ret ;
        }
        ret = Fowq_output_offset_and_size( (ASCII *) &memory_buffer[file_length*extension], file_length, owq ) ;
        if ( ret < 0 ) {
            free(memory_buffer) ;
            return ret ;
        }
        OWQ_mem(owq) = OWQ_buffer(owq) ;
        OWQ_length(owq) = ret ;
        break ;
    }
	case ft_directory:
	case ft_subdir:
		return -ENOSYS;
	default:
		return -ENOENT;
	}

    if ( ret >= 0 ) {
        ret = FS_output_owq(owq) ; // put data as string into buffer and return length
    }
    return ret;
}

int FS_output_integer(int value, char *buf, const size_t size,
					  const struct parsedname *pn)
{
	size_t suglen = SimpleFileLength(pn);
	/* should only need suglen+1, but uClibc's snprintf()
	   seem to trash 'len' if not increased */
	int len;
	char *c;
	if (!(c = (char *) malloc(suglen + 2))) {
		return -ENOMEM;
	}
	if (suglen > size)
		suglen = size;
	UCLIBCLOCK;
	len = snprintf(c, suglen + 1, "%*d", (int) suglen, value);
	UCLIBCUNLOCK;
	if ((len < 0) || ((size_t) len > suglen)) {
		free(c);
		return -EMSGSIZE;
	}
	memcpy(buf, c, (size_t) len);
	free(c);
	return len;
}

int FS_output_unsigned(UINT value, char *buf, const size_t size,
					   const struct parsedname *pn)
{
	size_t suglen = SimpleFileLength(pn);
	int len;
	/*
	   char c[suglen+2] ;
	   defining this REALLY doesn't work on gcc 3.3.2 either...
	   It will corrupt "const size_t size"! after returning from this function
	 */
	/* should only need suglen+1, but uClibc's snprintf()
	   seem to trash 'len' if not increased */
	char *c;
	if (!(c = (char *) malloc(suglen + 2))) {
		return -ENOMEM;
	}
	if (suglen > size)
		suglen = size;
	UCLIBCLOCK;
	len = snprintf(c, suglen + 1, "%*u", (int) suglen, value);
	UCLIBCUNLOCK;
	if ((len < 0) || ((size_t) len > suglen)) {
		free(c);
		return -EMSGSIZE;
	}
	memcpy(buf, c, (size_t) len);
	free(c);
	return len;
}

int FS_output_float(_FLOAT value, char *buf, const size_t size,
					const struct parsedname *pn)
{
	size_t suglen = SimpleFileLength(pn);
	/* should only need suglen+1, but uClibc's snprintf()
	   seem to trash 'len' if not increased */
	int len;
	char *c;
	if (!(c = (char *) malloc(suglen + 2))) {
		return -ENOMEM;
	}
	if (suglen > size)
		suglen = size;
	UCLIBCLOCK;
	len = snprintf(c, suglen + 1, "%*G", (int) suglen, value);
	UCLIBCUNLOCK;
	if ((len < 0) || ((size_t) len > suglen)) {
		free(c);
		return -EMSGSIZE;
	}
	memcpy(buf, c, (size_t) len);
	free(c);
	return len;
}

int FS_output_date(_DATE value, char *buf, const size_t size,
				   const struct parsedname *pn)
{
	char c[26];
	(void) pn;
	if (size < 24)
		return -EMSGSIZE;
	ctime_r(&value, c);
	memcpy(buf, c, 24);
	return 24;
}
int FS_output_ascii(ASCII * buf, size_t size, off_t offset, ASCII * answer,
					size_t length)
{
	if (offset > (off_t) length)
		return -EFAULT;
	length -= offset;
	if (length > size)
		length = size;
	if (length)
		memcpy(buf, &answer[offset], length);
	return length;
}

/* If the string is properly terminated, we can use a simpler routine */
int FS_output_ascii_z(ASCII * buf, size_t size, off_t offset,
					  ASCII * answer)
{
	return FS_output_ascii(buf, size, offset, answer, strlen(answer));
}
