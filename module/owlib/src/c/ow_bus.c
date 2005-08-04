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

#include <sys/time.h>

// Mode Commands
#define MODE_DATA                      0xE1
#define MODE_COMMAND                   0xE3
#define MODE_STOP_PULSE                0xF1

// Data/Command mode select bits
#define MODSEL_DATA                    0x00
#define MODSEL_COMMAND                 0x02

/** BUS_send_data
    Send a data and expect response match
    puts into data mode if needed.
    return 0=good
    bad return sendback_data
    -EIO if memcpy
 */
int BUS_send_data( const unsigned char * const data, const size_t len, const struct parsedname * pn ) {
    int ret ;
    if ( len>16 ) {
        int dlen = len-(len>>1) ;
        (ret=BUS_send_data(data,dlen,pn)) || (ret=BUS_send_data(&data[dlen],len>>1,pn)) ;
    } else {
        unsigned char resp[16] ;
        if ((ret=BUS_sendback_data( data, resp, len,pn ))) {
            STAT_ADD1(BUS_send_data_memcmp_errors);
        } else if ((ret=memcmp(data, resp, (size_t) len))) {
            ret = -EPROTO ;
            STAT_ADD1(BUS_send_data_errors);
        }
    }
    return ret ;
}

/** DS2480_readin_data
  Send 0xFFs and return response block
  puts into data mode if needed.
 Returns 0=good
   Bad sendback_data
 */
int BUS_readin_data( unsigned char * const data, const size_t len, const struct parsedname * pn ) {
  int ret = BUS_sendback_data( memset(data, 0xFF, (size_t) len),data,len,pn) ;
  if(ret) {
      STAT_ADD1(BUS_readin_data_errors);
  }
  return ret;
}

