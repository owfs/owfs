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

/* ow_opt -- owlib specific command line options processing */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_pid.h"

int max_clients ;
int ftp_timeout ;

static int OW_ArgSerial( const char * arg ) ;
static int OW_ArgParallel( const char * arg ) ;
static int OW_ArgI2C( const char * arg ) ;
static int OW_ArgHA7( const char * arg ) ;
static int OW_ArgFake( const char * arg ) ;

const struct option owopts_long[] = {
    {"device",     required_argument,NULL,'d'},
    {"usb",        optional_argument,NULL,'u'},
    {"help",       no_argument,      NULL,'h'},
    {"port",       required_argument,NULL,'p'},
    {"server",     required_argument,NULL,'s'},
    {"readonly",   required_argument,NULL,'r'},
    {"write",      required_argument,NULL,'w'},
    {"Celsius",    no_argument,      NULL,'C'},
    {"Fahrenheit", no_argument,      NULL,'F'},
    {"Kelvin",     no_argument,      NULL,'K'},
    {"Rankine",    no_argument,      NULL,'R'},
    {"version",    no_argument,      NULL,'V'},
    {"format",     required_argument,NULL,'f'},
    {"pid_file",   required_argument,NULL,'P'},
    {"pid-file",   required_argument,NULL,'P'},
    {"background", no_argument,&background, 1},
    {"foreground", no_argument,&background, 0},
    {"error_print",required_argument,NULL,257},
    {"error-print",required_argument,NULL,257},
    {"error_level",required_argument,NULL,258},
    {"error-level",required_argument,NULL,258},
    {"morehelp",   no_argument,      NULL,259},
    {"fuse_opt",   required_argument,NULL,260}, /* owfs, fuse mount option */
    {"fuse-opt",   required_argument,NULL,260}, /* owfs, fuse mount option */
    {"fuse_open_opt",required_argument,NULL,267}, /* owfs, fuse open option */
    {"fuse-open-opt",required_argument,NULL,267}, /* owfs, fuse open option */
    {"msec_read",  required_argument,NULL,268}, /* Time out for serial reads */
    {"msec-read",  required_argument,NULL,268}, /* Time out for serial reads */
    {"max_clients", required_argument, NULL, 269}, /* ftp max connections */
    {"ftp_timeout", required_argument, NULL, 270}, /* ftp max connections */
    {"HA7", required_argument, NULL, 271}, /* HA7Net */
    {"ha7", required_argument, NULL, 271}, /* HA7Net */
    {"FAKE", required_argument, NULL, 272}, /* Fake */
    {"Fake", required_argument, NULL, 272}, /* Fake */
    {"fake", required_argument, NULL, 272}, /* Fake */
    {"link", no_argument,   &LINK_mode,1}, /* link in ascii mode */
    {"LINK", no_argument,   &LINK_mode,1}, /* link in ascii mode */
    {"nolink", no_argument,   &LINK_mode,0}, /* link not in ascii mode */
    {"NOLINK", no_argument,   &LINK_mode,0}, /* link not in ascii mode */
    {"altUSB", no_argument, &altUSB, 1}, /* Willy Robison's tweaks */
    {0,0,0,0},
} ;

/* Globals */
unsigned long int usec_read = 500000 ;

/* Parses one argument */
/* return 0 if ok */
int owopt( const int c , const char * arg, enum opt_program op ) {
    switch (c) {
    case 'h':
        ow_help( op ) ;
        return 1 ;
    case 'u':
        return OW_ArgUSB( arg ) ;
    case 'd':
        return OW_ArgSerial( arg ) ;
    case 't':
        Timeout(arg) ;
        break ;
    case 'r':
        readonly = 1 ;
        break ;
    case 'w':
        readonly = 0 ;
        break ;
    case 'C':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
        break ;
    case 'F':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit) ;
        break ;
    case 'R':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine) ;
        break ;
    case 'K':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin) ;
        break ;
    case 'V':
        printf("libow version:\n\t" VERSION "\n") ;
        return 1 ;
    case 's':
        return OW_ArgNet( arg ) ;
    case 'p':
//printf("Arg: -p [%s]\n", arg);
        return OW_ArgServer( arg ) ;
    case 'f':
        if (!strcasecmp(arg,"f.i"))        set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
        else if (!strcasecmp(arg,"fi"))    set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fi);
        else if (!strcasecmp(arg,"f.i.c")) set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdidc);
        else if (!strcasecmp(arg,"f.ic"))  set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdic);
        else if (!strcasecmp(arg,"fi.c"))  set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fidc);
        else if (!strcasecmp(arg,"fic"))   set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fic);
        else {
             LEVEL_DEFAULT("Unrecognized format type %s\n",arg)
             return 1 ;
        }
        break ;
    case 'P':
        if ( arg==NULL || strlen(arg)==0 ) {
            LEVEL_DEFAULT("No PID file specified\n")
            return 1 ;
        } else if ( (pid_file=strdup(arg)) == NULL ) {
            fprintf( stderr,"Insufficient memory to store the PID filename: %s\n",arg) ;
            return 1 ;
        }
        break ;
    case 257:
        error_print = atoi(arg) ;
        break ;
    case 258:
        error_level = atoi(arg) ;
        break ;
    case 259:
        ow_morehelp(op) ;
        return 1 ;
    case 260: /* fuse_opt, handled in owfs.c */
        break ;
    case 267: /* fuse_open_opt, handled in owfs.c */
        break ;
    case 268:
        usec_read = atol(arg)*1000 ; /* entered in msec, stored as usec */
        if (usec_read < 500000) usec_read = 500000 ;
        break ;
    case 269:
        max_clients = atoi(arg) ;
        break ;
    case 270:
        ftp_timeout = atoi(arg) ;
        break ;
    case 271:
        return OW_ArgHA7( arg ) ;
    case 272:
        return OW_ArgFake( arg ) ;
    case 0:
        break ;
    default:
        return 1 ;
    }
    return 0 ;
}

