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

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */

static int DS9097_PowerByte(const unsigned char byte, const unsigned int delay) ;
static int DS9097_ProgramPulse( void ) ;
static int DS9097_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int DS9097_reset( const struct parsedname * const pn ) ;
static int DS9097_read(unsigned char * const buf, const size_t size ) ;
static int DS9097_write( const unsigned char * const bytes, const size_t num ) ;
static int DS9097_level(int new_level) ;
static int DS9097_read_bits( unsigned char * const bits , const int length ) ;
static int DS9097_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length ) ;
static int DS9097_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len ) ;
static void DS9097_setroutines( struct interface_routines * const f ) ;

#define	OneBit	0xFF
#define ZeroBit 0xC0

/* Device-specific functions */
static void DS9097_setroutines( struct interface_routines * const f ) {
    f->write = DS9097_write ;
    f->read  = DS9097_read ;
    f->reset = DS9097_reset ;
    f->next_both = DS9097_next_both ;
    f->level = DS9097_level ;
    f->PowerByte = DS9097_PowerByte ;
    f->ProgramPulse = DS9097_ProgramPulse ;
    f->sendback_data = DS9097_sendback_data ;
    f->select        = BUS_select_low ;
}

/* Open a DS9097 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS9097_detect( void ) {
    /* Set up low-level routines */
    DS9097_setroutines( & iroutines ) ;
    /* Reset the bus */
    Adapter = adapter_DS9097 ; /* OWFS assigned value */
    adapter_name = "DS9097" ;
    busmode = bus_serial ;
    return DS9097_reset(NULL) ;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'byte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
// Delay delay msec and return to normal
//
/* Returns 0=good
   bad = -EIO
 */
static int DS9097_PowerByte(unsigned char byte, unsigned int delay) {
    int ret ;

    // flush the buffers
    COM_flush();

    // send the packet
    ret=BUS_send_data(&byte,1) ;
    if (ret) return ret ;

// indicate the port is now at power delivery
    ULevel = MODE_STRONG5;
    // delay
    UT_delay( delay ) ;

    // return to normal level
    return BUS_level(MODE_NORMAL) ;
}

static int DS9097_ProgramPulse( void ) {
    return -EIO; /* not available */
}


static int DS9097_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
    struct stateinfo * si = pn->si ;
    unsigned char search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    unsigned char bit_number ;
    unsigned char last_zero ;
    unsigned char bits[3] ;
    int ret ;

    // initialize for search
    last_zero = 0;

    // if the last call was not the last one
    if ( !si->AnyDevices ) si->LastDevice = 1 ;
    if ( si->LastDevice ) return -ENODEV ;

    /* Appropriate search command */
    if ( (ret=BUS_send_data(&search,1)) ) return ret ;
      // loop to do the search
    for ( bit_number=0 ;; ++bit_number ) {
        bits[1] = bits[2] = 0xFF ;
        if ( bit_number==0 ) { /* First bit */
	    /* get two bits (AND'ed bit and AND'ed complement) */
            if ( (ret=DS9097_sendback_bits(&bits[1],&bits[1],2)) ) return ret ;
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
	        /* Send chosen bit path, then check match on next two */
                if ( (ret=DS9097_sendback_bits(bits,bits,3)) ) return ret ;
            } else { /* last bit */
                if ( (ret=DS9097_sendback_bits(bits,bits,1)) ) return ret ;
                break ;
            }
        }
        if ( bits[1] ) {
            if ( bits[2] ) { /* 1,1 */
                break ; /* No devices respond */
            } else { /* 1,0 */
                search_direction = 1;  // bit write value for search
            }
        } else {
            if ( bits[2] ) { /* 0,1 */
                search_direction = 0;  // bit write value for search
            } else  if (bit_number < si->LastDiscrepancy) {
                // if this discrepancy if before the Last Discrepancy
                // on a previous next then pick the same as last time
                search_direction = UT_getbit(serialnumber,bit_number) ;
            } else if (bit_number == si->LastDiscrepancy) {
                search_direction = 1 ; // if equal to last pick 1, if not then pick 0
            } else {
                search_direction = 0 ;
                // if 0 was picked then record its position in LastZero
                last_zero = bit_number;
                // check for Last discrepancy in family
                if (last_zero < 9) si->LastFamilyDiscrepancy = last_zero;
            }
        }
        UT_setbit(serialnumber,bit_number,search_direction) ;

        // serial number search direction write bit
        //if ( (ret=DS9097_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(serialnumber,8) || (bit_number<64) || (serialnumber[0] == 0)) return -EIO ;
      // if the search was successful then

    si->LastDiscrepancy = last_zero;
    si->LastDevice = (last_zero == 0);
    return 0 ;
}

/* Set the 1-Wire Net line level.  The values for new_level are
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:    0 GOOD, !0 Error
   Actually very simple for passive adapter
*/
/* return 0=good
  -EIO not supported
 */
static int DS9097_level(int new_level) {
    if (new_level == ULevel) {     // check if need to change level
        return 0 ;
    }
    switch (new_level) {
    case MODE_NORMAL:
    case MODE_STRONG5:
        ULevel = new_level ;
        return 0 ;
    case MODE_PROGRAM:
        ULevel = MODE_NORMAL ;
        return -EIO ;
    }
    return -EIO ;
}

