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

#include "owfs_config.h"
#include "ow.h"
#include "ow_devices.h"

static int device_compare( const void * a , const void * b ) ;
static int file_compare( const void * a , const void * b ) ;
static void Device2Tree( const struct device * d, enum pn_type type ) ;

struct device * DeviceSimultaneous ;

static int device_compare( const void * a , const void * b ) {
    return strcmp( ((const struct device *)a)->code , ((const struct device *)b)->code ) ;
}

static int file_compare( const void * a , const void * b ) {
    return strcmp( ((const struct filetype *)a)->name , ((const struct filetype *)b)->name ) ;
}

void * Tree[] = {NULL, NULL, NULL, NULL, NULL, };


static void Device2Tree( const struct device * d, enum pn_type type ) {
    tsearch( d, &Tree[type], device_compare ) ;
    if (d->ft) qsort( d->ft,(size_t) d->nft,sizeof(struct filetype),file_compare ) ;
/*
{
int i ;
printf("Device(%d) %s: ",type,d->name);
for(i=0;i<d->nft;++i){ printf( "%s ",d->ft[i].name);}
printf("\n");
}
*/
}

void DeviceSort( void ) {
    /* Sort the filetypes for the unrecognized device */
    qsort( NoDevice.ft,(size_t) NoDevice.nft,sizeof(struct filetype),file_compare) ;

    Device2Tree( & d_DS1420 ,       pn_real ) ;
    Device2Tree( & d_DS18S20 ,      pn_real ) ;
    Device2Tree( & d_DS18B20 ,      pn_real ) ;
    Device2Tree( & d_DS1822 ,       pn_real ) ;
    Device2Tree( & d_DS1921 ,       pn_real ) ;
    Device2Tree( & d_DS1963S,       pn_real ) ;
    Device2Tree( & d_DS1963L,       pn_real ) ;
    Device2Tree( & d_DS1991 ,       pn_real ) ;
    Device2Tree( & d_DS1992 ,       pn_real ) ;
    Device2Tree( & d_DS1993 ,       pn_real ) ;
    Device2Tree( & d_DS1995 ,       pn_real ) ;
    Device2Tree( & d_DS1996 ,       pn_real ) ;
    Device2Tree( & d_DS2401 ,       pn_real ) ;
    Device2Tree( & d_DS2404 ,       pn_real ) ;
    Device2Tree( & d_DS2404S ,      pn_real ) ;
    Device2Tree( & d_DS2405 ,       pn_real ) ;
    Device2Tree( & d_DS2406 ,       pn_real ) ;
    Device2Tree( & d_DS2408 ,       pn_real ) ;
    Device2Tree( & d_DS2409 ,       pn_real ) ;
    Device2Tree( & d_DS2415 ,       pn_real ) ;
    Device2Tree( & d_DS2417 ,       pn_real ) ;
    Device2Tree( & d_DS2423 ,       pn_real ) ;
    Device2Tree( & d_DS2436 ,       pn_real ) ;
    Device2Tree( & d_DS2438 ,       pn_real ) ;
    Device2Tree( & d_DS2450 ,       pn_real ) ;
    Device2Tree( & d_DS2502 ,       pn_real ) ;
    Device2Tree( & d_DS1982U ,      pn_real ) ;
    Device2Tree( & d_DS2505 ,       pn_real ) ;
    Device2Tree( & d_DS1985U ,      pn_real ) ;
    Device2Tree( & d_DS2506 ,       pn_real ) ;
    Device2Tree( & d_DS1986U ,      pn_real ) ;
    Device2Tree( & d_DS2433 ,       pn_real ) ;
    Device2Tree( & d_DS2760 ,       pn_real ) ;
    Device2Tree( & d_DS2890 ,       pn_real ) ;
    Device2Tree( & d_DS28E04 ,       pn_real ) ;
    Device2Tree( & d_LCD ,          pn_real ) ;
    Device2Tree( & d_stats_bus ,    pn_statistics ) ;
    Device2Tree( & d_stats_cache ,  pn_statistics ) ;
    Device2Tree( & d_stats_directory, pn_statistics ) ;
    Device2Tree( & d_stats_errors , pn_statistics ) ;
    Device2Tree( & d_stats_read ,   pn_statistics ) ;
    Device2Tree( & d_stats_thread , pn_statistics ) ;
    Device2Tree( & d_stats_write ,  pn_statistics ) ;
    Device2Tree( & d_set_cache ,    pn_settings ) ;
    Device2Tree( & d_sys_adapter ,  pn_system ) ;
    Device2Tree( & d_sys_process ,  pn_system ) ;
    Device2Tree( & d_sys_structure, pn_system ) ;
    Device2Tree( & d_simultaneous , pn_real ) ;

    /* Match simultaneous for special processing */
    {
        struct parsedname pn ;
        pn.type = pn_real ;
        FS_devicefind( "simultaneous", &pn ) ;
        DeviceSimultaneous = pn.dev ;
    }

    /* structure uses same tree as real */
    Tree[pn_structure] = Tree[pn_real] ;
}

void FS_devicefind( const char * code, struct parsedname * pn ) {
   const struct device d = { code, NULL, 0,0,NULL } ;
   struct device_opaque * p = tfind( &d, & Tree[pn->type], device_compare ) ;
   if (p) {
       pn->dev = p->key ;
   } else {
       pn->dev = &NoDevice ;
   }
}
