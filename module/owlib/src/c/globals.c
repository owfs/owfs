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
#include "ow_devices.h"

/* Globals for port and bus communication */
/* connections globals stored in ow_connect.c */
/* i.e. connection_in * indevices ...         */

#if OW_MT
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

/* State informatoin, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
#if OW_CACHE
int32_t SemiGlobal = ((uint8_t)fdi)<<24 | ((uint8_t)temp_celsius)<<16 | ((uint8_t)1)<<8 | ((uint8_t)1) ;
#else
int32_t SemiGlobal = ((uint8_t)fdi)<<24 | ((uint8_t)temp_celsius)<<16 | ((uint8_t)1)<<8 ;
#endif

struct global Global ;

/* Statistics globals are stored in ow_stats.c */

