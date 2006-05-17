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
#include "ow_2415.h"

/* ------- Prototypes ----------- */

/* DS2415/DS1904 Digital clock in a can */
 uREAD_FUNCTION( FS_r_counter ) ;
uWRITE_FUNCTION( FS_w_counter ) ;
 dREAD_FUNCTION( FS_r_date ) ;
dWRITE_FUNCTION( FS_w_date ) ;
 yREAD_FUNCTION( FS_r_run ) ;
yWRITE_FUNCTION( FS_w_run ) ;
 uREAD_FUNCTION( FS_r_flags ) ;
uWRITE_FUNCTION( FS_w_flags ) ;
 yREAD_FUNCTION( FS_r_enable ) ;
yWRITE_FUNCTION( FS_w_enable ) ;
 iREAD_FUNCTION( FS_r_interval ) ;
iWRITE_FUNCTION( FS_w_interval ) ;
 iREAD_FUNCTION( FS_r_itime ) ;
iWRITE_FUNCTION( FS_w_itime ) ;

/* ------- Structures ----------- */

struct filetype DS2415[] = {
    F_STANDARD   ,
    {"flags"     ,     1,  NULL, ft_unsigned, ft_stable  , {u:FS_r_flags}  , {u:FS_w_flags},   {v:NULL}, } ,
    {"running"   ,     1,  NULL, ft_yesno   , ft_stable  , {y:FS_r_run}    , {y:FS_w_run},     {v:NULL}, } ,
    {"udate"     ,    12,  NULL, ft_unsigned, ft_second  , {u:FS_r_counter}, {u:FS_w_counter}, {v:NULL}, } ,
    {"date"      ,    24,  NULL, ft_date    , ft_second  , {d:FS_r_date}   , {d:FS_w_date},    {v:NULL}, } ,
} ;
DeviceEntry( 24, DS2415 ) ;

struct filetype DS2417[] = {
    F_STANDARD   ,
    {"enable"    ,     1,  NULL, ft_yesno , ft_stable  , {y:FS_r_enable} , {y:FS_w_enable},   {v:NULL}, } ,
    {"interval"  ,     1,  NULL, ft_integer,ft_stable  , {i:FS_r_interval},{i:FS_w_interval}, {v:NULL}, } ,
    {"itime"     ,     1,  NULL, ft_integer,ft_stable  , {i:FS_r_itime}  , {i:FS_w_itime},    {v:NULL}, } ,
    {"running"   ,     1,  NULL, ft_yesno , ft_stable  , {y:FS_r_run}    , {y:FS_w_run},      {v:NULL}, } ,
    {"udate"     ,    12,  NULL, ft_unsigned,ft_second , {u:FS_r_counter}, {u:FS_w_counter},  {v:NULL}, } ,
    {"date"      ,    24,  NULL, ft_date  , ft_second  , {d:FS_r_date}   , {d:FS_w_date},     {v:NULL}, } ,
} ;
DeviceEntry( 27, DS2417 ) ;

static int itimes[] = { 1, 4, 32, 64, 2048, 4096, 65536, 131072, } ;

/* ------- Functions ------------ */
/* DS2415/DS1904 Digital clock in a can */

/* DS1904 */
static int OW_r_clock( DATE * d , const struct parsedname * pn ) ;
static int OW_r_control( BYTE * cr , const struct parsedname * pn ) ;
static int OW_w_clock( const DATE d , const struct parsedname * pn ) ;
static int OW_w_control( const BYTE cr , const struct parsedname * pn ) ;

/* set clock */
static int FS_w_counter(const UINT * u , const struct parsedname * pn) {
    if ( OW_w_clock( (DATE) u[0] , pn ) ) return -EINVAL ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const DATE *d , const struct parsedname * pn) {
    if ( OW_w_clock( d[0], pn ) ) return -EINVAL ;
    return 0 ;
}

/* write running */
static int FS_w_run(const int * y , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_control( &cr, pn) || OW_w_control( (BYTE) (y[0]?cr|0x0C:cr&0xF3), pn) ) return -EINVAL ;
    return 0 ;
}

/* write running */
static int FS_w_enable(const int * y , const struct parsedname * pn) {
    BYTE cr ;

    if ( OW_r_control( &cr, pn) || OW_w_control( (BYTE) (y[0]?cr|0x80:cr&0x7F), pn) ) return -EINVAL ;
    return 0 ;
}

/* write flags */
static int FS_w_flags(const UINT * u , const struct parsedname * pn) {
    BYTE cr ;

    if ( OW_r_control( &cr, pn) || OW_w_control( (BYTE) ((cr&0x0F)|((((UINT)u[0])&0x0F)<<4)), pn) ) return -EINVAL ;
    return 0 ;
}

