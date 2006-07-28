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

#include <config.h>
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
    {"alarm"            ,   4, NULL,  ft_unsigned,fc_volatile,{u:FS_r_alarm},    {v:NULL},          {v:NULL}, } ,
    {"set_alarm"        ,   4, NULL,  ft_unsigned,fc_stable,  {u:FS_r_set_alarm},{u:FS_w_set_alarm},{v:NULL}, } ,
    {"pages"            ,   0, NULL,  ft_subdir  ,fc_volatile,{v:NULL},          {v:NULL},          {v:NULL}, } ,
    {"pages/page"       ,  32, &A2404,ft_binary  ,fc_stable,  {b:FS_r_page},     {b:FS_w_page},     {v:NULL}, } ,
    {"memory"           , 512, NULL,  ft_binary  ,fc_stable,  {b:FS_r_memory},   {b:FS_w_memory},   {v:NULL}, } ,
    {"running"          ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x10}, } ,
    {"auto"             ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x20}, } ,
    {"start"            ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x40}, } ,
    {"delay"            ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x80}, } ,
    {"date"             ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x202}, } ,
    {"udate"            ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x202}, } ,
    {"interval"         ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x207}, } ,
    {"uinterval"        ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x207}, } ,
    {"cycle"            ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter4}, {u:FS_w_counter4}, {s: 0x20C}, } ,
    {"trigger"          ,   0, NULL,  ft_subdir  ,fc_volatile,{v:NULL},          {v:NULL},          {v:NULL}, },
    {"trigger/date"     ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x210}, } ,
    {"trigger/udate"    ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x210}, } ,
    {"trigger/interval" ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x215}, } ,
    {"trigger/uinterval",  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x215}, } ,
    {"trigger/cycle"    ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter4}, {u:FS_w_counter4}, {s: 0x21A}, } ,
    {"readonly"         ,   0, NULL,  ft_subdir  ,fc_volatile,{v:NULL},          {v:NULL},          {v:NULL}, } ,
    {"readonly/memory"  ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x08}, } ,
    {"readonly/cycle"   ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x04}, } ,
    {"readonly/interval",   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x02}, } ,
    {"readonly/clock"   ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x01}, } ,
} ;
DeviceEntryExtended( 04, DS2404 , DEV_alarm ) ;

struct filetype DS2404S[] = {
    F_STANDARD          ,
    {"alarm"            ,   4, NULL,  ft_unsigned,fc_volatile,{u:FS_r_alarm},    {v:NULL},          {v:NULL}, } ,
    {"set_alarm"        ,   4, NULL,  ft_unsigned,fc_stable,  {u:FS_r_set_alarm},{u:FS_w_set_alarm},{v:NULL}, } ,
    {"pages"            ,   0, NULL,  ft_subdir  ,fc_volatile,{v:NULL},          {v:NULL},          {v:NULL}, } ,
    {"pages/page"       ,  32, &A2404,ft_binary  ,fc_stable,  {b:FS_r_page},     {b:FS_w_page},     {v:NULL}, } ,
    {"memory"           , 512, NULL,  ft_binary  ,fc_stable,  {b:FS_r_memory},   {b:FS_w_memory},   {v:NULL}, } ,
    {"running"          ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x10}, } ,
    {"auto"             ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x20}, } ,
    {"start"            ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x40}, } ,
    {"delay"            ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x80}, } ,
    {"date"             ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x202}, } ,
    {"udate"            ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x202}, } ,
    {"interval"         ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x207}, } ,
    {"uinterval"        ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x207}, } ,
    {"cycle"            ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter4}, {u:FS_w_counter4}, {s: 0x20C}, } ,
    {"trigger"          ,   0, NULL,  ft_subdir  ,fc_volatile,{v:NULL},          {v:NULL},          {v:NULL}, },
    {"trigger/date"     ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x210}, } ,
    {"trigger/udate"    ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x210}, } ,
    {"trigger/interval" ,  24, NULL,  ft_date    ,fc_second,  {d:FS_r_date},     {d:FS_w_date},     {s: 0x215}, } ,
    {"trigger/uinterval",  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter5}, {u:FS_w_counter5}, {s: 0x215}, } ,
    {"trigger/cycle"    ,  12, NULL,  ft_unsigned,fc_second,  {u:FS_r_counter4}, {u:FS_w_counter4}, {s: 0x21A}, } ,
    {"readonly"         ,   0, NULL,  ft_subdir  ,fc_volatile,{v:NULL},          {v:NULL},          {v:NULL}, } ,
    {"readonly/memory"  ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x08}, } ,
    {"readonly/cycle"   ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x04}, } ,
    {"readonly/interval",   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x02}, } ,
    {"readonly/clock"   ,   1, NULL,  ft_yesno   ,fc_stable,  {y:FS_r_flag},     {y:FS_w_flag},     {c: 0x01}, } ,
} ;
DeviceEntryExtended( 84, DS2404S  , DEV_alarm ) ;

/* ------- Functions ------------ */

/* DS1902 */
static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( BYTE * data, const size_t size, const off_t offset, const struct parsedname * pn ) ;
static int OW_r_ulong( uint64_t * L, const size_t size, const off_t offset , const struct parsedname * pn ) ;
static int OW_w_ulong( const uint64_t * L, const size_t size, const off_t offset , const struct parsedname * pn ) ;

static UINT Avals[] = { 0, 1, 10, 11, 100, 101, 110, 111, } ;

/* 1902 */
static int FS_r_page(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EINVAL ;
    return size ;
}

