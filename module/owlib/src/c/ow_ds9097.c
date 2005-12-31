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

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */

static int DS9097_PowerByte( const unsigned char byte, const unsigned int delay, const struct parsedname * const pn) ;
static int DS9097_ProgramPulse( const struct parsedname * const pn ) ;
static int DS9097_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int DS9097_reset( const struct parsedname * const pn ) ;
static int DS9097_reconnect( const struct parsedname * const pn ) ;
static int DS9097_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length, const struct parsedname * const pn ) ;
static int DS9097_sendback_data( const unsigned char * data , unsigned char * resp , const size_t len, const struct parsedname * pn ) ;
static void DS9097_setroutines( struct interface_routines * const f ) ;
static int DS9097_send_and_get( const unsigned char * const bussend, unsigned char * const busget, const size_t length, const struct parsedname * pn ) ;

#define	OneBit	0xFF
#define ZeroBit 0xC0

/* Device-specific functions */
static void DS9097_setroutines( struct interface_routines * const f ) {
    f->reset = DS9097_reset ;
    f->next_both = DS9097_next_both ;
    f->PowerByte = DS9097_PowerByte ;
    f->ProgramPulse = DS9097_ProgramPulse ;
    f->sendback_data = DS9097_sendback_data ;
    f->select        = BUS_select_low ;
    f->overdrive = NULL ;
    f->testoverdrive = NULL ;
    f->reconnect = DS9097_reconnect ;
}

/* Open a DS9097 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS9097_detect( struct connection_in * in ) {
    struct stateinfo si ;
    struct parsedname pn ;
    int ret ;
    
    /* Set up low-level routines */
    DS9097_setroutines( & (in->iroutines) ) ;

    /* Reset the bus */
    in->Adapter = adapter_DS9097 ; /* OWFS assigned value */
    in->adapter_name = "DS9097" ;
    in->busmode = bus_serial ;
    
    pn.si = &si ;
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    if((ret = DS9097_reset(&pn))) {
        STAT_ADD1(DS9097_detect_errors);
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
static int DS9097_PowerByte(unsigned char byte, unsigned int delay, const struct parsedname * const pn) {
    int ret ;

    // flush the buffers
    COM_flush(pn);

    // send the packet
    if((ret=BUS_send_data(&byte,1,pn))) {
        STAT_ADD1(DS9097_PowerByte_errors);
      return ret ;
    }
    // delay
    UT_delay( delay ) ;

    return ret;
}

static int DS9097_reconnect( const struct parsedname * const pn ) {
    STAT_ADD1(BUS_reconnect);

    if ( !pn || !pn->in ) return -EIO;
    STAT_ADD1(pn->in->bus_reconnect);

    COM_close(pn->in);
    usleep(100000);
    if(!COM_open(pn->in)) {
      if(!DS9097_detect(pn->in)) {
        LEVEL_DEFAULT("DS9097 adapter reconnected\n");
        return 0;
      }
    }
    STAT_ADD1(BUS_reconnect_errors);
    STAT_ADD1(pn->in->bus_reconnect_errors);
    LEVEL_DEFAULT("Failed to reconnect DS9097 adapter!\n");
    return -EIO ;
}

static int DS9097_ProgramPulse( const struct parsedname * const pn ) {
    (void) pn ;
    return -EIO; /* not available */
}


static int DS9097_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
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
            if ( (ret=DS9097_sendback_bits(&bits[1],&bits[1],2,pn)) ) {
                STAT_ADD1(DS9097_next_both_errors);
                return ret ;
            }
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                /* Send chosen bit path, then check match on next two */
                if ( (ret=DS9097_sendback_bits(bits,bits,3,pn)) ) {
                    STAT_ADD1(DS9097_next_both_errors);
                    return ret ;
                }
            } else { /* last bit */
                if ( (ret=DS9097_sendback_bits(bits,bits,1,pn)) ) {
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

/* DS9097 Reset -- A little different from DS2480B */
/* Puts in 9600 baud, sends 11110000 then reads response */
static int DS9097_reset( const struct parsedname * const pn ) {
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
        STAT_ADD1(DS9097_reset_tcsetattr_errors);
        return -EIO ;
    }
    if ( (ret=DS9097_send_and_get(&resetbyte,&c,1,pn)) ) {
        STAT_ADD1(DS9097_reset_errors);
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
        STAT_ADD1(DS9097_reset_tcsetattr_errors);
        return -EFAULT ;
    }
    /* Flush the input and output buffers */
    COM_flush(pn) ;
    return 0 ;
}

/* Symmetric */
/* send bits -- read bits */
/* Actually uses bit zero of each byte */
int DS9097_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length, const struct parsedname * const pn ) {
    int ret ;
    int i ;

    /* Split into smaller packets? */
    if ( length > UART_FIFO_SIZE ) return DS9097_sendback_bits(outbits,inbits,UART_FIFO_SIZE,pn)
    ||DS9097_sendback_bits(&outbits[UART_FIFO_SIZE],&inbits[UART_FIFO_SIZE],length-UART_FIFO_SIZE,pn) ;

    /* Encode bits */
    for ( i=0 ; i<length ; ++i ) pn->in->combuffer[i] = outbits[i] ? OneBit : ZeroBit ;

    /* Communication with DS9097 routine */
    if ( (ret= DS9097_send_and_get(pn->in->combuffer,inbits,(unsigned)length,pn)) ) {
        STAT_ADD1(DS9097_sendback_bits_errors);
        return ret ;
    }

    /* Decode Bits */
    for ( i=0 ; i<length ; ++i ) inbits[i] &= 0x01 ;

    return 0 ;
}

