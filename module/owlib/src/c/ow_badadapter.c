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

/* All the rest of the program sees is the BadAdapter_detect and the entry in iroutines */

static int BadAdapter_PowerByte(const unsigned char byte, const unsigned int delay, const struct parsedname * pn) ;
static int BadAdapter_ProgramPulse( const struct parsedname * pn ) ;
static int BadAdapter_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * pn) ;
static int BadAdapter_reset( const struct parsedname * const pn ) ;
static int BadAdapter_read(unsigned char * const buf, const size_t size, const struct parsedname * pn ) ;
static int BadAdapter_write( const unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) ;
static int BadAdapter_level(int new_level, const struct parsedname * const pn) ;
static int BadAdapter_select(const struct parsedname * const pn) ;
static int BadAdapter_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len, const struct parsedname * const pn ) ;

/* Device-specific functions */
int BadAdapter_detect( struct connection_in * in ) {
    in->fd = -1 ;
#ifdef OW_USB
    in->connin.usb.usb = NULL ;
#endif
    in->Adapter = adapter_Bad ; /* OWFS assigned value */
    in->iroutines.write         = BadAdapter_write         ;
    in->iroutines.read          = BadAdapter_read          ;
    in->iroutines.reset         = BadAdapter_reset         ;
    in->iroutines.next_both     = BadAdapter_next_both     ;
    in->iroutines.level         = BadAdapter_level         ;
    in->iroutines.PowerByte     = BadAdapter_PowerByte     ;
    in->iroutines.ProgramPulse  = BadAdapter_ProgramPulse  ;
    in->iroutines.sendback_data = BadAdapter_sendback_data ;
    in->iroutines.select        = BadAdapter_select        ;
    in->adapter_name="Bad Adapter" ;
    return 0 ;
}

static int BadAdapter_PowerByte(const unsigned char byte, const unsigned int delay, const struct parsedname * const pn) {
    (void) pn ;
    (void) byte ;
    (void) delay ;
    return -ENXIO ;
}
static int BadAdapter_ProgramPulse( const struct parsedname * const pn ) {
    (void) pn ;
    return -ENXIO ;
}
static int BadAdapter_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
    (void) serialnumber ;
    (void) search ;
    (void) pn ;
    return -ENXIO ;
}
static int BadAdapter_reset( const struct parsedname * const pn ) {
    (void) pn ;
    return -ENXIO ;
}
static int BadAdapter_read(unsigned char * const buf, const size_t size, const struct parsedname * const pn ) {
    (void) pn ;
    (void) buf ;
    (void) size ;
    return -ENXIO ;
}
static int BadAdapter_write( const unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) {
    (void) pn ;
    (void) bytes ;
    (void) num ;
    return -ENXIO ;
}
static int BadAdapter_level(int new_level, const struct parsedname * const pn) {
    (void) pn ;
    (void) new_level ;
    return -ENXIO ;
}
static int BadAdapter_select(const struct parsedname * const pn) {
    (void) pn ;
    return -ENXIO ;
}
static int BadAdapter_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len, const struct parsedname * const pn ) {
    (void) pn ;
    (void) data ;
    (void) resp ;
    (void) len ;
    return -ENXIO ;
}
