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
	my_pthread_mutex_lock(&(in->bus_mutex));
	if (in->busmode == bus_i2c && in->connin.i2c.channels > 1) {
		my_pthread_mutex_lock(&(in->connin.i2c.head->connin.i2c.i2c_mutex));
	}
#endif							/* OW_MT */
	gettimeofday(&(in->last_lock), NULL);	/* for statistics */
	STAT_ADD1_BUS(e_bus_locks, in);
}

void BUS_unlock_in(struct connection_in *in)
{
	struct timeval *t;
	long sec, usec;
	if (!in) {
		return;
	}

	gettimeofday(&(in->last_unlock), NULL);

	/* avoid update if system-clock have changed */
	STATLOCK;
	sec = in->last_unlock.tv_sec - in->last_lock.tv_sec;
	if ((sec >= 0) && (sec < 60)) {
		usec = in->last_unlock.tv_usec - in->last_lock.tv_usec;
		total_bus_time.tv_sec += sec;
		total_bus_time.tv_usec += usec;
		if (total_bus_time.tv_usec >= 1000000) {
			total_bus_time.tv_usec -= 1000000;
			++total_bus_time.tv_sec;
		} else if (total_bus_time.tv_usec < 0) {
			total_bus_time.tv_usec += 1000000;
			--total_bus_time.tv_sec;
		}

		t = &in->bus_time;
		t->tv_sec += sec;
		t->tv_usec += usec;
		if (t->tv_usec >= 1000000) {
			t->tv_usec -= 1000000;
			++t->tv_sec;
		} else if (t->tv_usec < 0) {
			t->tv_usec += 1000000;
			--t->tv_sec;
		}
	}
	++in->bus_stat[e_bus_unlocks];
	STATUNLOCK;
#if OW_MT
	if (in->busmode == bus_i2c && in->connin.i2c.channels > 1) {
		my_pthread_mutex_unlock(&(in->connin.i2c.head->connin.i2c.i2c_mutex));
	}
	my_pthread_mutex_unlock(&(in->bus_mutex));
#endif							/* OW_MT */
}
