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

/* ------- Prototypes ----------- */

/* DS2406 switch */
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 fREAD_FUNCTION( FS_r_volt ) ;
 fREAD_FUNCTION( FS_r_temp ) ;
 fREAD_FUNCTION( FS_r_current ) ;
 fREAD_FUNCTION( FS_r_extsense ) ;
 fREAD_FUNCTION( FS_r_vis ) ;
 fREAD_FUNCTION( FS_r_accum ) ;
 fREAD_FUNCTION( FS_r_offset ) ;
 fREAD_FUNCTION( FS_thermocouple ) ;
 fREAD_FUNCTION( FS_rangelow ) ;
 fREAD_FUNCTION( FS_rangehigh ) ;
fWRITE_FUNCTION( FS_w_offset ) ;
yWRITE_FUNCTION( FS_w_pio ) ;
 yREAD_FUNCTION( FS_sense ) ;

/* ------- Structures ----------- */
struct thermocouple {
    FLOAT v1, v2 ;
    FLOAT rangeLow, rangeHigh ;
    FLOAT low[11] ;
    FLOAT mid[11] ;
    FLOAT high[11];
    FLOAT mV[11] ;
} ;

struct thermocouple type_b =
{
        0.437,1.822,0,1820,
        { -3.34549e+09,7.57222e+09,-7.34952e+09,3.9973e+09,-1.3362e+09,2.83366e+08,-3.80268e+07,3.13466e+06,-151226,4738.17,31.9924,},
        { -149.905,1643.07,-7937.72,22240.5,-40004.8,48278.9,-39657.6,22027.5,-8104.23,2140.22,-26.3893,},
        { -1.10165e-08,1.13612e-06,-5.18188e-05,0.00138477,-0.024161,0.289846,-2.44837,14.7621,-65.4035,303.638,201.601,},
        { 3.4607e-21,-1.54436e-18,2.33598e-16,-6.23624e-15,-2.18387e-12,2.88144e-10,-1.60479e-08,4.47121e-07,1.95406e-07,-0.000227614,8.70069e-05,},
};

struct thermocouple type_e =
{
        -3.306,9.86,-270,1000,
        { -0.00186348,-0.119593,-3.40982,-56.8497,-613.48,-4475.37,-22342.6,-75348.3,-164231,-208853,-117692,},
        { -2.29826e-09,4.45093e-08,2.57026e-07,-1.17477e-05,8.44013e-05,4.74888e-05,-0.00303068,0.0185878,-0.248035,17.057,0.00354977,},
        { -1.23525e-15,4.71812e-13,-7.8381e-11,7.53249e-09,-4.72537e-07,2.07906e-05,-0.000672767,0.0167714,-0.340304,17.683,-1.55323,},
        { -1.07611e-22,1.31153e-20,4.65438e-18,-5.31167e-16,-7.78362e-14,7.82564e-12,7.64931e-10,-1.21373e-07,5.06679e-05,0.0586129,-0.000325966,},
};

struct thermocouple type_j =
{
        14.055,27.057,-210,1200,
        { -1.86216e-09,7.06203e-08,-7.40759e-07,-1.68076e-06,5.62956e-05,3.08928e-05,-0.00342003,0.0233241,-0.217611,19.8171,-0.0342962,},
        { 1.04705e-09,-2.12282e-07,1.92779e-05,-0.00103258,0.0361236,-0.8624,14.2278,-160.156,1177.08,-5082.05,9897.53,},
        { 4.75069e-13,-2.37677e-10,5.28345e-08,-6.86486e-06,0.000576832,-0.0327315,1.26987,-33.2645,563.208,-5548.9,24405.1,},
        { -6.97491e-24,1.3658e-21,2.04292e-19,-5.09902e-17,-1.49481e-15,4.67638e-13,1.34732e-10,-8.86546e-08,3.04846e-05,0.0503846,-1.05193e-06,},
};

struct thermocouple type_k =
{
        11.176,22.691,-270,1370,
        { -2.46084e-07,6.81532e-06,-4.21632e-05,-0.000322014,0.00355914,0.0028323,-0.0906577,0.124291,0.192566,24.9404,-0.722592,},
        { -2.14528e-09,3.38578e-07,-2.36415e-05,0.000959251,-0.0249579,0.432955,-5.03485,38.3162,-179.073,470.662,-403.75,},
        { 1.22956e-13,-4.95882e-11,8.73994e-09,-8.91551e-07,5.85688e-05,-0.00259803,0.0789352,-1.61945,21.3873,-139.99,562.639,},
        { -6.76827e-23,4.59786e-21,3.02777e-18,-1.80296e-16,-5.00204e-14,2.58284e-12,3.67046e-10,-1.31662e-07,2.61205e-05,0.0394427,-0.000165505,},
};

