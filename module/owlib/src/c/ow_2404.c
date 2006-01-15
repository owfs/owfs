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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
	( or a closely related family of chips )

	The connection to the larger program is through the "device" data structure,
	  which must be declared in the acompanying header file.

	The device structure holds the
	  family code,
	  name,
	  device type (chip, interface or pseudo)
	  number of properties,
	  list of property structures, called "filetype".

	Each filetype structure holds the
	  name,
	  estimated length (in bytes),
	  aggregate structure pointer,
	  data format,
	  read function,
	  write funtion,
	  generic data pointer

	The aggregate structure, is present for properties that several members
	(e.g. pages of memory or entries in a temperature log. It holds:
	  number of elements
	  whether the members are lettered or numbered
	  whether the elements are stored together and split, or separately and joined
*/

#include "owfs_config.h"
#include "ow_2404.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;
 uREAD_FUNCTION( FS_r_alarm ) ;
 uREAD_FUNCTION( FS_r_set_alarm ) ;
uWRITE_FUNCTION( FS_w_set_alarm ) ;
 uREAD_FUNCTION( FS_r_counter4 ) ;
uWRITE_FUNCTION( FS_w_counter4 ) ;
 uREAD_FUNCTION( FS_r_counter5 ) ;
uWRITE_FUNCTION( FS_w_counter5 ) ;
 dREAD_FUNCTION( FS_r_date ) ;
dWRITE_FUNCTION( FS_w_date ) ;
 yREAD_FUNCTION( FS_r_flag ) ;
yWRITE_FUNCTION( FS_w_flag ) ;

/* ------- Structures ----------- */

struct aggregate A2404 = { 16, ag_numbers, ag_separate, } ;
struct filetype DS2404[] = {
    F_STANDARD          ,
    {"alarm"            ,     4,  NULL,   ft_unsigned, ft_volatile, {u:FS_r_alarm} , {v:NULL} , NULL , } ,
    {"set_alarm"        ,     4,  NULL,   ft_unsigned, ft_stable,   {u:FS_r_set_alarm} , {u:FS_w_set_alarm} , NULL , } ,
    {"pages"            ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page"       ,    32,  &A2404, ft_binary  , ft_stable  , {b:FS_r_page}  , {b:FS_w_page}  , NULL, } ,
    {"memory"           ,   512,  NULL,   ft_binary  , ft_stable, {b:FS_r_memory} , {b:FS_w_memory} , NULL, } ,
    {"running"          ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x10, } ,
    {"auto"             ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x20, } ,
    {"start"            ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x40, } ,
    {"delay"            ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x80, } ,
    {"date"             ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}     , (void *) (size_t) 0x202, } ,
    {"udate"            ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x202 , } ,
    {"interval"         ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}     , (void *) (size_t) 0x207, } ,
    {"uinterval"        ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x207 , } ,
    {"cycle"            ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter4}, {u:FS_w_counter4}, (void *) (size_t) 0x20C , } ,
    {"trigger"          ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"trigger/date"     ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}   , (void *) (size_t) 0x210, } ,
    {"trigger/udate"    ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x210 , } ,
    {"trigger/interval" ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}   , (void *) (size_t) 0x215, } ,
    {"trigger/uinterval",    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x215 , } ,
    {"trigger/cycle"    ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter4}, {u:FS_w_counter4}, (void *) (size_t) 0x21A , } ,
    {"readonly"         ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"readonly/memory"  ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x08, } ,
    {"readonly/cycle"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x04, } ,
    {"readonly/interval",     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x02, } ,
    {"readonly/clock"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x01, } ,
} ;
DeviceEntryExtended( 04, DS2404 , DEV_alarm ) ;

