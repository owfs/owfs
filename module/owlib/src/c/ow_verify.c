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

/* ------- Prototypes ----------- */
static int BUS_verify(BYTE search, const struct parsedname * pn) ;

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
    return BUS_verify(0xEC, pn ) ;
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
    return BUS_verify(0xF0, pn ) ;
}

// BUS_verify called from BUS_normalverify or BUS_alarmverify
//   tests if device is present in requested mode
//   serialnumber is 1-wire device address (64 bits)
//   return 0 good, 1 bad
static int BUS_verify(BYTE search, const struct parsedname * pn) {
    BYTE buffer[24] ;
    struct parsedname pncopy ;
    int i, goodbits=0 ;
    struct transaction_log tv[] = {
        TRXN_START,
        {&search, NULL, 1, trxn_match,},
        {buffer, buffer, 24, trxn_read,},
        TRXN_END,
    } ;

    // set all bits at first
    memset( buffer , 0xFF , 24 ) ;

    // shallow copy -- no dev so just bus is selected
    memcpy( &pncopy, pn, sizeof(struct parsedname) ) ; /* shallow copy */
    pncopy.dev = NULL ;

   // now set or clear apropriate bits for search
   for (i = 0; i < 64; i++) UT_setbit(buffer,3*i+2,UT_getbit(pn->sn,i)) ;

   // send/recieve the transfer buffer
   if ( BUS_transaction(tv,&pncopy) ) return 1 ;
   for ( i=0 ; (i<64) && (goodbits<64) ; i++ )
   {
       switch (UT_getbit(buffer,3*i)<<1 | UT_getbit(buffer,3*i+1)) {
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
   // check to see if there were enough good bits to be successful
   // remember 1 is bad!
   return goodbits<8 ;
}
