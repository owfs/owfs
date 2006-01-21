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

int LINK_mode ; /* flag to use LINKs in ascii mode */

//static void byteprint( const unsigned char * b, int size ) ;
static int LINK_write(const unsigned char * buf, const size_t size, const struct parsedname * pn ) ;
static int LINK_read(unsigned char * buf, const size_t size, const struct parsedname * pn ) ;
static int LINK_reset( const struct parsedname * pn ) ;
static int LINK_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * pn) ;
static int LINK_PowerByte(const unsigned char byte, const unsigned int delay, const struct parsedname * pn) ;
static int LINK_sendback_data( const unsigned char * data, unsigned char * resp, const size_t len, const struct parsedname * pn ) ;
static int LINK_select(const struct parsedname * pn) ;
static int LINK_reconnect( const struct parsedname * pn ) ;
static int LINK_byte_bounce( const unsigned char * out, unsigned char * in, const struct parsedname * pn ) ;
static int LINK_CR( const struct parsedname * pn ) ;

static void LINK_setroutines( struct interface_routines * f ) {
    f->reset = LINK_reset ;
    f->next_both = LINK_next_both ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte = LINK_PowerByte ;
//    f->ProgramPulse = ;
    f->sendback_data = LINK_sendback_data ;
//    f->sendback_bits = ;
    f->select        = LINK_select ;
    f->reconnect = LINK_reconnect ;
}

#define LINK_string(x)  ((unsigned char *)(x))

/* Called from DS2480_detect, and is set up to DS9097U emulation by default */
int LINK_detect( struct connection_in * in ) {
    struct parsedname pn ;
    struct stateinfo si ;

    pn.si = &si ;
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    // set the baud rate to 9600
    COM_speed(B9600,&pn);
    if ( LINK_reset(&pn)==0 && LINK_write(LINK_string(" "),1,&pn)==0 ) {
        unsigned char tmp[36] = "(none)";
        char * stringp = (char *) tmp ;
        /* read the version string */
        memset(tmp,0,36) ;
        LINK_read(tmp,36,&pn) ; // ignore return value -- will time out, probably
        COM_flush(&pn) ;

        /* Now find the dot for the version parsing */
        strsep( &stringp , "." ) ;
        if ( stringp && stringp[0] ) {
            switch (stringp[0] ) {
                case '0':
                    in->Adapter = adapter_LINK_10 ;
                    in->adapter_name = "LINK v1.0" ;
                    break ;
                case '1':
                    in->Adapter = adapter_LINK_11 ;
                    in->adapter_name = "LINK v1.1" ;
                    break ;
                case '2':
                default:
                    in->Adapter = adapter_LINK_12 ;
                    in->adapter_name = "LINK v1.2" ;
                    break ;
            }
            /* Set up low-level routines */
            LINK_setroutines( & (in->iroutines) ) ;
            return 0 ;
        }
    }
    LEVEL_DEFAULT("LINK detection error -- back to emulation mode\n");
    return 0  ;
}

static int LINK_reset( const struct parsedname * pn ) {
    unsigned char resp[3] ;
    COM_flush(pn) ;
    if ( LINK_write(LINK_string("\rr"),2,pn) ) return -errno ;
//    sleep(1) ;
    if ( LINK_read(resp,4,pn) ) return -errno ;
    switch( resp[1] ) {
        case 'P':
            pn->si->AnyDevices=1 ;
            break ;
        case 'N':
            pn->si->AnyDevices=0 ;
            break ;
        default:
            pn->si->AnyDevices=0 ;
            LEVEL_CONNECT("1-wire bus short circuit.\n")
    }
    return 0 ;
}

