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
	if (pn == NULL || pn->selected_connection == NULL) {
		return;
	}
	gettimeofday(&(pn->selected_connection->bus_read_time), NULL);
	r = &pn->selected_connection->bus_read_time;
	w = &pn->selected_connection->bus_write_time;

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
	gettimeofday(&(pn->selected_connection->bus_write_time), NULL);
	return;
}

int FS_type(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return Fowq_output_offset_and_size_z(pn->selected_device->readable_name, owq);
}

int FS_code(struct one_wire_query *owq)
{
	ASCII code[2];
	struct parsedname *pn = PN(owq);
	num2string(code, pn->sn[0]);
	return Fowq_output_offset_and_size(code, 2, owq);
}

int FS_ID(struct one_wire_query *owq)
{
	ASCII id[12];
	struct parsedname *pn = PN(owq);
	bytes2string(id, &(pn->sn[1]), 6);
	return Fowq_output_offset_and_size(id, 12, owq);
}

int FS_r_ID(struct one_wire_query *owq)
{
	int sn_index, id_index;
	ASCII id[12];
	struct parsedname *pn = PN(owq);
	for (sn_index = 6, id_index = 0; sn_index > 0; --sn_index, id_index += 2)
		num2string(&id[id_index], pn->sn[sn_index]);
	return Fowq_output_offset_and_size(id, 12, owq);
}

int FS_crc8(struct one_wire_query *owq)
{
	ASCII crc[2];
	struct parsedname *pn = PN(owq);
	num2string(crc, pn->sn[7]);
	return Fowq_output_offset_and_size(crc, 2, owq);
}

int FS_address(struct one_wire_query *owq)
{
	ASCII ad[16];
	struct parsedname *pn = PN(owq);
	bytes2string(ad, pn->sn, 8);
	return Fowq_output_offset_and_size(ad, 16, owq);
}

int FS_r_address(struct one_wire_query *owq)
{
	int sn_index, ad_index;
	ASCII ad[16];
	struct parsedname *pn = PN(owq);
	for (sn_index = 7, ad_index = 0; sn_index >= 0; --sn_index, ad_index += 2)
		num2string(&ad[ad_index], pn->sn[sn_index]);
	return Fowq_output_offset_and_size(ad, 16, owq);
}
