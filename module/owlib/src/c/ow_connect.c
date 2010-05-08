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
/* typical connection in would be gtmhe serial port or USB */

/* Globals */
struct inbound_control Inbound_Control = {
	.active = 0,
	.next_index = 0,
	.head = NULL,
	.next_fake = 0,
	.next_tester = 0,
	.next_mock = 0,
#if OW_W1
	.w1_seq = 0,
	.w1_entry_mark = 0,
	.w1_file_descriptor = FILE_DESCRIPTOR_BAD,
	.w1_pid = 0,
#endif /* OW_W1 */
};

struct outbound_control Outbound_Control = {
	.active = 0,
	.next_index = 0,
	.head = NULL,
};

struct connection_in *find_connection_in(int bus_number)
{
	struct connection_in *c_in;
	// step through inbound linked list

	for (c_in = Inbound_Control.head; c_in != NULL; c_in = c_in->next) {
		if (c_in->index == bus_number) {
			return c_in;
		}
	}
	LEVEL_DEBUG("Couldn't find bus number %d",bus_number);
	return NULL;
}

int SetKnownBus( int bus_number, struct parsedname * pn)
{
	struct connection_in * found = find_connection_in( bus_number ) ;
	if ( found==NULL ) {
		return 1 ;
	}
	pn->state |= ePS_bus;
	pn->selected_connection=found ;
	pn->known_bus=found;
	return 0 ;
}

enum bus_mode get_busmode(struct connection_in *in)
{
	if (in == NULL) {
		return bus_unknown;
	}
	return in->busmode;
}

int BusIsServer(struct connection_in *in)
{
	return (in->busmode == bus_server) || (in->busmode == bus_zero);
}

/* Make a new connection_in entry, and place it in the chain */
/* Based on a shallow copy of "in" if not NULL */
struct connection_in *NewIn(const struct connection_in *in)
{
	size_t len = sizeof(struct connection_in);
	struct connection_in *now = (struct connection_in *) owmalloc(len);
	if (now) {
		if (in) {
			memcpy(now, in, len);
		} else {
			memset(now, 0, len);
		}

		// Housekeeping to place in linked list
		// Locking done at a higher level
		now->next = Inbound_Control.head;	/* put in linked list at start */
		Inbound_Control.head = now;
		now->index = Inbound_Control.next_index++;
		++Inbound_Control.active ;

		/* Initialize dir-at-once structures */
		DirblobInit(&(now->main));
		DirblobInit(&(now->alarm));

#if OW_MT
		_MUTEX_INIT(now->bus_mutex);
		_MUTEX_INIT(now->dev_mutex);
		now->dev_db = NULL;
#endif							/* OW_MT */
		/* Support DS1994/DS2404 which require longer delays, and is automatically
		 * turned on in *_next_both().
		 * If it's turned off, it will result into a faster reset-sequence.
		 */
		now->ds2404_compliance = 0;
		/* Flag first pass as need to clear all branches if DS2409 present */
		now->branch.sn[0] = BUSPATH_BAD ;
		/* Arbitrary guess at root directory size for allocating cache blob */
		now->last_root_devs = 10;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for bus master structure,");
	}
	now->AnyDevices = anydevices_unknown ;
	return now;
}

struct connection_out *NewOut(void)
{
	size_t len = sizeof(struct connection_out);
	struct connection_out *now = (struct connection_out *) owmalloc(len);
	if (now) {
		memset(now, 0, len);
		now->next = Outbound_Control.head;
		Outbound_Control.head = now;
		now->index = Outbound_Control.next_index++;
		++Outbound_Control.active ;
#if OW_MT
		_MUTEX_INIT(now->accept_mutex);
		_MUTEX_INIT(now->out_mutex);
		my_pthread_cond_init(&(now->setup_cond), NULL);
#endif							/* OW_MT */
		// Zero sref's -- done with struct memset
		//now->sref0 = 0 ;
		//now->sref1 = 0 ;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for server structure,");
	}
	return now;
}

