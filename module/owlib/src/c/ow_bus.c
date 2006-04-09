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

static int BUS_select_raw(int depth, const struct parsedname * const pn) ;

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
        if ((ret=BUS_sendback_data( data, resp, len,pn ))==0) {
            if ((ret=memcmp(data, resp, (size_t) len))) {
                ret = -EPROTO ;
                STAT_ADD1_BUS(BUS_echo_errors,pn->in);
            }
        }
    }
    return ret ;
}

/** readin_data
  Send 0xFFs and return response block
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

static int BUS_selection_error( int ret ) {
    STAT_ADD1(BUS_select_low_errors);
    LEVEL_CONNECT("SELECTION ERROR\n");
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
    // match Serial Number command 0x55
    unsigned char sent[9] = { 0x55, } ;
    unsigned char alo[] = { 0xCC, 0x66, } ;
    int pl = pn->pathlength ;

    LEVEL_DEBUG("Selecting a path (and device) path=%s SN="SNformat"\n",pn->path,SNvar(pn->sn));

    /* Very messy, we may need to clear all the DS2409 couplers up the the current branch */
    if ( pl == 0 ) { /* no branches, overdrive possible */
        if ( pn->in->branch.sn[0] ) { // need to root branch */
            LEVEL_DEBUG("Clearing root branch\n") ;
            if ( BUS_select_raw(0,pn) || BUS_send_data(alo,2,pn) ) return 1 ;
        }
        pn->in->branch.sn[0] = 0x00 ; // flag as no branches turned on
        if(pn->in->use_overdrive_speed) { // overdrive?
            if((ret=BUS_testoverdrive(pn)) < 0) {
                BUS_selection_error(ret) ;
                return ret ;
            } else {
                //printf("use overdrive speed\n");
                sent[0] = 0x69 ;
            }
        }
    } else if ( memcmp( pn->in->branch.sn, pn->bp[pl-1].sn, 8 ) ) { /* different path */
        int iclear ;
        LEVEL_DEBUG("Clearing all branches to level %d\n",pl) ;
        for ( iclear = 0 ; iclear <= pl ; ++iclear ) {
            // All lines off
            if ( BUS_select_raw(iclear,pn) || BUS_send_data( alo,2,pn) ) return 1 ;
        }
        memcpy( pn->in->branch.sn, pn->bp[pl-1].sn, 8 ) ;
        pn->in->branch.branch =  pn->bp[pl-1].branch ;
    } else if ( pn->in->branch.branch != pn->bp[pl-1].branch ) { /* different branch */
        LEVEL_DEBUG("Clearing last branches (level %d)\n",pl) ;
        if ( BUS_select_raw(pl,pn) || BUS_send_data( alo,2,pn) ) return 1 ; // clear just last level
        pn->in->branch.branch =  pn->bp[pl-1].branch ;
    }

    /* Now select */
    if ( BUS_select_raw(pn->pathlength,pn) ) return 1 ;

    if ( pn->pathlength ) {
        memcpy( pn->in->branch.sn, pn->bp[pn->pathlength-1].sn,8 ) ;
    } else {
        pn->in->branch.sn[0] = 0x00 ;
    }

    if ( pn->dev && (pn->dev != DeviceThermostat) ) {
//printf("Really select %s\n",pn->dev->code);
        memcpy( &sent[1], pn->sn, 8 ) ;
        if ( (ret=BUS_send_data(sent,1,pn)) ) {
            BUS_selection_error(ret) ;
            return ret ;
        }
        if(sent[0] == 0x69) {
            if((ret=BUS_overdrive(ONEWIREBUSSPEED_OVERDRIVE, pn))< 0) {
                BUS_selection_error(ret) ;
                return ret ;
            }
        }
        if ( (ret=BUS_send_data(&sent[1],8,pn)) ) {
            BUS_selection_error(ret) ;
            return ret ;
        }
        return ret ;
    }
    return 0 ;
}

