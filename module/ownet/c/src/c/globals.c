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
#include "ow_server.h"

/* Globals for port and bus communication */
/* connections globals stored in ow_connect.c */
/* i.e. connection_in * head_inbound_list ...         */

#if OW_MT
/* mutex attribute -- needed for uClibc programming */
/* we create at start, and destroy at end */
pthread_mutexattr_t *pmattr = NULL;
pthread_mutexattr_t mattr;
#endif							/* OW_MT */

/* information about this process */
pid_t pid_num;
/* char * pid_file in ow_opt.c */

/* Times for directory information */
time_t start_time;
time_t dir_time;				/* time of last directory scan */

/* State information, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
#if OW_CACHE
int32_t SemiGlobal =
	((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8 | ((uint8_t) 1);
#else
int32_t SemiGlobal =
	((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8;
#endif

struct global Global = {
    .announce_off = 0 ,
    .announce_name = NULL ,
#if OW_ZERO
    .browse = NULL ,
#endif

    .progname = NULL , // "One Wire File System" , Can't allocate here since it's freed
    .want_background = 1 ,
    .now_background = 0 ,

    .error_level = e_err_default ,
    .error_print = e_err_print_mixed ,

    .readonly = 0 ,
    .SimpleBusName = "None" ,
} ;
/* Statistics globals are stored in ow_stats.c */

#include "ow_server.h"
struct ow_global ow_Global = {
    .timeout_network = 1 ,
    .timeout_server = 10 ,
	.autoserver = 0 ,
} ;
