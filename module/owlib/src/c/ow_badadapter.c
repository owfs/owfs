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

static int BadAdapter_PowerByte(const unsigned char byte, const unsigned int delay) ;
static int BadAdapter_ProgramPulse( void ) ;
static int BadAdapter_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int BadAdapter_reset( const struct parsedname * const pn ) ;
static int BadAdapter_read(unsigned char * const buf, const size_t size ) ;
static int BadAdapter_write( const unsigned char * const bytes, const size_t num ) ;
static int BadAdapter_level(int new_level) ;
static int BadAdapter_select(const struct parsedname * const pn) ;
static int BadAdapter_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len ) ;

/* Device-specific functions */
int BadAdapter_detect( void ) {
    Adapter = adapter_Bad ; /* OWFS assigned value */
    iroutines.write         = BadAdapter_write         ;
    iroutines.read          = BadAdapter_read          ;
    iroutines.reset         = BadAdapter_reset         ;
    iroutines.next_both     = BadAdapter_next_both     ;
    iroutines.level         = BadAdapter_level         ;
    iroutines.PowerByte     = BadAdapter_PowerByte     ;
    iroutines.ProgramPulse  = BadAdapter_ProgramPulse  ;
    iroutines.sendback_data = BadAdapter_sendback_data ;
    iroutines.select        = BadAdapter_select        ;
    adapter_name="Bad Adapter" ;
    return 0 ;
}

static int BadAdapter_PowerByte(const unsigned char byte, const unsigned int delay) {
    (void) byte ;
    (void) delay ;
    return -ENXIO ;
}
static int BadAdapter_ProgramPulse( void ) {
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
static int BadAdapter_read(unsigned char * const buf, const size_t size ) {
    (void) buf ;
    (void) size ;
    return -ENXIO ;
}
static int BadAdapter_write( const unsigned char * const bytes, const size_t num ) {
    (void) bytes ;
    (void) num ;
    return -ENXIO ;
}
static int BadAdapter_level(int new_level) {
    (void) new_level ;
    return -ENXIO ;
}
static int BadAdapter_select(const struct parsedname * const pn) {
    (void) pn ;
    return -ENXIO ;
}
static int BadAdapter_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len ) {
    (void) data ;
    (void) resp ;
    (void) len ;
    return -ENXIO ;
}
