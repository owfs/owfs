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
    {"device",    required_argument,NULL,'d'},
    {"usb",       optional_argument,NULL,'u'},
    {"help",      no_argument,      NULL,'h'},
    {"port",      required_argument,NULL,'p'},
    {"readonly",  required_argument,NULL,'r'},
    {"write",     required_argument,NULL,'w'},
    {"Celsius",   no_argument,      NULL,'C'},
    {"Fahrenheit",no_argument,      NULL,'F'},
    {"Kelvin",    no_argument,      NULL,'K'},
    {"Rankine",   no_argument,      NULL,'R'},
    {"version",   no_argument,      NULL,'V'},
    {"format",    required_argument,NULL,'f'},
    {"fuse-opt",  required_argument,NULL,260},
    {"pid-file",  required_argument,NULL,'P'},
    {"background",no_argument,&background, 1},
    {"foreground",no_argument,&background, 0},
    {0,0,0,0},
} ;

char * pid_file = NULL ;
char * fuse_opt = NULL ;

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
        "                 Note that -d and -u are mutually exclusive\n"
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
        if ( arg ) {
            useusb = atoi(arg) ;
            if ( useusb < 1 ) {
                fprintf(stderr,"USB option %s implies no USB detection.\n",arg) ;
                useusb=0 ;
            } else if ( useusb>1 ) {
                syslog(LOG_INFO,"USB adapter %d requested.\n",useusb) ;
            }
        } else {
            useusb=1 ;
        }
        USBSetup() ;
#else /* OW_USB */
        fprintf(stderr,"Attempt to use USB adapter with no USB support in libow. Recompile libow with libusb support.\n") ;
#endif /* OW_USB */
        return 0 ;
    case 'd':
        ComSetup(arg) ;
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
        tempscale = temp_celsius ;
        return 0 ;
    case 'F':
        tempscale = temp_fahrenheit ;
        return 0 ;
    case 'R':
        tempscale = temp_rankine ;
        return 0 ;
    case 'K':
        tempscale = temp_kelvin ;
        return 0 ;
    case 'V':
        printf("libow version:\n\t" VERSION "\n") ;
        return 1 ;
    case 'p':
        sscanf(optarg, "%i", &portnum);
        return 0;
    case 'f':
        if (strcasecmp(arg,"f.i")==0) devform=fdi;
        else if (strcasecmp(arg,"fi")==0) devform=fi;
        else if (strcasecmp(arg,"f.i.c")==0) devform=fdidc;
        else if (strcasecmp(arg,"f.ic")==0) devform=fdic;
        else if (strcasecmp(arg,"fi.c")==0) devform=fidc;
        else if (strcasecmp(arg,"fic")==0) devform=fic;
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
    case 0:
        return 0 ;
    default:
        return 1 ;
    }
}
