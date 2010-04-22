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
static ZERO_OR_ERROR FS_input_yesno(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_integer(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_unsigned(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_float(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_date(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_ascii(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_array_with_commas(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_ascii_array(struct one_wire_query *owq);
static ZERO_OR_ERROR FS_input_array_no_commas(struct one_wire_query *owq);
static size_t FS_check_length(struct one_wire_query *owq);

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


// Routines to take write data (ascii representation) and interpret it and place into the proper fields.
ZERO_OR_ERROR OWQ_parse_input(struct one_wire_query *owq)
{
	switch (OWQ_pn(owq).extension) {
	case EXTENSION_BYTE:
		return FS_input_unsigned(owq);
	case EXTENSION_ALL:
		switch (OWQ_pn(owq).selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_alias:
			return FS_input_ascii_array(owq);
		case ft_binary:
			return FS_input_array_no_commas(owq);
		default:
			return FS_input_array_with_commas(owq);
		}
	default:
		switch (OWQ_pn(owq).selected_filetype->format) {
		case ft_integer:
			return FS_input_integer(owq);
		case ft_yesno:
		case ft_bitfield:
			return FS_input_yesno(owq);
		case ft_unsigned:
			return FS_input_unsigned(owq);
		case ft_pressure:
		case ft_temperature:
		case ft_tempgap:
		case ft_float:
			return FS_input_float(owq);
		case ft_date:
			return FS_input_date(owq);
		case ft_vascii:
		case ft_alias:
		case ft_ascii:
		case ft_binary:
			return FS_input_ascii(owq);
		case ft_directory:
		case ft_subdir:
		case ft_unknown:
			return -ENOENT;
		}
	}
	return -EINVAL;				// should never be reached if all the cases are truly covered
}


/* return 0 if ok */
static ZERO_OR_ERROR FS_input_yesno(struct one_wire_query *owq)
{
	char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH + 1];
	char *input_buffer = default_input_buffer;

	char *end;
	int I;
	ZERO_OR_ERROR ret;

	/* allocate more space if buffer is really long */
	if (OWQ_size(owq) > DEFAULT_INPUT_BUFFER_LENGTH) {
		input_buffer = owmalloc(OWQ_size(owq) + 1);
		if (input_buffer == NULL) {
			return -ENOMEM;
		}
	}
	memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
	input_buffer[OWQ_size(owq)] = '\0';	// make sure null-ended

	//printf("YESNO: %s\n",input_buffer);
	errno = 0;
	I = strtol(input_buffer, &end, 10);
	if ((errno == 0) && (end != input_buffer)) {	// NUMBER?
		//printf("YESNO number = %d\n",I) ;
		OWQ_Y(owq) = (I != 0);
		ret = 0;
	} else {					// WORD?
		char *non_blank;
		ret = -EFAULT;			//default error until a non-blank found
		for (non_blank = input_buffer; non_blank[0]; ++non_blank) {
			if (non_blank[0] != ' ' && non_blank[0] != '\t') {
				ret = 0;		// now assume good
				if (strncasecmp(non_blank, "y", 1) == 0) {
					OWQ_Y(owq) = 1;
					break;
				}
				if (strncasecmp(non_blank, "n", 1) == 0) {
					OWQ_Y(owq) = 0;
					break;
				}
				if (strncasecmp(non_blank, "on", 2) == 0) {
					OWQ_Y(owq) = 1;
					break;
				}
				if (strncasecmp(non_blank, "off", 3) == 0) {
					OWQ_Y(owq) = 0;
					break;
				}
				ret = -EINVAL;
				break;
			}
		}
	}
	/* free specially long buffer */
	if (input_buffer != default_input_buffer) {
		owfree(input_buffer);
	}
	return ret;
}

/* parse a value for write from buffer to value_object */
/* return 0 if ok */
static ZERO_OR_ERROR FS_input_integer(struct one_wire_query *owq)
{
	char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH + 1];
	char *input_buffer = default_input_buffer;
	char *end;

	/* allocate more space if buffer is really long */
	if (OWQ_size(owq) > DEFAULT_INPUT_BUFFER_LENGTH) {
		input_buffer = owmalloc(OWQ_size(owq) + 1);
		if (input_buffer == NULL) {
			return -ENOMEM;
		}
	}

	memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
	input_buffer[OWQ_size(owq)] = '\0';	// make sure null-ended
	errno = 0;
	OWQ_I(owq) = strtol(input_buffer, &end, 10);

	/* free specially long buffer */
	if (input_buffer != default_input_buffer)
		owfree(input_buffer);

	if (errno) {
		return -errno;			// conversion error
	}
	if (end == input_buffer) {
		return -EINVAL;			// nothing valid found for conversion
	}
	return 0;					// good return
}

/* parse a value for write from buffer to value_object */
/* return 0 if ok */
static ZERO_OR_ERROR FS_input_unsigned(struct one_wire_query *owq)
{
	char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH + 1];
	char *input_buffer = default_input_buffer;
	char *end;

	/* allocate more space if buffer is really long */
	if (OWQ_size(owq) > DEFAULT_INPUT_BUFFER_LENGTH) {
		input_buffer = owmalloc(OWQ_size(owq) + 1);
		if (input_buffer == NULL) {
			return -ENOMEM;
		}
	}

	memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
	input_buffer[OWQ_size(owq)] = '\0';	// make sure null-ended
	errno = 0;
	OWQ_U(owq) = strtoul(input_buffer, &end, 10);

	/* free specially long buffer */
	if (input_buffer != default_input_buffer) {
		owfree(input_buffer);
	}

	if (errno) {
		return -errno;			// conversion error
	}
	if (end == input_buffer) {
		return -EINVAL;			// nothing valid found for conversion
	}
	return 0;					// good return
}

/* parse a value for write from buffer to value_object */
/* return 0 if ok */
static ZERO_OR_ERROR FS_input_float(struct one_wire_query *owq)
{
	char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH + 1];
	char *input_buffer = default_input_buffer;
	char *end;

	_FLOAT F;

	/* allocate more space if buffer is really long */
	if (OWQ_size(owq) > DEFAULT_INPUT_BUFFER_LENGTH) {
		input_buffer = owmalloc(OWQ_size(owq) + 1);
		if (input_buffer == NULL) {
			return -ENOMEM;
		}
	}

	memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
	input_buffer[OWQ_size(owq)] = '\0';	// make sure null-ended
	errno = 0;
	F = strtod(input_buffer, &end);

	/* free specially long buffer */
	if (input_buffer != default_input_buffer) {
		owfree(input_buffer);
	}

	if (errno) {
		return -errno;			// conversion error
	}
	if (end == input_buffer) {
		return -EINVAL;			// nothing valid found for conversion
	}

	switch (OWQ_pn(owq).selected_filetype->format) {
	case ft_pressure:
		OWQ_F(owq) = fromPressure(F, PN(owq));
		break;
	case ft_temperature:
		OWQ_F(owq) = fromTemperature(F, PN(owq));
		break;
	case ft_tempgap:
		OWQ_F(owq) = fromTempGap(F, PN(owq));
		break;
	default:
		OWQ_F(owq) = F;
		break;
	}
	return 0;					// good return
}

/* return 0 if ok */
static ZERO_OR_ERROR FS_input_date(struct one_wire_query *owq)
{
	char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH + 1];
	char *input_buffer = default_input_buffer;

	struct tm tm;
	ZERO_OR_ERROR ret = 0;				// default ok

	/* allocate more space if buffer is really long */
	if (OWQ_size(owq) > DEFAULT_INPUT_BUFFER_LENGTH) {
		input_buffer = owmalloc(OWQ_size(owq) + 1);
		if (input_buffer == NULL) {
			return -ENOMEM;
		}
	}

	memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
	input_buffer[OWQ_size(owq)] = '\0';	// make sure null-ended

	if (OWQ_size(owq) < 2 || input_buffer[0] == '\0' || input_buffer[0] == '\n') {
		OWQ_D(owq) = time(NULL);
	} else if ((strptime(input_buffer, "%T %a %b %d %Y", &tm) == NULL)	// 12:27:02 Tuesday March 23 2007
			   && (strptime(input_buffer, "%b %d %Y %T", &tm) == NULL)	// March 23 2007 12:27:03
			   && (strptime(input_buffer, "%a %b %d %Y %T", &tm) == NULL)	// Tuesday March 23 2007 12:27:02
			   && (strptime(input_buffer, "%c", &tm) == NULL)
			   && (strptime(input_buffer, "%D %T", &tm) == NULL)) {
		ret = -EINVAL;
	} else {
		OWQ_D(owq) = mktime(&tm);
	}

	/* free specially long buffer */
	if (input_buffer != default_input_buffer) {
		owfree(input_buffer);
	}

	return ret;
}

static size_t FS_check_length(struct one_wire_query *owq)
{
	/* need to check property length */
	size_t size = OWQ_size(owq) ;
	size_t filelength = FileLength(PN(owq)) ;
	off_t offset = OWQ_offset(owq) ;
	
	// check overall length
	if ( filelength < size ) {
		size = filelength ;
	}
	
	// check overall offset
	if ( (size_t) offset > filelength ) {
		size = 0 ;
	// and check offset plus size
	} else if ( offset + size > filelength ) {
		// cannot be negative despite compiler warnings
		size = filelength - offset ;
	}

	return size ;
}

static int FS_input_ascii(struct one_wire_query *owq)
{
	OWQ_length(owq) = OWQ_size(owq) = FS_check_length(owq) ;
	return 0;
}

/* returns 0 if ok */
/* creates a new allocated memory area IF no error */
static ZERO_OR_ERROR FS_input_array_with_commas(struct one_wire_query *owq)
{
	int elements = OWQ_pn(owq).selected_filetype->ag->elements;
	int extension;
	char *end = OWQ_buffer(owq) + OWQ_size(owq);
	char *comma = NULL;			// assignment to avoid compiler warning
	char *buffer_position;

	if (OWQ_offset(owq)!=0) {
		return -EINVAL;
	}

	for (extension = 0; extension < elements; ++extension) {
		struct one_wire_query * owq_single ;
		// find start of buffer span
		if (extension == 0) {
			buffer_position = OWQ_buffer(owq);
		} else {
			buffer_position = comma + 1;
			if (buffer_position >= end) {
				return -EINVAL;
			}
		}
		// find end of buffer span
		if (extension == elements - 1) {
			comma = end;
		} else {
			comma = memchr(buffer_position, ',', end - buffer_position);
			if (comma == NULL) {
				return -EINVAL;
			}
		}
		//Debug_Bytes("FS_input_array_with_commas -- to end",buffer_position,end-buffer_position) ;
		//Debug_Bytes("FS_input_array_with_commas -- to comma",buffer_position,comma-buffer_position) ;
		// set up single element
		owq_single = OWQ_create_separate( extension, owq ) ;
		if ( owq_single == NULL ) {
			return -ENOMEM ;
		}
		OWQ_assign_read_buffer(buffer_position, comma - buffer_position, 0, owq_single) ;
		if (OWQ_parse_input(owq_single)) {
			OWQ_destroy(owq_single) ;
			return -EINVAL;
		}
		memcpy(&(OWQ_array(owq)[extension]), &OWQ_val(owq_single), sizeof(union value_object));
		OWQ_destroy(owq_single) ;
	}
	return 0;
}

/* returns 0 if ok */
/* Basically pack the entries onto the buffer array and find the position with the cumulative length entries */
static ZERO_OR_ERROR FS_input_ascii_array(struct one_wire_query *owq)
{
	int elements = OWQ_pn(owq).selected_filetype->ag->elements;
	int extension;
	char *end = OWQ_buffer(owq) + OWQ_size(owq);
	char *buffer_position = OWQ_buffer(owq);
	size_t suglen = OWQ_pn(owq).selected_filetype->suglen;

	if (OWQ_offset(owq)!=0) {
		return -EINVAL;
	}

	for (extension = 0; extension < elements; ++extension) {
		char *comma;
		size_t allowed_length = suglen ;
		size_t entry_length ;
		// find end of buffer span
		if (extension == elements - 1) {	// last element
			entry_length = end - buffer_position ;
			if ( entry_length < suglen ) {
				allowed_length = entry_length ;
			}
			OWQ_array_length(owq, extension) = allowed_length ;
			return 0;
		}
		// pre-terminal element
		comma = memchr(buffer_position, ',', end - buffer_position);
		if (comma == NULL) {
			return -EINVAL;
		}
		entry_length = comma - buffer_position;
		if ( entry_length < suglen ) {
			allowed_length = entry_length ;
		}
		// Set the entry size
		OWQ_array_length(owq, extension) = allowed_length ;
		// move the rest of the buffer (after the comma) to the end of this entry
		memmove(buffer_position + allowed_length, comma + 1, end - comma - 1);
		// shorten the buffer length by the comma and discarded chars
		end -= entry_length - allowed_length - 1 ;
		// move the buffer start to the start of the next entry
		buffer_position += allowed_length ;
	}
	return -ERANGE;				// never reach !
}


/* returns 0 if ok */
/* creates a new allocated memory area IF no error */
static ZERO_OR_ERROR FS_input_array_no_commas(struct one_wire_query *owq)
{
	int elements = OWQ_pn(owq).selected_filetype->ag->elements;
	int extension;
	int suglen = FileLength(PN(owq)) ;

	if ((OWQ_offset(owq) != 0)
		|| ((int) OWQ_size(owq) != suglen * elements)) {
		return -EINVAL;
	}

	for (extension = 0; extension < elements; ++extension) {
		OWQ_array_length(owq, extension) = suglen;
	}
	return 0;
}
