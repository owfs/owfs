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

static struct timeval tvnetfirst = { 1, 0, } ;
static struct timeval tvnet = { 0, 100000, } ;

int HA7_mode ; /* flag to use HA7s in ascii mode */

//static void byteprint( const BYTE * b, int size ) ;
static int HA7_write( int fd, const ASCII * msg, struct connection_in * in ) ;
static int HA7_toHA7( int fd, const ASCII * msg, struct connection_in * in ) ;
static int HA7_getlock( int fd, struct connection_in * in ) ;
static int HA7_releaselock( int fd, struct connection_in * in ) ;
static int HA7_read(int fd, ASCII ** buffer ) ;
static int HA7_reset( const struct parsedname * pn ) ;
static int HA7_next_both(struct device_search * ds, const struct parsedname * pn) ;
static int HA7_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) ;
static int HA7_sendback_data( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) ;
static int HA7_select(const struct parsedname * pn) ;
static void HA7_setroutines( struct interface_routines * f ) ;
static void HA7_close( struct connection_in * in ) ;

static void HA7_setroutines( struct interface_routines * f ) {
    f->detect        = HA7_detect       ;
    f->reset         = HA7_reset         ;
    f->next_both     = HA7_next_both     ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte     = HA7_PowerByte     ;
//    f->ProgramPulse = ;
    f->sendback_data = HA7_sendback_data ;
//    f->sendback_bits = ;
    f->select        = HA7_select        ;
    f->reconnect     = NULL               ;
    f->close         = HA7_close        ;
}

#define HA7_string(x)  ((BYTE *)(x))

int HA7_detect( struct connection_in * in ) {
    struct parsedname pn ;
    int fd ;

    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;
    LEVEL_CONNECT("HA7 detect\n") ;
    /* Set up low-level routines */
    HA7_setroutines( & (in->iroutines) ) ;
    in->connin.ha7.locked = 0 ;
    in->connin.ha7.snlist = NULL;

    if ( in->name == NULL ) return -1 ;
    if ( ClientAddr( in->name, in ) ) return -1 ;
    if ( (fd=ClientConnect(in)) < 0 ) return -EIO ; 

    in->Adapter = adapter_HA7 ;
    if ( HA7_toHA7(fd,"ReleaseLock.html",in)==0 ) {
        ASCII * buf ;
        HA7_read( fd, &buf ) ;
        in->adapter_name = "HA7Net" ;
        in->busmode = bus_ha7 ;
        return 0 ;
    }
    close(fd) ;
    return -EIO ;
}

