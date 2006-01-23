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

#ifdef OW_PARPORT

#include <linux/ppdev.h>
#include <sys/ioctl.h>

/* DOS driver */
#define WRITE0 0xFE
#define WRITE1 0xFF
#define RESET  0xFD

struct timespec usec2   = { 0,   2000 };
struct timespec usec4   = { 0,   4000 };
struct timespec usec8   = { 0,   8000 };
struct timespec usec12  = { 0,  12000 };
struct timespec usec20  = { 0,  20000 };
struct timespec usec400 = { 0, 400000 };

struct ppdev_frob_struct ENIhigh = { (unsigned char)~0x1C, 0x04 } ;
struct ppdev_frob_struct ENIlow  = { (unsigned char)~0x1C, 0x06 } ;

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */
static int DS1410bit( unsigned char out, unsigned char * in, int fd ) ;
static int DS1410_reset( const struct parsedname * pn ) ;
static int DS1410_reconnect( const struct parsedname * pn ) ;
static int DS1410_sendback_bits( const unsigned char * data , unsigned char * resp , const size_t len, const struct parsedname * pn ) ;
static void DS1410_setroutines( struct interface_routines * f ) ;
static int DS1410_open( struct connection_in * in ) ;
static void DS1410_close( struct connection_in * in ) ;
static int DS1410_ODtoggle( unsigned char * od, int fd ) ;
static int DS1410_ODoff( const struct parsedname * pn ) ;
static int DS1410_ODon( const struct parsedname * pn ) ;
static int DS1410_ODcheck( unsigned char * od, int fd ) ;
static int DS1410_PTtoggle( int fd ) ;
static int DS1410_PTon( int fd ) ;
static int DS1410_PToff( int fd ) ;
static int DS1410Present( unsigned char * p, int fd ) ;

/* Device-specific functions */
static void DS1410_setroutines( struct interface_routines * f ) {
    f->reset = DS1410_reset ;
    f->next_both = BUS_next_both_low ;
//    f->overdrive = ;
 //   f->testoverdrive = ;
    f->PowerByte = BUS_PowerByte_low ; ;
//    f->ProgramPulse = ;
    f->sendback_data = BUS_sendback_data_low ;
    f->sendback_bits = DS1410_sendback_bits ;
    f->select        = BUS_select_low ;
    f->reconnect = DS1410_reconnect ;
    f->close = DS1410_close ;
}

/* Open a DS1410 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS1410_detect( struct connection_in * in ) {
    struct stateinfo si ;
    struct parsedname pn ;
    unsigned char od ;
    
    /* Set up low-level routines */
    DS1410_setroutines( & (in->iroutines) ) ;

    /* Reset the bus */
    in->Adapter = adapter_DS1410 ; /* OWFS assigned value */
    in->adapter_name = "DS1410" ;
    in->busmode = bus_parallel ;
    
    pn.si = &si ;
    FS_ParsedName(NULL,&pn) ; // minimal parsename -- no destroy needed
    pn.in = in ;

    if ( DS1410_open(in) ) return -EIO ; // Also exits "Passthru mode"

    if ( DS1410_ODcheck(&od,in->fd) ) {
        LEVEL_CONNECT("Cannot check Overdrive mode on DS1410E at %s\n",in->name) ;
    } else if ( od ) {
        DS1410_ODoff(&pn) ;
    }
    return DS1410_reset(&pn) ;
}

static int DS1410_reconnect( const struct parsedname * pn ) {
    STAT_ADD1(BUS_reconnect);

    if ( !pn || !pn->in ) return -EIO;
    STAT_ADD1(pn->in->bus_reconnect);

    BUS_close(pn->in);
    usleep(100000);
    if( DS1410_detect(pn->in) ) {
        STAT_ADD1(BUS_reconnect_errors);
        STAT_ADD1(pn->in->bus_reconnect_errors);
        LEVEL_DEFAULT("Failed to reconnect DS1410 adapter!\n");
        return -EIO ;
    }
    LEVEL_DEFAULT("DS1410 adapter reconnected\n");
    return 0;
}

static int DS1410_reset( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    unsigned char ad ;
    printf("DS1410E reset try\n");
    if ( DS1410bit( RESET, &ad, fd ) ) return -EIO ;
    pn->si->AnyDevices = ad ;
    printf("DS1410 reset success, AnyDevices=%d\n",pn->si->AnyDevices);
    return 0 ;
}

static int DS1410_open( struct connection_in * in ) {
    LEVEL_CONNECT("Opening port %s\n",NULLSTRING(in->name)) ;
    if ( (in->fd = open(in->name, O_RDWR)) < 0 ) {
        LEVEL_CONNECT("Cannot open DS1410E at %s\n",in->name) ;
    } else if ( ioctl(in->fd,PPCLAIM ) ) {
        LEVEL_CONNECT("Cannot claim DS1410E at %s\n",in->name) ;
        close( in->fd ) ;
        in->fd = -1 ;
    } else if ( DS1410_PToff(in->fd) ) {
        LEVEL_CONNECT("Cannot exit PassThru mode for DS1410E at %s\nIs there really an adapter there?\n",in->name) ;
    } else {
        return 0 ;
    }
    STAT_ADD1_BUS(BUS_open_errors,in) ;
    return -EIO ;
}