/* Select without worring about branch turn-offs */
static int BUS_select_raw(int depth, const struct parsedname * const pn) {
    int ret ;
    int ibranch ;
    // match Serial Number command 0x55
    unsigned char sent[9] = { 0x55, } ;
    unsigned char branch[2] = { 0xCC, 0x33, } ; /* Main, Aux */
    unsigned char resp[3] ;

    
    // reset the 1-wire
    // send/recieve the transfer buffer
    // verify that the echo of the writes was correct
    if ( (ret=BUS_reset(pn)) ) {
        BUS_selection_error(ret) ;
        return ret ;
    }
    for ( ibranch=0 ; ibranch < depth ; ++ibranch ) {
        memcpy( &sent[1], pn->bp[ibranch].sn, 8 ) ;
       /* Perhaps support overdrive here ? */
        if ( (ret=BUS_send_data(sent,9,pn)) ) {
            BUS_selection_error(ret) ;
            return ret ;
        }
        if ( (ret=BUS_send_data(&branch[pn->bp[ibranch].branch],1,pn)) || (ret=BUS_readin_data(resp,3,pn)) ) {
            BUS_selection_error(ret) ;
            return ret ;
        }
        if ( resp[2] != branch[pn->bp[ibranch].branch] ) {
            STAT_ADD1(BUS_select_low_branch_errors);
            return -EINVAL ;
        }
    }
    return 0 ;
}

//--------------------------------------------------------------------------
/** The 'owFirst' doesn't find the first device on the 1-Wire Net.
 instead, it sets up for DS2480_next interator.

 serialnumber -- 8byte (64 bit) serial number found

 Returns:   0-device found 1-no dev or error
*/
int BUS_first(struct device_search * ds, const struct parsedname * const pn ) {
    // reset the search state
    memset(ds->sn,0,8);  // clear the serial number
    ds->LastDiscrepancy = -1;
    ds->LastFamilyDiscrepancy = -1;
    ds->LastDevice = 0 ;
    pn->in->ExtraReset = 0 ;
    ds->search = 0xF0 ;

    if ( !pn->in->AnyDevices ) {
      LEVEL_DATA("BUS_first: No data will be returned\n");
    }

    return BUS_next(ds,pn) ;

}

int BUS_first_alarm(struct device_search * ds, const struct parsedname * const pn ) {
    // reset the search state
    memset(ds->sn,0,8);  // clear the serial number
    ds->LastDiscrepancy = -1;
    ds->LastFamilyDiscrepancy = -1;
    ds->LastDevice = 0 ;
    ds->search = 0xEC ;

    return BUS_next(ds,pn) ;
}

int BUS_first_family(const unsigned char family, struct device_search * ds, const struct parsedname * const pn ) {
    // reset the search state
    memset(ds->sn,0,8);  // clear the serial number
    ds->sn[0] = family ;
    ds->LastDiscrepancy = 63;
    ds->LastFamilyDiscrepancy = -1;
    ds->LastDevice = 0 ;
    ds->search = 0xF0 ;

    return BUS_next(ds,pn) ;
}


static int BUS_next_redoable( const struct device_search * ds, struct device_search * ds2, const struct parsedname * pn ) {
    memcpy( ds2, ds, sizeof(struct device_search) ) ;
    if ( BUS_select(pn) ) {
        printf("Couldn't select\n");
        return -EINVAL ;
    }
    return BUS_next_both( &ds2, pn ) ;
}

static int BUS_next_anal( struct device_search * ds, const struct parsedname * const pn) {
    struct device_search ds1, ds2 ;
    int i ;
    int ret ;
    BUS_next_redoable( ds, &ds2, pn ) ;
    for( i=0 ; i<5 ; ++i ) {
        memcpy( &ds1, &ds2 , sizeof(struct device_search) ) ;
        ret = BUS_next_redoable( ds, &ds2, pn ) ;
        if ( memcmp( &ds1, &ds2 , sizeof(struct device_search) ) == 0 ) {
            memcpy( ds, &ds2 , sizeof(struct device_search) ) ;
            printf("ANAL success i=%d\n",i) ;
            return ret ;
        }
    }
    printf("ANAL failure\n");
    return -EIO ;
}


