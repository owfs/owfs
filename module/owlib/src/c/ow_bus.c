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
#include "ow_counters.h"
#include "ow_connection.h"

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
    LEVEL_CONNECT("SELECTION ERROR\n");
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
    if ( pn->dev && (pn->dev != DeviceThermostat) ) {
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

// ----------------------------------------------------------------
// Low level default routines, often superceded by more capable adapters

/* Symmetric */
/* send bytes, and read back -- calls lower level bit routine */
int BUS_sendback_data_low( const unsigned char * data, unsigned char * resp , const size_t len, const struct parsedname * pn ) {
    unsigned int i, bits = len<<3 ;
    int ret ;
    int remain = len - (UART_FIFO_SIZE>>3) ;

    /* Split into smaller packets? */
    if ( remain>0 ) return BUS_sendback_data( data,resp,UART_FIFO_SIZE>>3,pn)
                || BUS_sendback_data( &data[UART_FIFO_SIZE>>3],&resp[UART_FIFO_SIZE>>3],remain,pn) ;

    /* Encode bits */
    for ( i=0 ; i<bits ; ++i ) pn->in->combuffer[i] = UT_getbit(data,i) ? 0xFF : 0x00 ;
    
    /* Communication with DS9097 routine */
    if ( (ret=BUS_sendback_bits(pn->in->combuffer,pn->in->combuffer,bits,pn) ) ) {
        STAT_ADD1(DS9097_sendback_data_errors);
        return ret ;
    }

    /* Decode Bits */
    for ( i=0 ; i<bits ; ++i ) UT_setbit(resp,i,pn->in->combuffer[i]&0x01) ;

    return 0 ;
}

// Send 8 bits of communication to the 1-Wire Net
// Delay delay msec and return to normal
//
// Power is supplied by normal pull-up resistor, nothing special
//
/* Returns 0=good
   bad = -EIO
 */
int BUS_PowerByte_low(unsigned char byte, unsigned int delay, const struct parsedname * const pn) {
    int ret ;
    // send the packet
    if((ret=BUS_send_data(&byte,1,pn))) {
        STAT_ADD1(DS9097_PowerByte_errors);
        return ret ;
    }
    // delay
    UT_delay( delay ) ;

    return ret;
}

/* Low level search routines -- bit banging */
/* Not used by more advanced adapters */
int BUS_next_both_low(unsigned char * serialnumber, unsigned char search, const struct parsedname * pn) {
    struct stateinfo * si = pn->si ;
    int search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    int bit_number ;
    int last_zero = -1 ;
    unsigned char bits[3] ;
    int ret ;

    // initialize for search
    // if the last call was not the last one
    if ( !si->AnyDevices ) si->LastDevice = 1 ;
    if ( si->LastDevice ) return -ENODEV ;

    /* Appropriate search command */
    if ( (ret=BUS_send_data(&search,1,pn)) ) return ret ;
      // loop to do the search
    for ( bit_number=0 ;; ++bit_number ) {
        bits[1] = bits[2] = 0xFF ;
        if ( bit_number==0 ) { /* First bit */
            /* get two bits (AND'ed bit and AND'ed complement) */
            if ( (ret=BUS_sendback_bits(&bits[1],&bits[1],2,pn)) ) {
                STAT_ADD1(DS9097_next_both_errors);
                return ret ;
            }
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                /* Send chosen bit path, then check match on next two */
                if ( (ret=BUS_sendback_bits(bits,bits,3,pn)) ) {
                    STAT_ADD1(DS9097_next_both_errors);
                    return ret ;
                }
            } else { /* last bit */
                if ( (ret=BUS_sendback_bits(bits,bits,1,pn)) ) {
                    STAT_ADD1(DS9097_next_both_errors);
                    return ret ;
                }
                break ;
            }
        }
        if ( bits[1] ) {
            if ( bits[2] ) { /* 1,1 */
                break ; /* No devices respond */
            } else { /* 1,0 */
                search_direction = 1;  // bit write value for search
            }
        } else if ( bits[2] ) { /* 0,1 */
            search_direction = 0;  // bit write value for search
        } else if (bit_number > si->LastDiscrepancy) {  /* 0,0 looking for last discrepancy in this new branch */
            // Past branch, select zeros for now
            search_direction = 0 ;
            last_zero = bit_number;
        } else if (bit_number == si->LastDiscrepancy) {  /* 0,0 -- new branch */
            // at branch (again), select 1 this time
            search_direction = 1 ; // if equal to last pick 1, if not then pick 0
        } else  if (UT_getbit(serialnumber,bit_number)) { /* 0,0 -- old news, use previous "1" bit */
            // this discrepancy is before the Last Discrepancy
            search_direction = 1 ;
        } else {  /* 0,0 -- old news, use previous "0" bit */
            // this discrepancy is before the Last Discrepancy
            search_direction = 0 ;
            last_zero = bit_number;
        }
        // check for Last discrepancy in family
        //if (last_zero < 9) si->LastFamilyDiscrepancy = last_zero;
        UT_setbit(serialnumber,bit_number,search_direction) ;

        // serial number search direction write bit
        //if ( (ret=DS9097_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(serialnumber,8) || (bit_number<64) || (serialnumber[0] == 0)) {
        STAT_ADD1(DS9097_next_both_errors);
      /* A minor "error" and should perhaps only return -1 to avoid
      * reconnect */
        return -EIO ;
    }
    if((serialnumber[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }
    // if the search was successful then

    si->LastDiscrepancy = last_zero;
//    printf("Post, lastdiscrep=%d\n",si->LastDiscrepancy) ;
    si->LastDevice = (last_zero < 0);
    return 0 ;
}

