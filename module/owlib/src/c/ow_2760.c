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

/* Changes
    7/2004 Extensive improvements based on input from Serg Oskin
*/

#include "owfs_config.h"
#include "ow_2760.h"
#include "thermocouple.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 fREAD_FUNCTION( FS_r_volt ) ;
 fREAD_FUNCTION( FS_r_temp ) ;
 fREAD_FUNCTION( FS_r_current ) ;
 fREAD_FUNCTION( FS_r_vis ) ;
 fREAD_FUNCTION( FS_r_accum ) ;
 fREAD_FUNCTION( FS_r_offset ) ;
 fREAD_FUNCTION( FS_thermocouple ) ;
fWRITE_FUNCTION( FS_w_offset ) ;
yWRITE_FUNCTION( FS_w_pio ) ;
 yREAD_FUNCTION( FS_sense ) ;

/* ------- Structures ----------- */

struct aggregate A2760p = { 16, ag_numbers, ag_separate, } ;
struct filetype DS2760[] = {
    F_STANDARD   ,
    {"memory"    ,   256,  NULL,    ft_binary  , ft_volatile, {b:FS_r_mem}    , {b:FS_w_mem} , NULL, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"pages/page",    16,  NULL,    ft_binary  , ft_volatile, {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"typeK"     ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & thermoK, } ,
    {"typeN"     ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & thermoN, } ,
    {"volt"      ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_r_volt}   , {v:NULL}     , NULL, } ,
    {"temperature",   12,  NULL,    ft_float   , ft_volatile, {f:FS_r_temp}   , {v:NULL}     , NULL, } ,
    {"current"   ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_r_current}, {v:NULL}     , NULL, } ,
    {"vis"       ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_r_vis}    , {v:NULL}     , NULL, } ,
    {"accumulator",   12,  NULL,    ft_float   , ft_volatile, {f:FS_r_accum}  , {v:NULL}     , NULL, } ,
    {"offset"    ,    12,  NULL,    ft_float   , ft_stable  , {f:FS_r_offset} , {f:FS_w_offset},NULL,} ,
    {"PIO"       ,     1,  NULL,    ft_yesno   , ft_volatile, {v:NULL}        , {y:FS_w_pio} , NULL, } ,
    {"sensed"    ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_sense}    , {v:NULL}     , NULL, } ,
} ;
DeviceEntry( 30, DS2760 )

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_sram( unsigned char * data , const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_w_eeprom( const size_t page, const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static FLOAT OW_type(FLOAT T, FLOAT V, const struct thermo * thermo ) ;

/* 2406 memory read */
static int FS_r_mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not a "paged" endeavor, the CRC comes after a full read */
    if ( OW_r_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return size ;
}

/* 2406 memory write */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
    if ( OW_r_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return 0 ;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* write is "byte at a time" -- not paged */
    if ( OW_w_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_vis(FLOAT *V , const struct parsedname * pn) {
    int ret = FS_r_current(V,pn) ;
    V[0] *= .025 ;
    return ret ;
}

static int FS_thermocouple(FLOAT *F , const struct parsedname * pn) {
    FLOAT T, V;
    int ret ;
    if ((ret=FS_r_vis(&V,pn))) return ret ;
    if ((ret=FS_r_temp(&T,pn))) return ret ;
    F[0]=OW_type(T,V,(struct thermo *)pn->ft->data) ;
    return 0 ;
}

static int FS_r_current(FLOAT * C , const struct parsedname * pn) {
    unsigned char c[2] ;
    int ret = OW_r_sram(c,2,0x0E,pn) ;
    if (ret) return ret ;
    C[0] = (FLOAT) (((int16_t)(c[0]<<8|c[1]))>>3)*.000625 ;
    return 0 ;
}

static int FS_r_accum(FLOAT * A , const struct parsedname * pn) {
    unsigned char a[2] ;
    int ret = OW_r_sram(a,2,0x10,pn) ;
    if (ret) return ret ;
    A[0] = (FLOAT) (((int16_t)(a[0]<<8|a[1])))*.00025 ;
    return 0 ;
}

static int FS_r_offset(FLOAT * O , const struct parsedname * pn) {
    unsigned char o ;
    int ret = OW_r_mem(&o,1,0x33,pn) ;
    if (ret) return ret ;
    O[0] = (FLOAT) (int8_t)o * .000625 ;
    return 0 ;
}

static int FS_w_offset(const FLOAT * O , const struct parsedname * pn) {
    unsigned char o ;
    if ( O[0] < -.08 ) return -ERANGE ;
    if ( O[0] > .08 ) return -ERANGE ;
    o = (uint8_t) (O[0]*1600) ; // 1600 == 1/.000625
    return OW_w_mem(&o,1,0x33,pn) ? -EINVAL : 0 ;
}

static int FS_r_volt(FLOAT * V , const struct parsedname * pn) {
    unsigned char v[2] ;
    int ret = OW_r_sram(v,2,0x0C,pn) ;
    if (ret) return ret ;
    V[0] = (FLOAT) (((int16_t)(v[0]<<8|v[1]))>>5)*.00488 ;
    return 0 ;
}

static int FS_r_temp(FLOAT * T , const struct parsedname * pn) {
    unsigned char t[2] ;
    int ret = OW_r_sram(t,2,0x18,pn) ;
    if (ret) return ret ;
    // .00390625 == 1/256
    /* change to selected temperature units */
    T[0] = Temperature( (FLOAT) (((int16_t)(t[0]<<8|t[1]))>>5) * .125 ) ;
    return 0 ;
}

/* 2406 switch PIO sensed*/
static int FS_sense(int * y , const struct parsedname * pn) {
    unsigned char data ;
    int ret = OW_r_sram(&data,1,0x08,pn);
    if (ret) return -EINVAL ;
    y[0] = UT_getbit(&data,6) ;
    return 0 ;
}

/* write PIO -- bit 6 */
static int FS_w_pio(const int * y, const struct parsedname * pn) {
    unsigned char data ;
    int ret = OW_r_sram(&data,1,0x08,pn);
    if (ret) return -EINVAL ;
    UT_setbit( &data , 6 , y[0]==0 ) ;
    UT_setbit( &data , 7 , 1 ) ; // set PS bit after read to clear latch?!?
    if ( OW_w_mem(&data,1,0x08,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0xB8, page&0xFF, } ;
    int ret ;
    if ( offset > page+16 ) return 0 ;
    if ( offset + size <= page ) return 0 ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 2 ) ;
    BUSUNLOCK
    return ret ;
}

static int OW_r_mem( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    return OW_r_eeprom(0x20,size,offset,pn)
        || OW_r_eeprom(0x30,size,offset,pn)
        || OW_r_sram(data,size,offset,pn) ;
}

/* just read the sram -- eeprom may need to be recalled if you want it */
static int OW_r_sram( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x69, offset&0xFF , } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 2 ) || BUS_readin_data( data, size ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    return 0 ;
}

/* Special processing for eeprom -- page is the address of a 16 byte page (e.g. 0x20) */
static int OW_w_eeprom( const size_t page, const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x48, page, } ;
    unsigned char cmp[16] ;
    size_t pagenext = page + 16 ;
    size_t eoffset = (offset<page) ? page : offset ;
    size_t enext = size+offset ;
    size_t esize ;
    int ret ;

    if ( offset >= pagenext ) return 0 ;
    if ( enext <= page ) return 0 ;
    if ( enext > pagenext ) enext = pagenext ;
    esize = enext - eoffset ;

    // reread and compare memory before writing
    ret = OW_r_sram(cmp,esize,eoffset,pn) || memcmp(cmp,&data[offset-eoffset],esize) ;
    if (ret) return ret ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,2) ;
    BUSUNLOCK
    if (ret) return ret ;
    // delay to write eeprom -- with bus unlocked
    UT_delay(10) ;
    return 0 ;
}

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x6C, offset&0xFF , } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,2) || BUS_send_data(data,size) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    /* special processing for region 0x20--0x3F which is eeprom */
    return OW_w_eeprom(0x20,data,size,offset,pn) || OW_w_eeprom(0x30,data,size,offset,pn) ;
}

static FLOAT OW_type(FLOAT T, FLOAT V, const struct thermo * thermo ) {
    FLOAT E ;
    FLOAT R ;
    int r ;
    int i ;

    /* First compute ref voltage for ref temperature */
    for ( r = (thermo->volt->n)-1 ; r>0 ; --r ) { /* find correct polynomial */
        if ( T > thermo->volt->p[r]->minf ) break ;
    }
    i = thermo->volt->p[r]->order ;
    for ( E = thermo->volt->p[r]->coef[i] ; i ; ) {
        --i ;
        E = thermo->volt->p[r]->coef[i] + E*T ;
    }
    E *= .001 ; /* convert mV to V */

    /* Correct measured voltage by adding reference voltage */
    E += V ; /* Add measured voltage to ref voltage */

    /* Now compute temperature from corrected voltage */
    for ( r = (thermo->temp->n)-1 ; r>0 ; --r ) { /* find correct polynomial */
        if ( E > thermo->temp->p[r]->minf ) break ;
    }
    i = thermo->temp->p[r]->order ;
    for ( R = thermo->temp->p[r]->coef[i] ; i ; ) {
        --i ;
        R = thermo->temp->p[r]->coef[i] + E*R ;
    }
    return R ;
}