static int HA7_reset( const struct parsedname * pn ) {
    ASCII * resp ;
    int fd=ClientConnect(pn->in) ;
    int ret = 0 ;
    
    if ( fd < 0 ) return -EIO ;
    if ( HA7_toHA7(fd,"Reset.html",pn->in) || HA7_read( fd, &resp) ) {
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
    close( fd ) ;
    return 0 ;
}

static int HA7_next_both(struct device_search * ds, const struct parsedname * pn) {
    ASCII * resp ;
    int ret = 0 ;

    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    ++(ds->LastDiscrepancy) ;
    if ( ds->LastDiscrepancy == 0 || pn->in->connin.ha7.snlist == NULL ) {
        int fd ;
        if ( pn->in->connin.ha7.snlist ) free(pn->in->connin.ha7.snlist) ;
        pn->in->connin.ha7.found = 0 ;
        fd = ClientConnect(pn->in) ;
        if ( fd < 0 ) {
            if ( HA7_getlock( fd, pn->in ) ) {
                if ( HA7_toHA7( fd, "Search.html", pn->in ) || HA7_read( fd,&resp ) ) {
                    int allocated = pn->in->last_root_devs +10; // root dir estimated length
                    ASCII * p = resp ;
                    BYTE * s ;
                    pn->in->connin.ha7.snlist = malloc(allocated*8+8) ;
                    if ( pn->in->connin.ha7.snlist ) {
                        do {
                            /* search for a device */
                            if ( 0 ) { /* found a device */
                                if ( pn->in->connin.ha7.found >= allocated ) {
                                    void * temp = pn->in->connin.ha7.snlist ;
                                    //printf("About to reallocate allocated = %d %d\n",(int)allocated,(int)(allocated) ) ;
                                    allocated += 10 ;
                                    //printf("About to reallocate allocated = %d %d\n",(int)allocated,(int)(allocated) ) ;
                                    pn->in->connin.ha7.snlist = (BYTE *) realloc( temp, allocated*8+8 ) ;
                                    if ( pn->in->connin.ha7.snlist==NULL ) free(temp ) ;
                                    //printf("Reallocated\n") ;
                                }
                                if ( pn->in->connin.ha7.snlist == NULL) {
                                    pn->in->connin.ha7.found = 0 ;
                                    ret = -ENOMEM ;
                                }
                                s = &(pn->in->connin.ha7.snlist[8*pn->in->connin.ha7.found]) ;
                                s[7] = string2num(&p[0]) ;
                                s[6] = string2num(&p[2]) ;
                                s[5] = string2num(&p[4]) ;
                                s[4] = string2num(&p[6]) ;
                                s[3] = string2num(&p[8]) ;
                                s[2] = string2num(&p[10]) ;
                                s[1] = string2num(&p[12]) ;
                                s[0] = string2num(&p[14]) ;
                                ++ pn->in->connin.ha7.found ;
                            }
                        } while ( 0 ) ;
                    } else {
                        ret = -ENOMEM ;
                    }
                }
                HA7_releaselock( fd, pn->in ) ;
            }
            close( fd ) ;
        }
    }
    if ( ret ) {
        return ret ;
    } else if ( ds->LastDiscrepancy >= pn->in->connin.ha7.found ) {
        ds->LastDevice = 1 ;
        return -ENODEV ;
    } else {
        memcpy( ds->sn, &(pn->in->connin.ha7.snlist[8*ds->LastDiscrepancy]), 8 );
    }
    // CRC check
    if ( CRC8(ds->sn,8) || (ds->sn[0] == 0)) {
        /* A minor "error" and should perhaps only return -1 to avoid reconnect */
        return -EIO ;
    }

    if((ds->sn[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }
    return 0 ;
}


/* Read from Link or Link-E
   0=good else bad
   Note that buffer length should 1 exta char long for ethernet reads
*/
static int HA7_read(int fd, ASCII ** buffer ) {
    ASCII buf[4096] ;
    ssize_t r ;
    if ( (r=readn( fd, buf, 4096, &tvnet )) != 4096 ) { 
        LEVEL_CONNECT("HA7_read (ethernet) error\n") ;
        write( 1, buf, r) ;
        return -EIO ;
    }
    return 0 ;
}
static int HA7_write( int fd, const ASCII * msg, struct connection_in * in ) {
    ssize_t r, sl = strlen(msg);
    ssize_t size = sl ;
    while(sl > 0) {
        r = write(fd,&msg[size-sl],sl) ;
        if(r < 0) {
            if(errno == EINTR) {
                STAT_ADD1_BUS(BUS_write_interrupt_errors,in);
                continue;
            }
            ERROR_CONNECT("Trouble writing data to HA7: %s\n",SAFESTRING(in->name)) ;
            break;
        }
        sl -= r;
    }
    gettimeofday( &(in->bus_write_time) , NULL );
    if(sl > 0) {
        STAT_ADD1_BUS(BUS_write_errors,in);
        return -EIO;
    }
    return 0;
}

static int HA7_toHA7( int fd, const ASCII * msg, struct connection_in * in ) {
    int ret ; 
    ret = HA7_write(fd, "GET /1Wire/", in ) ;
    if ( ret ) return ret ;
    ret = HA7_write(fd, msg, in ) ;
    if ( ret ) return ret ;
    return HA7_write(fd, " HTTP/1.0\n\n", in ) ;
}

static int HA7_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) {
#if 0
    
    if ( HA7_write(HA7_string("p"),1,pn) || HA7_byte_bounce(&byte,resp,pn) ) {
        STAT_ADD1_BUS(BUS_PowerByte_errors,pn->in) ;
        return -EIO ; // send just the <CR>
    }
    
    // delay
    UT_delay( delay ) ;

    // flush the buffers
#endif /* 0 */
    return 0 ;
}

// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int HA7_sendback_data( const BYTE * data, BYTE * resp, const size_t size, const struct parsedname * pn ) {
    size_t i ;
    size_t left ;
    BYTE * buf = pn->in->combuffer ;
#if 0
    if ( size == 0 ) return 0 ;
    if ( HA7_write() ) return -EIO ;
//    for ( i=0; ret==0 && i<size ; ++i ) ret = HA7_byte_bounce( &data[i], &resp[i], pn ) ;
    for ( left=size; left ; ) {
        i = (left>16)?16:left ;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
        bytes2string( (char *)buf, &data[size-left], i ) ;
        if ( HA7_write( buf, i<<1, pn ) || HA7_read( buf, i<<1, pn, 0 ) ) return -EIO ;
        string2bytes( (char *)buf, &resp[size-left], i ) ;
        left -= i ;
    }
#endif /* 0 */
    return 0 ;
}

static int HA7_select(const struct parsedname * pn) {
    if ( pn->pathlength > 0 ) {
        LEVEL_CALL("Attempt to use a branched path (DS2409 main or aux) with the ascii-mode HA7\n") ;
        return -ENOTSUP ; /* cannot do branching with HA7 ascii */
    }
    return BUS_select_low(pn) ;
}

static void HA7_close( struct connection_in * in ) {
    if ( in->connin.ha7.snlist ) free( in->connin.ha7.snlist ) ;
    FreeClientAddr( in ) ;
}

static int HA7_getlock( int fd, struct connection_in * in ) {
    (void) fd ;
    (void) in ;
}

static int HA7_releaselock( int fd, struct connection_in * in ) {
    (void) fd ;
    (void) in ;
}
