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

static int DS9097_reset( const struct parsedname * pn ) ;
static int DS9097_sendback_bits( const unsigned char * outbits , unsigned char * inbits , const size_t length, const struct parsedname * pn ) ;
static void DS9097_setroutines( struct interface_routines * f ) ;
static int DS9097_send_and_get( const unsigned char * bussend, unsigned char * busget, const size_t length, const struct parsedname * pn ) ;

#define	OneBit	0xFF
#define ZeroBit 0xC0

/* Device-specific functions */
static void DS9097_setroutines( struct interface_routines * const f ) {
    f->detect        = DS9097_detect         ;
    f->reset         = DS9097_reset          ;
    f->next_both     = BUS_next_both_low     ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte     = BUS_PowerByte_low     ;
//    f->ProgramPulse = ;
    f->sendback_data = BUS_sendback_data_low ;
    f->sendback_bits = DS9097_sendback_bits  ;
    f->select        = BUS_select_low        ;
    f->reconnect     = NULL                  ;
    f->close         = COM_close             ;
}

/* Open a DS9097 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
// no bus locking here (higher up)
int DS9097_detect( struct connection_in * in ) {
    struct parsedname pn ;
    
    /* open the COM port */
    if ( COM_open( in ) ) return -ENODEV ;

    /* Set up low-level routines */
    DS9097_setroutines( & (in->iroutines) ) ;

    /* Reset the bus */
    in->Adapter = adapter_DS9097 ; /* OWFS assigned value */
    in->adapter_name = "DS9097" ;
    in->busmode = bus_serial ;
    
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    return DS9097_reset(&pn) ;
}

/* DS9097 Reset -- A little different from DS2480B */
/* Puts in 9600 baud, sends 11110000 then reads response */
// return 1 shorted, 0 ok, <0 error
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
    if ( cfsetospeed(&term, B9600)<0 || cfsetispeed(&term, B9600)<0 ) {
        ERROR_CONNECT("Cannot set speed (9600): %s\n",SAFESTRING(pn->in->name)) ;
    }
    if (tcsetattr(fd, TCSANOW, &term ) < 0 ) {
        ERROR_CONNECT("Cannot set attributes: %s\n",SAFESTRING(pn->in->name)) ;
        STAT_ADD1_BUS(BUS_tcsetattr_errors,pn->in);
        return -EIO ;
    }
    if ( (ret=DS9097_send_and_get(&resetbyte,&c,1,pn)) ) return ret ;

    switch(c) {
    case 0:
        STAT_ADD1_BUS( BUS_short_errors, pn->in ) ;
        LEVEL_CONNECT("1-wire bus short circuit.\n")
        ret = 1 ; // short circuit
        /* fall through */
    case 0xF0:
        pn->in->AnyDevices = 0 ;
        break ;
    default:
        pn->in->AnyDevices = 1 ;
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
    if ( cfsetospeed(&term, B38400)<0 || cfsetispeed(&term, B38400)<0 ) {
        ERROR_CONNECT("Cannot set speed (38400): %s\n",SAFESTRING(pn->in->name)) ;
    }
#else
    if ( cfsetospeed(&term, B115200)<0 || cfsetispeed(&term, B115200)<0 ) {
        ERROR_CONNECT("Cannot set speed (115200): %s\n",SAFESTRING(pn->in->name)) ;
    }
#endif

    if(tcsetattr(fd, TCSANOW, &term) < 0 ) {
        ERROR_CONNECT("Cannot set attributes: %s\n",SAFESTRING(pn->in->name)) ;
        STAT_ADD1_BUS(BUS_tcsetattr_errors,pn->in);
        return -EFAULT ;
    }
    /* Flush the input and output buffers */
    COM_flush(pn) ;
    return ret ;
}

/* Symmetric */
/* send bits -- read bits */
/* Actually uses bit zero of each byte */
/* Dispatches DS9097_MAX_BITS "bits" at a time */
#define DS9097_MAX_BITS 24
int DS9097_sendback_bits( const unsigned char * outbits , unsigned char * inbits , const size_t length, const struct parsedname * pn ) {
    int ret ;
    unsigned char d[DS9097_MAX_BITS] ;
    size_t l=0 ;
    size_t i=0 ;
    size_t start = 0 ;

    if ( length==0 ) return 0 ;

    /* Split into smaller packets? */
    do {
        d[l++] = outbits[i++] ? OneBit : ZeroBit ;
        if ( l==DS9097_MAX_BITS || i==length ) {
            /* Communication with DS9097 routine */
            if ( (ret= DS9097_send_and_get(d,&inbits[start],l,pn)) ) {
                STAT_ADD1_BUS( BUS_bit_errors, pn->in ) ;
                return ret ;
            }
            l = 0 ;
            start = i ;
        }
    } while ( i<length ) ;
            
    /* Decode Bits */
    for ( i=0 ; i<length ; ++i ) inbits[i] &= 0x01 ;

    return 0 ;
}

/* Routine to send a string of bits and get another string back */
/* This seems rather COM-port specific */
/* Indeed, will move to DS9097 */
static int DS9097_send_and_get( const unsigned char * bussend, unsigned char * busget, const size_t length, const struct parsedname * pn ) {
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
                    STAT_ADD1_BUS(BUS_write_interrupt_errors,pn->in);
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
            STAT_ADD1_BUS(BUS_write_errors,pn->in);
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
                /* We make it 10 * standard since 10 bytes required for 1 bit */
                tv.tv_sec = usec_read / 100000 ;
                tv.tv_usec = usec_read % 100000;
#else
                tv.tv_sec = usec_read / 1000000 ;
                tv.tv_usec = usec_read % 1000000 ;
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
                        STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
                        return -EIO ; /* error */
                    }
                    update_max_delay(pn);
                    r = read( pn->in->fd, &busget[length-gl], gl ) ; /* get available bytes */
                    //printf("SAG postread ret=%d\n",r);
                    if (r < 0) {
                        if(errno == EINTR) {
                            /* read() was interrupted, try again */
                            STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                            continue;
                        }
                        STAT_ADD1_BUS(BUS_read_errors,pn->in);
                        return r ;
                    }
                    gl -= r ;
                } else if(rc < 0) { /* select error */
                    if(errno == EINTR) {
                        /* select() was interrupted, try again */
                        STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                        continue;
                    }
                    STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
                    return -EINTR;
                } else { /* timed out */
                    STAT_ADD1_BUS(BUS_read_timeout_errors,pn->in);
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
