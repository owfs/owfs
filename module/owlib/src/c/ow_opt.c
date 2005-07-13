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
    {"pid_file",   required_argument,NULL,'P'},
    {"pid-file",   required_argument,NULL,'P'},
    {"background", no_argument,&background, 1},
    {"foreground", no_argument,&background, 0},
    {"error_print",required_argument,NULL,257},
    {"error-print",required_argument,NULL,257},
    {"error_level",required_argument,NULL,258},
    {"error-level",required_argument,NULL,258},
    {"morehelp",   no_argument,      NULL,259},
    {"fuse_opt",   required_argument,NULL,260}, /* owfs */
    {"fuse-opt",   required_argument,NULL,260}, /* owfs */
    {"NFS_program",required_argument,NULL,261}, /* ownfsd */
    {"NFS-program",required_argument,NULL,261}, /* ownfsd */
    {"NFS_version",required_argument,NULL,262}, /* ownfsd */
    {"NFS-version",required_argument,NULL,262}, /* ownfsd */
    {"tcp_only",   no_argument,      NULL,263}, /* ownfsd */
    {"tcp-only",   no_argument,      NULL,263}, /* ownfsd */
    {"udp_only",   no_argument,      NULL,264}, /* ownfsd */
    {"udp-only",   no_argument,      NULL,264}, /* ownfsd */
    {"NFS_port",   required_argument,NULL,265}, /* ownfsd */
    {"NFS-port",   required_argument,NULL,265}, /* ownfsd */
    {"mount_port", required_argument,NULL,266}, /* ownfsd */
    {"mount-port", required_argument,NULL,266}, /* ownfsd */
    {"no_portmapper", no_argument,   &(NFS_params.no_portmapper),1}, /* ownfsd */
    {"no-portmapper", no_argument,   &(NFS_params.no_portmapper),1}, /* ownfsd */
    {0,0,0,0},
} ;

/* Globals */
char * pid_file = NULL ;
char * fuse_opt = NULL ;
unsigned int nr_adapters = 0;

static void ow_help( enum opt_program op ) {
    printf(
    "1-wire access programs\n"
    "By Paul H Alfille and others. See http://owfs.sourceforge.net\n"
    "\n"
    ) ;
    switch(op) {
    case opt_owfs:
        printf("Syntax: %s [options] device mountpoint\n",progname);
        break ;
    case opt_httpd:
    case opt_server:
        printf("Syntax: %s [options] device clientport\n",progname);
        break ;
    case opt_nfsd:
    case opt_ftpd:
    default:
        printf("Syntax: %s [options] device\n",progname);
        break ;
    }
    printf(
    "\n"
    "1-wire side (device):\n"
    "  -d --device    devicename      |Serial port device name of 1-wire adapter\n"
    "                                 |  e.g: -d /dev/ttyS0\n"
#ifdef OW_USB
    "  -u --usb       [number]        |USB 1-wire adapter. Choose first one unless number specified\n"
#endif /* OW_USB */
    "                                 |  e.g: -u -u3  first and third USB 1wire adapters\n"
    "  -s --server    [host:]port     |owserver program that talks to adapter. Give tcp/ip address.\n"
    "                                 |  e.g: -s embeddedhost:3030\n"
    "\n"
    ) ;
    switch(op) {
    case opt_owfs:
        printf(
        "\n"
        "Client side:\n"
        "  mountpoint                     |Directory the 1-wire filesystem is mounted\n"
        ) ;
        break ;
    case opt_httpd:
    case opt_ftpd:
    case opt_nfsd:
    case opt_server:
    printf(
        "\n"
        "Client side:\n"
        "  -p --port                      |tcp/ip address that program can be found\n"
        ) ;
    break ;
    default:
    break ;
}
    printf(
    "\n"
    "  -h --help                      |This basic help page\n"
    "     --morehelp                  |Optional items help page\n"
    ) ;
}