static void DS1410_close( struct connection_in * in ) {
    LEVEL_CONNECT("Closing port %s\n",NULLSTRING(in->name)) ;
    if ( in->fd >= 0 ) {
        DS1410_PTon(in->fd) ;
        ioctl(in->fd, PPRELEASE ) ;
        close( in->fd ) ;
    }
    in->fd = -1 ;
}

/* Symmetric */
/* send a bit -- read a bit */
static int DS1410_sendback_bits( const unsigned char * data, unsigned char * resp , const size_t len, const struct parsedname * pn ) {
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
static int DS1410bit( unsigned char out, unsigned char * in, int fd ) {
    unsigned char CF=0xCF, EC=0xEC, FD=0xFD, FE=0xFE, FF=0xFF;
    unsigned char st, cl, cl2 ;
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
    } while ( !((st ^ 0x80) & 0x90) ) ;
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
    printf("DS1410 bit success %d->%d\n",(int)out,(int)in[0]);
    return 0 ;
}

/* Basic design from DOS driver, WWW entries from win driver */
static int DS1410_ODcheck( unsigned char * od, int fd ) {
    unsigned char CF=0xCF, EC=0xEC, FD=0xFD, FE=0xFE, FF=0xFF;
    unsigned char st, cl, cl2 ;
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
static int DS1410_ODtoggle( unsigned char * od, int fd ) {
    unsigned char CF=0xCF, EC=0xEC, FC=0xFC, FD=0xFD, FE=0xFE, FF=0xFF;
    unsigned char st, cl, cl2 ;
    printf("DS1410E OD toggle\n");
    if (
        0
        ||ioctl( fd, PPWDATA, &EC )
        ||nanosleep( &usec4, NULL )
        ||ioctl( fd, PPWDATA, &FC )
        ||ioctl( fd, PPRCONTROL, &cl)
       ) return 1 ;
    cl = ( cl|0x04) & 0x1C ;
    cl2 = cl | 0x02 ;
    cl &= 0xFD ;
    if (
        0
        ||ioctl( fd, PPWCONTROL, &cl2)
        ||nanosleep( &usec8, NULL )
        ||ioctl( fd, PPRSTATUS, &st )
        ||nanosleep( &usec8, NULL )
        ||ioctl( fd, PPWCONTROL, &cl)
        ||ioctl( fd, PPWDATA, &CF )
        ||nanosleep( &usec8, NULL )
       ) return 1 ;
    od[0] = ( ( st ^ 0x80 ) & 0x90 ) ? 1 : 0 ;
    printf("DS1410 OD toggle success %d\n",(int)od[0]);
    return 0 ;
}
static int DS1410_ODon( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    unsigned char od ;
    if ( DS1410_ODtoggle( &od, fd ) ) return 1 ;
    if ( od && DS1410_ODtoggle( &od, fd ) ) return 1 ;
    return 0 ;
}

static int DS1410_ODoff( const struct parsedname * pn ) {
    int fd = pn->in->fd ;
    unsigned char od, cmd[] = { 0x3C, } ;
    if ( BUS_reset( pn ) || BUS_send_data( cmd, 1, pn ) || DS1410_ODtoggle( &od, fd ) ) return 1 ;
    if ( od ) return 0 ;
    if ( DS1410_ODtoggle( &od, fd ) ) return 1 ;
    return 0 ;
}

/* passthru */
static int DS1410_PTtoggle( int fd ) {
    unsigned char od ;
    printf("DS1410 Passthrough toggle\n");
    if (
        0
        ||DS1410_ODtoggle( &od, fd )
        ||DS1410_ODtoggle( &od, fd )
        ||DS1410_ODtoggle( &od, fd )
        ||DS1410_ODtoggle( &od, fd )
       ) return 1 ;
    UT_delay( 20 ) ; /* 20 msec!! */
    printf("DS1410 Passthrough success\n");
    return 0 ;
}

/* passthru */
/* Return 0 if successful */
static int DS1410_PTon( int fd ) {
    unsigned char p ;
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
static int DS1410_PToff( int fd ) {
    unsigned char p ;
    LEVEL_CONNECT("Attempting to switch DS1410E out of PassThru mode\n") ;
    DS1410_PTtoggle( fd ) ;
    DS1410Present(&p,fd) ;
    if ( p ) return 0 ;
    DS1410_PTtoggle( fd ) ;
    DS1410Present(&p,fd) ;
    if ( p ) return 0 ;
    DS1410_PTtoggle( fd ) ;
    DS1410Present(&p,fd) ;
    return !p ;
}

static int DS1410Present( unsigned char * p, int fd ) {
    unsigned char CF=0xCF, EC=0xEC, FD=0xFD, FE=0xFE, FF=0xFF;
    unsigned char st, cl, cl2 ;
    int pass = 0 ;
    int i = 0 ;
    printf("DS1410 present?\n") ;
    if (
        0
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
        if (++i>100) {
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
    printf("DS1410 present success %d\n",(int)p[0]);
    return 0 ;
}

#endif /* OW_PARPORT */
