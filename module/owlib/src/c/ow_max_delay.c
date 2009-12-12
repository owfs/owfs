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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

/* ------- Prototypes ------------ */

/* ------- Functions ------------ */

/*
 * Funcation to calculated maximum delay between writing a
 * command to the bus, and reading the answer.
 * This will probably not work with simultanious reading...
 */
void update_max_delay(struct connection_in *connection)
{
	long sec, usec;
	struct timeval *r, *w;
	struct timeval last_delay;
	if (connection == NULL) {
		return;
	}
	gettimeofday(&(connection->bus_read_time), NULL);
	r = &(connection->bus_read_time);
	w = &(connection->bus_write_time);

	sec = r->tv_sec - w->tv_sec;
	if ((sec >= 0) && (sec <= 5)) {
		usec = r->tv_usec - w->tv_usec;
		last_delay.tv_sec = sec;
		last_delay.tv_usec = usec;

		while (last_delay.tv_usec >= 1000000) {
			last_delay.tv_usec -= 1000000;
			last_delay.tv_sec++;
		}
		if ((last_delay.tv_sec > max_delay.tv_sec)
			|| ((last_delay.tv_sec >= max_delay.tv_sec)
				&& (last_delay.tv_usec > max_delay.tv_usec))
			) {
			STATLOCK;
			max_delay.tv_sec = last_delay.tv_sec;
			max_delay.tv_usec = last_delay.tv_usec;
			STATUNLOCK;
		}
	}
	/* DS9097(1410)_send_and_get() call this function many times, therefore
	 * I reset bus_write_time after every calls */
	gettimeofday(&(connection->bus_write_time), NULL);
	return;
}
