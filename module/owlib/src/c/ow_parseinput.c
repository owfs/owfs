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

/* return 0 if ok */
static int FS_input_integer(struct one_wire_query * owq )
{
    char cp[OWQ_size(owq) + 1];
    char *end;

    memcpy(cp, OWQ_buffer(owq), OWQ_size(owq));
    cp[OWQ_size(owq)] = '\0';
    errno = 0;
    OWQ_I(owq) = strtol(cp, &end, 10);
    return end == cp || errno;
}

/* return 0 if ok */
static int FS_input_unsigned(struct one_wire_query * owq )
{
    char cp[OWQ_size(owq) + 1];
    char *end;

    memcpy(cp, OWQ_buffer(owq), OWQ_size(owq));
    cp[OWQ_size(owq)] = '\0';
    errno = 0;
    OWQ_U(owq) = strtoul(cp, &end, 10);
    return end == cp || errno;
}

/* return 0 if ok */
static int FS_input_float(struct one_wire_query * owq )
{
    char cp[OWQ_size(owq) + 1];
    char *end;

    memcpy(cp, OWQ_buffer(owq), OWQ_size(owq));
    cp[OWQ_size(owq)] = '\0';
    errno = 0;
    OWQ_F(owq) = strtoud(cp, &end, 10);
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