struct filetype DS2404S[] = {
    F_STANDARD          ,
    {"alarm"            ,     4,  NULL,   ft_unsigned, ft_volatile, {u:FS_r_alarm} , {v:NULL} , NULL , } ,
    {"set_alarm"        ,     4,  NULL,   ft_unsigned, ft_stable,   {u:FS_r_set_alarm} , {u:FS_w_set_alarm} , NULL , } ,
    {"pages"            ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page"       ,    32,  &A2404, ft_binary  , ft_stable  , {b:FS_r_page}  , {b:FS_w_page}  , NULL, } ,
    {"memory"           ,   512,  NULL,   ft_binary  , ft_stable, {b:FS_r_memory} , {b:FS_w_memory} , NULL, } ,
    {"running"          ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x10, } ,
    {"auto"             ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x20, } ,
    {"start"            ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x40, } ,
    {"delay"            ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x80, } ,
    {"date"             ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}     , (void *) (size_t) 0x202, } ,
    {"udate"            ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x202 , } ,
    {"interval"         ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}     , (void *) (size_t) 0x207, } ,
    {"uinterval"        ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x207 , } ,
    {"cycle"            ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter4}, {u:FS_w_counter4}, (void *) (size_t) 0x20C , } ,
    {"trigger"          ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"trigger/date"     ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}   , (void *) (size_t) 0x210, } ,
    {"trigger/udate"    ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x210 , } ,
    {"trigger/interval" ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}   , (void *) (size_t) 0x215, } ,
    {"trigger/uinterval",    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter5}, {u:FS_w_counter5}, (void *) (size_t) 0x215 , } ,
    {"trigger/cycle"    ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter4}, {u:FS_w_counter4}, (void *) (size_t) 0x21A , } ,
    {"readonly"         ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"readonly/memory"  ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x08, } ,
    {"readonly/cycle"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x04, } ,
    {"readonly/interval",     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x02, } ,
    {"readonly/clock"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x01, } ,
} ;
DeviceEntryExtended( 84, DS2404S  , DEV_alarm ) ;

/* ------- Functions ------------ */

/* DS1902 */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_r_ulong( unsigned long int * L, const size_t size, const size_t offset , const struct parsedname * pn ) ;
static int OW_w_ulong( const unsigned long int * L, const size_t size, const size_t offset , const struct parsedname * pn ) ;

static unsigned int Avals[] = { 0, 1, 10, 11, 100, 101, 110, 111, } ;

/* 1902 */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EINVAL ;
    return size ;
}

static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is consecutive, unchecked. No paging */
    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EFAULT ;
    return 0 ;
}

static int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* paged write */
//    if ( OW_w_mem( buf, size, (size_t) offset, pn) ) return -EFAULT ;
    if ( OW_write_paged( buf, size, (size_t) offset, pn, 32, OW_w_mem ) ) return -EFAULT ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const DATE * d , const struct parsedname * pn) {
    const unsigned int D = (unsigned int) d[0] ;
    return FS_w_counter5( &D, pn ) ;
}

/* read clock */
static int FS_r_date( DATE * d , const struct parsedname * pn) {
    unsigned int D ;
    if ( FS_r_counter5(&D,pn) ) return -EINVAL ;
    d[0] = (DATE) D ;
    return 0 ;
}

/* set clock */
static int FS_w_counter5(const unsigned int *u, const struct parsedname * pn) {
    unsigned long int L = ( (unsigned long int) u[0] ) << 8 ;
    return OW_w_ulong( &L, 5, (size_t) pn->ft->data, pn ) ;
}

/* set clock */
static int FS_w_counter4(const unsigned int *u, const struct parsedname * pn) {
    unsigned long int L = ( (unsigned long int) u[0] ) ;
    return OW_w_ulong( &L, 4, (size_t) pn->ft->data, pn ) ;
}

/* read clock */
static int FS_r_counter5(unsigned int *u, const struct parsedname * pn) {
    unsigned long int L ;
    if ( OW_r_ulong( &L, 5, (size_t) pn->ft->data, pn ) ) return -EINVAL ;
    u[0] = L>>8 ;
    return 0 ;
}

/* read clock */
static int FS_r_counter4(unsigned int *u, const struct parsedname * pn) {
    unsigned long int L ;
    if ( OW_r_ulong( &L, 4, (size_t) pn->ft->data, pn ) ) return -EINVAL ;
    u[0] = L ;
    return 0 ;
}

