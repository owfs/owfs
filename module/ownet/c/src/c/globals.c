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

/* State information, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
#if OW_CACHE
int32_t SemiGlobal = ((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8 | ((uint8_t) 1);
#else
int32_t SemiGlobal = ((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8;
#endif

struct global Global = {
#if OW_ZERO
	.browse = NULL,
#endif

	.progname = NULL,			// "One Wire File System" , Can't allocate here since it's freed
	.now_background = 0,

	.error_level = e_err_default,
	.error_print = e_err_print_mixed,

	.readonly = 0,
};

/* Statistics globals are stored in ow_stats.c */

#include "ow_server.h"
struct ow_global ow_Global = {
	.timeout_network = 1,
	.timeout_server = 10,
	.autoserver = 0,
};
