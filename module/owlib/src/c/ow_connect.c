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
#include "ow_connection.h"

/* Routines for handling a linked list of connections in and out */
/* typical connection in would be gthe serial port or USB */

/* Globals */
struct connection_out *head_outbound_list = NULL;
int count_outbound_connections = 0;
struct connection_in *head_inbound_list = NULL;
int count_inbound_connections = 0;

struct connection_in *find_connection_in(int bus_number)
{
	struct connection_in *c_in;
	// step through head_inbound_list linked list
	for (c_in = head_inbound_list; c_in != NULL; c_in = c_in->next) {
		if (c_in->index == bus_number)
			return c_in;
	}
	return NULL;
}

enum bus_mode get_busmode(struct connection_in *in)
{
	if (in == NULL)
		return bus_unknown;
	return in->busmode;
}

int BusIsServer(struct connection_in *in)
{
	return (in->busmode == bus_server) || (in->busmode == bus_zero);
}

/* Make a new head_inbound_list, and place it in the chain */
/* Based on a shallow copy of "in" if not NULL */
struct connection_in *NewIn(const struct connection_in *in)
{
	size_t len = sizeof(struct connection_in);
	struct connection_in *now = (struct connection_in *) malloc(len);
	if (now) {
		if (in) {
			memcpy(now, in, len);
		} else {
			memset(now, 0, len);
		}
		now->next = head_inbound_list;	/* put in linked list at start */
		head_inbound_list = now;
		now->index = count_inbound_connections++;
#if OW_MT
		pthread_mutex_init(&(now->bus_mutex), pmattr);
		pthread_mutex_init(&(now->dev_mutex), pmattr);
		now->dev_db = NULL;
#endif							/* OW_MT */
		/* Support DS1994/DS2404 which require longer delays, and is automatically
		 * turned on in *_next_both().
		 * If it's turned off, it will result into a faster reset-sequence.
		 */
		now->ds2404_compliance = 0;
		/* Flag first pass as need to clear all branches if DS2409 present */
		now->buspath_bad = 1;
		/* Arbitrary guess at root directory size for allocating cache blob */
		now->last_root_devs = 10;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for adapter structure,\n");
	}
	return now;
}

struct connection_out *NewOut(void)
{
	size_t len = sizeof(struct connection_out);
	struct connection_out *now = (struct connection_out *) malloc(len);
	if (now) {
		memset(now, 0, len);
		now->next = head_outbound_list;
		head_outbound_list = now;
		now->index = count_outbound_connections++;
#if OW_MT
		pthread_mutex_init(&(now->accept_mutex), pmattr);
		pthread_mutex_init(&(now->out_mutex), pmattr);
#endif							/* OW_MT */
		// Zero sref's -- done with struct memset
		//now->sref0 = 0 ;
		//now->sref1 = 0 ;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for server structure,\n");
	}
	return now;
}

void FreeIn(void)
{
	struct connection_in *next = head_inbound_list;
	struct connection_in *now;

	head_inbound_list = NULL;
	count_inbound_connections = 0;
	while (next) {
		now = next;
		next = now->next;
		LEVEL_DEBUG("FreeIn: busmode=%d\n", get_busmode(now));
#if OW_MT
		pthread_mutex_destroy(&(now->bus_mutex));
		pthread_mutex_destroy(&(now->dev_mutex));
		if (now->dev_db)
			tdestroy(now->dev_db, free);
		now->dev_db = NULL;
#endif							/* OW_MT */
		switch (get_busmode(now)) {
		case bus_zero:
			if (now->connin.tcp.type)
				free(now->connin.tcp.type);
			if (now->connin.tcp.domain)
				free(now->connin.tcp.domain);
			if (now->connin.tcp.fqdn)
				free(now->connin.tcp.fqdn);
			// fall through
		case bus_server:
			LEVEL_DEBUG("FreeClientAddr\n");
			FreeClientAddr(now);
			break;
		case bus_link:
		case bus_serial:
		case bus_passive:
			COM_close(now);
			break;
		case bus_usb:
#if OW_USB
			DS9490_close(now);
			now->name = NULL;	// rather than a static data string;
#endif
			break;
		case bus_elink:
		case bus_ha7net:
		case bus_etherweather:
			BUS_close(now);
			break;
		case bus_fake:
			DirblobClear(&(now->connin.fake.db));
			break;
		case bus_tester:
			DirblobClear(&(now->connin.tester.db));
			break;
		case bus_i2c:
#if OW_MT
			if (now->connin.i2c.index == 0) {
				pthread_mutex_destroy(&(now->connin.i2c.i2c_mutex));
			}
#endif							/* OW_MT */
		default:
			break;
		}
		if (now->name) {
			free(now->name);
			now->name = NULL;
		}
		free(now);
	}
}

void FreeOut(void)
{
	struct connection_out *next = head_outbound_list;
	struct connection_out *now;

	head_outbound_list = NULL;
	count_outbound_connections = 0;
	while (next) {
		now = next;
		next = now->next;
		if (now->name) {
			free(now->name);
			now->name = NULL;
		}
		if (now->host) {
			free(now->host);
			now->host = NULL;
		}
		if (now->service) {
			free(now->service);
			now->service = NULL;
		}
		if (now->ai) {
			freeaddrinfo(now->ai);
			now->ai = NULL;
		}
#if OW_MT
		pthread_mutex_destroy(&(now->accept_mutex));
		pthread_mutex_destroy(&(now->out_mutex));
#endif							/* OW_MT */
#if OW_ZERO
		if (libdnssd != NULL) {
			if (now->sref0)
				DNSServiceRefDeallocate(now->sref0);
			if (now->sref1)
				DNSServiceRefDeallocate(now->sref1);
		}
#endif
		free(now);
	}
}
