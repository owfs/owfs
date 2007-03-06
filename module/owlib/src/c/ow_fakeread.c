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
static int FS_read_fake_single(struct one_wire_query * owq);
static int FS_read_fake_array(struct one_wire_query * owq);

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

#define Random (((_FLOAT)rand())/RAND_MAX)
#define Random_y (rand()&0x01)
#define Random_t (Random*100)
#define Random_d (time(NULL)*(1-.1*Random))
#define Random_i (rand()&0xFF)
#define Random_u (rand()&0xFF)
#define Random_b (rand()&0xFF)
#define Random_a (32+(rand()&0x3F))
#define Random_f (10*Random)

int FS_read_fake(struct one_wire_query * owq)
{
    switch (OWQ_pn(owq).extension) {
        case -1:                    /* array */
            return FS_read_fake_array(owq);
            case -2:                    /* bitfield */
        default:
            return FS_read_fake_single(owq);
    }
}

static int FS_read_fake_single(struct one_wire_query * owq)
{
    switch (OWQ_pn(owq).ft->format) {
        case ft_integer:
            OWQ_I(owq) = Random_i ;
            break ;
        case ft_yesno:
            OWQ_Y(owq) = Random_y ;
            break ;
        case ft_bitfield:
            if ( OWQ_pn(owq).extension == -2 ) {
                OWQ_U(owq) = Random_u ;
            } else {
                OWQ_Y(owq) = Random_y ;
            }
            break ;
        case ft_unsigned:
            OWQ_U(owq) = Random_u ;
            break ;
        case ft_temperature:
        case ft_tempgap:
        case ft_float:
            OWQ_F(owq) = Random_f ;
            break ;
        case ft_date:
            OWQ_D(owq) = Random_d ;
            break ;
        case ft_vascii:
        case ft_ascii:
        {
            size_t i ;
            for ( i=0 ; i < OWQ_size(owq) ; ++i ) {
                OWQ_buffer(owq)[i] = Random_a ;
            }
        }
        break ;
        case ft_binary:
        {
            size_t i ;
            for ( i=0 ; i < OWQ_size(owq) ; ++i ) {
                OWQ_buffer(owq)[i] = Random_b ;
            }
        }
        break ;
        case ft_directory:
        case ft_subdir:
            return -ENOENT ;
    }
    return FS_output_owq(owq) ; // put data as string into buffer and return length
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->ft->ag->combined==ag_separate */
static int FS_read_fake_array(struct one_wire_query * owq)
{
    size_t buffer_space_left = OWQ_size(owq);
    char *pointer_into_buffer = OWQ_buffer(owq);
    struct one_wire_query struct_owq_single;
    struct one_wire_query * owq_single = &struct_owq_single;
    struct parsedname * pn_single = &OWQ_pn(owq_single) ;

    STAT_ADD1(read_array);      /* statistics */

    if (OWQ_offset(owq) != 0) return -ERANGE;

    /* shallow copy */
    memcpy(owq_single, owq, sizeof(struct one_wire_query));

    for (pn_single->extension = 0; pn_single->extension < pn_single->ft->ag->elements;
         ++pn_single->extension) {
        int read_or_error ;
        /* Add a separating comma if not the first element */
        if (pn_single->extension && pn_single->ft->format != ft_binary) {
            if (buffer_space_left == 0)
                return -ERANGE;
            *pointer_into_buffer = ',';
            ++pointer_into_buffer;
            --buffer_space_left;
        }
        OWQ_buffer(owq_single) = pointer_into_buffer ;
        OWQ_size(owq_single) = buffer_space_left ;
        OWQ_offset(owq_single) = 0 ;
        read_or_error = FS_read_fake_single(owq_single) ;
        if ( read_or_error < 0 ) return read_or_error ;
        buffer_space_left -= read_or_error;
        pointer_into_buffer += read_or_error;
    }

    LEVEL_DEBUG("FS_readfake_all: size=%d buffer_space_left=%d used=%d\n", OWQ_size(owq), buffer_space_left,
                OWQ_size(owq) - buffer_space_left);
    return Fowq_output_offset_and_size( OWQ_buffer(owq), OWQ_size(owq) - buffer_space_left, owq ) ;
}
