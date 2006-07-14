#if 0
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

#if OW_PARPORT

#include <linux/ppdev.h>
#include <sys/ioctl.h>

/* DOS driver */
#define WRITE0 ((BYTE)0xFE)
#define WRITE1 ((BYTE)0xFF)
#define RESET  ((BYTE)0xFD)

struct timespec usec2   = { 0,    2000 };
struct timespec usec4   = { 0,    4000 };
struct timespec usec8   = { 0,    8000 };
struct timespec usec12  = { 0,   12000 };
struct timespec usec20  = { 0,   20000 };
struct timespec usec400 = { 0,  500000 };
struct timespec msec16  = { 0,16000000 };

struct ppdev_frob_struct ENIhigh = { (BYTE)~0x1C, 0x04 } ;
struct ppdev_frob_struct ENIlow  = { (BYTE)~0x1C, 0x06 } ;

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */
static int DS1410bit( BYTE out, BYTE * in, int fd ) ;
static int DS1410_reset( const struct parsedname * pn ) ;
static int DS1410_sendback_bits( const BYTE * data , BYTE * resp , const size_t len, const struct parsedname * pn ) ;
static void DS1410_setroutines( struct interface_routines * f ) ;
static int DS1410_open( const struct parsedname * pn ) ;
static void DS1410_close( struct connection_in * in ) ;
static int DS1410_ODtoggle( BYTE * od, int fd ) ;
static int DS1410_ODoff( const struct parsedname * pn ) ;
static int DS1410_ODon( const struct parsedname * pn ) ;
static int DS1410_ODcheck( BYTE * od, int fd ) ;
static int DS1410_PTtoggle( int fd ) ;
static int DS1410_PTon( int fd ) ;
static int DS1410_PToff(  const struct parsedname * pn ) ;
static int DS1410Present( BYTE * p, int fd ) ;

/* Device-specific functions */
static void DS1410_setroutines( struct interface_routines * f ) {
    f->detect        = DS1410_detect         ;
    f->reset         = DS1410_reset          ;
    f->next_both     = NULL                  ;
//    f->overdrive = ;
 //   f->testoverdrive = ;
    f->PowerByte     = NULL                  ;
//    f->ProgramPulse = ;
    f->sendback_data = NULL                  ;
    f->sendback_bits = DS1410_sendback_bits  ;
    f->select        = NULL                  ;
    f->reconnect     = NULL                  ;
    f->close         = DS1410_close          ;
    f->transaction   = NULL                  ;
    f->flags         = ADAP_FLAG_overdrive   ;
}

/* Open a DS1410 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
// NOT called with bus locked! 
int DS1410_detect( struct connection_in * in ) {
    struct parsedname pn ;
    BYTE od ;
    
    /* Set up low-level routines */
    DS1410_setroutines( & (in->iroutines) ) ;

    /* Reset the bus */
    in->Adapter = adapter_DS1410 ; /* OWFS assigned value */
    in->adapter_name = "DS1410" ;
    in->busmode = bus_parallel ;
    
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    if ( DS1410_open(&pn) ) return -EIO ; // Also exits "Passthru mode"

    if ( DS1410_ODcheck(&od,in->fd) ) {
        LEVEL_CONNECT("Cannot check Overdrive mode on DS1410E at %s\n",in->name) ;
    } else if ( od ) {
        DS1410_ODoff(&pn) ;
    }
    return DS1410_reset(&pn) ;
}

// return 1 if shorted, 0 ok, else <0
static int DS1410_reset( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    BYTE ad ;
    printf("DS1410E reset try\n");
    if ( DS1410bit( RESET, &ad, fd ) ) return -EIO ;
    pn->in->AnyDevices = ad ;
    printf("DS1410 reset success, AnyDevices=%d\n",pn->in->AnyDevices);
    return 0 ;
}

static int DS1410_open(  const struct parsedname * pn ) {
    LEVEL_CONNECT("Opening port %s\n",SAFESTRING(pn->in->name)) ;
    if ( (pn->in->fd = open(pn->in->name, O_RDWR)) < 0 ) {
        LEVEL_CONNECT("Cannot open DS1410E at %s\n",pn->in->name) ;
    } else if ( ioctl(pn->in->fd,PPCLAIM ) ) {
        LEVEL_CONNECT("Cannot claim DS1410E at %s\n",pn->in->name) ;
        close( pn->in->fd ) ;
        pn->in->fd = -1 ;
    } else if ( DS1410_PToff(pn) ) {
        LEVEL_CONNECT("Cannot exit PassThru mode for DS1410E at %s\nIs there really an adapter there?\n",pn->in->name) ;
    } else {
        return 0 ;
    }
    STAT_ADD1_BUS(BUS_open_errors,pn->in) ;
    return -EIO ;
}

static void DS1410_close( struct connection_in * in ) {
    LEVEL_CONNECT("Closing port %s\n",SAFESTRING(in->name)) ;
    if ( in->fd >= 0 ) {
        DS1410_PTon(in->fd) ;
        ioctl(in->fd, PPRELEASE ) ;
        close( in->fd ) ;
    }
    in->fd = -1 ;
}

