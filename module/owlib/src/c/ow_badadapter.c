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
#include "ow_connection.h"

/* All the rest of the program sees is the BadAdapter_detect and the entry in iroutines */

static int BadAdapter_PowerByte(const unsigned char byte, const unsigned int delay, const struct parsedname * pn) ;
static int BadAdapter_ProgramPulse( const struct parsedname * pn ) ;
static int BadAdapter_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * pn) ;
static int BadAdapter_reset( const struct parsedname * pn ) ;
static int BadAdapter_reconnect( const struct parsedname * pn ) ;
static int BadAdapter_select(const struct parsedname * pn) ;
static int BadAdapter_sendback_data( const unsigned char * data, unsigned char * resp, const size_t len, const struct parsedname * pn ) ;
static void BadAdapter_close( struct connection_in * in ) ;

/* Device-specific functions */
int BadAdapter_detect( struct connection_in * in ) {
    in->fd = -1 ;
#ifdef OW_USB
    in->connin.usb.usb = NULL ;
#endif
    in->Adapter = adapter_Bad ; /* OWFS assigned value */
    in->iroutines.reset         = BadAdapter_reset         ;
    in->iroutines.next_both     = BadAdapter_next_both     ;
    in->iroutines.PowerByte     = BadAdapter_PowerByte     ;
    in->iroutines.ProgramPulse  = BadAdapter_ProgramPulse  ;
    in->iroutines.sendback_data = BadAdapter_sendback_data ;
    in->iroutines.select        = BadAdapter_select        ;
    in->iroutines.reconnect     = BadAdapter_reconnect     ;
    in->iroutines.close         = BadAdapter_close         ;
    in->adapter_name="Bad Adapter" ;
    return 0 ;
}

static int BadAdapter_PowerByte(const unsigned char byte, const unsigned int delay, const struct parsedname * pn) {
    (void) pn ;
    (void) byte ;
    (void) delay ;
    return -ENOTSUP ;
}
static int BadAdapter_ProgramPulse( const struct parsedname * pn ) {
    (void) pn ;
    return -ENOTSUP ;
}
static int BadAdapter_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * pn) {
    (void) serialnumber ;
    (void) search ;
    (void) pn ;
    return -ENOTSUP ;
}
static int BadAdapter_reset( const struct parsedname * pn ) {
    (void) pn ;
    return -ENOTSUP ;
}
static int BadAdapter_select(const struct parsedname * pn) {
    (void) pn ;
    return -ENOTSUP ;
}
static int BadAdapter_reconnect(const struct parsedname * pn) {
    (void) pn ;
    return -ENOTSUP ;
}
static int BadAdapter_sendback_data( const unsigned char * data , unsigned char * resp, const size_t len, const struct parsedname * pn ){
    (void) pn ;
    (void) data ;
    (void) resp ;
    (void) len ;
    return -ENOTSUP ;
}

static void BadAdapter_close( struct connection_in * in ) {
    (void) in ;
}