/* write flags */
static int FS_w_interval(const int * i , const struct parsedname * pn) {
    BYTE cr ;

    if ( OW_r_control( &cr, pn) || OW_w_control( (BYTE) ((cr&0x8F)|((((UINT)i[0])&0x07)<<4)), pn) ) return -EINVAL ;
    return 0 ;
}

/* write flags */
static int FS_w_itime(const int * i , const struct parsedname * pn) {
    BYTE cr ;

    if ( OW_r_control( &cr, pn) ) return -EINVAL ;

    if ( i[0] == 0 ) {
        cr &= 0x7F ; /* disable */
    } else if ( i[0] == 1 ) {
        cr = (cr&0x8F) | 0x00 ; /* set interval */
    } else if ( i[0] <= 4 ) {
        cr = (cr&0x8F) | 0x10 ; /* set interval */
    } else if ( i[0] <= 32 ) {
        cr = (cr&0x8F) | 0x20 ; /* set interval */
    } else if ( i[0] <= 64 ) {
        cr = (cr&0x8F) | 0x30 ; /* set interval */
    } else if ( i[0] <= 2048 ) {
        cr = (cr&0x8F) | 0x40 ; /* set interval */
    } else if ( i[0] <= 4096 ) {
        cr = (cr&0x8F) | 0x50 ; /* set interval */
    } else if ( i[0] <= 65536 ) {
        cr = (cr&0x8F) | 0x60 ; /* set interval */
    } else {
        cr = (cr&0x8F) | 0x70 ; /* set interval */
    }

    if ( OW_w_control(cr,pn) ) return -EINVAL ;
    return 0 ;
}

/* read flags */
int FS_r_flags(UINT * u , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * u = cr>>4 ;
    return 0 ;
}

/* read flags */
int FS_r_interval(int * i , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * i = (cr>>4)&0x07 ;
    return 0 ;
}

/* read flags */
int FS_r_itime(int * i , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * i = itimes[(cr>>4)&0x07] ;
    return 0 ;
}

/* read running */
int FS_r_run(int * y , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * y = ((cr&0x08)!=0) ;
    return 0 ;
}

/* read running */
int FS_r_enable(int * y , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * y = ((cr&0x80)!=0) ;
    return 0 ;
}

/* read clock */
int FS_r_counter(UINT * u , const struct parsedname * pn) {
    return OW_r_clock( (DATE *) u, pn) ? -EINVAL : 0 ;
}

/* read clock */
int FS_r_date( DATE * d , const struct parsedname * pn) {
    if ( OW_r_clock(d,pn) ) return -EINVAL ;
    return 0 ;
}

/* 1904 clock-in-a-can */
static int OW_r_control( BYTE * cr , const struct parsedname * pn ) {
    BYTE r[] = { 0x66, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { r, NULL, 1, trxn_match, } ,
        { NULL, cr, 1, trxn_read, } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;

    return 0 ;
}

/* 1904 clock-in-a-can */
static int OW_r_clock( DATE * d , const struct parsedname * pn ) {
    BYTE r[] = { 0x66, } ;
    BYTE data[5] ;
    struct transaction_log t[] = {
        TRXN_START,
        { r, NULL, 1, trxn_match, } ,
        { NULL, data, 5, trxn_read, } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;

//    d[0] = (((((((UINT) data[4])<<8)|data[3])<<8)|data[2])<<8)|data[1] ;
    d[0] = UT_toDate( &data[1] ) ;
    return 0 ;
}

static int OW_w_clock( const DATE d , const struct parsedname * pn ) {
    BYTE r[] = { 0x66, } ;
    BYTE w[6] = { 0x99, } ;
    struct transaction_log tread[] = {
        TRXN_START,
        { r, NULL, 1, trxn_match, } ,
        { NULL, &w[1], 1, trxn_read, } ,
        TRXN_END,
    } ;
    struct transaction_log twrite[] = {
        TRXN_START,
        { w, NULL, 6, trxn_match, } ,
        TRXN_END,
    } ;

    /* read in existing control byte to preserve bits 4-7 */
    if ( BUS_transaction( tread, pn ) ) return 1 ;

    UT_fromDate( d, &w[2] ) ;
    
    if ( BUS_transaction( twrite, pn ) ) return 1 ;
    return 0 ;
}

static int OW_w_control( const BYTE cr , const struct parsedname * pn ) {
    BYTE w[2] = { 0x99, cr, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { w, NULL, 2, trxn_match, } ,
        TRXN_END,
    } ;

    /* read in existing control byte to preserve bits 4-7 */
    if ( BUS_transaction( t, pn ) ) return 1 ;

    return 0 ;
}
