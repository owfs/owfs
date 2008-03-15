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

struct stateinfo StateInfo = {
    .lib_setup_done = 0 ,
    .lib_start_done = 0 ,
    .lib_finish_done = 0 ,
    .lock_setup_done = 0 ,
    .start_time = 0 ,
    .dir_time =0 ,
    .shutdown_in_progress = 0,
};
