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

#ifdef OW_PARPORT

#include <linux/ppdev.h>
#include <sys/ioctl.h>

#define WRITE0 0xFE
#define WRITE1 0xFF

struct timespec usec2   = { 0,   2000 };
struct timespec usec4   = { 0,   4000 };
struct timespec usec12  = { 0,  12000 };
struct timespec usec400 = { 0, 400000 };


/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */
static int DS1410databyte( const unsigned char d, int fd ) ;
static int DS1410cmdbyte( const unsigned char c, int fd ) ;
static int DS1410status( int * result, int fd ) ;
static int DS1410wait( int fd ) ;
static int DS1410bit( int * bit, int fd ) ;
static int DS1410_PowerByte( const unsigned char byte, const unsigned int delay, const struct parsedname * const pn) ;
static int DS1410_ProgramPulse( const struct parsedname * const pn ) ;
static int DS1410_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
static int DS1410_reset( const struct parsedname * const pn ) ;
static int DS1410_reconnect( const struct parsedname * const pn ) ;
static int DS1410_read( unsigned char * const buf, const size_t size, const struct parsedname * const pn ) ;
static int DS1410_write( const unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) ;
static int DS1410_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len, const struct parsedname * const pn ) ;
static void DS1410_setroutines( struct interface_routines * const f ) ;

/* Device-specific functions */
static void DS1410_setroutines( struct interface_routines * const f ) {
    f->write = DS1410_write ;
    f->read  = DS1410_read ;
    f->reset = DS1410_reset ;
    f->next_both = DS1410_next_both ;
    f->PowerByte = DS1410_PowerByte ;
    f->ProgramPulse = DS1410_ProgramPulse ;
    f->sendback_data = DS1410_sendback_data ;
    f->select        = BUS_select_low ;
    f->overdrive = NULL ;
    f->testoverdrive = NULL ;
    f->reconnect = DS1410_reconnect ;
}

