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

int LI_reset( const struct parsedname * const pn )
{
    char resp[3] ;
    COM_flush() ;
    if ( BUS_write("r",1) ) return -errno ;
    sleep(1) ;
    if ( BUS_read(resp,3) ) return -errno ;
    switch( resp[0] ) {
    case 'P':
        pn->si->AnyDevices=1 ;
        break ;
    case 'N':
        pn->si->AnyDevices=0 ;
        break ;
    default:
        pn->si->AnyDevices=0 ;
        syslog(LOG_INFO,"1-wire bus short circuit.\n") ;
    }
    return 0 ;
}

