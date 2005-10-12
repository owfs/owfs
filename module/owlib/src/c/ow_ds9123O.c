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

/* DS9123O support
   PIC (Actually PICBrick) based
   PIC16C745

   Actually the adapter supports several protocols, including 1-wire, 2-wire, 3-wire, SPI...
   but we only support 1-wire

   USB 1.1 based, using HID interface

   ENDPOINT 1, 8-byte commands
   simple commands: reset, in/out 1bit, 1byte, ...
*/


/* All the rest of the program sees is the DS9123O_detect and the entry in iroutines */

static int DS9123O_PowerByte( const unsigned char byte, const unsigned int delay, const struct parsedname * const pn) ;
static int DS9123O_ProgramPulse( const struct parsedname * const pn ) ;
static int DS9123O_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int DS9123O_reset( const struct parsedname * const pn ) ;
static int DS9123O_reconnect( const struct parsedname * const pn ) ;
static int DS9123O_read( unsigned char * const buf, const size_t size, const struct parsedname * const pn ) ;
static int DS9123O_write( const unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) ;
static int DS9123O_level(int new_level, const struct parsedname * const pn) ;
static int DS9123O_read_bits( unsigned char * const bits , const int length, const struct parsedname * const pn ) ;
static int DS9123O_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length, const struct parsedname * const pn ) ;
static int DS9123O_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len, const struct parsedname * const pn ) ;
static void DS9123O_setroutines( struct interface_routines * const f ) ;
static int DS9123O_send_and_get( const unsigned char * const bussend, const size_t sendlength, unsigned char * const busget, const size_t getlength, const struct parsedname * pn ) ;

#define	OneBit	0xFF
#define ZeroBit 0xC0

/*'*********************************************
'Available One Wire PICBrick Dispatch Commands
 *'*********************************************/

#define DS9123O_VendorID     0x04FA
#define DS9123O_ProductID    0x9123

#define DS9123O_owreset     0x10
#define DS9123O_owresetskip     0x11
#define DS9123O_owtx8     0x12
#define DS9123O_owrx8     0x13
#define DS9123O_owpacketread     0x14
#define DS9123O_owpacketwrite     0x15
#define DS9123O_owread8bytes     0x16
#define DS9123O_owdqhi     0x17
#define DS9123O_owdqlow     0x18
#define DS9123O_owreadbit     0x19
#define DS9123O_owwritebit     0x1A
#define DS9123O_owtx8turbo     0x1B
#define DS9123O_owrx8turbo     0x1C
#define DS9123O_owtx1turbo     0x1D
#define DS9123O_owrx1turbo     0x1E
#define DS9123O_owresetturbo     0x1F
#define DS9123O_owpacketreadturbo     0x80
#define DS9123O_owpacketwriteturbo     0x81
#define DS9123O_owread8bytesturbo     0x82
#define DS9123O_owtx1rx2     0x83
#define DS9123O_owrx2     0x84
#define DS9123O_owtx1rx2turbo     0x85
#define DS9123O_owrx2turbo     0x86