// Free the important parts of a connection_in structure
// unlinking needs to be done elsewhere
// Then free structure itself
void FreeIn(struct connection_in * now)
{
	if ( now==NULL ) {
		return ;
	}
	//LEVEL_DEBUG("%p next=%p busmode=%d\n", now, now->next, (int)get_busmode(now));
	--Inbound_Control.active ;
#if OW_MT
	_MUTEX_DESTROY(now->bus_mutex);
	_MUTEX_DESTROY(now->dev_mutex);

	if (now->dev_db) {
		tdestroy(now->dev_db, owfree_func);
	}
	now->dev_db = NULL;
#endif							/* OW_MT */
	switch (get_busmode(now)) {
	case bus_link:
	case bus_serial:
	case bus_passive:
		COM_close(now);
		break;
	case bus_usb:
#if OW_USB
		BUS_close(now);
#endif
		break;
	case bus_zero:
	case bus_server:
		if (now->connin.tcp.type) {
			owfree(now->connin.tcp.type);
		}
		if (now->connin.tcp.domain) {
			owfree(now->connin.tcp.domain);
		}
		if (now->connin.tcp.name) {
			owfree(now->connin.tcp.name);
		}
		FreeClientAddr(now);
		break;
	case bus_ha7net:
		BUS_close(now);
		break ;
	case bus_elink:
	case bus_etherweather:
		BUS_close(now);
		break;
	case bus_fake:
		break;
	case bus_tester:
		break;
	case bus_mock:
		break;
	case bus_i2c:
#if OW_MT
		if (now->connin.i2c.index == 0) {
			_MUTEX_DESTROY(now->connin.i2c.i2c_mutex);
		}
#endif							/* OW_MT */
		break ;
	case bus_w1:
#if OW_W1
#if OW_MT
		_MUTEX_DESTROY(Inbound_Control.w1_mutex);
		_MUTEX_DESTROY(Inbound_Control.w1_read_mutex);
#endif							/* OW_MT */
#endif							/* OW_W1 */
		break ;
	case bus_bad:
	default:
		break;
	}
	if (now->name) {
		//LEVEL_DEBUG("[%s]\n", now->name);
		owfree(now->name);
	}
	DirblobClear(&(now->main));
	DirblobClear(&(now->alarm));
	owfree(now);
}

/* Free all connection_in in reverse order (Added to head on creation, head-first deletion) */
void FreeInAll( void )
{
	struct connection_in *next ;

	while ( Inbound_Control.head ) {
		next = Inbound_Control.head->next;
		//LEVEL_DEBUG("%p next=%p", Inbound_Control.head, Inbound_Control.head->next);
		FreeIn(Inbound_Control.head);
		Inbound_Control.head = next;
	}
}

void RemoveIn( struct connection_in * conn )
{
	struct connection_in * now ;

	if ( conn == NULL ) {
		return ;
	}

	if ( Inbound_Control.head == conn ) {
		Inbound_Control.head = conn->next ;
	} else {
		for ( now = Inbound_Control.head ; now != NULL ; now = now->next ) {
			if ( now->next == conn ) {
				now->next = conn->next ;
				break ;
			}
		}
	}

	FreeIn( conn ) ;
}

void FreeOutAll(void)
{
	struct connection_out *next = Outbound_Control.head;
	struct connection_out *now;

	Outbound_Control.head = NULL;
	Outbound_Control.active = 0;
	while (next) {
		now = next;
		next = now->next;
		if (now->zero.name) {
			owfree(now->zero.name);
			now->zero.name = NULL;
		}
		if (now->zero.type) {
			owfree(now->zero.type);
			now->zero.type = NULL;
		}
		if (now->zero.domain) {
			owfree(now->zero.domain);
			now->zero.domain = NULL;
		}
		if (now->name) {
			owfree(now->name);
			now->name = NULL;
		}
		if (now->host) {
			owfree(now->host);
			now->host = NULL;
		}
		if (now->service) {
			owfree(now->service);
			now->service = NULL;
		}
		if (now->ai) {
			freeaddrinfo(now->ai);
			now->ai = NULL;
		}
#if OW_MT
		_MUTEX_DESTROY(now->accept_mutex);
		_MUTEX_DESTROY(now->out_mutex);
		my_pthread_cond_destroy(&(now->setup_cond));
#endif							/* OW_MT */
#if OW_ZERO
		if (libdnssd != NULL) {
			if (now->sref0) {
				DNSServiceRefDeallocate(now->sref0);
			}
			if (now->sref1) {
				DNSServiceRefDeallocate(now->sref1);
			}
		}
#endif
		owfree(now);
	}
}
