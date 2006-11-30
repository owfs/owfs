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

#include "owshell.h"

static void OW_parsevalue( int * var, const ASCII * str ) ;

const struct option owopts_long[] = {
    {"help",       no_argument,      NULL,'h'},
    {"server",     required_argument,NULL,'s'},
    {"Celsius",    no_argument,      NULL,'C'},
    {"Fahrenheit", no_argument,      NULL,'F'},
    {"Kelvin",     no_argument,      NULL,'K'},
    {"Rankine",    no_argument,      NULL,'R'},
    {"version",    no_argument,      NULL,'V'},
    {"format",     required_argument,NULL,'f'},
    
    {"timeout_network"   , required_argument, NULL, 307, } , // timeout -- tcp wait
    {"timeout_server"    , required_argument, NULL, 308, } , // timeout -- server wait
    {"autoserver",    no_argument,      NULL,275},

    {0,0,0,0},
} ;

/* Parses one argument */
/* return 0 if ok */
void owopt( const int c , const char * arg ) {
    //printf("Option %c (%d) Argument=%s\n",c,c,SAFESTRING(arg)) ;
    switch (c) {
    case 'h':
        ow_help() ;
        exit(0) ;
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
        printf("owshell version:\n\t" VERSION "\n") ;
        exit(0) ;
    case 's':
        OW_ArgNet( arg ) ;
        break ;
    case 'f':
        if (!strcasecmp(arg,"f.i"))        set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
        else if (!strcasecmp(arg,"fi"))    set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fi);
        else if (!strcasecmp(arg,"f.i.c")) set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdidc);
        else if (!strcasecmp(arg,"f.ic"))  set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fdic);
        else if (!strcasecmp(arg,"fi.c"))  set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fidc);
        else if (!strcasecmp(arg,"fic"))   set_semiglobal(&SemiGlobal, DEVFORMAT_MASK, DEVFORMAT_BIT, fic);
        else {
            fprintf(stderr,"Unrecognized format type %s\n",arg) ;
            exit(1) ;
        }
        break ;
    case 275: // autoserver
#if OW_ZERO
        if(libdnssd == NULL) {
	  fprintf(stderr, "Zeroconf/Bonjour is disabled since dnssd library isn't found.\n");
	  exit(0);
	} else {
	  OW_Browse() ;
	}
#else
	fprintf(stderr, "Zeroconf/Bonjour is disabled since it's compiled without support.\n");
	exit(0);
#endif
        break ;
    case 307: case 308:
        OW_parsevalue(&((int *) &Global.timeout_volatile)[c-301],arg) ;
    case 0:
        break ;
    default:
        exit(1) ;
    }
}

void OW_ArgNet( const char * arg ) {
    struct connection_in * in ;
    if ( indevice ) {
        fprintf(stderr,"Cannot link to more than one owserver. (%s and %s).\n",indevice->name,arg) ;
        exit(1) ;
    } else if ( (in = NewIn())==NULL ) {
        fprintf(stderr,"Memory exhausted.\n") ;
        exit(1) ;
    }
    in->name = strdup(arg) ;
    in->busmode = bus_server ;
}

static void OW_parsevalue( int * var, const ASCII * str ) {
    int I ;
    errno = 0 ;
    I = strtol(str,NULL,10) ;
    if ( errno ) {
        fprintf(stderr,"Bad configuration value %s\n",str) ;
        exit(1) ;
    }
    var[0] = I ;
}