/* Symmetric */
/* send a bit -- read a bit */
static int DS1410_sendback_bits( const BYTE * data, BYTE * resp , const size_t len, const struct parsedname * pn ) {
    int i ;
    for ( i=0 ; i<len ; ++i ) {
        if ( DS1410bit( data[i]?WRITE1:WRITE0, &resp[i], pn->in->fd ) ) {
            STAT_ADD1_BUS( BUS_bit_errors , pn->in ) ;
            return -EIO ;
        }
    }
    return 0 ;
}

/* Basic design from Assembly driver */
static int DS1410bit( BYTE out, BYTE * in, int fd ) {
    BYTE CF=0xCF, EC=0xEC, FD=0xFD, FE=0xFE, FF=0xFF;
    BYTE st, cl, cl2 ;
    int i = 0 ;
    printf("DS1410E bit try (%.2X)\n",(int)out);
    if (
        0
        ||ioctl( fd, PPWDATA, &EC )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWDATA, &out )
        ||ioctl( fd, PPRCONTROL, &cl)
       ) return 1 ;
    cl = ( cl&0x1C) | 0x06 ;
    cl2 = cl & 0xFD ;
    if (
        0
        ||ioctl( fd, PPWCONTROL, &cl)
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWDATA, &FF )
       ) return 1 ;
    do {
        if (
            0
            ||nanosleep( &usec4, NULL )
            ||ioctl( fd, PPRSTATUS, &st )
            ||(++i>100)
           ) return 1 ;
    } while ( ((st ^ 0x80) & 0x90)==0 ) ;
    if (
        0
        ||ioctl( fd, PPWDATA, &FE )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
       ) return 1 ;
    if ( ( (st ^ 0x80) & 0x90 ) && ( out == RESET ) ) {
        if (
            0
            ||nanosleep( &usec400, NULL )
            ||ioctl( fd, PPWDATA, &FF )
            ||nanosleep( &usec4, NULL )
            ||ioctl( fd, PPWDATA, &FE )
            ||nanosleep( &usec4, NULL )
            ||ioctl( fd, PPRSTATUS, &st )
           ) return 1 ;
    }
    in[0] = ( (st ^ 0x80) & 0x90 ) ? 1 : 0 ;
    if (
        0
        ||ioctl( fd, PPWCONTROL, &cl2)
        ||ioctl( fd, PPWDATA, &CF )
        ||nanosleep( &usec12, NULL )
       ) return 1 ;
    printf("DS1410 bit success %d->%d counter=%d\n",(int)out,(int)in[0],i);
    return 0 ;
}

/* Basic design from DOS driver, WWW entries from win driver */
static int DS1410_ODcheck( BYTE * od, int fd ) {
    BYTE CF=0xCF, EC=0xEC, FD=0xFD, FE=0xFE, FF=0xFF;
    BYTE st, cl, cl2 ;
    int i = 0 ;
    printf("DS1410E check OD\n");
    if (
        0
        ||ioctl( fd, PPWDATA, &EC )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWDATA, &FF )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPRCONTROL, &cl)
       ) return 1 ;
    cl = ( cl&0x1C) | 0x06 ;
    cl2 = cl & 0xFD ;
    if (
        0
        ||ioctl( fd, PPWCONTROL, &cl)
        ||nanosleep( &usec8, NULL )
        ||nanosleep( &usec8, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||ioctl( fd, PPWDATA, &FF )
       ) return 1 ;
    od[0] = ( ( st ^ 0x80 ) & 0x90 ) ? 1 : 0 ;
    do {
        if (
            0
            ||nanosleep( &usec4, NULL )
            ||ioctl( fd, PPRSTATUS, &st )
            ||(++i>200)
           ) return 1 ;
    } while ( !((st ^ 0x80) & 0x90) ) ;
    if (
        0
        ||ioctl( fd, PPWDATA, &FE )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||ioctl( fd, PPWCONTROL, &cl2)
        ||ioctl( fd, PPWDATA, &CF )
        ||nanosleep( &usec4, NULL )
       ) return 1 ;
    printf("DS1410 OD status %d\n",(int)od[0]);
    return 0 ;
}

/* Basic design from DOS driver */
static int DS1410_ODtoggle( BYTE * od, int fd ) {
    BYTE CF=0xCF, EC=0xEC, FC=0xFC, FD=0xFD, FE=0xFE, FF=0xFF;
    BYTE st, cl, cl2 ;
    if (
        0
        ||ioctl( fd, PPWDATA, &EC )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWDATA, &FC )
        ||ioctl( fd, PPRCONTROL, &cl)
       ) return 1 ;
    cl2 = ( cl|0x04) & 0x1C ;
    cl = cl2 | 0x02 ;
    cl2 &= 0xFD ;
    if (
        0
        ||ioctl( fd, PPWCONTROL, &cl)
        ||nanosleep( &usec8, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||nanosleep( &usec8, NULL )
        ||ioctl( fd, PPWCONTROL, &cl2)
        ||ioctl( fd, PPWDATA, &CF )
        ||nanosleep( &usec8, NULL )
       ) return 1 ;
    od[0] = ( ( st ^ 0x80 ) & 0x90 ) ? 1 : 0 ;
    //printf("DS1410 OD toggle success %d\n",(int)od[0]);
    return 0 ;
}
static int DS1410_ODon( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    BYTE od ;
    if ( DS1410_ODtoggle( &od, fd ) ) return 1 ;
    if ( od && DS1410_ODtoggle( &od, fd ) ) return 1 ;
    return 0 ;
}

