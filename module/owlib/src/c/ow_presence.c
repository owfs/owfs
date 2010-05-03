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
#include "ow_standard.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Prototypes ------------ */
static INDEX_OR_ERROR CheckPresence_low(struct parsedname *pn);
static INDEX_OR_ERROR CheckThisConnection(int bus_nr, struct parsedname *pn) ;

/* ------- Functions ------------ */

/* Check if device exists -- >=0 yes, -1 no */
INDEX_OR_ERROR CheckPresence(struct parsedname *pn)
{
	INDEX_OR_ERROR bus_nr;
	
	if (NotRealDir(pn)) {
		return INDEX_DEFAULT;
	}
	
	if ((pn->selected_device == DeviceSimultaneous)
		|| (pn->selected_device == DeviceThermostat)) {
		return INDEX_DEFAULT;
	}
	
	/* If set, already found bus. */
	/* Use UnsetKnownBus to clear and allow a new search */
	if (KnownBus(pn)) {
		return pn->known_bus->index;
	}
	
	if (Cache_Get_Device(&bus_nr, pn) == 0) {
		LEVEL_DEBUG("Found device on bus %d",bus_nr);
		SetKnownBus(bus_nr, pn);
		return bus_nr;
	}
	
	LEVEL_DETAIL("Checking presence of %s", SAFESTRING(pn->path));
	
	if ( Inbound_Control.active == 0 ) { // No adapters
		return INDEX_BAD ;
	}
	bus_nr = CheckPresence_low(pn);	// check only allocated inbound connections
	if ( INDEX_VALID(bus_nr) ) {
		SetKnownBus(bus_nr, pn);
		return bus_nr;
	}
	UnsetKnownBus(pn);
	return INDEX_BAD;
}

/* See if a cached location is accurate -- called with "Known Bus" set */
INDEX_OR_ERROR ReCheckPresence(struct parsedname *pn)
{
	INDEX_OR_ERROR bus_nr;
	
	if (NotRealDir(pn)) {
		return INDEX_DEFAULT;
	}
	
	if ((pn->selected_device == DeviceSimultaneous)
		|| (pn->selected_device == DeviceThermostat)) {
		return INDEX_DEFAULT;
	}
	
	if (KnownBus(pn)) {
		if ( INDEX_VALID( CheckThisConnection(pn->known_bus->index,pn) ) ) {
			return pn->known_bus->index ;
		}
	}
	
	if (Cache_Get_Device(&bus_nr, pn) == 0) {
		LEVEL_DEBUG("Found device on bus %d",bus_nr);
		if ( INDEX_VALID( CheckThisConnection(bus_nr,pn) ) ) {
			SetKnownBus(bus_nr, pn);
			return bus_nr ;
		}
	}
	
	UnsetKnownBus(pn);
	Cache_Del_Device(pn) ;
	return CheckPresence(pn);
}

/* Check if device exists -- -1 no, >=0 yes (bus number) */
/* lower level, cycle through the devices */
#if OW_MT

struct checkpresence_struct {
	struct connection_in *in;
	struct parsedname *pn;
	INDEX_OR_ERROR bus_nr;
};

static void * CheckPresence_callback(void * v)
{
	pthread_t thread;
	int threadbad = 1;
	struct checkpresence_struct * cps = (struct checkpresence_struct *) v ;
	struct checkpresence_struct next_cps = { cps->in->next, cps->pn, INDEX_BAD };
	
	threadbad = (next_cps.in == NULL)
	|| pthread_create(&thread, NULL, CheckPresence_callback, (void *) (&next_cps));
	
	cps->bus_nr = CheckThisConnection( cps->in->index, cps->pn ) ;
	
	if (threadbad == 0) {		/* was a thread created? */
		void *vv;
		if (pthread_join(thread, &vv)==0) {
			if ( INDEX_VALID(next_cps.bus_nr) ) {
				cps->bus_nr = next_cps.bus_nr ;
			}
		}
	}
	return NULL ;
}

static INDEX_OR_ERROR CheckPresence_low(struct parsedname *pn)
{
	struct checkpresence_struct cps = { Inbound_Control.head , pn, INDEX_BAD };
		
	if ( cps.in != NULL ) {
		CheckPresence_callback( (void *) (&cps) ) ;
	}
	return cps.bus_nr;
}

#else							/* OW_MT */

static INDEX_OR_ERROR CheckPresence_low(struct parsedname *pn)
{
	struct connection_in * in ;
	
	for ( in=Inbound_Control.head ; in ; in=in->next ) {
		int bus_nr = CheckThisConnection( in->index, pn ) ;
		if ( INDEX_VALID(bus_nr) ) {
			return bus_nr ;
		}
	}
	return -ENOENT;				// no success
}
#endif							/* OW_MT */

ZERO_OR_ERROR FS_present(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	if (NotRealDir(pn) || pn->selected_device == DeviceSimultaneous || pn->selected_device == DeviceThermostat) {
		OWQ_Y(owq) = 1;
	} else if ( pn->selected_connection->iroutines.flags & ADAP_FLAG_presence_from_dirblob ) {
		OWQ_Y(owq) = (DirblobSearch(pn->sn, &(pn->selected_connection->main)) >= 0);
	} else {
		struct transaction_log t[] = {
			TRXN_NVERIFY,
			TRXN_END,
		};
		OWQ_Y(owq) = BAD(BUS_transaction(t, pn)) ? 0 : 1;
	}
	return 0;
}

static INDEX_OR_ERROR CheckThisConnection(int bus_nr, struct parsedname *pn)
{
	struct parsedname s_pn_copy;
	struct parsedname * pn_copy = &s_pn_copy ;
	struct connection_in * in = find_connection_in(bus_nr) ;

	if ( in == NULL ) {
		return INDEX_BAD ;
	}
	
	memcpy(pn_copy, pn, sizeof(struct parsedname));	// shallow copy
	pn_copy->selected_connection = in;
	
	if (TestConnection(pn_copy)) {	// reconnect successful?
		return INDEX_BAD;
	} else if (BusIsServer(in)) {
		if (ServerPresence(pn_copy) >= 0) {
			/* Device was found on this in-device, return it's index */
			LEVEL_DEBUG("Presence found on server bus %s",SAFESTRING(in->name)) ;
			Cache_Add_Device(in->index,pn_copy->sn) ; // add or update cache */
			return in->index;
		}
	} else if ( in->iroutines.flags & ADAP_FLAG_presence_from_dirblob ) {
		if ( DirblobSearch(pn_copy->sn, &(in->main)) >= 0 ) {
			LEVEL_DEBUG("Presence found on fake-like bus %s",SAFESTRING(in->name)) ;
			return in->index;
		}
	} else {
		struct transaction_log t[] = {
			TRXN_NVERIFY,
			TRXN_END,
		};
		/* this can only be done on local buses */
		if ( GOOD( BUS_transaction(t, pn_copy) ) ) {
			/* Device was found on this in-device, return it's index */
			LEVEL_DEBUG("Presence found on local bus %s\n",SAFESTRING(in->name)) ;
			Cache_Add_Device(in->index,pn_copy->sn) ; // add or update cache */
			return in->index ;
		}
	}
	LEVEL_DEBUG("Presence NOT found on bus %s",SAFESTRING(in->name)) ;
	return INDEX_BAD ;
}