/* alarm */
static int FS_w_set_alarm(const unsigned int *u, const struct parsedname * pn) {
    unsigned char c ;
    switch ( u[0] ) {
    case   0:   c=0<<3 ;   break ;
    case   1:   c=1<<3 ;   break ;
    case  10:   c=2<<3 ;   break ;
    case  11:   c=3<<3 ;   break ;
    case 100:   c=4<<3 ;   break ;
    case 101:   c=5<<3 ;   break ;
    case 110:   c=6<<3 ;   break ;
    case 111:   c=7<<3 ;   break ;
    default: return -ERANGE ;
    }
    if ( OW_w_mem( &c, 1, 0x200, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_alarm( unsigned int * u, const struct parsedname * pn ) {
    unsigned char c ;
    if ( OW_r_mem(&c,1,0x200,pn) ) return -EINVAL ;
    u[0] = Avals[c&0x07] ;
    return 0 ;
}

static int FS_r_set_alarm( unsigned int * u, const struct parsedname * pn ) {
    unsigned char c ;
    if ( OW_r_mem(&c,1,0x200,pn) ) return -EINVAL ;
    u[0] = Avals[(c>>3)&0x07] ;
    return 0 ;
}

/* write flag */
static int FS_w_flag(const int * y , const struct parsedname * pn) {
    unsigned char cr ;
    unsigned char fl = (unsigned char) pn->ft->data ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    if ( y[0] ) {
        if ( cr & fl ) return 0 ;
    } else {
        if ( (cr & fl) == 0 ) return 0 ;
    }
    cr ^= fl ; /* flip the bit */
    if ( OW_w_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    return 0 ;
}

/* read flag */
static int FS_r_flag(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    unsigned char fl = (unsigned char) pn->ft->data ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    y[0] = (cr&fl)?1:0 ;
    return 0 ;
}

/* PAged access -- pre-screened */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[4+32] = { 0x0F, offset&0xFF , offset>>8, } ;
    int ret ;

    /* Copy to scratchpad */
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,3,pn) || BUS_send_data(data,size,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,1,pn) || BUS_readin_data(&p[1],3+size,pn) || memcmp(&p[4], data, size) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,4,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) {
    unsigned char p[3] = { 0xF0, offset&0xFF , offset>>8, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p, 3,pn) || BUS_readin_data(data, (int) size,pn) ;
    BUSUNLOCK(pn);
    return ret ;

}

/* read 4 or 5 byte number */
static int OW_r_ulong( unsigned long int * L, const size_t size, const size_t offset , const struct parsedname * pn ) {
    unsigned char data[5] = {0x00, 0x00, 0x00, 0x00, 0x00, } ;
    if ( size > 5 ) return -ERANGE ;
    if ( OW_r_mem( data, size, offset, pn ) ) return -EINVAL ;
    L[0] = ((unsigned long int) data[0]) 
         + (((unsigned long int) data[1])<<8) 
         + (((unsigned long int) data[2])<<16) 
         + (((unsigned long int) data[3])<<24) 
         + (((unsigned long int) data[4])<<32) ;
    return 0 ;
}

/* write 4 or 5 byte number */
static int OW_w_ulong( const unsigned long int * L, const size_t size, const size_t offset , const struct parsedname * pn ) {
    unsigned char data[5] = {0x00, 0x00, 0x00, 0x00, 0x00, } ;
    if ( size > 5 ) return -ERANGE ;
    data[0] = L[0] & 0xFF ;
    data[1] = (L[0]>>8) & 0xFF ;
    data[2] = (L[0]>>16) & 0xFF ;
    data[3] = (L[0]>>24) & 0xFF ;
    data[4] = (L[0]>>32) & 0xFF ;
    if ( OW_w_mem( data, size, offset, pn ) ) return -EINVAL ;
    return 0 ;
}

