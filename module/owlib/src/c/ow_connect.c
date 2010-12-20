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
	.head = NO_CONNECTION,
	.next_fake = 0,
	.next_tester = 0,
	.next_mock = 0,
	.w1_monitor = NO_CONNECTION ,
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

	for (c_in = Inbound_Control.head; c_in != NO_CONNECTION; c_in = c_in->next) {
		if (c_in->index == bus_number) {
			return c_in;
		}
	}
	LEVEL_DEBUG("Couldn't find bus number %d",bus_number);
	return NO_CONNECTION;
}

int SetKnownBus( int bus_number, struct parsedname * pn)
{
	struct connection_in * found = find_connection_in( bus_number ) ;
	if ( found == NO_CONNECTION ) {
		return 1 ;
	}
	pn->state |= ePS_bus;
	pn->selected_connection=found ;
	pn->known_bus=found;
	return 0 ;
}

enum bus_mode get_busmode(struct connection_in *in)
{
	if (in == NO_CONNECTION) {
		return bus_unknown;
	}
	return in->busmode;
}

// return true (non-zero) if the connection_in exists, and is remote
int BusIsServer(struct connection_in *in)
{
	if ( in == NO_CONNECTION ) {
		return 0 ;
	}
	return (in->busmode == bus_server) || (in->busmode == bus_zero);
}

/* Make a new connection_in entry, but DON'T place it in the chain (yet)*/
/* Based on a shallow copy of "in" if not NULL */
struct connection_in *AllocIn(const struct connection_in *old_in)
{
	size_t len = sizeof(struct connection_in);
	struct connection_in *new_in = (struct connection_in *) owmalloc(len);
	if (new_in != NO_CONNECTION) {
		if (old_in == NO_CONNECTION) {
			// Not a copy
			memset(new_in, 0, len);
			SOC(new_in)->file_descriptor = FILE_DESCRIPTOR_BAD ;
			SOC(new_in)->type = ct_unknown ;
			SOC(new_in)->state = cs_virgin ;
			SOC(new_in)->devicename = NULL ;
			/* Not yet a valid bus */
			new_in->iroutines.flags = ADAP_FLAG_sham ;
		} else {
			// Copy of prior bus
			memcpy(new_in, old_in, len);
			if ( SOC(new_in)->devicename != NULL ) {
				// Don't make the name point to the same string, make a copy
				SOC(new_in)->devicename = owstrdup( SOC(old_in)->devicename ) ;
			}
		}

		/* not yet linked */
		new_in->next = NO_CONNECTION ;

		/* Initialize dir-at-once structures */
		DirblobInit(&(new_in->main));
		DirblobInit(&(new_in->alarm));

		/* Support DS1994/DS2404 which require longer delays, and is automatically
		 * turned on in *_next_both().
		 * If it's turned off, it will result into a faster reset-sequence.
		 */
		new_in->ds2404_found = 0;
		/* Flag first pass as need to clear all branches if DS2409 present */
		new_in->branch.sn[0] = BUSPATH_BAD ;
		/* Arbitrary guess at root directory size for allocating cache blob */
		new_in->last_root_devs = 10;
		new_in->AnyDevices = anydevices_unknown ;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for bus master structure");
	}
	return new_in;
}

/* Place a new connection in the chain */
struct connection_in *LinkIn(struct connection_in *now)
{
	if (now) {
		// Housekeeping to place in linked list
		// Locking done at a higher level
		now->next = Inbound_Control.head;	/* put in linked list at start */
		Inbound_Control.head = now;
		now->index = Inbound_Control.next_index++;
		++Inbound_Control.active ;

		/* Initialize dir-at-once structures */
		DirblobInit(&(now->main));
		DirblobInit(&(now->alarm));

		_MUTEX_INIT(now->bus_mutex);
		_MUTEX_INIT(now->dev_mutex);
		now->dev_db = NULL;
	}
	return now;
}

/* Create a new inbound structure and place it in the chain */
struct connection_in *NewIn(const struct connection_in *in) {
	return LinkIn( AllocIn(in) ) ;
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

		// Zero sref's -- done with struct memset
		//now->sref0 = 0 ;
		//now->sref1 = 0 ;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for server structure,");
	}
	return now;
}

/* Free all connection_in in reverse order (Added to head on creation, head-first deletion) */
void FreeInAll( void )
{
	while ( Inbound_Control.head ) {
		RemoveIn(Inbound_Control.head);
	}
}

void RemoveIn( struct connection_in * conn )
{
	/* NULL safe */
	if ( conn == NO_CONNECTION ) {
		return ;
	}

	/* First unlink from list */
	if ( Inbound_Control.head == conn ) {
		/* Head of list, easy */
		Inbound_Control.head = conn->next ;
		--Inbound_Control.active ;
	} else {
		struct connection_in * now ;
		/* find in list and splice out */
		for ( now = Inbound_Control.head ; now != NO_CONNECTION ; now = now->next ) {
			/* Works even if not linked, since won't match and will just fall through */
			if ( now->next == conn ) {
				now->next = conn->next ;
				--Inbound_Control.active ;
				break ;
			}
		}
	}

	/* Now free up thread-sync resources */
	if ( conn->next != NO_CONNECTION ) {
		/* Only if actually linked in and possibly active */
		_MUTEX_DESTROY(conn->bus_mutex);
		_MUTEX_DESTROY(conn->dev_mutex);
		SAFETDESTROY( conn->dev_db, owfree_func);
	}

	/* Close master-specific resources */
	BUS_close(conn) ;

	/* Next free up internal resources */
	COM_close( conn ) ;
	SAFEFREE(SOC(conn)->devicename) ;
	DirblobClear(&(conn->main));
	DirblobClear(&(conn->alarm));
	
	/* Finally delete the structure */
	owfree(conn);
	conn = NO_CONNECTION ;
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
		LEVEL_DEBUG("Freeing outbound %s #%d",now->zero.name,now->index);
		SAFEFREE(now->zero.name) ;
		SAFEFREE(now->zero.type) ;
		SAFEFREE(now->zero.domain) ;
		SAFEFREE(now->name) ;
		SAFEFREE(now->host) ;
		SAFEFREE(now->service) ;
		if (now->ai) {
			freeaddrinfo(now->ai);
			now->ai = NULL;
		}
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