static int BUS_selection_error( const struct parsedname * const pn, int ret ) {
    STAT_ADD1(BUS_select_low_errors);

    /* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
    if(ret >= -1) return ret;

    ret = BUS_reconnect( pn ) ;
    if(ret) { LEVEL_DATA("BUS_selection_error: reconnect error = %d\n", ret) ; }
    return ret ;
}   

//--------------------------------------------------------------------------
/** Select
   -- selects a 1-wire device to respond to further commands.
   First resets, then climbs down the branching tree,
    finally 'selects' the device.
   If no device is listed in the parsedname structure,
    only the reset and branching is done. This allows selective listing.
   Return 0=good, else
    reset, send_data, sendback_data
 */
/* Now you might wonder, why the low in BUS_select_low?
   There is a vague thought that higher level selection -- specifically
   for the DS9490 with it's intrinsic path commands might be implemented.
   Obviously not yet.
   Well, you asked
*/
int BUS_select_low(const struct parsedname * const pn) {
    int ret ;
    int ibranch ;
    // match Serial Number command 0x55
    unsigned char sent[9] = { 0x55, } ;
    unsigned char branch[2] = { 0xCC, 0x33, } ; /* Main, Aux */
    unsigned char resp[3] ;

//printf("SELECT\n");

    if(pn->in->use_overdrive_speed) {
        if((ret=BUS_testoverdrive(pn)) < 0) {
            BUS_selection_error(pn, ret) ;
            return ret ;
        } else {
            //printf("use overdrive speed\n");
            sent[0] = 0x69 ;
        }
    }

    // reset the 1-wire
    // send/recieve the transfer buffer
    // verify that the echo of the writes was correct
    if ( (ret=BUS_reset(pn)) ) {
        BUS_selection_error(pn, ret) ;
        return ret ;
    }
    for ( ibranch=0 ; ibranch < pn->pathlength ; ++ibranch ) {
        memcpy( &sent[1], pn->bp[ibranch].sn, 8 ) ;
//printf("select ibranch=%d %.2X %.2X.%.2X%.2X%.2X%.2X%.2X%.2X %.2X\n",ibranch,send[0],send[1],send[2],send[3],send[4],send[5],send[6],send[7],send[8]);
       /* Perhaps support overdrive here ? */
        if ( (ret=BUS_send_data(sent,9,pn)) ) {
            BUS_selection_error(pn, ret) ;
            return ret ;
        }
//printf("select2 branch=%d\n",pn->bp[ibranch].branch);
        if ( (ret=BUS_send_data(&branch[pn->bp[ibranch].branch],1,pn)) || (ret=BUS_readin_data(resp,3,pn)) ) {
            BUS_selection_error(pn, ret) ;
            return ret ;
        }
        if ( resp[2] != branch[pn->bp[ibranch].branch] ) {
//printf("select3=%d resp=%.2X %.2X %.2X\n",ret,resp[0],resp[1],resp[2]);
            STAT_ADD1(BUS_select_low_branch_errors);
            return -EINVAL ;
        }
    }
    if ( pn->dev ) {
//printf("Really select %s\n",pn->dev->code);
        memcpy( &sent[1], pn->sn, 8 ) ;
        if ( (ret=BUS_send_data(sent,1,pn)) ) {
            BUS_selection_error(pn, ret) ;
            return ret ;
        }
        if(sent[0] == 0x69) {
            if((ret=BUS_overdrive(ONEWIREBUSSPEED_OVERDRIVE, pn))< 0) {
                BUS_selection_error(pn, ret) ;
                return ret ;
            }
        }
        if ( (ret=BUS_send_data(&sent[1],8,pn)) ) {
            BUS_selection_error(pn, ret) ;
            return ret ;
        }
        return ret ;
    }
    return 0 ;
}

//--------------------------------------------------------------------------
/** The 'owFirst' doesn't find the first device on the 1-Wire Net.
 instead, it sets up for DS2480_next interator.

 serialnumber -- 8byte (64 bit) serial number found

 Returns:   0-device found 1-no dev or error
*/
int BUS_first(unsigned char * serialnumber, const struct parsedname * const pn ) {
    int ret ;
    // reset the search state
    memset(serialnumber,0,8);  // clear the serial number
    pn->si->LastDiscrepancy = -1;
    pn->si->LastFamilyDiscrepancy = -1;
    pn->si->LastDevice = 0 ;
    pn->si->ExtraReset = 0 ;

    if ( !pn->si->AnyDevices ) {
      LEVEL_DATA("BUS_first: No data will be returned\n");
    }

    ret = BUS_next(serialnumber,pn) ;

    /* BUS_reconnect() is called in DS9490_next_both().
     * Should perhaps move up the logic to this place instead. */
#if 0
    /* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
    if(ret >= -1) return ret;
    {
      int ret2 = BUS_reconnect( pn ) ;
      if(ret2) LEVEL_CONNECT("BUS_first returned error = %d\n", ret2) ;
    }
#endif
    return ret ;
}

int BUS_first_alarm(unsigned char * serialnumber, const struct parsedname * const pn ) {
    int ret ;
    // reset the search state
    memset(serialnumber,0,8);  // clear the serial number
    pn->si->LastDiscrepancy = -1 ;
    pn->si->LastFamilyDiscrepancy = -1 ;
    pn->si->LastDevice = 0 ;
    ret = BUS_next_alarm(serialnumber,pn) ;

    /* BUS_reconnect() is called in DS9490_next_both().
     * Should perhaps move up the logic to this place instead. */
#if 0
    /* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
    if(ret >= -1) return ret;
    {
      int ret2 = BUS_reconnect( pn ) ;
      if(ret2) LEVEL_CONNECT("BUS_first_alarm returned error = %d\n", ret2) ;
    }
#endif
    return ret;
}

int BUS_first_family(const unsigned char family, unsigned char * serialnumber, const struct parsedname * const pn ) {
    // reset the search state
    serialnumber[0] = family ;
    serialnumber[1] = 0x00 ;
    serialnumber[2] = 0x00 ;
    serialnumber[3] = 0x00 ;
    serialnumber[4] = 0x00 ;
    serialnumber[5] = 0x00 ;
    serialnumber[6] = 0x00 ;
    serialnumber[7] = 0x00 ;
    pn->si->LastDiscrepancy = 63;
    pn->si->LastFamilyDiscrepancy = -1 ;
    pn->si->LastDevice = 0 ;
    return BUS_next(serialnumber,pn) ;
}

//--------------------------------------------------------------------------
/** The DS2480_next function does a general search.  This function
 continues from the previous search state. The search state
 can be reset by using the DS2480_first function.

 Returns:  0=No problems, 1=Problems

 Sets LastDevice=1 if no more
*/
int BUS_next(unsigned char * serialnumber, const struct parsedname * const pn) {
    return BUS_next_both( serialnumber, 0xF0, pn ) ;
}

int BUS_next_alarm(unsigned char * serialnumber, const struct parsedname * const pn) {
    return BUS_next_both( serialnumber, 0xEC, pn ) ;
}
