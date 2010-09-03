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

/* locks are to handle multithreading */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

void BUS_lock(const struct parsedname *pn)
{
	if (pn) {
		BUS_lock_in(pn->selected_connection);
	}
}

void BUS_unlock(const struct parsedname *pn)
{
	if (pn) {
		BUS_unlock_in(pn->selected_connection);
	}
}

void BUS_lock_in(struct connection_in *in)
{
	if (!in) {
		return;
	}
#if OW_MT
	_MUTEX_LOCK(in->bus_mutex);
	if (in->busmode == bus_i2c && in->master.i2c.channels > 1) {
		_MUTEX_LOCK(in->master.i2c.head->master.i2c.all_channel_lock);
	}
#endif							/* OW_MT */
	timernow( &(in->last_lock) );	/* for statistics */
	STAT_ADD1_BUS(e_bus_locks, in);
}

void BUS_unlock_in(struct connection_in *in)
{
	struct timeval tv;

	if (!in) {
		return;
	}

	timernow( &tv );

	if ( timercmp( &tv, &(in->last_lock), <) ) {
		LEVEL_DEBUG("System clock moved backward");
		timernow( &(in->last_lock) );
	}
	timersub( &tv, &(in->last_lock), &tv ) ;

	STATLOCK;
	timeradd( &tv, &(in->bus_time), &(in->bus_time) ) ;
	++in->bus_stat[e_bus_unlocks];
	STATUNLOCK;

#if OW_MT
	if (in->busmode == bus_i2c && in->master.i2c.channels > 1) {
		_MUTEX_UNLOCK(in->master.i2c.head->master.i2c.all_channel_lock);
	}
	_MUTEX_UNLOCK(in->bus_mutex);
#endif							/* OW_MT */
}
