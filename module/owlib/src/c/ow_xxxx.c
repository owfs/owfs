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
#include "ow_xxxx.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Prototypes ------------ */

/* ------- Functions ------------ */

/*
 * Funcation to calculated maximum delay between writing a
 * command to the bus, and reading the answer.
 * This will probably not work with simultanious reading...
 */
void update_max_delay(const struct parsedname *pn)
{
	long sec, usec;
	struct timeval *r, *w;
	struct timeval last_delay;
	if (!pn || !pn->in)
		return;
	gettimeofday(&(pn->in->bus_read_time), NULL);
	r = &pn->in->bus_read_time;
	w = &pn->in->bus_write_time;

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
	gettimeofday(&(pn->in->bus_write_time), NULL);
	return;
}

int FS_type(char *buf, size_t size, off_t offset,
			const struct parsedname *pn)
{
	return FS_output_ascii_z(buf, size, offset, pn->dev->name);
}

int FS_code(char *buf, size_t size, off_t offset,
			const struct parsedname *pn)
{
	ASCII code[2];
	num2string(code, pn->sn[0]);
	return FS_output_ascii(buf, size, offset, code, 2);
}

int FS_ID(char *buf, size_t size, off_t offset,
		  const struct parsedname *pn)
{
	ASCII id[12];
	bytes2string(id, &(pn->sn[2]), 6);
	return FS_output_ascii(buf, size, offset, id, 12);
}

int FS_r_ID(char *buf, size_t size, off_t offset,
			const struct parsedname *pn)
{
	size_t i;
	ASCII id[12];
	for (i = 0; i < 6; ++i)
		num2string(id + (i << 1), pn->sn[6 - i]);
	return FS_output_ascii(buf, size, offset, id, 12);
}

int FS_crc8(char *buf, size_t size, off_t offset,
			const struct parsedname *pn)
{
	ASCII crc[2];
	num2string(crc, pn->sn[7]);
	return FS_output_ascii(buf, size, offset, crc, 2);
}

int FS_address(char *buf, size_t size, off_t offset,
			   const struct parsedname *pn)
{
	ASCII ad[16];
	bytes2string(ad, pn->sn, 8);
	return FS_output_ascii(buf, size, offset, ad, 16);
}

int FS_r_address(char *buf, size_t size, off_t offset,
				 const struct parsedname *pn)
{
	size_t i;
	ASCII ad[16];
	for (i = 0; i < 8; ++i)
		num2string(ad + (i << 1), pn->sn[7 - i]);
	return FS_output_ascii(buf, size, offset, ad, 16);
}
