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
static int FS_input_yesno(struct one_wire_query * owq) ;
static int FS_input_integer(struct one_wire_query * owq) ;
static int FS_input_unsigned(struct one_wire_query * owq) ;
static int FS_input_float(struct one_wire_query * owq) ;
static int FS_input_date(struct one_wire_query * owq) ;
static int FS_input_ascii(struct one_wire_query * owq ) ;
static int FS_input_array_with_commas(struct one_wire_query * owq ) ;
static int FS_input_array_no_commas(struct one_wire_query * owq ) ;

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

int FS_input_owq( struct one_wire_query * owq)
{
    switch (OWQ_pn(owq).extension) {
        case -2:
            return FS_input_unsigned(owq) ;
        case -1:
            switch (OWQ_pn(owq).ft->format) {
                case ft_binary:
                    return FS_input_array_no_commas(owq) ;
                default:
                    return FS_input_array_with_commas(owq) ;
            }
        default:
            switch (OWQ_pn(owq).ft->format) {
                case ft_integer:
                    return FS_input_integer(owq) ;
                case ft_yesno:
                case ft_bitfield:
                    return FS_input_yesno(owq) ;
                case ft_unsigned:
                    return FS_input_unsigned(owq) ;
                case ft_temperature:
                case ft_tempgap:
                case ft_float:
                    return FS_input_float(owq) ;
                case ft_date:
                    return FS_input_date(owq) ;
                case ft_vascii:
                case ft_ascii:
                case ft_binary:
                    return FS_input_ascii(owq) ;
                case ft_directory:
                case ft_subdir:
                    return -ENOENT ;
            }
    }
    return -EINVAL ; // should never be reached if all the cases are truly covered
}


/* return 0 if ok */
static int FS_input_yesno(struct one_wire_query * owq)
{
    char default_input_buffer[DEFAULT_INPUT_BUFFER_LENGTH+1] ;
    char * input_buffer = default_input_buffer ;

    char *end;
    int I ;
    int ret ;

    /* allocate more space if buffer is really long */
    if ( OWQ_size(owq)>DEFAULT_INPUT_BUFFER_LENGTH ) {
        input_buffer = malloc(OWQ_size(owq)+1) ;
        if ( input_buffer == NULL ) return -ENOMEM ;
    }
    memcpy(input_buffer, OWQ_buffer(owq), OWQ_size(owq));
    input_buffer[OWQ_size(owq)] = '\0'; // make sure null-ended

    errno = 0 ;
    I = strtol(input_buffer, &end, 10);
    if ( (errno==0) && (end!=input_buffer) ) { // NUMBER?
        OWQ_Y(owq) = (I!=0) ;
        ret = 0 ;
    } else { // WORD?
        char * non_blank ;
        ret = -EFAULT ; //default error until a non-blank found
        for ( non_blank = input_buffer ; non_blank[0] ; ++non_blank ) {
            if ( non_blank[0]!=' ' && non_blank[0]!='\t') {
                ret = 0 ; // now assume good
                if ( strncasecmp(non_blank,"y",1)==0 ) {
                    OWQ_Y(owq) = 1 ;
                    break ;
                }
                if ( strncasecmp(non_blank,"n",1)==0 ) {
                    OWQ_Y(owq) = 0 ;
                    break ;
                }
                if ( strncasecmp(non_blank,"on",2)==0 ) {
                    OWQ_Y(owq) = 1 ;
                    break ;
                }
                if ( strncasecmp(non_blank,"off",3)==0 ) {
                    OWQ_Y(owq) = 0 ;
                    break ;
                }
                ret = -EINVAL ;
                break ;
            }
        }
    }
    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;
    return 1 ;
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
    if (end == input_buffer) return -EINVAL; // nothing valid found for conversion
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
    if (end == input_buffer) return -EINVAL; // nothing valid found for conversion
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
    F = strtod(input_buffer, &end);

    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;

    if (errno) return -errno ; // conversion error
    if (end == input_buffer) return -EINVAL; // nothing valid found for conversion

    switch ( OWQ_pn(owq).ft->format ) {
        case ft_temperature:
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
    } else if (  (strptime(input_buffer, "%T %a %b %d %Y", &tm) == NULL) // 12:27:02 Tuesday March 23 2007
                  && (strptime(input_buffer, "%b %d %Y %T", &tm)    == NULL) // March 23 2007 12:27:03
               && (strptime(input_buffer, "%a %b %d %Y %T", &tm) == NULL) // Tuesday March 23 2007 12:27:02
                          && (strptime(input_buffer, "%c", &tm)             == NULL)
                          && (strptime(input_buffer, "%D %T", &tm) == NULL) ) {
		ret = -EINVAL;
	} else {
		OWQ_D(owq) = mktime(&tm);
	}

    /* free specially long buffer */
    if ( input_buffer != default_input_buffer ) free(input_buffer ) ;

	return ret ;
}