struct thermocouple type_n =
{
        -0.26,7.631,-270,1300,
        { -0.384204,-8.51012,-81.238,-437.501,-1461.59,-3138.2,-4341.49,-3775.54,-1949.5,-493.211,-57.4264,},
        { 1.79316e-06,-7.06517e-05,0.00119405,-0.0113121,0.0659068,-0.244075,0.574314,-0.793326,-0.362399,38.3962,0.0137761,},
        { 1.50132e-13,-3.64746e-11,3.78312e-09,-2.17623e-07,7.49952e-06,-0.000151796,0.00137062,0.0144983,-0.707806,36.6116,4.4746,},
        { -1.67472e-22,-5.75376e-21,7.87952e-18,2.66768e-16,-1.44225e-13,-4.94289e-12,1.51444e-09,1.06098e-08,1.15203e-05,0.0260828,-0.000928019,},
};

struct thermocouple type_r =
{
        1.942,12.687,-50,1768,
        { -2.97833,28.6494,-117.396,268.213,-377.551,347.099,-226.83,130.345,-93.3971,188.87,-0.0165138,},
        { -1.1534e-08,1.07402e-06,-4.2822e-05,0.000981669,-0.0145961,0.149842,-1.09292,5.68469,-22.0162,151.516,11.2428,},
        { -1.60073e-06,0.00027281,-0.020812,0.936039,-27.4906,550.959,-7632.14,72164.2,-445780,1.62479e+06,-2.65314e+06,},
        { -2.61253e-22,5.71644e-20,4.54127e-18,-1.99547e-15,1.3374e-13,3.09864e-12,-4.60676e-10,-1.99743e-08,1.43025e-05,0.00528674,3.13725e-05,},
};

struct thermocouple type_s =
{
        7.981,11.483,-50,1768,
        { -6.46807e-05,0.00270224,-0.0485888,0.492361,-3.09548,12.5625,-33.438,59.3949,-76.2119,186.708,-0.185908,},
        { 0.00082531,-0.0846629,3.88937,-105.403,1866.54,-22574,188861,-1.07949e+06,4.03486e+06,-8.90662e+06,8.81855e+06,},
        { -1.42197e-05,0.00217057,-0.148489,5.99566,-158.252,2853.22,-35589.5,303277,-1.68982e+06,5.55955e+06,-8.20142e+06,},
        { -3.61972e-22,1.85682e-19,-3.38826e-17,2.28563e-15,2.46683e-14,-8.74753e-12,2.22002e-10,-1.75089e-08,1.24207e-05,0.00540621,2.31793e-05,},
};

struct thermocouple type_t =
{
        -1.785,2.556,-270,400,
        { -0.0856873,-3.35591,-58.1948,-587.947,-3829.53,-16790.4,-50151.1,-100705,-130042,-97452.3,-32202.6,},
        { 0.000151427,3.35406e-05,-0.00372056,0.00326535,0.022672,-0.0222902,-0.0694538,0.116881,-0.67161,25.8257,-0.0024547,},
        { -5.44871e-11,6.49228e-09,-3.38465e-07,1.02385e-05,-0.000201171,0.00272364,-0.0263712,0.192645,-1.30813,27.0402,-0.900659,},
        { -1.1498e-22,7.18468e-21,5.31691e-18,-2.88407e-16,-9.51445e-14,4.02711e-12,8.60657e-10,-5.74685e-08,4.17544e-05,0.0386747,-0.000253129,},
};


