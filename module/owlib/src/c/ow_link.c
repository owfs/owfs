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
#include "ow_counters.h"
#include "ow_connection.h"

static struct timeval tvnet = { 0, 200000, } ;

//static void byteprint( const BYTE * b, int size ) ;
static int LINK_write(const BYTE * buf, const size_t size, const struct parsedname * pn ) ;
static int LINK_read(BYTE * buf, const size_t size, const struct parsedname * pn , int ExtraEbyte ) ;
static int LINK_read_low(BYTE * buf, const size_t size, const struct parsedname * pn ) ;
static int LINK_reset( const struct parsedname * pn ) ;
static int LINK_next_both(struct device_search * ds, const struct parsedname * pn) ;
static int LINK_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) ;
static int LINK_sendback_data( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) ;
static int LINK_byte_bounce( const BYTE * out, BYTE * in, const struct parsedname * pn ) ;
static int LINK_CR( const struct parsedname * pn ) ;
static void LINK_setroutines( struct interface_routines * f ) ;
static void LINKE_setroutines( struct interface_routines * f ) ;
static int LINKE_preamble( const struct parsedname * pn ) ;
static void LINKE_close( struct connection_in * in ) ;

static void LINK_setroutines( struct interface_routines * f ) {
    f->detect        = LINK_detect        ;
    f->reset         = LINK_reset         ;
    f->next_both     = LINK_next_both     ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte     = LINK_PowerByte     ;
//    f->ProgramPulse = ;
    f->sendback_data = LINK_sendback_data ;
//    f->sendback_bits = ;
    f->select        = NULL               ;
    f->reconnect     = NULL               ;
    f->close         = COM_close          ;
    f->transaction   = NULL               ;
    f->flags         = ADAP_FLAG_2409path ;
}

static void LINKE_setroutines( struct interface_routines * f ) {
    f->detect        = LINKE_detect       ;
    f->reset         = LINK_reset         ;
    f->next_both     = LINK_next_both     ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte     = LINK_PowerByte     ;
//    f->ProgramPulse = ;
    f->sendback_data = LINK_sendback_data ;
//    f->sendback_bits = ;
    f->select        = NULL               ;
    f->reconnect     = NULL               ;
    f->close         = LINKE_close        ;
    f->transaction   = NULL               ;
    f->flags         = ADAP_FLAG_2409path ;
}

#define LINK_string(x)  ((BYTE *)(x))

/* Called from DS2480_detect, and is set up to DS9097U emulation by default */
// bus locking done at a higher level
int LINK_detect( struct connection_in * in ) {
    struct parsedname pn ;

    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    /* Set up low-level routines */
    LINK_setroutines( & (in->iroutines) ) ;

    /* Open the com port */
    if ( COM_open(in) ) return -ENODEV ;

    // set the baud rate to 9600
    COM_speed(B9600,&pn);
    COM_flush(&pn) ;
    if ( LINK_reset(&pn)==0 && LINK_write(LINK_string(" "),1,&pn)==0 ) {
        BYTE tmp[36] = "(none)";
        char * stringp = (char *) tmp ;
        /* read the version string */
        memset(tmp,0,36) ;
        LINK_read(tmp,36,&pn,0) ; // ignore return value -- will time out, probably
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
            return 0 ;
        }
    }
    LEVEL_DEFAULT("LINK detection error\n");
    return -ENODEV  ;
}

int LINKE_detect( struct connection_in * in ) {
    struct parsedname pn ;

    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;
    LEVEL_CONNECT("LinkE detect\n") ;
    /* Set up low-level routines */
    LINKE_setroutines( & (in->iroutines) ) ;

    if ( in->name == NULL ) return -1 ;
    if ( ClientAddr( in->name, in ) ) return -1 ;
    if ( (pn.in->fd=ClientConnect(in)) < 0 ) return -EIO ; 

    in->Adapter = adapter_LINK_E ;
    if ( LINK_write(LINK_string(" "),1,&pn)==0 ) {
        char buf[18] ;
        if ( LINKE_preamble( &pn ) || LINK_read( (BYTE *)buf, 17, &pn, 1 ) || strncmp( buf, "Link", 4 ) ) return -ENODEV ;
//        printf("LINKE\n");
        in->adapter_name = "Link-Hub-E" ;
        return 0 ;
    }
    return -EIO ;
}

