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
#include "ow_2415.h"

/* ------- Prototypes ----------- */

/* DS2415/DS1904 Digital clock in a can */
 uREAD_FUNCTION( FS_r_counter ) ;
uWRITE_FUNCTION( FS_w_counter ) ;
 aREAD_FUNCTION( FS_r_date ) ;
aWRITE_FUNCTION( FS_w_date ) ;
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
    {"flags"     ,     1,  NULL, ft_unsigned, ft_stable  , {u:FS_r_flags}  , {u:FS_w_flags}, NULL, } ,
    {"running"   ,     1,  NULL, ft_yesno   , ft_stable  , {y:FS_r_run}    , {y:FS_w_run}, NULL,   } ,
    {"counter"   ,    12,  NULL, ft_unsigned, ft_second  , {u:FS_r_counter}, {u:FS_w_counter}, NULL, } ,
    {"date"      ,    26,  NULL, ft_ascii   , ft_second  , {a:FS_r_date}   , {a:FS_w_date}, NULL, } ,
} ;
DeviceEntry( 24, DS2415 )

struct filetype DS2417[] = {
    F_STANDARD   ,
    {"enable"    ,     1,  NULL, ft_yesno , ft_stable  , {y:FS_r_enable} , {y:FS_w_enable}, NULL, } ,
    {"interval"  ,     1,  NULL, ft_integer,ft_stable  , {i:FS_r_interval},{i:FS_w_interval}, NULL, } ,
    {"itime"     ,     1,  NULL, ft_integer,ft_stable  , {i:FS_r_itime}  , {i:FS_w_itime}, NULL, } ,
    {"running"   ,     1,  NULL, ft_yesno , ft_stable  , {y:FS_r_run}    , {y:FS_w_run}, NULL,   } ,
    {"counter"   ,    12,  NULL, ft_unsigned,ft_second , {u:FS_r_counter}, {u:FS_w_counter}, NULL, } ,
    {"date"      ,    26,  NULL, ft_ascii , ft_second  , {a:FS_r_date}   , {a:FS_w_date}, NULL, } ,
} ;
DeviceEntry( 27, DS2417 )

static int itimes[] = { 1, 4, 32, 64, 2048, 4096, 65536, 131072, } ;

/* ------- Functions ------------ */
/* DS2415/DS1904 Digital clock in a can */

/* DS1904 */
static int OW_r_clock( unsigned int * num , const struct parsedname * pn ) ;
static int OW_r_control( unsigned char * cr , const struct parsedname * pn ) ;
static int OW_w_clock( const unsigned int num , const struct parsedname * pn ) ;
static int OW_w_control( const unsigned char cr , const struct parsedname * pn ) ;

/* set clock */
static int FS_w_counter(const unsigned int * u , const struct parsedname * pn) {
    if ( OW_w_clock(u[0],pn) ) return -EINVAL ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    struct tm tm ;

    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset != 0 ) return -EFAULT ;
    if ( size==0 || buf[0]=='\0' ) {
        if ( OW_w_clock( (unsigned int) time(NULL), pn) ) return -EINVAL ;
        return 0 ;
    } else if ( strptime(buf,"%a %b %d %T %Y",&tm) || strptime(buf,"%b %d %T %Y",&tm) || strptime(buf,"%c",&tm) || strptime(buf,"%D %T",&tm) ) {
        if ( OW_w_clock( (unsigned int) mktime(&tm), pn) ) return -EINVAL ;
        return 0 ;
    }
    return -EINVAL ;
}

/* write running */
static int FS_w_run(const int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_control( &cr, pn) || OW_w_control( (unsigned char) (y[0]?cr|0x0C:cr&0xF3), pn) ) return -EINVAL ;
    return 0 ;
}

/* write running */
static int FS_w_enable(const int * y , const struct parsedname * pn) {
    unsigned char cr ;

    if ( OW_r_control( &cr, pn) || OW_w_control( (unsigned char) (y[0]?cr|0x80:cr&0x7F), pn) ) return -EINVAL ;
    return 0 ;
}

/* write flags */
static int FS_w_flags(const unsigned int * u , const struct parsedname * pn) {
    unsigned char cr ;

    if ( OW_r_control( &cr, pn) || OW_w_control( (unsigned char) ((cr&0x0F)|((((unsigned int)u[0])&0x0F)<<4)), pn) ) return -EINVAL ;
    return 0 ;
}

/* write flags */
static int FS_w_interval(const int * i , const struct parsedname * pn) {
    unsigned char cr ;

    if ( OW_r_control( &cr, pn) || OW_w_control( (unsigned char) ((cr&0x8F)|((((unsigned int)i[0])&0x07)<<4)), pn) ) return -EINVAL ;
    return 0 ;
}

/* write flags */
static int FS_w_itime(const int * i , const struct parsedname * pn) {
    unsigned char cr ;

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
int FS_r_flags(unsigned int * u , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * u = cr>>4 ;
    return 0 ;
}

/* read flags */
int FS_r_interval(int * i , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * i = (cr>>4)&0x07 ;
    return 0 ;
}

/* read flags */
int FS_r_itime(int * i , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * i = itimes[(cr>>4)&0x07] ;
    return 0 ;
}

/* read running */
int FS_r_run(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * y = ((cr&0x08)!=0) ;
    return 0 ;
}

/* read running */
int FS_r_enable(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_control(&cr,pn) ) return -EINVAL ;
    * y = ((cr&0x80)!=0) ;
    return 0 ;
}

/* read clock */
int FS_r_counter(unsigned int * u , const struct parsedname * pn) {
    return OW_r_clock(u,pn) ? -EINVAL : 0 ;
}

/* read clock */
int FS_r_date(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned int num ; /* must me 32bit (or more) */
    if ( offset ) return -EFAULT ;
    if ( size<26 ) return -EMSGSIZE ;

    if ( OW_r_clock(&num,pn) ) return -EINVAL ;
    if ( ctime_r((time_t *) &num,buf) ) return 24 ;
    return -EINVAL ;
}

/* 1904 clock-in-a-can */
static int OW_r_control( unsigned char * cr , const struct parsedname * pn ) {
    unsigned char r = 0x66 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &r , 1 ) || BUS_readin_data( cr , 1 ) ;
    BUS_unlock() ;
    return ret ;
}

/* 1904 clock-in-a-can */
static int OW_r_clock( unsigned int * num , const struct parsedname * pn ) {
    unsigned char r = 0x66 ;
    unsigned char data[5] ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &r , 1 ) || BUS_readin_data( data , 5 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    *num = (((((((unsigned int) data[4])<<8)|data[3])<<8)|data[2])<<8)|data[1] ;
    return 0 ;
}

static int OW_w_clock( const unsigned int num , const struct parsedname * pn ) {
    unsigned char r = 0x66 ;
    unsigned char w[6] = { 0x99, } ;
    int ret ;

    /* read in existing control byte to preserve bits 4-7 */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &r , 1 ) || BUS_readin_data( &w[1] , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    w[2] = num & 0xFF ;
    w[3] = (num>>8) & 0xFF ;
    w[4] = (num>>16) & 0xFF ;
    w[5] = (num>>24) & 0xFF ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 6 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_control( const unsigned char cr , const struct parsedname * pn ) {
    unsigned char w[2] = { 0x99, cr, } ;
    int ret ;
    
    /* read in existing control byte to preserve bits 4-7 */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) ;
    BUS_unlock() ;
    return ret ;
}




