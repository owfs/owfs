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
#include "ow_connection.h"

/* ------- Globals ----------- */

#if OW_MT
pthread_mutex_t connin_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef __UCLIBC__
/* vsnprintf() doesn't seem to be thread-safe in uClibc
   even if thread-support is enabled. */
pthread_mutex_t uclibc_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif							/* __UCLIBC__ */
#endif							/* OW_MT */

/* Essentially sets up mutexes to protect global data/devices */
void LockSetup(void)
{
#if OW_MT
	pthread_mutex_init(&connin_mutex, pmattr);
#ifdef __UCLIBC__
	pthread_mutex_init(&uclibc_mutex, pmattr);
#endif							/* UCLIBC */
#endif							/* OW_MT */
}

void BUS_lock_in(struct connection_in *in)
{
	if (!in)
		return;
#if OW_MT
	pthread_mutex_lock(&(in->bus_mutex));
#endif							/* OW_MT */
}

void BUS_unlock_in(struct connection_in *in)
{
	if (!in)
		return;

#if OW_MT
	pthread_mutex_unlock(&(in->bus_mutex));
#endif							/* OW_MT */
}
