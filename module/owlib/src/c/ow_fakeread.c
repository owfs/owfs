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
static int FS_read_fake_single(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_read_fake_array(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

#define Random (((FLOAT)rand())/RAND_MAX)
#define Random_y (rand()&0x01)
#define Random_t (Random*100)
#define Random_d (time(NULL)*(1-.1*Random))
#define Random_i (rand()&0xFF)
#define Random_u (rand()&0xFF)
#define Random_b (rand()&0xFF)
#define Random_a (32+(rand()&0x3F))
#define Random_f (10*Random)

int FS_read_fake( char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    switch (pn->extension) {
    case -1: /* array */
        return FS_read_fake_array(buf, size, offset, pn ) ;
    case -2: /* bitfield */
    default:
        return FS_read_fake_single(buf, size, offset, pn ) ;
    }
}

static int FS_read_fake_single( char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    int sz ;
    size_t s = 0 ;

    /* Mounting fuse with "direct_io" will cause a second read with offset
     * at end-of-file... Just return 0 if offset == size */
    s = SimpleFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;
   
    switch( pn->ft->format ) {
        case ft_integer: {
            int i = Random_i ;
            LEVEL_DEBUG("FS_parse_readfake: (integer) %d\n", i ) ;
            sz = FS_output_integer( i , buf , size , pn ) ;
            break;
            }
        case ft_bitfield:
            if ( pn->extension != -2 ) {
                buf[0] = Random_y ? '1' : '0' ;
                return 1 ;
            } else {
                UINT u = rand() % pn->ft->ag->elements ;
                LEVEL_DEBUG("FS_parse_readfake: (bitfield) %u\n", u ) ;
                sz = FS_output_unsigned( u , buf , size , pn ) ;
                break ;
            }
        case ft_unsigned: {
            UINT u = Random_u ;
            LEVEL_DEBUG("FS_parse_readfake: (unsigned) %u\n", u ) ;
            sz = FS_output_unsigned( u , buf , size , pn ) ;
            break ;
            }
        case ft_float: {
            FLOAT f = Random_f ;
            LEVEL_DEBUG("FS_parse_readfake: (float) %G\n", f ) ;
            sz = FS_output_float( f , buf , size , pn ) ;
            break ;
            }
        case ft_temperature: {
            FLOAT f = Random_t ;
            LEVEL_DEBUG("FS_parse_readfake: (temperature) %G\n", f ) ;
            sz = FS_output_float( Temperature(f,pn) , buf , size , pn ) ;
            break ;
            }
        case ft_tempgap: {
            FLOAT f = Random_t ;
            LEVEL_DEBUG("FS_parse_readfake: (tempgap) %G\n", f ) ;
            sz = FS_output_float( TemperatureGap(f,pn) , buf , size , pn ) ;
            break ;
            }
        case ft_date: {
            DATE d = Random_d ;
            LEVEL_DEBUG("FS_parse_readfake: (date) %lu\n", (unsigned long int) d ) ;
            sz = FS_output_date( d , buf , size , pn ) ;
            break;
            }
        case ft_yesno: {
            int y = Random_y ;
            if (size < 1) return -EMSGSIZE ;
            LEVEL_DEBUG("FS_parse_readfake: (yesno) %d\n", y ) ;
            buf[0] = y ? '1' : '0' ;
            return 1 ;
            }
        case ft_vascii:
        case ft_ascii: {
            //size_t s = SimpleFileLength(pn) ;
            //if ( offset > s ) return -ERANGE ;
            s -= offset ;
            if ( s > size ) s = size ;
            {
                size_t i ;
                for ( i=0 ; i<s ; ++i ) buf[i] = Random_a ;
            }
            return (pn->ft->read.a)(buf,s,offset,pn) ;
            }
        case ft_binary: {
            //size_t s = SimpleFileLength(pn) ;
            //if ( offset > s ) return -ERANGE ;
            s -= offset ;
            if ( s > size ) s = size ;
            {
                size_t i ;
                for ( i=0 ; i<s ; ++i ) buf[i] = Random_b ;
            }
            return (pn->ft->read.b)((BYTE*)buf,s,offset,pn) ;
            }
        case ft_directory:
        case ft_subdir:
            return -ENOSYS ;
        default:
            return -ENOENT ;
    }

    /* Return correct buffer according to offset for most data-types here */
    if((sz > 0) && offset) {
      memcpy(buf, &buf[offset], (size_t)sz - (size_t)offset);
      return sz - offset;
    }
    return sz ;
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->ft->ag->combined==ag_separate */
static int FS_read_fake_array(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t left = size ;
    char * p = buf ;
    int r ;
    struct parsedname pn2 ;
    size_t s, sz;

    STAT_ADD1(read_array) ; /* statistics */

    s = SimpleFullFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;

    /* shallow copy */
    memcpy( &pn2 , pn , sizeof(struct parsedname) ) ;

    for ( pn2.extension=0 ; pn2.extension<pn2.ft->ag->elements ; ++pn2.extension ) {
        /* Add a separating comma if not the first element */
        if (pn2.extension && pn2.ft->format!=ft_binary) {
            if ( left == 0 ) return -ERANGE ;
            *p = ',' ;
            ++ p ;
            -- left ;
        }
        if ( (r=FS_read_fake_single(p,left,(const off_t)0,&pn2)) < 0 ) return r ;
        left -= r ;
        p += r ;
    }
    sz = size - left ;

    LEVEL_DEBUG("FS_readfake_all: size=%d left=%d sz=%d\n", size, left, sz);
    if((sz > 0) && offset) {
        memcpy(buf, &buf[offset], sz - (size_t)offset);
        return sz - offset;
    }
    return size - left ;
}