/* Symmetric */
/* send bytes, and read back -- calls lower level bit routine */
static int DS9097_sendback_data( const unsigned char * data, unsigned char * resp , const size_t len, const struct parsedname * pn ) {
    unsigned int i, bits = len<<3 ;
    int ret ;
    int remain = len - (UART_FIFO_SIZE>>3) ;

    /* Split into smaller packets? */
    if ( remain>0 ) return DS9097_sendback_data( data,resp,UART_FIFO_SIZE>>3,pn)
        || DS9097_sendback_data( &data[UART_FIFO_SIZE>>3],&resp[UART_FIFO_SIZE>>3],remain,pn) ;

    /* Encode bits */
    for ( i=0 ; i<bits ; ++i ) pn->in->combuffer[i] = UT_getbit(data,i) ? OneBit : ZeroBit ;
    
    /* Communication with DS9097 routine */
    if ( (ret=DS9097_send_and_get(pn->in->combuffer,pn->in->combuffer,bits,pn) ) ) {
        STAT_ADD1(DS9097_sendback_data_errors);
        return ret ;
    }

    /* Decode Bits */
    for ( i=0 ; i<bits ; ++i ) UT_setbit(resp,i,pn->in->combuffer[i]&0x01) ;

    return 0 ;
}

/* Routine to send a string of bits and get another string back */
/* This seems rather COM-port specific */
/* Indeed, will move to DS9097 */
static int DS9097_send_and_get( const unsigned char * const bussend, unsigned char * const busget, const size_t length, const struct parsedname * pn ) {
    size_t gl = length ;
    ssize_t r ;
    size_t sl = length ;
    int rc ;

    if ( sl > 0 ) {
        /* first flush... should this really be done? Perhaps it's a good idea
        * so read function below doesn't get any other old result */
        COM_flush(pn);
    
        /* send out string, and handle interrupted system call too */
        while(sl > 0) {
            if(!pn->in) break;
            r = write(pn->in->fd, &bussend[length-sl], sl);
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
    if ( busget && length ) {
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
                    r = read( pn->in->fd, &busget[length-gl], gl ) ; /* get available bytes */
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