static int FS_input_ascii(struct one_wire_query * owq )
{
    OWQ_mem(owq) = OWQ_buffer(owq) ;
    OWQ_length(owq) = OWQ_size(owq) ;
    return 0 ;
}

/* returns 0 if ok */
/* creates a new allocated memory area IF no error */
static int FS_input_array_with_commas(struct one_wire_query * owq )
{
    int elements = OWQ_pn(owq).ft->ag->elements ;
    union value_object * value_object_array = calloc((size_t) elements, sizeof(union value_object)) ;
    int extension ;
    char * end = OWQ_buffer(owq) + OWQ_size(owq) ;
    char * comma = NULL ; // assignment to avoid compiler warning
    char * buffer_position ;
    struct one_wire_query owq_single ;

    if ( value_object_array == NULL ) return -ENOMEM ;

    if ( OWQ_offset(owq) ) {
        free(value_object_array) ;
        return -EINVAL ;
    }
    
    for ( extension = 0 ; extension < elements ; ++extension ) {
        // find start of buffer span
        if ( extension == 0 ) {
            buffer_position = OWQ_buffer(owq) ;
        } else {
            buffer_position = comma + 1 ;
            if ( buffer_position >= end ) {
                free(value_object_array) ;
                return -EINVAL ;
            }
        }
        // find end of buffer span
        if ( extension == elements-1 ) {
            comma = end ;
        } else {
            comma = memchr(buffer_position, ',', end - buffer_position ) ;
            if ( comma == NULL ) {
                free(value_object_array) ;
                return -EINVAL ;
            }
        }
        // set up single element
        memcpy( &owq_single, owq, sizeof(owq_single) ) ;
        OWQ_pn(&owq_single).extension = extension ;
        OWQ_buffer(&owq_single) = buffer_position ;
        OWQ_size(&owq_single) = comma - buffer_position ;
        if ( FS_input_owq(&owq_single) ) {
            free( value_object_array ) ;
            return -EINVAL ;
        }
        memcpy( &(value_object_array[extension]), &OWQ_val(&owq_single), sizeof(union value_object) ) ;
    }
    OWQ_array(owq) = value_object_array ;
    return 0 ;
}


/* returns 0 if ok */
/* creates a new allocated memory area IF no error */
static int FS_input_array_no_commas(struct one_wire_query * owq )
{
    int elements = OWQ_pn(owq).ft->ag->elements ;
    union value_object * value_object_array = calloc((size_t) elements, sizeof(union value_object)) ;
    int extension ;
    int suglen = OWQ_pn(owq).ft->suglen ;

    if ( value_object_array == NULL ) return -ENOMEM ;

    if ( (OWQ_offset(owq)!=0) || ((int) OWQ_size(owq)!=suglen*elements) ) {
        free(value_object_array) ;
        return -EINVAL ;
    }
    
    OWQ_array(owq) = value_object_array ;
    for ( extension = 0 ; extension < elements ; ++extension ) {
        OWQ_array_mem(owq,extension) = OWQ_buffer(owq) + suglen*extension ;
        OWQ_array_length(owq,extension) = suglen ;
    }
    return 0 ;
}

