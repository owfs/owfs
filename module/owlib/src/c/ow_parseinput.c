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

#define   DEFAULT_INPUT_BUFFER_LENGTH   128

/* ------- Prototypes ----------- */
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

/* parse a value for write from buffer to value_object */
/* return 0 if ok */
static int FS_input_integer(struct one_wire_query * owq )
{
    char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH+1] ;
    char * input_buffer = default_input_buffer ;
    char *end;

    /* allocate more space if buffer is really long */
    if ( OWQ_size(owq)>DEFAULT_INPUT_BUFFER_LENGTH ) {
        input_buffer = malloc(OWQ_size(owq)+1) ;
        if ( input_buffer == NULL ) return -ENOMEM ;
    }

    memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
    input_buffer[OWQ_size(owq)] = '\0'; // make sure null-ended
    errno = 0;
    OWQ_I(owq) = strtol(input_buffer, &end, 10);

    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;

    if (errno) return -errno ; // conversion error
    if (end == cp) return -EINVAL; // nothing valid found for conversion
    return 0 ; // good return
}

/* parse a value for write from buffer to value_object */
/* return 0 if ok */
static int FS_input_unsigned(struct one_wire_query * owq )
{
    char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH+1] ;
    char * input_buffer = default_input_buffer ;
    char *end;

    /* allocate more space if buffer is really long */
    if ( OWQ_size(owq)>DEFAULT_INPUT_BUFFER_LENGTH ) {
        input_buffer = malloc(OWQ_size(owq)+1) ;
        if ( input_buffer == NULL ) return -ENOMEM ;
    }

    memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
    input_buffer[OWQ_size(owq)] = '\0'; // make sure null-ended
    errno = 0;
    OWQ_U(owq) = strtoul(input_buffer, &end, 10);

    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;

    if (errno) return -errno ; // conversion error
    if (end == cp) return -EINVAL; // nothing valid found for conversion
    return 0 ; // good return
}

/* parse a value for write from buffer to value_object */
/* return 0 if ok */
static int FS_input_float(struct one_wire_query * owq )
{
    char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH+1] ;
    char * input_buffer = default_input_buffer ;
    char *end;

    _FLOAT F ;

    /* allocate more space if buffer is really long */
    if ( OWQ_size(owq)>DEFAULT_INPUT_BUFFER_LENGTH ) {
        input_buffer = malloc(OWQ_size(owq)+1) ;
        if ( input_buffer == NULL ) return -ENOMEM ;
    }

    memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
    input_buffer[OWQ_size(owq)] = '\0'; // make sure null-ended
    errno = 0;
    F = strtod(input_buffer, &end, 10);

    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;

    if (errno) return -errno ; // conversion error
    if (end == cp) return -EINVAL; // nothing valid found for conversion

    switch ( OWQ_pn(owq).ft->format ) {
        case ft_temperture:
            OWQ_F(owq) = fromTemperature( F, &OWQ_pn(owq) ) ;
            break ;
        case ft_tempgap:
            OWQ_F(owq) = fromTempGap( F, &OWQ_pn(owq) ) ;
            break ;
        default:
            OWQ_F(owq) = F ;
            break ;
    }
    return 0 ; // good return
}

/* return 0 if ok */
static int FS_input_date(struct one_wire_query * owq )
{
    char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH+1] ;
    char * input_buffer = default_input_buffer ;

	struct tm tm;
    int ret = 0 ; // default ok

    /* allocate more space if buffer is really long */
    if ( OWQ_size(owq)>DEFAULT_INPUT_BUFFER_LENGTH ) {
        input_buffer = malloc(OWQ_size(owq)+1) ;
        if ( input_buffer == NULL ) return -ENOMEM ;
    }

    memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
    input_buffer[OWQ_size(owq)] = '\0'; // make sure null-ended

	if ( OWQ_size(owq)< 2 || input_buffer[0] == '\0' || input_buffer[0] == '\n') {
		OWQ_D(owq) = time(NULL);
	} else if ( strptime(input_buffer, "%a %b %d %T %Y", &tm) == NULL
			   && strptime(input_buffer, "%b %d %T %Y", &tm) == NULL
			   && strptime(input_buffer, "%c", &tm) == NULL
			   && strptime(input_buffer, "%D %T", &tm) == NULL) {
		ret = -EINVAL;
	} else {
		OWQ_D(owq) = mktime(&tm);
	}

    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;

	return ret ;
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
