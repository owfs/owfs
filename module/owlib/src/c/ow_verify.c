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
#include "ow_xxxx.h"

/* ------- Prototypes ----------- */
static int BUS_verify(BYTE search, BYTE * locator, const struct parsedname * pn) ;

/* ------- Functions ------------ */

// BUS_verify called from BUS_normalverify or BUS_alarmverify
//   tests if device is present in requested mode
//   serialnumber is 1-wire device address (64 bits)
//   return 0 good, 1 bad
int BUS_alarmverify(const struct parsedname * pn) {
    if(get_busmode(pn->in) == bus_remote) {
      //printf("BUS_normalverify: bus is remote... shouldn't come here.\n");
      return 1;
    }
    return BUS_verify(0xEC, NULL, pn ) ;
}

// BUS_verify called from BUS_normalverify or BUS_alarmverify
//   tests if device is present in requested mode
//   serialnumber is 1-wire device address (64 bits)
//   return 0 good, 1 bad
int BUS_normalverify(const struct parsedname * pn) {
    if(get_busmode(pn->in) == bus_remote) {
      //printf("BUS_normalverify: bus is remote... shouldn't come here.\n");
      return 1;
    }
    return BUS_verify(0xF0, NULL, pn ) ;
}

// BUS_verify called from BUS_normalverify or BUS_alarmverify
//   tests if device is present in requested mode
//   serialnumber is 1-wire device address (64 bits)
//   return 0 good, 1 bad
static int BUS_verify(BYTE search, BYTE * locator, const struct parsedname * pn) {
    BYTE buffer[35] ;
    struct parsedname pncopy ;
    int i, goodbits=0 ;
    int len = locator?35:25 ;
    struct transaction_log tv[] = {
        TRXN_START,
        {buffer, buffer, len, trxn_read,},
        TRXN_END,
    } ;
    
    // set all bits at first
    memset( buffer , 0xFF , len ) ;
    buffer[0] = search ;
    if ( locator ) buffer[25] = 0x00 ;
    
printf("Verify start len=%d\n",len) ;

    // shallow copy -- no dev so just bus is selected
    memcpy( &pncopy, pn, sizeof(struct parsedname) ) ; /* shallow copy */
    pncopy.dev = NULL ;

    // now set or clear apropriate bits for search
    for (i = 0; i < 64; i++) UT_setbit(buffer,3*i+10,UT_getbit(pn->sn,i)) ;

    // send/recieve the transfer buffer
    if ( BUS_transaction(tv,&pncopy) ) { printf("Bad transaction\n"); return 1 ; }
    if ( buffer[0] != search ) return 1 ;
    for ( i=0 ; (i<64) && (goodbits<64) ; i++ )
    {
        switch (UT_getbit(buffer,3*i+8)<<1 | UT_getbit(buffer,3*i+9)) {
        case 0:
            break ;
        case 1:
            if ( ! UT_getbit(pn->sn,i) ) goodbits++ ;
            break ;
        case 2:
            if ( UT_getbit(pn->sn,i) ) goodbits++ ;
            break ;
        case 3: // No device on line
            return 1 ;
        }
    }
printf("Verify end result = %d\n",goodbits<8) ;
    // check to see if there were enough good bits to be successful
    // remember 1 is bad!
    if ( locator ) memcpy( locator, &buffer[25], 10 ) ;
    return goodbits<8 ;
}

int FS_locator(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    BYTE loc[10] ; // key and 8 byte default
    size_t i ;
    size_t siz = size>>1 ;
    size_t off = offset>>1 ;
printf("Locator\n");
    if(get_busmode(pn->in) != bus_fake) {
        if ( BUS_verify( 0xF0, loc, pn ) ) return -ENOENT ;
    }
    for ( i= 0 ; i < siz ; ++i ) num2string( buf+2*i+offset, loc[i+off+2] ) ;
    return size ;
}

int FS_r_locator(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    BYTE loc[10] ; // key and 8 byte default
    size_t i ;
    size_t siz = size>>1 ;
    size_t off = offset>>1 ;
printf("rLocator\n");
    if(get_busmode(pn->in) != bus_fake) {
        if ( BUS_verify( 0xF0, loc, pn ) ) return -ENOENT ;
    }
    for ( i= 0 ; i < siz ; ++i ) num2string( buf+2*i+offset, loc[9-i-off] ) ;
    return size ;
}