static void ow_morehelp( enum opt_program op ) {
    printf(
    "1-wire access programs\n"
    "By Paul H Alfille and others. See http://owfs.sourceforge.net\n"
    "\n"
    ) ;
    printf(
    "  -C --Celsius                   |Celsius(default) temperature scale\n"
    "  -F --Fahrenheit                |Fahrenheit temperature scale\n"
    "  -K --Kelvin                    |Kelvin temperature scale\n"
    "  -R --Rankine                   |Rankine temperature scale\n"
    "  -P --pid_file                  |file where program id (pid) will be stored\n"
    "     --background                |become a deamon process(default)\n"
    "     --foreground                |stay in foreground\n"
    "  -r --readonly                  |no writing to 1-wire bus\n"
    "  -w --write                     |allow reading and writing to bus(default)\n"
#ifdef OW_CACHE
    "  -t                             |cache timeout (in seconds)\n"
#endif /* OW_CACHE */    
    "  -f --format                    |format for 1-wire unique serial IDs display\n"
    "                                 |  f[.]i[[.]c] f-amily i-d c-rc (all in hex)\n"
    "     --error_print               |Where information/error messages are printed\n"
    "                                 |  0-mixed(default) 1-syslog 2-stderr 3-suppressed\n"
    "     --error_level               |What kind of information is printed\n"
    "                                 |  0-fatal 1-connections 2-calls 3-data\n"
    "  -V --version                   |Program and library versions\n"
    ) ;
    switch(op) {
    case opt_owfs:
        printf(
        "     --fuse_opt                  |Options to send to fuse_mount (must be quoted)\n"
        "                                 |  e.g: --fuse_opt=\"-x -f\"\n"
        ) ;
        break ;
        case opt_httpd:
        case opt_ftpd:
        case opt_server:
            printf(
            "\n"
            "Client side:\n"
            "  -p --port                      |tcp/ip address that program can be found\n"
            ) ;
            break ;
        case opt_nfsd:
            printf(
            "     --NFS_program               |NFS: 100003, default OWNFS 300003\n"
            "     --NFS_version               |default 3\n"
            "     --tcp_only                  |no udp transport (overides udp_only)\n"
            "     --udp_only                  |no tcp transport (overides tcp_only)\n"
            "     --NFS_port                  |default 0=auto, 2049 traditional\n"
            "     --mount_port                |default 0=auto, usually same as NFS_port.\n"
            "     --no_portmapper             |Don't use the standard portmapper.\n"
            ) ;
            break ;
        default:
            break ;
    }
    printf(
    "\n"
    "  -h --help                      |Basic help page\n"
    "     --morehelp                  |This optional items help page\n"
    ) ;
}

/* Parses one argument */
/* return 0 if ok */
            int owopt( const int c , const char * const arg, enum opt_program op ) {
    switch (c) {
    case 'h':
        ow_help( op ) ;
        return 1 ;
    case 'u':
#ifdef OW_USB
        return OW_ArgUSB( arg ) ;
#else /* OW_USB */
        LEVEL_DEFAULT("Attempt to use USB adapter with no USB support in libow. Recompile libow with libusb support.\n")
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
             LEVEL_DEFAULT("Unrecognized format type %s\n",arg)
             return 1 ;
        }
        return 0 ;
    case 'P':
        if ( arg==NULL || strlen(arg)==0 ) {
            LEVEL_DEFAULT("No PID file specified\n")
            return 1 ;
        } else if ( (pid_file=strdup(arg)) == NULL ) {
            fprintf( stderr,"Insufficient memory to store the PID filename: %s\n",arg) ;
            return 1 ;
        }
        return 0 ;
    case 257:
        error_print = atoi(arg) ;
        return 0 ;
    case 258:
        error_level = atoi(arg) ;
        return 0 ;
    case 259:
        ow_morehelp(op) ;
        return 1 ;
    case 260: /* fuse-opt */
    case 261:
        NFS_params.NFS_program = atoi(arg) ;
        return 0 ;
    case 262:
        NFS_params.NFS_version = atoi(arg) ;
        return 0 ;
    case 263:
        NFS_params.tcp_only = 1 ;
        NFS_params.udp_only = 0 ;
        return 0 ;
    case 264:
        NFS_params.tcp_only = 0 ;
        NFS_params.udp_only = 1 ;
        return 0 ;
    case 265:
        NFS_params.NFS_port = atoi(arg) ;
        return 0 ;
    case 266:
        NFS_params.mount_port = atoi(arg) ;
        return 0 ;
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
        LEVEL_DEFAULT("Cannot allocate memory for network struct\n")
        return 1 ;
    }
    in->name = strdup(arg) ;
    in->busmode = bus_remote ;
    return 0 ;
}

int OW_ArgServer( const char * arg ) {
    struct connection_out * out = NewOut() ;
    if ( out==NULL ) {
        LEVEL_DEFAULT("Cannot allocate memory for server struct\n")
        return 1 ;
    }
    out->name = strdup(arg) ;
    return 0 ;
}

int OW_ArgSerial( const char * arg ) {
    struct connection_in * in = NewIn() ;
    if ( in==NULL ) {
        LEVEL_DEFAULT("Cannot allocate memory for serial struct\n")
        return 1 ;
    }
    in->connin.serial.speed = B9600;
    in->name = strdup(arg) ;
    in->busmode = bus_serial ;
    return 0 ;
}

int OW_ArgUSB( const char * arg ) {
    struct connection_in * in = NewIn() ;
    if ( in==NULL ) {
        LEVEL_DEFAULT("Cannot allocate memory for USB struct\n")
        return 1 ;
    }
    in->busmode = bus_usb ;
    if ( arg ) {
        in->fd = atoi(arg) ;
//printf("ArgUSB fd=%d\n",in->fd);
        if ( in->fd < 1 ) {
            LEVEL_CONNECT("USB option %s implies no USB detection.\n",arg)
            in->fd=0 ;
        } else if ( in->fd>1 ) {
            LEVEL_CONNECT("USB adapter %d requested.\n",in->fd)
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