/* Calls from visual basic program:
DS9123OwithOther.frm:4730:            WriteToPIC doowtx8turbo, Command, doportblongpulselow, PinMask, 4, 1, nullcmd, nullcmd
DS9123OwithOther.frm:4732:            WriteToPIC doowtx8, Command, doportblongpulselow, PinMask, 4, 1, nullcmd, nullcmd
DS9123OwithOther.frm:4750:            WriteToPIC doowtx8turbo, data, nullcmd, nullcmd, _
DS9123OwithOther.frm:4753:            WriteToPIC doowtx8, data, nullcmd, nullcmd, _
DS9123OwithOther.frm:4783:'            OutputReportData(0) = doowpacketwriteturbo
DS9123OwithOther.frm:4785:'            OutputReportData(0) = doowpacketwrite
DS9123OwithOther.frm:4798:            WriteToPIC doowpacketwriteturbo, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4801:            WriteToPIC doowpacketwrite, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4841:'            OutputReportData(0) = doowpacketwriteturbo
DS9123OwithOther.frm:4843:'            OutputReportData(0) = doowpacketwrite
DS9123OwithOther.frm:4857:            WriteToPIC doowpacketwriteturbo, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4860:            WriteToPIC doowpacketwrite, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4886:            WriteToPIC doowtx1turbo, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:4889:            WriteToPIC doowwritebit, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:4908:            WriteToPIC doowdqhi, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:4911:            WriteToPIC doowdqlow, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:4965:                    OutputReportData(I) = doowread8bytesturbo
DS9123OwithOther.frm:4967:                    OutputReportData(I) = doowread8bytes
DS9123OwithOther.frm:4988:                WriteToPIC doowpacketread, CByte(bytestoread), nullcmd, nullcmd, _
DS9123OwithOther.frm:4991:                WriteToPIC doowpacketreadturbo, CByte(bytestoread), nullcmd, nullcmd, _
DS9123OwithOther.frm:5016:            WriteToPIC doowrx1turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5019:            WriteToPIC doowreadbit, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5041:            WriteToPIC doowrx2turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5044:            WriteToPIC doowrx2, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5074:            WriteToPIC doowtx1rx2turbo, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:5077:            WriteToPIC doowtx1rx2, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:5103:            WriteToPIC doowrx8turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5106:            WriteToPIC doowrx8, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5126:            WriteToPIC doowresetturbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5129:            WriteToPIC doowreset, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5154:            WriteToPIC doowresetturbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5157:            WriteToPIC doowreset, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5174:            WriteToPIC doowresetturbo, doowtx8turbo, &HCC, nullcmd, _
DS9123OwithOther.frm:5177:            WriteToPIC doowresetskip, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5202:            WriteToPIC doowresetturbo, doowtx8turbo, Command, nullcmd, _
DS9123OwithOther.frm:5205:            WriteToPIC doowreset, doowtx8, Command, nullcmd, _
DS9123OwithOther.frm:5230:            WriteToPIC doowresetturbo, doowtx8turbo, &HCC, doowtx8turbo, _
DS9123OwithOther.frm:5233:            WriteToPIC doowresetskip, doowtx8, Command, nullcmd, _
DS9123OwithOther.frm:7087:            WriteToPIC doowrx8turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:7090:            WriteToPIC doowrx8, nullcmd, nullcmd, nullcmd, _
*/


/* Device-specific functions */
static void DS9123O_setroutines( struct interface_routines * const f ) {
    f->write = DS9123O_write ;
    f->read  = DS9123O_read ;
    f->reset = DS9123O_reset ;
    f->next_both = DS9123O_next_both ;
    f->level = DS9123O_level ;
    f->PowerByte = DS9123O_PowerByte ;
    f->ProgramPulse = DS9123O_ProgramPulse ;
    f->sendback_data = DS9123O_sendback_data ;
    f->select        = BUS_select_low ;
    f->overdrive = NULL ;
    f->testoverdrive = NULL ;
    f->reconnect = DS9123O_reconnect ;
}

/* Open a DS9123O after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS9123O_detect( struct connection_in * in ) {
    struct stateinfo si ;
    struct parsedname pn ;
    int ret ;
    
    memset(&pn, 0, sizeof(struct parsedname));
    pn.si = &si ;
    /* Set up low-level routines */
    DS9123O_setroutines( & (in->iroutines) ) ;

    /* Reset the bus */
    in->Adapter = adapter_DS9123O ; /* OWFS assigned value */
    in->adapter_name = "DS9123O" ;
    in->busmode = bus_serial ;
    
    if ( (ret=FS_ParsedName(NULL,&pn)) ) {
        STAT_ADD1(DS9123O_detect_errors);
      return ret ;
    }
    pn.in = in ;

    if((ret = DS9123O_reset(&pn))) {
        STAT_ADD1(DS9123O_detect_errors);
    }
    return ret;
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
static int DS9123O_PowerByte(unsigned char byte, unsigned int delay, const struct parsedname * const pn) {
    int ret ;

    // flush the buffers
    COM_flush(pn);

    // send the packet
    if((ret=BUS_send_data(&byte,1,pn))) {
        STAT_ADD1(DS9123O_PowerByte_errors);
      return ret ;
    }
// indicate the port is now at power delivery
    pn->in->ULevel = MODE_STRONG5;
    // delay
    UT_delay( delay ) ;

    // return to normal level
    if((ret=BUS_level(MODE_NORMAL,pn))) {
        STAT_ADD1(DS9123O_PowerByte_errors);
    }
    return ret;
}

