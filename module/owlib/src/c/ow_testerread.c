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
static int FS_read_tester_single(struct one_wire_query * owq);
static int FS_read_tester_array(struct one_wire_query * owq);

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

int FS_read_tester(struct one_wire_query * owq)
{
    switch (OWQ_pn(owq).extension) {
        case EXTENSION_ALL:                    /* array */
            return FS_read_tester_array(owq);
		case EXTENSION_BYTE:                    /* bitfield */
        default:
            return FS_read_tester_single(owq);
    }
}

static int FS_read_tester_single(struct one_wire_query * owq)
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    int tester_bus = (pn->sn[2]<<8) + pn->sn[1] ;
    int device     = (pn->sn[6]<<8) + pn->sn[5] ;
    int family_code=  pn->sn[0] ;
    int calculated_value = family_code + tester_bus + device ;
    
    switch (OWQ_pn(owq).ft->format) {
        case ft_integer:
            OWQ_I(owq) = calculated_value ;
            break ;
        case ft_yesno:
            OWQ_Y(owq) = calculated_value & 0x1 ;
            break ;
        case ft_bitfield:
            if ( OWQ_pn(owq).extension == EXTENSION_BYTE ) {
                OWQ_U(owq) = calculated_value ;
            } else {
                OWQ_Y(owq) = calculated_value & 0x1 ;
            }
            break ;
        case ft_unsigned:
            OWQ_U(owq) = calculated_value ;
            break ;
        case ft_temperature:
        case ft_tempgap:
        case ft_float:
            OWQ_F(owq) = (_FLOAT) calculated_value * 0.1 ; ;
            break ;
        case ft_date:
            OWQ_D(owq) = 1174622400 ;
            break ;
        case ft_vascii:
        case ft_ascii:
        {
            ASCII address[16] ;
            size_t length_left = OWQ_size(owq) ;
            size_t buffer_index ;
            
            bytes2string( address, pn->sn, 8 ) ;
            for ( buffer_index = 0 ; buffer_index < OWQ_size(owq) ; buffer_index += sizeof(address) ) {
                size_t copy_length = length_left ;
                if ( copy_length > sizeof(address) ) copy_length = sizeof(address) ;
                memcpy( &OWQ_buffer(owq)[buffer_index], address, copy_length ) ;
                length_left -= copy_length ;
            }
        }
        break ;
        case ft_binary:
        {
            size_t length_left = OWQ_size(owq) ;
            size_t buffer_index ;
            
            for ( buffer_index = 0 ; buffer_index < OWQ_size(owq) ; buffer_index += 8 ) {
                size_t copy_length = length_left ;
                if ( copy_length > 8 ) copy_length = 8 ;
                memcpy( &OWQ_buffer(owq)[buffer_index], pn->sn, copy_length ) ;
                length_left -= copy_length ;
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
/* called when pn->extension==EXTENSION_ALL and pn->ft->ag->combined==ag_separate */
static int FS_read_tester_array(struct one_wire_query * owq)
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
        read_or_error = FS_read_tester_single(owq_single) ;
        if ( read_or_error < 0 ) return read_or_error ;
        buffer_space_left -= read_or_error;
        pointer_into_buffer += read_or_error;
    }

    LEVEL_DEBUG("FS_readfake_all: size=%d buffer_space_left=%d used=%d\n", OWQ_size(owq), buffer_space_left,
                OWQ_size(owq) - buffer_space_left);
    return Fowq_output_offset_and_size( OWQ_buffer(owq), OWQ_size(owq) - buffer_space_left, owq ) ;
}