static int LINK_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * pn) {
    struct stateinfo * si = pn->si ;
    char resp[20] ;
    int ret ;

    (void) search ;
    if ( !si->AnyDevices ) si->LastDevice = 1 ;
    if ( si->LastDevice ) return -ENODEV ;

    COM_flush(pn) ;
    if ( si->LastDiscrepancy == -1 ) {
        if ( (ret=LINK_write(LINK_string("f"),1,pn)) ) return ret ;
        si->LastDiscrepancy = 0 ;
    } else {
        if ( (ret=LINK_write(LINK_string("n"),1,pn)) ) return ret ;
    }
    
    if ( (ret=LINK_read(LINK_string(resp),20,pn)) ) return ret ;

    switch ( resp[0] ) {
        case '-':
            si->LastDevice = 1 ;
        case '+':
            break ;
        case 'N' :
            si->AnyDevices = 0 ;
            return -ENODEV ;
        case 'X':
        default :
            return -EIO ;
    }

    serialnumber[7] = string2num(&resp[2]) ;
    serialnumber[6] = string2num(&resp[4]) ;
    serialnumber[5] = string2num(&resp[6]) ;
    serialnumber[4] = string2num(&resp[8]) ;
    serialnumber[3] = string2num(&resp[10]) ;
    serialnumber[2] = string2num(&resp[12]) ;
    serialnumber[1] = string2num(&resp[14]) ;
    serialnumber[0] = string2num(&resp[16]) ;
    
    // CRC check
    if ( CRC8(serialnumber,8) || (serialnumber[0] == 0)) {
        /* A minor "error" and should perhaps only return -1 to avoid reconnect */
        return -EIO ;
    }

    if((serialnumber[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }

    return 0 ;
}

/* Assymetric */
/* Read from LINK with timeout on each character */
// NOTE: from PDkit, reead 1-byte at a time
// NOTE: change timeout to 40msec from 10msec for LINK
// returns 0=good 1=bad
/* return 0=good,
          -errno = read error
          -EINTR = timeout
 */
static int LINK_read(unsigned char * buf, const size_t size, const struct parsedname * pn ) {
    size_t inlength = size ;
    fd_set fdset;
    ssize_t r ;
    struct timeval tval;
    int ret ;

    while(inlength > 0) {
        if(!pn->in) {
            ret = -EIO;
            STAT_ADD1(DS2480_read_null);
            break;
        }
        // set a descriptor to wait for a character available
        FD_ZERO(&fdset);
        FD_SET(pn->in->fd,&fdset);
        tval.tv_sec = 0;
        tval.tv_usec = 5000000;
        /* This timeout need to be pretty big for some reason.
        * Even commands like DS2480_reset() fails with too low
        * timeout. I raise it to 0.5 seconds, since it shouldn't
        * be any bad experience for any user... Less read and
        * timeout errors for users with slow machines. I have seen
        * 276ms delay on my Coldfire board.
        *
        * DS2480_reset()
        *   DS2480_sendback_cmd()
        *     DS2480_sendout_cmd()
        *       DS2480_write()
        *         write()
        *         tcdrain()   (all data should be written on serial port)
        *     DS2480_read()
        *       select()      (waiting 40ms should be enough!)
        *       read()
        *
        */

        // if byte available read or return bytes read
        ret = select(pn->in->fd+1,&fdset,NULL,NULL,&tval);
        if (ret > 0) {
            if( FD_ISSET( pn->in->fd, &fdset )==0 ) {
                ret = -EIO;  /* error */
                STAT_ADD1(DS2480_read_fd_isset);
                break;
            }
//            update_max_delay(pn);
            r = read(pn->in->fd,&buf[size-inlength],inlength);
            if ( r < 0 ) {
                if(errno == EINTR) {
                    /* read() was interrupted, try again */
                    STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                    continue;
                }
                ret = -errno;  /* error */
                STAT_ADD1(DS2480_read_read);
                break;
            }
            inlength -= r;
        } else if(ret < 0) {
            if(errno == EINTR) {
                /* select() was interrupted, try again */
                STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                continue;
            }
            STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
            return -EINTR;
        } else {
            STAT_ADD1_BUS(BUS_read_timeout_errors,pn->in);
            return -EINTR;
        }
    }
    if(inlength > 0) { /* signal that an error was encountered */
        STAT_ADD1_BUS(BUS_read_errors,pn->in);
        return ret;  /* error */
    }
    return 0;
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
 */
static int LINK_write(const unsigned char * buf, const size_t size, const struct parsedname * pn ) {
    ssize_t r, sl = size;

//    COM_flush(pn) ;
    while(sl > 0) {
        if(!pn->in) break;
        r = write(pn->in->fd,&buf[size-sl],sl) ;
        if(r < 0) {
            if(errno == EINTR) {
                STAT_ADD1_BUS(BUS_write_interrupt_errors,pn->in);
                continue;
            }
            break;
        }
        sl -= r;
    }
    if(pn->in) {
        tcdrain(pn->in->fd) ;
        gettimeofday( &(pn->in->bus_write_time) , NULL );
    }
    if(sl > 0) {
        STAT_ADD1_BUS(BUS_write_errors,pn->in);
        return -EIO;
    }
    return 0;
}

static int LINK_PowerByte(const unsigned char byte, const unsigned int delay, const struct parsedname * pn) {
    unsigned char pow ;
    
    if ( LINK_write(LINK_string("p"),1,pn) || LINK_byte_bounce(&byte,&pow,pn) ) {
        STAT_ADD1_BUS(BUS_PowerByte_errors,pn->in) ;
        return -EIO ; // send just the <CR>
    }
    
    // delay
    UT_delay( delay ) ;

    // flush the buffers
    return LINK_CR( pn ) ; // send just the <CR>
}

// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int LINK_sendback_data( const unsigned char * data, unsigned char * resp, const size_t size, const struct parsedname * pn ) {
    size_t i ;
    size_t left ;
    unsigned char * buf = pn->in->combuffer ;

    if ( size == 0 ) return 0 ;
    if ( LINK_write(LINK_string("b"),1,pn) ) return -EIO ;
//    for ( i=0; ret==0 && i<size ; ++i ) ret = LINK_byte_bounce( &data[i], &resp[i], pn ) ;
    for ( left=size; left ; ) {
        i = (left>16)?16:left ;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
        bytes2string( (char *)buf, &data[size-left], i ) ;
        if ( LINK_write( buf, i<<1, pn ) || LINK_read( buf, i<<1, pn ) ) return -EIO ;
        string2bytes( (char *)buf, &resp[size-left], i ) ;
        left -= i ;
    }
        
    return LINK_CR(pn) ;
}

static int LINK_reconnect( const struct parsedname * pn ) {
    STAT_ADD1(BUS_reconnect);

    if ( !pn || !pn->in ) return -EIO;
    STAT_ADD1(pn->in->bus_reconnect);

    COM_close(pn->in);
    usleep(100000);
    if(!COM_open(pn->in)) {
        if(!DS2480_detect(pn->in)) {
            LEVEL_DEFAULT("LINK adapter reconnected\n");
            return 0;
        }
    }
    STAT_ADD1(BUS_reconnect_errors);
    STAT_ADD1(pn->in->bus_reconnect_errors);
    LEVEL_DEFAULT("Failed to reconnect LINK adapter!\n");
    return -EIO ;
}

static int LINK_select(const struct parsedname * pn) {
    if ( pn->pathlength > 0 ) {
        LEVEL_CALL("Attempt to use a branched path (DS2409 main or aux) with the ascii-mode LINK\n") ;
        return -ENOTSUP ; /* cannot do branching with LINK ascii */
    }
    return BUS_select_low(pn) ;
}

/*
static void byteprint( const unsigned char * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) printf( "%.2X ",b[i] ) ;
    if ( size ) printf("\n") ;
}
*/

static int LINK_byte_bounce( const unsigned char * out, unsigned char * in, const struct parsedname * pn ) {
    unsigned char byte[2] ;

    num2string( (char *)byte, out[0] ) ;
    if ( LINK_write( byte, 2, pn ) || LINK_read( byte, 2, pn ) ) return -EIO ;
    in[0] = string2num( (char *)byte ) ;
    return 0 ;
}

static int LINK_CR( const struct parsedname * pn ) {
    unsigned char byte[2] ;
    if ( LINK_write( LINK_string("\r"), 1, pn ) || LINK_read( byte, 2, pn ) ) return -EIO ;
    return 0 ;
}
