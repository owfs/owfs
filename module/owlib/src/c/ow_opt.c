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
    {"device",required_argument,NULL,'d'},
    {"usb",optional_argument,NULL,'u'},
    {"help",no_argument,NULL,'h'},
    {"port",required_argument,NULL,'p'},
    {"Celsius",no_argument,NULL,'C'},
    {"Fahrenheit",no_argument,NULL,'F'},
    {"Kelvin",no_argument,NULL,'K'},
    {"Rankine",no_argument,NULL,'R'},
    {"version",no_argument,NULL,'V'},
    {"format",required_argument,NULL,'f'},
    {"pid-file",required_argument,NULL,'P'},
    {"background",no_argument,&background,1},
    {"foreground",no_argument,&background,0},
    {0,0,0,0},
} ;

void owopt( const char c , const char * const arg ) {
    static int useusb = 0 ;
//printf("owopts in\n");
    switch (c) {
    case 'h':
        fprintf(stderr,
        "    -d    --device   serial adapter. Serial port connecting to 1-wire network (e.g. /dev/ttyS0)\n"
#ifdef OW_USB
        "    -u[n]  --usb     USB adapter. Scans for usb connection to 1-wire bus.\n"
        "                Optional number for multiple usb adapters\n"
        "                Note that -d and -u are mutually exclusive\n"
#endif /* OW_USB */
#ifdef OW_CACHE
        "    -t     cache timeout (in seconds)\n"
#endif /* OW_CACHE */
        "    -h     --help    print help\n"
        "    --format f[.]i[[.]c] format of device names f_amily i_d c_rc\n"
        "    -C | -F | -K | -R Temperature scale --Celsius(default)|--Fahrenheit|--Kelvin|--Rankine\n"
        "    --foreground --background(default)\n"
        "    -P --pid-file fielname outs the PID of this program into filename\n"
        "    -V --version\n"
        ) ;
        break ;
    case 't':
    	Timeout( arg ) ;
	    break ;
    case 'u':
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
        USBSetup(useusb) ;
#ifndef OW_USB
        fprintf(stderr,"Attempt to use USB adapter with no USB support in libow. Recompile libow with libusb support.\n") ;
        useusb=0;
#endif /* OW_USB */
        break ;
    case 'd':
    	ComSetup(arg) ;
	    break ;
    case 'C':
	    tempscale = temp_celsius ;
	    break ;
    case 'F':
	    tempscale = temp_fahrenheit ;
	    break ;
    case 'R':
	    tempscale = temp_rankine ;
	    break ;
    case 'K':
	    tempscale = temp_kelvin ;
	    break ;
    case 'V':
	    printf("libow version:\n\t" VERSION "\n") ;
#ifdef OW_CACHE
        printf("libdb version:\n\t%s\n",db_version(NULL,NULL,NULL) ) ;
#endif /* OW_CACHE */
	    break ;
    case 'p':
        sscanf(optarg, "%i", &portnum);
        break;
    case 'f':
        if (strcasecmp(arg,"f.i")==0) devform=fdi;
        else if (strcasecmp(arg,"fi")==0) devform=fi;
        else if (strcasecmp(arg,"f.i.c")==0) devform=fdidc;
        else if (strcasecmp(arg,"f.ic")==0) devform=fdic;
        else if (strcasecmp(arg,"fi.c")==0) devform=fidc;
        else if (strcasecmp(arg,"fic")==0) devform=fic;
        else fprintf(stderr,"Unrecognized format type %s\n",arg);
        break;
    case 'P':
//printf("opopts pid_file for %s\n",arg);
        {
            FILE * pid = fopen( arg , "w+" ) ;
            if ( pid == NULL ) {
                perror("Error opening PID file") ;
                fprintf( stderr , "PID file attempted: %s\n",arg ) ;
                exit(1) ;
            }
//printf("opopts pid = %ld\n",(long unsigned int)getpid());
            fprintf(pid,"%lu",(long unsigned int)getpid() ) ;
            fclose(pid) ;
        }
        break ;
    case 0:
        break;
    }
//printf("owopts out\n");
}