/* Open a DS1410 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS1410_detect( struct connection_in * in ) {
    struct stateinfo si ;
    struct parsedname pn ;
    int ret ;
    
    /* Set up low-level routines */
    DS1410_setroutines( & (in->iroutines) ) ;

    /* Reset the bus */
    in->Adapter = adapter_DS1410 ; /* OWFS assigned value */
    in->adapter_name = "DS1410" ;
    in->busmode = bus_parallel ;
    
    pn.si = &si ;
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    if((ret = DS1410_reset(&pn))) {
        STAT_ADD1(DS1410_detect_errors);
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
static int DS1410_PowerByte(unsigned char byte, unsigned int delay, const struct parsedname * const pn) {
    int ret ;
    unsigned char resp ;

    // send the packet
    if((ret=BUS_sendback_data(&byte,&resp,1,pn))) {
        STAT_ADD1(DS1410_PowerByte_errors);
        return ret ;
    }
    // delay
    UT_delay( delay ) ;
    return ret;
}

static int DS1410_reconnect( const struct parsedname * const pn ) {
    STAT_ADD1(BUS_reconnect);

    if ( !pn || !pn->in ) return -EIO;
    STAT_ADD1(pn->in->bus_reconnect);

    COM_close(pn->in);
    usleep(100000);
    if(!COM_open(pn->in)) {
        if(!DS1410_detect(pn->in)) {
            LEVEL_DEFAULT("DS1410 adapter reconnected\n");
            return 0;
        }
    }
    STAT_ADD1(BUS_reconnect_errors);
    STAT_ADD1(pn->in->bus_reconnect_errors);
    LEVEL_DEFAULT("Failed to reconnect DS1410 adapter!\n");
    return -EIO ;
}

static int DS1410_ProgramPulse( const struct parsedname * const pn ) {
    (void) pn ;
    return -EIO; /* not available */
}


static int DS1410_next_both(unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) {
    struct stateinfo * si = pn->si ;
    int fd = pn->in->fd ;
    int search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    int bit_number ;
    int last_zero = -1 ;
    int bits[3] ;
    int ret ;

    // initialize for search
//    printf("Pre, lastdiscrep=%d\n",si->LastDiscrepancy) ;
    // if the last call was not the last one
    if ( !si->AnyDevices ) si->LastDevice = 1 ;
    if ( si->LastDevice ) return -ENODEV ;

    /* Appropriate search command */
    if ( (ret=BUS_send_data(&search,1,pn)) ) return ret ;
      // loop to do the search
    for ( bit_number=0 ;; ++bit_number ) {
        bits[1] = bits[2] = 1 ;
        if ( bit_number==0 ) { /* First bit */
            /* get two bits (AND'ed bit and AND'ed complement) */
            if ( DS1410bit(&bits[1],fd) || DS1410bit(&bits[2],fd) ) return -EIO ;
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                /* Send chosen bit path, then check match on next two */
                if ( DS1410bit(&bits[0],fd) || DS1410bit(&bits[1],fd) || DS1410bit(&bits[2],fd) ) return -EIO ;
            } else { /* last bit */
                if ( DS1410bit(&bits[0],fd) ) return -EIO ;
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
        //if ( (ret=DS1410_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(serialnumber,8) || (bit_number<64) || (serialnumber[0] == 0)) {
        STAT_ADD1(DS1410_next_both_errors);
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

/* Assymetric */
/* Send a byte to the 9097 passive adapter */
/* return 0 valid, else <0 error */
/* no matching read */
static int DS1410_write( const unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) {
    unsigned char FF ;
    int i ;
    for ( i=0 ; i<num ; ++i )
        if ( DS1410_sendback_data(&bytes[i],&FF,1,pn) )
            return -EIO ;
    return 0 ;
}

/* Assymetric */
/* Read bytes from the 9097 passive adapter */
/* Time out on each byte */
/* return 0 valid, else <0 error */
/* No matching read */
static int DS1410_read( unsigned char * const bytes, const size_t num, const struct parsedname * const pn ) {
    unsigned char FF = 0xFF ;
    int i ;
    for ( i=0 ; i<num ; ++i )
        if ( DS1410_sendback_data(&FF,&bytes[i],1,pn) )
            return -EIO ;
    return 0 ;
}

/* Symmetric */
/* send a bit -- read a bit */
static int DS1410_sendback_data( const unsigned char * data, unsigned char * const resp , const int len, const struct parsedname * const pn ) {
    int i ;
    int bits = len<<3 ;
    int fd = pn->in->fd ;
    for ( i=0 ; i<bits ; ++i ) {
        int b = UT_getbit( data, i ) ;
        if ( DS1410bit( &b, fd ) ) return -EIO ;
        UT_setbit( resp, i, b ) ;
    }
    return 0 ;
}

/* Data byte */
static int DS1410databyte( const unsigned char d, int fd ) {
    unsigned char data = d ;
    return -ioctl( fd, PPWDATA, &data ) ;
}

/* Command byte */
static int DS1410cmdbyte( const unsigned char c, int fd ) {
    unsigned char cmd = c ;
    return -ioctl( fd, PPWCONTROL, &cmd ) ;
}

/* wait on status */
static int DS1410wait( int fd ) {
    int count = 0 ;
    int result ;
    int ret ;

    do {
        ret = DS1410status( &result, fd ) ;
        if ( ++count > 1000 ) return -ETIME ;
        if ( nanosleep( &usec4, NULL ) ) return -errno ;
    } while ( ! (ret || result) ) ;
    return ret ;
}
    
/* read the parallel port status and do a test */
static int DS1410status( int * result, int fd ) {
    unsigned char st ;
    int ret = ioctl( fd, PPRSTATUS, &st ) ;
    result[0] = ( st ^ 0x80 ) & 0x90 ? 1 : 0 ;
    return -ret ;
}

static int DS1410bit( int * bit, int fd ) {
    unsigned char control ;
    if (
        DS1410databyte( 0xEC, fd )
        ||nanosleep( &usec2, NULL )
        ||ioctl( fd, PPRCONTROL, &control )
        ||DS1410databyte( bit[0]?WRITE1:WRITE0, fd )
       ) return 1 ;
    control = ( ( control | 0x04 ) & 0x1C ) ;
    if (
        DS1410cmdbyte( control|0x02, fd )
        ||DS1410wait( fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410databyte( 0xFF, fd )
        ||DS1410wait( fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410databyte( 0xFE, fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410status( bit, fd )
        ||DS1410cmdbyte( control&0xFD, fd )
        ||DS1410databyte( 0xCF,fd)
        ||nanosleep( &usec12,NULL)
       ) return 1 ;
    return 0 ;
}

static int DS1410_reset( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    unsigned char control ;
    if (
        DS1410databyte( 0xEC, fd )
        ||nanosleep( &usec2, NULL )
        ||ioctl( fd, PPRCONTROL, &control )
        ||DS1410databyte( 0xFD, fd )
       ) return 1 ;
    control = ( ( control | 0x04 ) & 0x1C ) ;
    if (
        DS1410cmdbyte( control|0x02, fd )
        ||DS1410wait( fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410databyte( 0xFF, fd )
        ||DS1410wait( fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410databyte( 0xFE, fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410status( &(pn->si->AnyDevices), fd )
        ||nanosleep( &usec400, NULL )
        ||DS1410databyte( 0xFF, fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410databyte( 0xFE, fd )
        ||nanosleep( &usec4, NULL )
        ||DS1410status( &(pn->si->AnyDevices), fd )
        ||DS1410cmdbyte( control&0xFD, fd )
        ||DS1410databyte( 0xCF,fd)
        ||nanosleep( &usec12,NULL)
       ) return 1 ;
    return 0 ;
}

#endif /* OW_PARPORT */
