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
#include "ow_devices.h"

char * progname = "" ;

/* Globals for port and bus communication */
/* connections globals stored in ow_connect.c */
/* i.e. connection_in * indevices ...         */

#ifdef OW_MT
/* mutex attribute -- needed for uClibc programming */
/* we create at start, and destroy at end */
pthread_mutexattr_t * pmattr = NULL ;
pthread_mutexattr_t mattr ;
#endif /* OW_MT */

/* information about this process */
pid_t pid_num ;
/* char * pid_file in ow_opt.c */

/* Times for directory information */
time_t start_time ;
time_t dir_time ; /* time of last directory scan */

int background = 1 ; /* operate in background mode */
int readonly = 0 ; /* readonly file system */

/* State informatoin, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
union semiglobal SemiGlobal  = { u:{
#ifdef OW_CACHE
    1, /* bit0=cacheenabled  bit1=return-buslist */
#else /* OW_CACHE */
    0, /* bit0=cacheenabled  bit1=return-buslist */
#endif /* OW_CACHE */
    1,
    (uint8_t)temp_celsius,
    (uint8_t)fdi, }
};

/* Statistics globalsw stored in ow_stats.c */