static int FS_r_memory(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is consecutive, unchecked. No paging */
    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EFAULT ;
    return 0 ;
}

static int FS_w_memory( const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* paged write */
//    if ( OW_w_mem( buf, size, (size_t) offset, pn) ) return -EFAULT ;
    if ( OW_write_paged( buf, size, (size_t) offset, pn, 32, OW_w_mem ) ) return -EFAULT ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const DATE * d , const struct parsedname * pn) {
    const UINT D = (UINT) d[0] ;
    return FS_w_counter5( &D, pn ) ;
}

/* read clock */
static int FS_r_date( DATE * d , const struct parsedname * pn) {
    UINT D ;
    if ( FS_r_counter5(&D,pn) ) return -EINVAL ;
    d[0] = (DATE) D ;
    return 0 ;
}

/* set clock */
static int FS_w_counter5(const UINT *u, const struct parsedname * pn) {
    uint64_t L = ( (uint64_t) u[0] ) << 8 ;
    return OW_w_ulong( &L, 5, pn->ft->data.s, pn ) ;
}

/* set clock */
static int FS_w_counter4(const UINT *u, const struct parsedname * pn) {
    uint64_t L = ( (uint64_t) u[0] ) ;
    return OW_w_ulong( &L, 4, pn->ft->data.s, pn ) ;
}

/* read clock */
static int FS_r_counter5(UINT *u, const struct parsedname * pn) {
    uint64_t L ;
    if ( OW_r_ulong( &L, 5, pn->ft->data.s, pn ) ) return -EINVAL ;
    u[0] = L>>8 ;
    return 0 ;
}

/* read clock */
static int FS_r_counter4(UINT *u, const struct parsedname * pn) {
    uint64_t L ;
    if ( OW_r_ulong( &L, 4, pn->ft->data.s, pn ) ) return -EINVAL ;
    u[0] = L ;
    return 0 ;
}

/* alarm */
static int FS_w_set_alarm(const UINT *u, const struct parsedname * pn) {
    BYTE c ;
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

static int FS_r_alarm( UINT * u, const struct parsedname * pn ) {
    BYTE c ;
    if ( OW_r_mem(&c,1,0x200,pn) ) return -EINVAL ;
    u[0] = Avals[c&0x07] ;
    return 0 ;
}

static int FS_r_set_alarm( UINT * u, const struct parsedname * pn ) {
    BYTE c ;
    if ( OW_r_mem(&c,1,0x200,pn) ) return -EINVAL ;
    u[0] = Avals[(c>>3)&0x07] ;
    return 0 ;
}

/* write flag */
static int FS_w_flag(const int * y , const struct parsedname * pn) {
    BYTE cr ;
    BYTE fl = pn->ft->data.c ;
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
    BYTE cr ;
    BYTE fl = pn->ft->data.c ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    y[0] = (cr&fl)?1:0 ;
    return 0 ;
}

/* PAged access -- pre-screened */
static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[4+32] = { 0x0F, offset&0xFF , offset>>8, } ;
    struct transaction_log tcopy[] = {
        TRXN_START,
        { p, NULL, 3, trxn_match } ,
        { data, NULL, size, trxn_match} ,
        TRXN_END,
    } ;
    struct transaction_log tread[] = {
        TRXN_START,
        { p, NULL, 1, trxn_match } ,
        { NULL, &p[1], size+3, trxn_read } ,
        TRXN_END,
    } ;
    struct transaction_log tsram[] = {
        TRXN_START,
        { p, NULL, 4, trxn_match } ,
        TRXN_END,
    } ;

    /* Copy to scratchpad */
    if ( BUS_transaction(tcopy, pn ) ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    if ( BUS_transaction(tread, pn ) ) return 1 ;
    if ( memcmp( &p[4], data, size ) ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    if ( BUS_transaction(tsram, pn ) ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( BYTE * data, const size_t size, const off_t offset, const struct parsedname * pn ) {
    BYTE p[3] = { 0xF0, offset&0xFF , offset>>8, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 3, trxn_match } ,
        { NULL, data, size, trxn_read} ,
        TRXN_END,
    } ;

    if ( BUS_transaction(t, pn ) ) return 1 ;

    return 0 ;
}

/* read 4 or 5 byte number */
static int OW_r_ulong( uint64_t * L, const size_t size, const off_t offset , const struct parsedname * pn ) {
    BYTE data[5] = {0x00, 0x00, 0x00, 0x00, 0x00, } ;
    if ( size > 5 ) return -ERANGE ;
    if ( OW_r_mem( data, size, offset, pn ) ) return -EINVAL ;
    L[0] = ((uint64_t) data[0])
         + (((uint64_t) data[1])<<8) 
         + (((uint64_t) data[2])<<16)
         + (((uint64_t) data[3])<<24)
         + (((uint64_t) data[4])<<32) ;
    return 0 ;
}

/* write 4 or 5 byte number */
static int OW_w_ulong( const uint64_t * L, const size_t size, const off_t offset , const struct parsedname * pn ) {
    BYTE data[5] = {0x00, 0x00, 0x00, 0x00, 0x00, } ;
    if ( size > 5 ) return -ERANGE ;
    data[0] = L[0] & 0xFF ;
    data[1] = (L[0]>>8) & 0xFF ;
    data[2] = (L[0]>>16) & 0xFF ;
    data[3] = (L[0]>>24) & 0xFF ;
    data[4] = (L[0]>>32) & 0xFF ;
    if ( OW_w_mem( data, size, offset, pn ) ) return -EINVAL ;
    return 0 ;
}