static int DS9123O_reconnect( const struct parsedname * const pn ) {
    STAT_ADD1(BUS_reconnect);

    if ( !pn || !pn->in ) return -EIO;
    STAT_ADD1(pn->in->bus_reconnect);

    COM_close(pn->in);
    usleep(100000);
    if(!COM_open(pn->in)) {
      if(!DS9123O_detect(pn->in)) {
        LEVEL_DEFAULT("DS9123O adapter reconnected\n");
        return 0;
      }
    }
    STAT_ADD1(BUS_reconnect_errors);
    STAT_ADD1(pn->in->bus_reconnect_errors);
    LEVEL_DEFAULT("Failed to reconnect DS9123O adapter!\n");
    return -EIO ;
}

static int DS9123O_ProgramPulse( const struct parsedname * const pn ) {
    (void) pn ;
    return -EIO; /* not available */
}


static int DS9123O_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
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
    if ( (ret=BUS_send_data(&search,1,pn)) ) return ret ;
      // loop to do the search
    for ( bit_number=0 ;; ++bit_number ) {
        bits[1] = bits[2] = 0xFF ;
        if ( bit_number==0 ) { /* First bit */
            /* get two bits (AND'ed bit and AND'ed complement) */
            if ( (ret=DS9123O_sendback_bits(&bits[1],&bits[1],2,pn)) ) {
                STAT_ADD1(DS9123O_next_both_errors);
                return ret ;
            }
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                /* Send chosen bit path, then check match on next two */
                if ( (ret=DS9123O_sendback_bits(bits,bits,3,pn)) ) {
                    STAT_ADD1(DS9123O_next_both_errors);
                    return ret ;
                }
            } else { /* last bit */
                if ( (ret=DS9123O_sendback_bits(bits,bits,1,pn)) ) {
                    STAT_ADD1(DS9123O_next_both_errors);
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
        //if ( (ret=DS9123O_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(serialnumber,8) || (bit_number<64) || (serialnumber[0] == 0)) {
        STAT_ADD1(DS9123O_next_both_errors);
      /* A minor "error" and should perhaps only return -1 to avoid
       * reconnect */
      return -EIO ;
    }
    if((*serialnumber & 0x7F) == 0x04) {
      /* We found a DS1994/DS2404 which require longer delays */
      pn->in->ds2404_compliance = 1 ;
    }
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
static int DS9123O_level(int new_level, const struct parsedname * const pn) {
    if (new_level == pn->in->ULevel) {     // check if need to change level
        return 0 ;
    }
    switch (new_level) {
    case MODE_NORMAL:
    case MODE_STRONG5:
        pn->in->ULevel = new_level ;
        return 0 ;
    case MODE_PROGRAM:
        pn->in->ULevel = MODE_NORMAL ;
        STAT_ADD1(DS9123O_level_errors);
        return -EIO ;
    }
    STAT_ADD1(DS9123O_level_errors);
    return -EIO ;
}

/* DS9123O Reset -- A little different frolm DS2480B */
/* Puts in 9600 baud, sends 11110000 then reads response */
static int DS9123O_reset( const struct parsedname * const pn ) {
    unsigned char resetbyte = 0xF0 ;
    unsigned char c;
    struct termios term;
    int fd = pn->in->fd ;
    int ret ;

    if(fd < 0) return -1;

    /* 8 data bits */
    tcgetattr(fd, &term);
    term.c_cflag = CS8 | CREAD | HUPCL | CLOCAL;
    cfsetospeed(&term, B9600);
    cfsetispeed(&term, B9600);
    if (tcsetattr(fd, TCSANOW, &term ) < 0 ) {
        STAT_ADD1(DS9123O_reset_tcsetattr_errors);
        return -EIO ;
    }
    if ( (ret=DS9123O_send_and_get(&resetbyte,1,&c,1,pn)) ) {
        STAT_ADD1(DS9123O_reset_errors);
        return ret ;
    }

    switch(c) {
    case 0:
        LEVEL_CONNECT("1-wire bus short circuit.\n")
        /* fall through */
    case 0xF0:
        pn->si->AnyDevices = 0 ;
        break ;
    default:
        pn->si->AnyDevices = 1 ;
        pn->in->ProgramAvailable = 0 ; /* from digitemp docs */
        if(pn->in->ds2404_compliance) {
            // extra delay for alarming DS1994/DS2404 compliance
            UT_delay(5);
        }
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

#ifndef B115200
    /* MacOSX support max 38400 in termios.h ? */
    cfsetispeed(&term, B38400);       /* Set input speed to 38.4k    */
    cfsetospeed(&term, B38400);       /* Set output speed to 38.4k   */
#else
    cfsetispeed(&term, B115200);       /* Set input speed to 115.2k    */
    cfsetospeed(&term, B115200);       /* Set output speed to 115.2k   */
#endif

    if(tcsetattr(fd, TCSANOW, &term) < 0 ) {
        STAT_ADD1(DS9123O_reset_tcsetattr_errors);
        return -EFAULT ;
    }
    /* Flush the input and output buffers */
    COM_flush(pn) ;
    return 0 ;
}

/* Assymetric */
/* Send a byte to the 9097 passive adapter */
/* return 0 valid, else <0 error */
/* no matching read */
static int DS9123O_write( const unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) {
    int ret;
    unsigned int i ;
    int remain = num - (UART_FIFO_SIZE>>3) ;
    unsigned int num8 = num<<3 ;
    if ( remain>0 ) return DS9123O_write(bytes,UART_FIFO_SIZE>>3,pn) || DS9123O_write(&bytes[UART_FIFO_SIZE>>3],(unsigned)remain,pn) ;
    for ( i=0;i<num8;++i) pn->in->combuffer[i] = UT_getbit(bytes,i)?OneBit:ZeroBit;
    ret = DS9123O_send_and_get(pn->in->combuffer,num8,NULL,0,pn) ;
    if(ret) {
        STAT_ADD1(DS9123O_write_errors);
    }
    return ret;
}

/* Assymetric */
/* Read bytes from the 9097 passive adapter */
/* Time out on each byte */
/* return 0 valid, else <0 error */
/* No matching read */
static int DS9123O_read( unsigned char * const byte, const size_t num, const struct parsedname * const pn ) {
    int ret ;
    int remain = (int)num-(UART_FIFO_SIZE>>3) ;
    unsigned int i, num8 = num<<3 ;
    if ( remain > 0 ) {
        return DS9123O_read( byte, UART_FIFO_SIZE>>3,pn ) || DS9123O_read( &byte[UART_FIFO_SIZE>>3],remain,pn) ;
    }
    if ( (ret=DS9123O_send_and_get(NULL,0,pn->in->combuffer,num8,pn)) ) {
        STAT_ADD1(DS9123O_read_errors);
        return ret ;
    }
    for ( i=0 ; i<num8 ; ++i ) UT_setbit(byte,i,pn->in->combuffer[i]&0x01) ;
    return 0 ;
}

/* Symmetric */
/* send bits -- read bits */
/* Actually uses bit zero of each byte */
int DS9123O_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length, const struct parsedname * const pn ) {
    int ret ;
    int i ;
    if ( length > UART_FIFO_SIZE ) return DS9123O_sendback_bits(outbits,inbits,UART_FIFO_SIZE,pn)
    ||DS9123O_sendback_bits(&outbits[UART_FIFO_SIZE],&inbits[UART_FIFO_SIZE],length-UART_FIFO_SIZE,pn) ;
    for ( i=0 ; i<length ; ++i ) pn->in->combuffer[i] = outbits[i] ? OneBit : ZeroBit ;
    if ( (ret= DS9123O_send_and_get(pn->in->combuffer,(unsigned)length,inbits,(unsigned)length,pn)) ) {
        STAT_ADD1(DS9123O_sendback_bits_errors);
        return ret ;
    }
    for ( i=0 ; i<length ; ++i ) inbits[i] &= 0x01 ;
    return 0 ;
}

/* Symmetric */
/* send a bit -- read a bit */
static int DS9123O_sendback_data( const unsigned char * data, unsigned char * const resp , const int len, const struct parsedname * const pn ) {
    int remain = len - (UART_FIFO_SIZE>>3) ;
//printf("sendback len=%d, U>>3=%d, remain=%d\n",len,(UART_FIFO_SIZE>>3),remain);
    if ( remain>0 ) {
        return DS9123O_sendback_data( data,resp,UART_FIFO_SIZE>>3,pn)
        || DS9123O_sendback_data( &data[UART_FIFO_SIZE>>3],&resp[UART_FIFO_SIZE>>3],remain,pn) ;
    } else {
        unsigned int i, bits = len<<3 ;
        int ret ;
        for ( i=0 ; i<bits ; ++i ) pn->in->combuffer[i] = UT_getbit(data,i) ? OneBit : ZeroBit ;
        if ( (ret=DS9123O_send_and_get(pn->in->combuffer,bits,pn->in->combuffer,bits,pn) ) ) {
            STAT_ADD1(DS9123O_sendback_data_errors);
            return ret ;
        }
        for ( i=0 ; i<bits ; ++i ) UT_setbit(resp,i,pn->in->combuffer[i]&0x01) ;
        return 0 ;
    }
}

/* Symetric */
/* gets specified number of bits, one per return byte */
static int DS9123O_read_bits( unsigned char * const bits , const int length, const struct parsedname * const pn ) {
    int i, ret ;
    if ( length > UART_FIFO_SIZE ) return DS9123O_read_bits(bits,UART_FIFO_SIZE,pn)||DS9123O_read_bits(&bits[UART_FIFO_SIZE],length-UART_FIFO_SIZE,pn) ;
    memset( pn->in->combuffer,0xFF,(size_t)length) ;
    if ( (ret=DS9123O_send_and_get(pn->in->combuffer,(unsigned)length,bits,(unsigned)length,pn)) ) {
        STAT_ADD1(DS9123O_read_bits_errors);
        return ret ;
    }
    for ( i=0;i<length;++i) bits[i]&=0x01 ;
    return 0 ;
}

/* Routine to send a string of bits and get another string back */
/* This seems rather COM-port specific */
/* Indeed, will move to DS9123O */
static int DS9123O_send_and_get( const unsigned char * const bussend, const size_t sendlength, unsigned char * const busget, const size_t getlength, const struct parsedname * pn ) {
    size_t gl = getlength ;
    ssize_t r ;
    size_t sl = sendlength ;
    int rc ;

    if ( sl > 0 ) {
        /* first flush... should this really be done? Perhaps it's a good idea
        * so read function below doesn't get any other old result */
        COM_flush(pn);
    
        /* send out string, and handle interrupted system call too */
        while(sl > 0) {
            if(!pn->in) break;
            r = write(pn->in->fd, &bussend[sendlength-sl], sl);
            if(r < 0) {
                if(errno == EINTR) {
                    /* write() was interrupted, try again */
                    STAT_ADD1(BUS_send_and_get_interrupted);
                    continue;
                }
                break;
            }
            sl -= r;
        }
        if(pn->in) {
            tcdrain(pn->in->fd) ; /* make sure everthing is sent */
            gettimeofday( &(pn->in->bus_write_time) , NULL );
        }
        if(sl > 0) {
            STAT_ADD1(BUS_send_and_get_errors);
            return -EIO;
        }
        //printf("SAG written\n");
    }
    
    /* get back string -- with timeout and partial read loop */
    if ( busget && getlength ) {
        while ( gl > 0 ) {
            fd_set readset;
            struct timeval tv;
            //printf("SAG readlength=%d\n",gl);
            if(!pn->in) break;
#if 1
                /* I can't imagine that 5 seconds timeout is needed???
                * Any comments Paul ? */
                tv.tv_sec = 5;
                tv.tv_usec = 0;
#else
                tv.tv_sec = 0;
                tv.tv_usec = 500000;  // 500ms
#endif
                /* Initialize readset */
                FD_ZERO(&readset);
                FD_SET(pn->in->fd, &readset);
            
                /* Read if it doesn't timeout first */
                rc = select( pn->in->fd+1, &readset, NULL, NULL, &tv );
                if( rc > 0 ) {
                //printf("SAG selected\n");
                    /* Is there something to read? */
                    if( FD_ISSET( pn->in->fd, &readset )==0 ) {
                        STAT_ADD1(BUS_send_and_get_errors);
                        return -EIO ; /* error */
                    }
                    update_max_delay(pn);
                    r = read( pn->in->fd, &busget[getlength-gl], gl ) ; /* get available bytes */
                    //printf("SAG postread ret=%d\n",r);
                    if (r < 0) {
                        if(errno == EINTR) {
                            /* read() was interrupted, try again */
                            STAT_ADD1(BUS_send_and_get_interrupted);
                            continue;
                        }
                        STAT_ADD1(BUS_send_and_get_errors);
                        return r ;
                    }
                    gl -= r ;
                } else if(rc < 0) { /* select error */
                    if(errno == EINTR) {
                        /* select() was interrupted, try again */
                        STAT_ADD1(BUS_send_and_get_interrupted);
                        continue;
                    }
                    STAT_ADD1(BUS_send_and_get_select_errors);
                    return -EINTR;
                } else { /* timed out */
                    STAT_ADD1(BUS_send_and_get_timeout);
                    return -EAGAIN;
                }
        }
    } else {
        /* I'm not sure about this... Shouldn't flush buffer if
        user decide not to read any bytes. Any comments Paul ? */
        //COM_flush(pn) ;
    }
    return 0 ;
}