static int DS1410_ODoff( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    BYTE od, cmd[] = { 0x3C, } ;
    if ( BUS_reset( pn ) || BUS_send_data( cmd, 1, pn ) || DS1410_ODtoggle( &od, fd ) ) return 1 ;
    if ( od ) return 0 ;
    if ( DS1410_ODtoggle( &od, fd ) ) return 1 ;
    return 0 ;
}

/* passthru */
static int DS1410_PTtoggle( int fd ) {
    BYTE od[4] ;
    char tog[]="ABCD" ;
    printf("DS1410 Passthrough toggle\n");
    if (
        0
        ||DS1410_ODtoggle( &od[0], fd )
        ||DS1410_ODtoggle( &od[1], fd )
        ||DS1410_ODtoggle( &od[2], fd )
        ||DS1410_ODtoggle( &od[3], fd )
        ||nanosleep( &msec16, NULL )
       ) return 1 ;
    tog[0] = od[0]?'1':'0' ;
    tog[1] = od[1]?'1':'0' ;
    tog[2] = od[2]?'1':'0' ;
    tog[3] = od[3]?'1':'0' ;
    printf("DS1410 Passthrough success %s\n",tog);
    return 0 ;
}

/* passthru */
/* Return 0 if successful */
static int DS1410_PTon( int fd ) {
    BYTE p ;
    LEVEL_CONNECT("Attempting to switch DS1410E into PassThru mode\n") ;
    DS1410_PTtoggle( fd ) ;
    DS1410Present(&p,fd) ;
    if ( p==0 ) return 0 ;
    DS1410_PTtoggle( fd ) ;
    DS1410Present(&p,fd) ;
    if ( p==0 ) return 0 ;
    DS1410_PTtoggle( fd ) ;
    DS1410Present(&p,fd) ;
    return p ;
}

/* passthru */
/* Return 0 if successful */
static int DS1410_PToff(  const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    BYTE p ;
    int ret = 1 ;
    LEVEL_CONNECT("Attempting to switch DS1410E out of PassThru mode\n") ;
    
    if ( ret ) { // always true the first time
        DS1410_PTtoggle( fd ) ;
        DS1410Present(&p,fd) ;
        if ( p==0 ) ret = 0 ;
    }
    if ( ret ) { // second try
        DS1410_PTtoggle( fd ) ;
        DS1410Present(&p,fd) ;
        if ( p==0 ) ret = 0 ;
    }
    if ( ret ) { // third try
        DS1410_PTtoggle( fd ) ;
        DS1410Present(&p,fd) ;
        if ( p==0 ) ret = 0 ;
    }
    if ( DS1410_ODcheck(&p,fd)==0 && p==1 ) DS1410_ODoff(pn) ; //leave OD mode
    return ret ;
}

static int DS1410Present( BYTE * p, int fd ) {
    BYTE CF=0xCF, EC=0xEC, FD=0xFD, FE=0xFE, FF=0xFF;
    BYTE st, cl, cl2 ;
    int pass = 0 ;
    int i = 0 ;
    printf("DS1410 present?\n") ;
    if (
        0
        ||DS1410bit( RESET, &st, fd )
        ||ioctl( fd, PPWDATA, &EC )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWDATA, &FF )
        ||ioctl( fd, PPRCONTROL, &cl)
       ) return 1 ;
    cl = ( cl&0x1C) | 0x06 ;
    cl2 = cl & 0xFD ;
    if (
        0
        ||ioctl( fd, PPWCONTROL, &cl)
        ||nanosleep( &usec8, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||ioctl( fd, PPWDATA, &FF )
       ) return 1 ;
    while ( 1 ) {
        if (
            0
            ||ioctl( fd, PPRSTATUS, &st )
           ) return 1 ;
        if ( (pass==0) && ((st^0x80)&0x90)==0 ) {
            pass = 1 ;
        } else if ( (pass==1) && ((st^0x80)&0x90)!=0 ) {
            p[0] = 1 ;
            break ;
        }
        if (
            0
            ||nanosleep( &usec4, NULL )
           ) return 1 ;
        if (++i>200) {
            p[0] = 0 ;
            break ;
        }
    }
    if (
        0
        ||ioctl( fd, PPWDATA, &FE )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWCONTROL, &cl2)
        ||ioctl( fd, PPWDATA, &CF )
        ||nanosleep( &usec12, NULL )
       ) return 1 ;
    printf("DS1410 present success %d reached pass=%d counter=%d\n",(int)p[0],pass,i);
    return 0 ;
}

#endif /* OW_PARPORT */
#else /* 0 */
int DS1410_detect( struct connection_in * in ) {
    (void) in ;
    return 0 ;
}
#endif /* 0 */
