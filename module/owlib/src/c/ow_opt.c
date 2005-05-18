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

#include "owfs_config.h"
#include "ow.h"

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
    {"pid-file",   required_argument,NULL,'P'},
    {"background", no_argument,&background, 1},
    {"foreground", no_argument,&background, 0},
    {"fuse-opt",   required_argument,NULL,260}, /* owfs */
    {"RPC_program",required_argument,NULL,261}, /* ownfsd */
    {"NFS_version",required_argument,NULL,262}, /* ownfsd */
    {"tcp_only",   no_argument,      NULL,263}, /* ownfsd */
    {"udp_only",   no_argument,      NULL,264}, /* ownfsd */
    {"NFS_port",   required_argument,NULL,265}, /* ownfsd */
    {"mount_port", required_argument,NULL,266}, /* ownfsd */
    {0,0,0,0},
} ;

/* Globals */
char * pid_file = NULL ;
char * fuse_opt = NULL ;
unsigned int nr_adapters = 0;

/* Parses one argument */
/* return 0 if ok */
int owopt( const int c , const char * const arg ) {
    switch (c) {
    case 'h':
        fprintf(stderr,
        "    -d    --device   serial adapter. Serial port connecting to 1-wire network (e.g. /dev/ttyS0)\n"
#ifdef OW_USB
        "    -u[n] --usb      USB adapter. Scans for usb connection to 1-wire bus.\n"
        "                 Optional number for multiple usb adapters\n"
        "    -s  --server     1-wire server daemon\n"
        "                 port number, server:port or /local-socket-path\n"
        "                 Each adapter chosen will be handled independly\n"
#endif /* OW_USB */
#ifdef OW_CACHE
        "    -t    cache timeout (in seconds)\n"
#endif /* OW_CACHE */
        "    -h    --help     print help\n"
        "    -r    --readonly Only allow reading of 1-wire devices.\n"
        "    -w    --write    Allow reading and writing (default).\n"
        "    --format f[.]i[[.]c] format of device names f_amily i_d c_rc\n"
        "    -C | -F | -K | -R Temperature scale --Celsius(default)|--Fahrenheit|--Kelvin|--Rankine\n"
        "    --foreground --background(default)\n"
        "    -P --pid-file filename     put the PID of this program into filename\n"
        "    -V --version\n"
        ) ;
        return 1 ;
        return 0 ;
    case 'u':
#ifdef OW_USB
//printf("OPT usb arg=%s\n",arg);
        return OW_ArgUSB( arg ) ;
#else /* OW_USB */
        fprintf(stderr,"Attempt to use USB adapter with no USB support in libow. Recompile libow with libusb support.\n") ;
        return 1 ;
#endif /* OW_USB */
    case 'd':
        return OW_ArgSerial( arg ) ;
    case 't':
        Timeout(arg) ;
        return 0 ;
    case 'r':
        readonly = 1 ;
        return 0 ;
    case 'w':
        readonly = 0 ;
        return 0 ;
    case 'C':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
        return 0 ;
    case 'F':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit) ;
        return 0 ;
    case 'R':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine) ;
        return 0 ;
    case 'K':
        set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin) ;
        return 0 ;
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
             fprintf(stderr,"Unrecognized format type %s\n",arg);
             return 1 ;
        }
        return 0 ;
    case 'P':
        if ( arg==NULL || strlen(arg)==0 ) {
            fprintf(stderr,"No PID file specified\n") ;
            return 1 ;
        } else if ( (pid_file=strdup(arg)) == NULL ) {
            fprintf( stderr,"Insufficient memory to store the PID filename: %s\n",arg) ;
            return 1 ;
        }
        return 0 ;
    case 260: /* fuse-opt */
    case 261:
    case 262:
    case 263:
    case 264:
    case 265:
    case 266:
    case 267:
    case 268:
    case 269:
    case 0:
        return 0 ;
    default:
        return 1 ;
    }
}

int OW_ArgNet( const char * arg ) {
    struct connection_in * in = NewIn() ;
    if ( in==NULL ) {
        fprintf(stderr,"Cannot allocate memory for network struct\n") ;
        return 1 ;
    }
    in->name = strdup(arg) ;
    in->busmode = bus_remote ;
    return 0 ;
}

int OW_ArgServer( const char * arg ) {
    struct connection_out * out = NewOut() ;
    if ( out==NULL ) {
        fprintf(stderr,"Cannot allocate memory for server struct\n") ;
        return 1 ;
    }
    out->name = strdup(arg) ;
    return 0 ;
}

int OW_ArgSerial( const char * arg ) {
    struct connection_in * in = NewIn() ;
    if ( in==NULL ) {
        fprintf(stderr,"Cannot allocate memory for serial struct\n") ;
        return 1 ;
    }
    in->name = strdup(arg) ;
    in->busmode = bus_serial ;
    return 0 ;
}

int OW_ArgUSB( const char * arg ) {
    struct connection_in * in = NewIn() ;
    if ( in==NULL ) {
        fprintf(stderr,"Cannot allocate memory for USB struct\n") ;
        return 1 ;
    }
    in->busmode = bus_usb ;
    if ( arg ) {
        in->fd = atoi(arg) ;
//printf("ArgUSB fd=%d\n",in->fd);
        if ( in->fd < 1 ) {
            fprintf(stderr,"USB option %s implies no USB detection.\n",arg) ;
            in->fd=0 ;
        } else if ( in->fd>1 ) {
            syslog(LOG_INFO,"USB adapter %d requested.\n",in->fd) ;
        }
    } else {
        in->fd=1 ;
    }
    return 0 ;
}

int OW_ArgGeneric( const char * arg ) {
    if ( arg && arg[0] ) {
        return arg[0]=='/' ? OW_ArgSerial(arg) : OW_ArgNet(arg) ;
    }
    return 1 ;
}
