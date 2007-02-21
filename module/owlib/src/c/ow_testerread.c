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
static int FS_read_tester_single(char *buf, const size_t size,
                               const off_t offset,
                               const struct parsedname *pn);
static int FS_read_tester_array(char *buf, const size_t size,
                              const off_t offset,
                              const struct parsedname *pn);

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

int FS_read_tester(char *buf, const size_t size, const off_t offset,
                 const struct parsedname *pn)
{
    switch (pn->extension) {
        case -1:                    /* array */
            return FS_read_tester_array(buf, size, offset, pn);
        case -2:                    /* bitfield */
        default:
            return FS_read_tester_single(buf, size, offset, pn);
    }
}

static int FS_read_tester_single(char *buf, const size_t size,
                               const off_t offset,
                               const struct parsedname *pn)
{
    int tester_bus = (pn->sn[2]<<8) + pn->sn[1] ;
    int device     = (pn->sn[6]<<8) + pn->sn[5] ;
    int family_code=  pn->sn[0] ;
    int calculated_value = family_code + tester_bus + device ;

    /* Mounting fuse with "direct_io" will cause a second read with offset
    * at end-of-file... Just return 0 if offset == size */
    if (offset > (off_t) 0) {
        return -ERANGE;
    }
    
    switch (pn->ft->format) {
        case ft_integer:{
            int i = calculated_value ;
            LEVEL_DEBUG("FS_parse_readfake: (integer) %d\n", i);
            return FS_output_integer(i, buf, size, pn);
        }
        case ft_bitfield:
            if (pn->extension != -2) {
                buf[0] = (calculated_value + pn->extension)&0x1 ? '1' : '0';
                return 1;
            } else {
                // alternating 1010 for index, shift if loworder should be 1
                UINT u = 0xAAAAAAAA >> (calculated_value&0x1) ;
                // mask for actual number of elements
                u |= ( (1<<(pn->ft->ag->elements)) - 1 ) ;
                LEVEL_DEBUG("FS_parse_readfake: (bitfield) %u\n", u);
                return FS_output_unsigned(u, buf, size, pn);
            }
        case ft_unsigned:{
            UINT u = calculated_value;
            LEVEL_DEBUG("FS_parse_readfake: (unsigned) %u\n", u);
            return FS_output_unsigned(u, buf, size, pn);
        }
        case ft_float:{
            _FLOAT f = (_FLOAT) calculated_value * 0.1;
            LEVEL_DEBUG("FS_parse_readfake: (float) %G\n", f);
            return FS_output_float(f, buf, size, pn);
        }
        case ft_temperature:{
            _FLOAT f = (_FLOAT) calculated_value * 0.1;
            LEVEL_DEBUG("FS_parse_readfake: (temperature) %G\n", f);
            return FS_output_float(Temperature(f, pn), buf, size, pn);
        }
        case ft_tempgap:{
            _FLOAT f = (_FLOAT) calculated_value * 0.1;
            LEVEL_DEBUG("FS_parse_readfake: (tempgap) %G\n", f);
            return FS_output_float(TemperatureGap(f, pn), buf, size, pn);
        }
        case ft_date:{
            _DATE d = 1174622400;
            LEVEL_DEBUG("FS_parse_readfake: (date) %lu\n",
                        (unsigned long int) d);
            return FS_output_date(d, buf, size, pn);
        }
        case ft_yesno:{
            int y = (calculated_value)&0x1;
            if (size < 1)
                return -EMSGSIZE;
            LEVEL_DEBUG("FS_parse_readfake: (yesno) %d\n", y);
            buf[0] = y ? '1' : '0';
            return 1;
        }
        case ft_vascii:
        case ft_ascii:{
            size_t s = SimpleFileLength(pn) ;
            size_t i ;
            if (s > size) {
                s = size;
            }
            buf[s-1] = '\0' ;
            for ( i=0 ; i<s ; i+=2 )
            {
                num2string( &buf[i], pn->sn[(i>>1)&0x07] ) ;
            }
            return s ;
        }
        case ft_binary:{
            size_t s = SimpleFileLength(pn) ;
            size_t i ;
            if (s > size) {
                s = size;
            }
            for ( i=0 ; i<s ; ++i )
            {
                buf[i] =  pn->sn[i&0x07] ;
            }
            return s ;
        }
        case ft_directory:
        case ft_subdir:
            return -ENOSYS;
        default:
            return -ENOENT;
    }
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->ft->ag->combined==ag_separate */
static int FS_read_tester_array(char *buf, const size_t size,
                              const off_t offset,
                              const struct parsedname *pn)
{
    size_t left = size;
    char *p ;
    struct parsedname pn2;
    size_t s = SimpleFullFileLength(pn);

    STAT_ADD1(read_array);      /* statistics */

    if (offset > (off_t) s)
        return -ERANGE;
    if (offset == (off_t) s)
        return 0;

    /* shallow copy */
    memcpy(&pn2, pn, sizeof(struct parsedname));

    for (pn2.extension = 0, p=buf ; pn2.extension < pn2.ft->ag->elements ; ++pn2.extension) {
        int r;
        /* Add a separating comma if not the first element */
        if (pn2.extension && pn2.ft->format != ft_binary) {
            if (left == 0)
                return -ERANGE;
            *p = ',';
            ++p;
            --left;
        }
        if ((r = FS_read_tester_single(p, left, (const off_t) 0, &pn2)) < 0)
            return r;
        left -= r;
        p += r;
    }

    LEVEL_DEBUG("FS_readtester_all: size=%d left=%d sz=%d\n", size, left, size - left ) ;
    
    if ((size > left) && offset) {
        memcpy(buf, &buf[offset], size - left - (size_t) offset);
        return size - left - offset;
    }
    return size - left;
}