int OW_ArgNet( const char * arg ) {
    struct connection_in * in = NewIn(NULL) ;
    if ( in==NULL ) return 1 ;
    in->name = strdup(arg) ;
    in->busmode = bus_remote ;
    return 0 ;
}

static int OW_ArgHA7( const char * arg ) {
#if OW_HA7
    if ( arg ) {
        struct connection_in * in = NewIn(NULL) ;
        if ( in==NULL ) return 1 ;
        in->name = strdup(arg) ;
        in->busmode = bus_ha7 ;
        return 0 ;
    } else { // Try multicast discovery
        return FS_FindHA7() ;
    }
#else /* OW_HA7 */
    LEVEL_DEFAULT("HA7 support (intentionally) not included in compilation. Reconfigure and recompile.\n");
    return 1 ;
#endif /* OW_HA7 */
}

static int OW_ArgFake( const char * arg ) {
    struct connection_in * in = NewIn(NULL) ;
    if ( in==NULL ) return 1 ;
    in->name = strdup(arg) ;
    in->busmode = bus_fake ;
    return 0 ;
}

int OW_ArgServer( const char * arg ) {
    struct connection_out * out = NewOut() ;
    if ( out==NULL ) return 1 ;
    out->name = strdup(arg) ;
    return 0 ;
}

int OW_ArgDevice( const char * arg ) {
    struct stat sbuf ;
    if ( stat( arg, &sbuf ) ) {
        LEVEL_DEFAULT("Cannot access device %s\n",arg) ;
        return 1 ;
    }
    if ( ! S_ISCHR( sbuf.st_mode ) ) {
        LEVEL_DEFAULT("Not a \"character\" device %s\n",arg) ;
        return 1 ;
    }
    if ( major(sbuf.st_rdev)==99 ) return OW_ArgParallel(arg) ;
    if ( major(sbuf.st_rdev)==89 ) return OW_ArgI2C(arg) ;
    return OW_ArgSerial(arg) ;
} 

static int OW_ArgSerial( const char * arg ) {
    struct connection_in * in = NewIn(NULL) ;
    if ( in==NULL ) return 1 ;
    in->connin.serial.speed = B9600;
    in->name = strdup(arg) ;
    in->busmode = bus_serial ;

    return 0 ;
}

static int OW_ArgParallel( const char * arg ) {
#if OW_PARPORT
    struct connection_in * in = NewIn(NULL) ;
    if ( in==NULL ) return 1 ;
    in->name = strdup(arg) ;
    in->busmode = bus_parallel ;
    return 0 ;
#else /* OW_PARPORT */
    LEVEL_DEFAULT("Parallel port support (intentionally) not included in compilation. For DS1410E. That's ok, it doesn't work anyways.\n") ;
    return 1 ;
#endif /* OW_PARPORT */
}

static int OW_ArgI2C( const char * arg ) {
#if OW_I2C
    struct connection_in * in = NewIn(NULL) ;
    if ( in==NULL ) return 1 ;
    in->name = strdup(arg) ;
    in->busmode = bus_i2c ;
    return 0 ;
#else /* OW_I2C */
    LEVEL_DEFAULT("I2C (smbus DS2482-X00) support (intentionally) not included in compilation. Reconfigure and recompile.\n");
    return 1 ;
#endif /* OW_I2C */
}

int OW_ArgUSB( const char * arg ) {
#if OW_USB
    struct connection_in * in = NewIn(NULL) ;
    if ( in==NULL ) return 1 ;
    in->busmode = bus_usb ;
    if ( arg == NULL ) {
        in->connin.usb.usb_nr = 1 ;
    } else if ( strcasecmp(arg,"all") == 0 ) {
        int n ;
        n = DS9490_enumerate() ;
        LEVEL_CONNECT("All USB adapters requested, %d found.\n",n) ;
        if ( n > 1 ) {
            int i ;
            for ( i=2 ; i<=n ; ++i ) {
                struct connection_in * in2 = NewIn(NULL) ;
                if ( in2==NULL ) return 1 ;
                in2->busmode = bus_usb ;
                in2->connin.usb.usb_nr = i ;
            }
        }
        // first one
        in->connin.usb.usb_nr = 1 ;
    } else { 
        in->connin.usb.usb_nr = atoi(arg) ;
        //printf("ArgUSB fd=%d\n",in->fd);
        if ( in->connin.usb.usb_nr < 1 ) {
            LEVEL_CONNECT("USB option %s implies no USB detection.\n",arg) ;
            in->connin.usb.usb_nr = 0 ;
        } else if ( in->connin.usb.usb_nr > 1 ) {
            LEVEL_CONNECT("USB adapter %d requested.\n",in->connin.usb.usb_nr) ;
        }
    }
    return 0 ;
#else /* OW_USB */
    LEVEL_DEFAULT("USB support (intentionally) not included in compilation. Check LIBUSB, then reconfigure and recompile.\n");
    return 1 ;
#endif /* OW_USB */
}

int OW_ArgGeneric( const char * arg ) {
    if ( arg && arg[0] ) {
        switch (arg[0]) {
            case '/':
                return OW_ArgDevice(arg) ;
            case 'u':
            case 'U':
                return OW_ArgUSB(&arg[1]) ;
            default:
                return OW_ArgNet(arg) ;
        }
    }
    return 1 ;
}
