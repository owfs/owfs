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

// Mode Commands
#define MODE_DATA                      0xE1
#define MODE_COMMAND                   0xE3
#define MODE_STOP_PULSE                0xF1

// Data/Command mode select bits
#define MODSEL_DATA                    0x00
#define MODSEL_COMMAND                 0x02

/** DS2480_send_data
    Send a data and expect response match
    puts into data mode if needed.
    return 0=good
    bad return sendback_data
    -EIO if memcpy
 */
int BUS_send_data( const unsigned char * const data , const int len ) {
    int ret ;
    if ( len>16 ) {
        int dlen = len-(len>>1) ;
        (ret=BUS_send_data(data,dlen)) || (ret=BUS_send_data(&data[dlen],len>>1)) ;
    } else {
        unsigned char resp[16] ;
        (ret=BUS_sendback_data( data, resp, len )) ||  (ret=memcmp(data, resp, (size_t) len)?-EIO:0) ;
    }
    return ret ;
}

/** DS2480_readin_data
  Send 0xFFs and return response block
  puts into data mode if needed.
 Returns 0=good
   Bad sendback_data
 */
int BUS_readin_data( unsigned char * const data , const int len ) {
    return BUS_sendback_data( memset(data, 0xFF, (size_t) len),data,len) ;
}

int BUS_send_and_get( const unsigned char * const bussend, const size_t sendlength, unsigned char * const busget, const size_t getlength ) {
    int gl = getlength ;
    int ret ;

/*
{
int i;
printf("SAG prewrite: ");
for(i=0;i<sendlength;++i)printf("%.2X ",bussend[i]);
printf("\n");
}
*/
    if ( sendlength>0 ) {
        /* first flush */
        COM_flush();
//printf("SAG sendlength=%d, getlength=%d\n",sendlength,getlength);
//{
//int i;
//printf("SAG write: ");
//for(i=0;i<sendlength;++i)printf("%.2X ",bussend[i]);
//printf("\n");
//}
        /* send out string */
        if ( write(devfd,bussend,sendlength) != (int)sendlength ) return -EIO; /* Send the bit */
    }
//printf("SAG written\n");
    /* get back string -- with timeout and partial read loop */
    if ( busget && getlength ) {
    while ( gl > 0 ) {
//printf("SAG readlength=%d\n",gl);
        fd_set readset;
        struct timeval tv = {5,0}; /* 5 seconds */
        /* Initialize readset */
        FD_ZERO(&readset);
        FD_SET(devfd, &readset);

        /* Read if it doesn't timeout first */
        if( select( devfd+1, &readset, NULL, NULL, &tv ) > 0 ) {
//printf("SAG selected\n");
        /* Is there something to read? */
        if( FD_ISSET( devfd, &readset )==0 ) return -EIO ; /* error */
//printf("SAG selected readset\n");
/*
{
int i;
printf("SAG prebuf: ");
for(i=0;i<getlength;++i)printf("%.2X ",busget[i]);
printf("\n");
}
*/
        ret = read( devfd, &busget[getlength-gl], (size_t) gl ) ; /* get available bytes */
//printf("SAG postread ret=%d\n",ret);
        if (ret<0) return ret ;
        gl -= ret ;
//printf("SAG postreadlength=%d\n",gl);
/*
{
int i;
printf("SAG postbuf: ");
for(i=0;i<getlength;++i)printf("%.2X ",busget[i]);
printf("\n");
}
*/
        } else { /* timed out */
//printf("SAG selected timeout\n");
        return -EAGAIN;
        }
    }
    } else {
        COM_flush() ;
    }
    return 0 ;
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
int BUS_select_low(const struct parsedname * const pn) {
    int ret ;
    int ibranch ;
    // match Serial Number command 0x55
    unsigned char sent[9] = { 0x55, } ;
    unsigned char branch[2] = { 0xCC, 0x33, } ; /* Main, Aux */
    unsigned char resp[3] ;

//printf("SELECT\n");
    // reset the 1-wire
    // send/recieve the transfer buffer
    // verify that the echo of the writes was correct
    if ( (ret=BUS_reset(pn)) ) return ret ;
    for ( ibranch=0 ; ibranch < pn->pathlength ; ++ibranch ) {
       memcpy( &sent[1], pn->bp[ibranch].sn, 8 ) ;
//printf("select ibranch=%d %.2X %.2X.%.2X%.2X%.2X%.2X%.2X%.2X %.2X\n",ibranch,send[0],send[1],send[2],send[3],send[4],send[5],send[6],send[7],send[8]);
        if ( (ret=BUS_send_data(sent,9)) ) return ret ;
//printf("select2 branch=%d\n",pn->bp[ibranch].branch);
        if ( (ret=BUS_send_data(&branch[pn->bp[ibranch].branch],1)) || (ret=BUS_readin_data(resp,3)) ) return ret ;
        if ( resp[2] != branch[pn->bp[ibranch].branch] ) return -EINVAL ;
//printf("select3=%d resp=%.2X %.2X %.2X\n",ret,resp[0],resp[1],resp[2]);
    }
    if ( pn->dev ) {
//printf("Really select %s\n",pn->dev->code);
        memcpy( &sent[1], pn->sn, 8 ) ;
        return BUS_send_data( sent , 9 ) ;
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
    // reset the search state
    pn->si->LastDiscrepancy = -1;
    pn->si->LastFamilyDiscrepancy = -1;
    pn->si->LastDevice = 0 ;
    return BUS_next(serialnumber,pn) ;
}

int BUS_first_alarm(unsigned char * serialnumber, const struct parsedname * const pn ) {
    // reset the search state
    pn->si->LastDiscrepancy = -1 ;
    pn->si->LastFamilyDiscrepancy = -1 ;
    pn->si->LastDevice = 0 ;
    return BUS_next_alarm(serialnumber,pn) ;
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