/* DS9097 Reset -- A little different frolm DS2480B */
/* Puts in 9600 baud, sends 11110000 then reads response */
static int DS9097_reset( const struct parsedname * const pn ) {
    unsigned char resetbyte = 0xF0 ;
    unsigned char c;
    struct termios term;
    int ret ;

    /* 8 data bits */
    tcgetattr(devfd, &term);
    term.c_cflag = CS8 | CREAD | HUPCL | CLOCAL;
    cfsetospeed(&term, B9600);
    cfsetispeed(&term, B9600);
    if (tcsetattr(devfd, TCSANOW, &term ) < 0 ) return -EIO ;
    if ( (ret=BUS_send_and_get(&resetbyte,1,&c,1)) ) return ret ;
    switch(c) {
    case 0:
        syslog(LOG_INFO,"1-wire bus short circuit.\n") ;
        /* fall through */
    case 0xF0:
        if (pn) pn->si->AnyDevices = 0 ;
        break ;
    default:
        if (pn) pn->si->AnyDevices = 1 ;
        ProgramAvailable = 0 ; /* from digitemp docs */
        UT_delay(5); //delay for DS1994
    }

    /* Reset all settings */
    term.c_lflag = 0;
    term.c_iflag = 0;
    term.c_oflag = 0;

    /* 1 byte at a time, no timer */
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;

    /* 6 data bits, Receiver enabled, Hangup, Dont change "owner" */
    term.c_cflag = CS6 | CREAD | HUPCL | CLOCAL;

    cfsetispeed(&term, B115200);       /* Set input speed to 115.2k    */
    cfsetospeed(&term, B115200);       /* Set output speed to 115.2k   */

    if(tcsetattr(devfd, TCSANOW, &term) < 0 ) return -EFAULT ;
    /* Flush the input and output buffers */
    COM_flush() ;
    return 0 ;
}

/* Assymetric */
/* Send a byte to the 9097 passive adapter */
/* return 0 valid, else <0 error */
/* no matching read */
static int DS9097_write( const unsigned char * const bytes, const size_t num ) {
    unsigned int i ;
    int remain = num - (UART_FIFO_SIZE>>3) ;
    unsigned int num8 = num<<3 ;
    if ( remain>0 ) return DS9097_write(bytes,UART_FIFO_SIZE>>3) || DS9097_write(&bytes[UART_FIFO_SIZE>>3],(unsigned)remain) ;
    for ( i=0;i<num8;++i) combuffer[i] = UT_getbit(bytes,i)?OneBit:ZeroBit;
    return BUS_send_and_get(combuffer,num8,NULL,0) ;
}

/* Assymetric */
/* Read bytes from the 9097 passive adapter */
/* Time out on each byte */
/* return 0 valid, else <0 error */
/* No matching read */
static int DS9097_read( unsigned char * const byte, const size_t num ) {
    int ret ;
    int remain = (int)num-(UART_FIFO_SIZE>>3) ;
    unsigned int i, num8 = num<<3 ;
    if ( remain > 0 ) {
        return DS9097_read( byte , UART_FIFO_SIZE>>3 ) || DS9097_read( &byte[UART_FIFO_SIZE>>3],remain) ;
    }
    if ( (ret=BUS_send_and_get(NULL,0,combuffer,num8)) ) return ret ;
    for ( i=0 ; i<num8 ; ++i ) UT_setbit(byte,i,combuffer[i]&0x01) ;
    return 0 ;
}

/* Symmetric */
/* send bits -- read bits */
/* Actually uses bit zero of each byte */
int DS9097_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length ) {
    int ret ;
    int i ;
    if ( length > UART_FIFO_SIZE ) return DS9097_sendback_bits(outbits,inbits,UART_FIFO_SIZE)||DS9097_sendback_bits(&outbits[UART_FIFO_SIZE],&inbits[UART_FIFO_SIZE],length-UART_FIFO_SIZE) ;
    for ( i=0 ; i<length ; ++i ) combuffer[i] = outbits[i] ? OneBit : ZeroBit ;
    if ( (ret= BUS_send_and_get(combuffer,(unsigned)length,inbits,(unsigned)length)) ) return ret ;
    for ( i=0 ; i<length ; ++i ) inbits[i] &= 0x01 ;
    return 0 ;
}

/* Symmetric */
/* send a bit -- read a bit */
static int DS9097_sendback_data( const unsigned char * data, unsigned char * const resp , const int len ) {
    int remain = len - (UART_FIFO_SIZE>>3) ;
//printf("sendback len=%d, U>>3=%d, remain=%d\n",len,(UART_FIFO_SIZE>>3),remain);
    if ( remain>0 ) {
        return DS9097_sendback_data( data,resp,UART_FIFO_SIZE>>3) || DS9097_sendback_data( &data[UART_FIFO_SIZE>>3],&resp[UART_FIFO_SIZE>>3],remain) ;
    } else {
        unsigned int i, bits = len<<3 ;
        int ret ;
        for ( i=0 ; i<bits ; ++i ) combuffer[i] = UT_getbit(data,i) ? OneBit : ZeroBit ;
        if ( (ret=BUS_send_and_get(combuffer,bits,combuffer,bits)) ) return ret ;
        for ( i=0 ; i<bits ; ++i ) UT_setbit(resp,i,combuffer[i]&0x01) ;
        return 0 ;
    }
}

/* Symetric */
/* gets specified number of bits, one per return byte */
static int DS9097_read_bits( unsigned char * const bits , const int length ) {
    int i, ret ;
    if ( length > UART_FIFO_SIZE ) return DS9097_read_bits(bits,UART_FIFO_SIZE)||DS9097_read_bits(&bits[UART_FIFO_SIZE],length-UART_FIFO_SIZE) ;
    memset( combuffer,0xFF,(size_t)length) ;
    if ( (ret=BUS_send_and_get(combuffer,(unsigned)length,bits,(unsigned)length)) ) return ret ;
    for ( i=0;i<length;++i) bits[i]&=0x01 ;
    return 0 ;
}
