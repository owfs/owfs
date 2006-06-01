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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

int fakes = 0 ;

/* All the rest of the program sees is the Fake_detect and the entry in iroutines */

static int Fake_reset( const struct parsedname * pn ) ;
static int Fake_overdrive( const UINT ov, const struct parsedname * pn ) ;
static int Fake_testoverdrive( const struct parsedname * pn ) ;
static int Fake_ProgramPulse( const struct parsedname * pn ) ;
static int Fake_sendback_bits( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) ;
static void Fake_close( struct connection_in * in ) ;
static int Fake_next_both(struct device_search * ds, const struct parsedname * pn) ;
static const char * namefind( char * name ) ;

/* Device-specific functions */
/* Note, the "Bad"adapter" ha not function, and returns "-ENOTSUP" (not supported) for most functions */
/* It does call lower level functions for higher ones, which of course is pointless since the lower ones don't work either */
int Fake_detect( struct connection_in * in ) {
    ASCII * newname ;
    ASCII * oldname = in->name ;

    in->fd = fakes ;
    in->iroutines.detect        = Fake_detect ;
    in->Adapter = adapter_Bad ; /* OWFS assigned value */
    in->iroutines.reset         = Fake_reset         ;
    in->iroutines.next_both     = Fake_next_both     ;
    in->iroutines.overdrive     = Fake_overdrive     ;
    in->iroutines.testoverdrive = Fake_testoverdrive ;
    in->iroutines.PowerByte     = BUS_PowerByte_low  ;
    in->iroutines.ProgramPulse  = Fake_ProgramPulse  ;
    in->iroutines.sendback_data = BUS_sendback_data_low    ;
    in->iroutines.sendback_bits = Fake_sendback_bits ;
    in->iroutines.select        = BUS_select_low     ;
    in->iroutines.reconnect     = NULL               ;
    in->iroutines.close         = Fake_close         ;

    in->connin.fake.devices=0 ;
    in->connin.fake.device=NULL ;
    in->adapter_name = "Simulated" ;
    in->Adapter = adapter_fake ;
    in->AnyDevices = 0 ;
    if ( (newname=(ASCII *)malloc(20)) ) {
        const ASCII * dev ;
        ASCII * rest = in->name ;
        int allocated = 0 ;

        snprintf(newname,18,"fake.%d",fakes) ;
        in->name = newname ;

        while (rest) {
            BYTE sn[8] ;
            for ( dev=strsep( &rest, " ," ) ; dev[0] ; ++dev ) {
                if ( dev[0]!=' ' && dev[0]!=',' ) break ;
            }
            if (
                (isxdigit(dev[0]) && isxdigit(dev[1]))
                ||
                (dev=namefind(dev))
            ) {
                sn[0] = string2num(dev) ;
                sn[1] = rand()&0xFF ;
                sn[2] = rand()&0xFF ;
                sn[3] = rand()&0xFF ;
                sn[4] = rand()&0xFF ;
                sn[5] = rand()&0xFF ;
                sn[6] = rand()&0xFF ;
                sn[7] = CRC8compute(sn,7,0) ;
                if ( in->connin.fake.devices >= allocated ) {
                    BYTE * temp = (BYTE *) realloc( in->connin.fake.device, allocated*8+88 ) ;
                    if ( temp ) {
                        allocated += 10 ;
                        in->connin.fake.device = temp ;
                    }
                }
                if ( in->connin.fake.devices < allocated ) {
                    memcpy( &(in->connin.fake.device[in->connin.fake.devices*8]), sn, 8 ) ;
                    in->AnyDevices = 1 ;
                    ++ in->connin.fake.devices ;
                }
            }
        }
        if (oldname) free(oldname) ;
    }
    ++ fakes ;
    return 0 ;
}

static int Fake_reset( const struct parsedname * pn ) {
    (void) pn ;
    return 0 ;
}
static int Fake_overdrive( const UINT ov, const struct parsedname * pn ) {
    (void) ov ;
    (void) pn ;
    return 0 ;
}
static int Fake_testoverdrive( const struct parsedname * pn ) {
    (void) pn ;
    return 0 ;
}
static int Fake_ProgramPulse( const struct parsedname * pn ) {
    (void) pn ;
    return 0 ;
}
static int Fake_sendback_bits( const BYTE * data , BYTE * resp, const size_t len, const struct parsedname * pn ){
    (void) pn ;
    (void) data ;
    (void) resp ;
    (void) len ;
    return 0 ;
}
static void Fake_close( struct connection_in * in ) {
    (void) in ;
}

static int Fake_next_both(struct device_search * ds, const struct parsedname * pn) {
    if ( ++ds->LastDiscrepancy >= pn->in->connin.fake.devices ) ds->LastDevice = 0 ;
    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;
    memcpy( ds->sn, &(pn->in->connin.fake.device[8*ds->LastDiscrepancy]), 8 ) ;
    return 0 ;
}

static const char * namefind( char * name ) {
    const struct device d = { NULL, name, 0,0,NULL } ;
    struct device_opaque * p ;
    /* Embedded function */
    int device_compare( const void * a , const void * b ) {
        return strcasecmp( ((const struct device *)a)->name , ((const struct device *)b)->name ) ;
    }

    p = tfind( &d, & Tree[pn_real], device_compare ) ;
    if (p) {
        return ((struct device *)p->key)->code ;
    }
    return NULL ;
}