struct aggregate A2760p = { 16, ag_numbers, ag_separate, } ;
struct filetype DS2760[] = {
    F_STANDARD   ,
    {"memory"    ,   256,  NULL,    ft_binary  , ft_volatile, {b:FS_r_mem}    , {b:FS_w_mem} , NULL, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"pages/page",    16,  NULL,    ft_binary  , ft_volatile, {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"typeB"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeB/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_b, } ,
    {"typeB/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_b, } ,
    {"typeB/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_b, } ,
    {"typeE"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeE/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_e, } ,
    {"typeE/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_e, } ,
    {"typeE/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_e, } ,
    {"typeJ"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeJ/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_j, } ,
    {"typeJ/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_j, } ,
    {"typeJ/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_j, } ,
    {"typeK"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeK/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_k, } ,
    {"typeK/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_k, } ,
    {"typeK/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_k, } ,
    {"typeN"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeN/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_n, } ,
    {"typeN/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_n, } ,
    {"typeN/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_n, } ,
    {"typeR"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeR/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_r, } ,
    {"typeR/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_r, } ,
    {"typeR/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_r, } ,
    {"typeS"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeS/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_s, } ,
    {"typeS/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_s, } ,
    {"typeS/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_s, } ,
    {"typeT"            ,  0, NULL, ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"typeT/temperature", 12, NULL, ft_float   , ft_volatile, {f:FS_thermocouple}, {v:NULL}  , & type_t, } ,
    {"typeT/range_low"  , 12, NULL, ft_float   , ft_volatile, {f:FS_rangelow} , {v:NULL}     , & type_t, } ,
    {"typeT/range_high" , 12, NULL, ft_float   , ft_volatile, {f:FS_rangehigh}, {v:NULL}     , & type_t, } ,
    {"volt"      ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_r_volt}   , {v:NULL}     , NULL, } ,
    {"temperature",   12,  NULL,    ft_float   , ft_volatile, {f:FS_r_temp}   , {v:NULL}     , NULL, } ,
    {"current"   ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_r_current}, {v:NULL}     , NULL, } ,
    {"ext_sense" ,    12,  NULL,    ft_float   , ft_volatile, {f:FS_r_extsense},{v:NULL}     , NULL, } ,
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
static int OW_w_sram( const unsigned char * data , const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_w_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_temp(FLOAT * T , const struct parsedname * pn) ;
static FLOAT polycomp( FLOAT x , FLOAT * coef ) ;

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

static int FS_rangelow(FLOAT *F , const struct parsedname * pn) {
    F[0] = Temperature(((struct thermocouple *) pn->ft->data)->rangeLow) ;
    return 0 ;
}

static int FS_rangehigh(FLOAT *F , const struct parsedname * pn) {
    F[0] = Temperature(((struct thermocouple *) pn->ft->data)->rangeHigh) ;
    return 0 ;
}

static int FS_thermocouple(FLOAT *F , const struct parsedname * pn) {
    FLOAT T, V;
    int ret ;
    struct thermocouple * thermo = (struct thermocouple *) pn->ft->data ;

    /* Get reference temperature */
    if ((ret=OW_r_temp(&T,pn))) return ret ; /* in C */

    /* Get measured voltage */
    if ((ret=FS_r_extsense(&V,pn))) return ret ;
    V *= .001 ; /* convert Volts to mVolts */

    /* Correct voltage by adding reference temperature voltage */
    V += polycomp(T,thermo->mV) ;

    /* Find right range, them compute temparature from voltage */
    if ( V < thermo->v1 ) {
    T = polycomp(V,thermo->low) ;
    } else if ( V < thermo->v2 ) {
    T = polycomp(V,thermo->mid) ;
    } else {
    T = polycomp(V,thermo->high) ;
    }

    /* Temperature units correction */
    F[0]=Temperature(T) ;
    return 0 ;
}

static int FS_r_current(FLOAT * C , const struct parsedname * pn) {
    unsigned char c[2] ;
    int ret = OW_r_sram(c,2,0x0E,pn) ;
    if (ret) return ret ;
    C[0] = (FLOAT) (((int16_t)(c[0]<<8|c[1]))>>3)*.000625 ;
    return 0 ;
}

static int FS_r_extsense(FLOAT * C , const struct parsedname * pn) {
    unsigned char c[2] ;
    int ret = OW_r_sram(c,2,0x0E,pn) ;
    if (ret) return ret ;
    C[0] = (FLOAT) (((int16_t)(c[0]<<8|c[1]))>>3)*.000015625 ;
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
    int ret = OW_r_temp( T , pn ) ;
    if (ret) return ret ;
    /* change to selected temperature units */
    T[0] = Temperature( T[0] ) ;
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

/* just read the sram -- eeprom may need to be recalled if you want it */
static int OW_r_sram( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x69, offset&0xFF , } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 2 ) || BUS_readin_data( data, size ) ;
    BUSUNLOCK

    return ret ;
}

static int OW_r_mem( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    return OW_r_eeprom(0x20,size,offset,pn)
        || OW_r_eeprom(0x30,size,offset,pn)
        || OW_r_sram(data,size,offset,pn) ;
}

/* Special processing for eeprom -- page is the address of a 16 byte page (e.g. 0x20) */
static int OW_w_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x48, page&0xFF, } ;
    int ret ;
    if ( offset > page+16 ) return 0 ;
    if ( offset + size <= page ) return 0 ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 2 ) ;
    BUSUNLOCK

    return ret ;
}

static int OW_w_sram( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x6C, offset&0xFF , } ;
    int ret  ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,2) || BUS_send_data(data,size) ;
    BUSUNLOCK

    return ret ;
}

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    return OW_r_eeprom(0x20,size,offset,pn)
        || OW_r_eeprom(0x30,size,offset,pn)
        || OW_w_sram(  data,size,offset,pn)
        || OW_w_eeprom(0x20,size,offset,pn)
        || OW_w_eeprom(0x30,size,offset,pn) ;
}

static int OW_r_temp(FLOAT * T , const struct parsedname * pn) {
    unsigned char t[2] ;
    int ret = OW_r_sram(t,2,0x18,pn) ;
    if (ret) return ret ;
    T[0] = (FLOAT) (((int16_t)(t[0]<<8|t[1]))>>5) * .125 ;
    return 0 ;
}

/* Compute a 10th order polynomial with cooef being a10, a9, ... a0 */
static FLOAT polycomp( FLOAT x , FLOAT * coef ) {
    FLOAT r = coef[0] ;
    int i ;
    for ( i=1; i<=10; ++i ) r = x*r + coef[i] ;
    return r ;
}