//--------------------------------------------------------------------------
/** The DS2480_next function does a general search.  This function
 continues from the previous search state. The search state
 can be reset by using the DS2480_first function.

 Returns:  0=No problems, 1=Problems

 Sets LastDevice=1 if no more
*/
int BUS_next( struct device_search * ds, const struct parsedname * const pn) {
    int ret ;
    //ret = pn->pathlength ? BUS_next_anal(ds,pn) : BUS_next_both( ds, pn ) ;
    if ( pn->pathlength ) BUS_select(pn) ;
    ret = BUS_next_both( ds, pn ) ;
    if (ret) { STAT_ADD1_BUS(BUS_next_errors,pn->in) ; }
    return ret ;
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
        STAT_ADD1_BUS(BUS_byte_errors,pn->in);
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
int BUS_PowerByte_low(unsigned char byte, unsigned char * resp, unsigned int delay, const struct parsedname * const pn) {
    int ret ;
    // send the packet
    if((ret=BUS_sendback_data(&byte,resp,1,pn))) {
        STAT_ADD1_BUS(BUS_PowerByte_errors,pn->in);
        return ret ;
    }
    // delay
    UT_delay( delay ) ;

    return ret;
}

/* Low level search routines -- bit banging */
/* Not used by more advanced adapters */
int BUS_next_both_low(struct device_search * ds, const struct parsedname * pn) {
    int search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    int bit_number ;
    int last_zero = -1 ;
    unsigned char bits[3] ;
    int ret ;

    // initialize for search
    // if the last call was not the last one
    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    /* Appropriate search command */
    if ( (ret=BUS_send_data(&(ds->search),1,pn)) ) return ret ;
      // loop to do the search
    for ( bit_number=0 ;; ++bit_number ) {
        bits[1] = bits[2] = 0xFF ;
        if ( bit_number==0 ) { /* First bit */
            /* get two bits (AND'ed bit and AND'ed complement) */
            if ( (ret=BUS_sendback_bits(&bits[1],&bits[1],2,pn)) ) return ret ;
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                /* Send chosen bit path, then check match on next two */
                if ( (ret=BUS_sendback_bits(bits,bits,3,pn)) ) return ret ;
            } else { /* last bit */
                if ( (ret=BUS_sendback_bits(bits,bits,1,pn)) ) return ret ;
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
        } else if (bit_number > ds->LastDiscrepancy) {  /* 0,0 looking for last discrepancy in this new branch */
            // Past branch, select zeros for now
            search_direction = 0 ;
            last_zero = bit_number;
        } else if (bit_number == ds->LastDiscrepancy) {  /* 0,0 -- new branch */
            // at branch (again), select 1 this time
            search_direction = 1 ; // if equal to last pick 1, if not then pick 0
        } else  if (UT_getbit(ds->sn,bit_number)) { /* 0,0 -- old news, use previous "1" bit */
            // this discrepancy is before the Last Discrepancy
            search_direction = 1 ;
        } else {  /* 0,0 -- old news, use previous "0" bit */
            // this discrepancy is before the Last Discrepancy
            search_direction = 0 ;
            last_zero = bit_number;
        }
        // check for Last discrepancy in family
        //if (last_zero < 9) si->LastFamilyDiscrepancy = last_zero;
        UT_setbit(ds->sn,bit_number,search_direction) ;

        // serial number search direction write bit
        //if ( (ret=DS9097_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(ds->sn,8) || (bit_number<64) || (ds->sn[0] == 0)) {
      /* A minor "error" and should perhaps only return -1 to avoid
      * reconnect */
        return -EIO ;
    }
    if((ds->sn[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }
    // if the search was successful then

    ds->LastDiscrepancy = last_zero;
//    printf("Post, lastdiscrep=%d\n",si->LastDiscrepancy) ;
    ds->LastDevice = (last_zero < 0);
    LEVEL_DEBUG("Generic_next_both SN found: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",ds->sn[0],ds->sn[1],ds->sn[2],ds->sn[3],ds->sn[4],ds->sn[5],ds->sn[6],ds->sn[7]) ;
    return 0 ;
}

// RESET called with bus locked
int BUS_reset(const struct parsedname * pn) {
    int ret = (pn->in->iroutines.reset)(pn) ;
    /* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
    if ( ret==1 ) {
        return 1 ;
    } else if (ret) {
        pn->in->reconnect_state ++ ; // Flag for eventual reconnection
        STAT_ADD1_BUS(BUS_reset_errors, pn->in ) ;
    } else {
        pn->in->reconnect_state = reconnect_ok ; // Flag as good!
    }

    return ret ;
}