static int LINK_reset( const struct parsedname * pn ) {
    BYTE resp[5] ;
    int ret = 0 ;
    
    if ( pn->in->Adapter!=adapter_LINK_E ) COM_flush(pn) ;
    if ( LINK_write(LINK_string("\rr"),2,pn) || LINK_read( resp,4,pn,1) ) {
        STAT_ADD1_BUS(BUS_reset_errors,pn->in) ;
        return -EIO ;
    }
    switch( resp[1] ) {
        case 'P':
            pn->in->AnyDevices=1 ;
            break ;
        case 'N':
            pn->in->AnyDevices=0 ;
            break ;
        default:
            ret = 1 ; // marker for shorted bus
            pn->in->AnyDevices=0 ;
            STAT_ADD1_BUS(BUS_short_errors,pn->in) ;
            LEVEL_CONNECT("1-wire bus short circuit.\n")
    }
    return 0 ;
}

static int LINK_next_both(struct device_search * ds, const struct parsedname * pn) {
    char resp[21] ;
    int ret ;

    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    if ( pn->in->Adapter!=adapter_LINK_E ) COM_flush(pn) ;
    if ( ds->LastDiscrepancy == -1 ) {
        if ( (ret=LINK_write(LINK_string("f"),1,pn)) ) return ret ;
        ds->LastDiscrepancy = 0 ;
    } else {
        if ( (ret=LINK_write(LINK_string("n"),1,pn)) ) return ret ;
    }
    
    if ( (ret=LINK_read(LINK_string(resp),20,pn,1)) ) {
        return ret ;
    }

    switch ( resp[0] ) {
        case '-':
            ds->LastDevice = 1 ;
        case '+':
            break ;
        case 'N' :
            pn->in->AnyDevices = 0 ;
            return -ENODEV ;
        case 'X':
        default :
            return -EIO ;
    }

    ds->sn[7] = string2num(&resp[2]) ;
    ds->sn[6] = string2num(&resp[4]) ;
    ds->sn[5] = string2num(&resp[6]) ;
    ds->sn[4] = string2num(&resp[8]) ;
    ds->sn[3] = string2num(&resp[10]) ;
    ds->sn[2] = string2num(&resp[12]) ;
    ds->sn[1] = string2num(&resp[14]) ;
    ds->sn[0] = string2num(&resp[16]) ;
    
    // CRC check
    if ( CRC8(ds->sn,8) || (ds->sn[0] == 0)) {
        /* A minor "error" and should perhaps only return -1 to avoid reconnect */
        return -EIO ;
    }

    if((ds->sn[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }

    LEVEL_DEBUG("LINK_next_both SN found: "SNformat"\n",SNvar(ds->sn)) ;
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
static int LINK_read_low(BYTE * buf, const size_t size, const struct parsedname * pn ) {
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
        tval.tv_sec = Global.timeout_serial ;
        tval.tv_usec = 0 ;
        /* This timeout need to be pretty big for some reason.
        * Even commands like DS2480_reset() fails with too low
        * timeout. I raise it to 0.5 seconds, since it shouldn't
        * be any bad experience for any user... Less read and
        * timeout errors for users with slow machines. I have seen
        * 276ms delay on my Coldfire board.
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
                ERROR_CONNECT("LINK read error: %s\n",SAFESTRING(pn->in->name)) ;
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
            ERROR_CONNECT("LINK select error: %s\n",SAFESTRING(pn->in->name)) ;
            STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
            return -EINTR;
        } else {
            ERROR_CONNECT("LINK timeout error: %s\n",SAFESTRING(pn->in->name)) ;
            STAT_ADD1_BUS(BUS_read_timeout_errors,pn->in);
            return -EINTR;
        }
    }
    if(inlength > 0) { /* signal that an error was encountered */
        ERROR_CONNECT("LINK read short error: %s\n",SAFESTRING(pn->in->name)) ;
        STAT_ADD1_BUS(BUS_read_errors,pn->in);
        return ret;  /* error */
    }
    //printf("Link_Read_Low <%*s>\n",(int)size,buf) ;
    return 0;
}

/* Read from Link or Link-E
   0=good else bad
   Note that buffer length should 1 exta char long for ethernet reads
*/
static int LINK_read(BYTE * buf, const size_t size, const struct parsedname * pn , int ExtraEbyte ) {
    if ( pn->in->Adapter !=adapter_LINK_E ) {
        return LINK_read_low( buf, size, pn ) ;
    } else if ( readn( pn->in->fd, buf, size+ExtraEbyte, &tvnet ) != (ssize_t)size+ExtraEbyte ) { /* NOTE NOTE extra byte length for buffer */
        LEVEL_CONNECT("LINK_read (ethernet) error\n") ;
        return -EIO ;
    }
    return 0 ;
}
//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
 */
static int LINK_write(const BYTE * buf, const size_t size, const struct parsedname * pn ) {
    ssize_t r, sl = size;
//    printf("LINK_write %.*s\n",size,buf) ;
//    COM_flush(pn) ;
    while(sl > 0) {
        if(!pn->in) break;
        r = write(pn->in->fd,&buf[size-sl],sl) ;
        if(r < 0) {
            if(errno == EINTR) {
                STAT_ADD1_BUS(BUS_write_interrupt_errors,pn->in);
                continue;
            }
            ERROR_CONNECT("Trouble writing data to LINK: %s\n",SAFESTRING(pn->in->name)) ;
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
    //printf("Link wrote <%*s>\n",(int)size,buf);
    return 0;
}

static int LINK_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) {
    
    if ( LINK_write(LINK_string("p"),1,pn) || LINK_byte_bounce(&byte,resp,pn) ) {
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
static int LINK_sendback_data( const BYTE * data, BYTE * resp, const size_t size, const struct parsedname * pn ) {
    size_t i ;
    size_t left ;
    BYTE * buf = pn->in->combuffer ;

    if ( size == 0 ) return 0 ;
    if ( LINK_write(LINK_string("b"),1,pn) ) return -EIO ;
//    for ( i=0; ret==0 && i<size ; ++i ) ret = LINK_byte_bounce( &data[i], &resp[i], pn ) ;
    for ( left=size; left ; ) {
        i = (left>16)?16:left ;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
        bytes2string( (char *)buf, &data[size-left], i ) ;
        if ( LINK_write( buf, i<<1, pn ) || LINK_read( buf, i<<1, pn, 0 ) ) return -EIO ;
        string2bytes( (char *)buf, &resp[size-left], i ) ;
        left -= i ;
    }
    return LINK_CR(pn) ;
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) printf( "%.2X ",b[i] ) ;
    if ( size ) printf("\n") ;
}
*/

static int LINK_byte_bounce( const BYTE * out, BYTE * in, const struct parsedname * pn ) {
    BYTE byte[2] ;

    num2string( (char *)byte, out[0] ) ;
    if ( LINK_write( byte, 2, pn ) || LINK_read( byte, 2, pn, 0 ) ) return -EIO ;
    in[0] = string2num( (char *)byte ) ;
    return 0 ;
}

static int LINK_CR( const struct parsedname * pn ) {
    BYTE byte[3] ;
    if ( LINK_write( LINK_string("\r"), 1, pn ) || LINK_read( byte, 2, pn, 1 ) ) return -EIO ;
    return 0 ;
}

/* read the telnet-formatted start of a response line from the Link-Hub-E */
static int LINKE_preamble( const struct parsedname * pn ) {
    BYTE byte[6] ;
    struct timeval tvnetfirst = { Global.timeout_network, 0, } ;
    if ( readn( pn->in->fd, byte, 6, &tvnetfirst )!=6  ) return -EIO ;
    LEVEL_CONNECT("Good preamble\n") ;
    return 0 ;
}

static void LINKE_close( struct connection_in * in ) {
    if ( in->fd >= 0 ) {
        close( in->fd ) ;
        in->fd = -1 ;
    }
    FreeClientAddr( in ) ;
}
