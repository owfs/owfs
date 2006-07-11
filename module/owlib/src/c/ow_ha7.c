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
static int HA7_directory( BYTE search, struct dirblob * db, const struct parsedname * pn ) ;

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
    /* Initialize dir-at-once structures */
    DirblobInit( &(in->connin.ha7.main) ) ;
    DirblobInit( &(in->connin.ha7.alarm) ) ;

    if ( in->name == NULL ) return -1 ;
    /* Add the port if it isn't there already */
    if ( strchr(in->name,':')==NULL ) {
        ASCII * temp = realloc( in->name, strlen(in->name)+3 ) ;
        if ( temp==NULL ) return -ENOMEM ;
        in->name = temp ;
        strcat( in->name, ":80" ) ;
    }
    if ( ClientAddr( in->name, in ) ) return -1 ;
    if ( (fd=ClientConnect(in)) < 0 ) return -EIO ; 
printf("HA7 detext Open = %d\n",fd);
    in->Adapter = adapter_HA7 ;
    if ( HA7_toHA7(fd,"ReleaseLock.html",in)==0 ) {
        ASCII * buf ;
        if ( HA7_read( fd, &buf )==0 ) {
            in->adapter_name = "HA7Net" ;
            in->busmode = bus_ha7 ;
            in->AnyDevices = 1 ;
            free(buf) ;
            close(fd) ;
printf("HA7 detext Close = %d\n",fd);
            return 0 ;
        }
    }
    close(fd) ;
printf("HA7 detext Close = %d\n",fd);
    return -EIO ;
}

static int HA7_reset( const struct parsedname * pn ) {
    ASCII * resp = NULL ;
    int fd=ClientConnect(pn->in) ;
    int ret = 0 ;
printf("HA7 reset Open = %d\n",fd);
    
    if ( fd < 0 ) {
        STAT_ADD1_BUS(BUS_reset_errors,pn->in) ;
        return -EIO ;
    }
    if ( HA7_toHA7(fd,"Reset.html",pn->in) ) {
        STAT_ADD1_BUS(BUS_reset_errors,pn->in) ;
        ret = -EIO ;
    } else if ( HA7_read( fd, &resp) ) {
        STAT_ADD1_BUS(BUS_reset_errors,pn->in) ;
        ret = -EIO ;
    }
    if ( resp ) free( resp ) ;
    close( fd ) ;
printf("HA7 reset Close = %d\n",fd);
    return 0 ;
}

static int HA7_directory( BYTE search, struct dirblob * db, const struct parsedname * pn ) {
    int fd ;
    int ret = 0 ;
    ASCII * s = (search==0xEC) ? "Search.html?Conditional=1" : "Search.html" ; 
    ASCII * resp =  NULL ;

    DirblobClear( db ) ;
printf("HA7 cleared path=%s\n",pn->path) ;
    if ( (fd=ClientConnect(pn->in)) < 0 ) {
printf("HA7 dir Open = %d\n",fd);
printf("HA7 can't open %d\n",fd) ;
        db->troubled = 1 ;
        return -EIO ;
    }
printf("HA7 fd=%d\n",fd) ;
    if ( HA7_toHA7( fd, s, pn->in ) ) {
        ret = -EIO ;
    } else if (HA7_read( fd,&resp ) ) {
        ret = -EIO ;
    } else {
        BYTE sn[8] ;
        ASCII * p = resp ;
        while ( (p=strstr(p,"<INPUT CLASS=\"HA7Value\" NAME=\"Address_")) && (p=strstr(p,"VALUE=\"")) ) {
            p += 7 ;
            if ( strspn(p,"0123456789ABCDEF") < 16 ) {
                ret = -EIO ;
                break ;
            }
            sn[7] = string2num(&p[0]) ;
            sn[6] = string2num(&p[2]) ;
            sn[5] = string2num(&p[4]) ;
            sn[4] = string2num(&p[6]) ;
            sn[3] = string2num(&p[8]) ;
            sn[2] = string2num(&p[10]) ;
            sn[1] = string2num(&p[12]) ;
            sn[0] = string2num(&p[14]) ;
printf("HA7 found "SNformat"\n",SNvar(sn)) ;
            if ( CRC8(sn,8) ) {
                ret = -EIO ;
                break ;
            }
            DirblobAdd( sn, db ) ;
        }
        free( resp ) ;
    }
    close( fd ) ;
printf("HA7 dir Close = %d\n",fd);
    return ret ;
}

static int HA7_next_both(struct device_search * ds, const struct parsedname * pn) {
    struct dirblob * db = (ds->search == 0xEC) ?
                            &(pn->in->connin.ha7.alarm) :
                            &(pn->in->connin.ha7.main)  ;
    int ret = 0 ;

printf("NextBoth %s\n",pn->path) ;
    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    if ( ++(ds->LastDiscrepancy) == 0 ) {
        if ( HA7_directory( ds->search, db, pn ) ) return -EIO ;
    }
printf("LastDiscrepancy = %d\n",ds->LastDiscrepancy ) ;
    ret = DirblobGet( ds->LastDiscrepancy, ds->sn, db ) ;
printf("DirblobGet=%d <"SNformat">\n",ret,SNvar(ds->sn)) ;
    switch (ret ) {
        case 0:
            if((ds->sn[0] & 0x7F) == 0x04) {
                /* We found a DS1994/DS2404 which require longer delays */
                pn->in->ds2404_compliance = 1 ;
            }
            break ;
        case -ENODEV:
            ds->LastDevice = 1 ;
            break ;
    }
    return ret ;
}

/* Read from Link or Link-E
   0=good else bad
   Note that buffer length should 1 exta char long for ethernet reads
*/
static int HA7_read(int fd, ASCII ** buffer ) {
    ASCII buf[4097] ;
    ASCII * start ;
    int ret = 0 ;
    ssize_t r,s ;
    
    *buffer = NULL ;
    buf[4096] = '\0' ; // just in case
    if ( (r=readn( fd, buf, 4096, &tvnetfirst )) < 0 ) { 
        LEVEL_CONNECT("HA7_read (ethernet) error = %d\n",r) ;
        write( 1, buf, r) ;
        ret = -EIO ;
    } else if ( strncmp("HTTP/1.1 200 OK",buf,15) ) { //Bad HTTP return code
        LEVEL_DATA("HA7 response problem:%32s\n",&buf[15]) ;
        ret = -EIO ;
    } else if ( (start=strstr( buf, "<body>" ))== NULL ) { 
        LEVEL_DATA("HA7 response no HTTP body to parse\n") ;
        ret = -EIO ;
    } else {
    // HTML body found, dump header
        s = buf + r - start ;
        write( 1, start, s) ;
        if ( (*buffer = malloc( s )) == NULL ) {
            ret = -ENOMEM ;
        } else {
            memcpy( *buffer, start, s ) ;
            while ( r == 4096 ) {
                if ( (r=readn( fd, buf, 4096, &tvnet )) < 0 ) {
                    LEVEL_DATA("Couldn't get rest of HA7 data (err=%d)\n",r) ;
                    ret = -EIO ;
                    break ;
                } else {
                    ASCII * temp = realloc( *buffer, s+r ) ;
                    if ( temp ) {
                        *buffer = temp ;
                        memcpy( &((*buffer)[s]), buf, r ) ;
                        s += r ;
                    } else {
                        ret = -ENOMEM ;
                        break ;
                    }
                }
            }
        }
    }
    if ( ret ) {
        if ( *buffer ) free( *buffer ) ;
        *buffer = NULL ;
    }
printf("HA7_read return value=%d\n",ret);
    return ret ;
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
printf("HA7 send to webserver: %s\n",msg) ;
    ret = HA7_write(fd, "GET /1Wire/", in ) ;
    if ( ret ) return ret ;
    ret = HA7_write(fd, msg, in ) ;
    if ( ret ) return ret ;
    return HA7_write(fd, " HTTP/1.0\n\n", in ) ;
}

static int HA7_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) {
printf("HA7 powerbyte\n");
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
printf("HA7 sendback data\n");
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
    if ( pn->dev ) {
        int fd ;
        ASCII addrdev[] = "AddressDevice.html?Address=001122334455667788" ;
        if ( (fd=ClientConnect(pn->in)) < 0 ) return -EIO ;
printf("HA7 select Open = %d\n",fd);
        num2string( &addrdev[27], pn->sn[7] ) ;
        num2string( &addrdev[29], pn->sn[6] ) ;
        num2string( &addrdev[31], pn->sn[5] ) ;
        num2string( &addrdev[33], pn->sn[4] ) ;
        num2string( &addrdev[35], pn->sn[3] ) ;
        num2string( &addrdev[37], pn->sn[2] ) ;
        num2string( &addrdev[39], pn->sn[1] ) ;
        num2string( &addrdev[41], pn->sn[0] ) ;
        if ( HA7_toHA7(fd,addrdev,pn->in)==0 ) {
            ASCII * buf ;
            if ( HA7_read( fd, &buf ) ) return -EIO ;
            free(buf) ;
        }
    }
    return 0 ;
}

static void HA7_close( struct connection_in * in ) {
    DirblobClear( &(in->connin.ha7.main) ) ;
    DirblobClear( &(in->connin.ha7.alarm) ) ;
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
