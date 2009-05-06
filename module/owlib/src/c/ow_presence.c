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
static int CheckPresence_low(struct connection_in *in, const struct parsedname *pn);

/* ------- Functions ------------ */

/* Check if device exists -- >=0 yes, -1 no */
int CheckPresence(struct parsedname *pn)
{
	int bus_nr;

	if (NotRealDir(pn)) {
		return 0;
	}

	if ((pn->selected_device == DeviceSimultaneous)
		|| (pn->selected_device == DeviceThermostat)) {
		return 0;
	}

	/* If set, already found bus. */
	/* Use UnsetKnownBus to clear and allow a new search */
	if (KnownBus(pn)) {
		return pn->known_bus->index;
	}

	if (Cache_Get_Device(&bus_nr, pn) == 0) {
		LEVEL_DEBUG("Found device on bus %d\n",bus_nr);
		SetKnownBus(bus_nr, pn);
		return bus_nr;
	}

	LEVEL_DETAIL("Checking presence of %s\n", SAFESTRING(pn->path));

	if ( Inbound_Control.active == 0 ) { // No adapters
		return -1 ;
	}
	bus_nr = CheckPresence_low(Inbound_Control.head, pn);	// check only allocated inbound connections
	if (bus_nr >= 0) {
		SetKnownBus(bus_nr, pn);
		Cache_Add_Device(bus_nr, pn->sn);
		return bus_nr;
	}
	UnsetKnownBus(pn);
	return -1;
}

/* Check if device exists -- -1 no, >=0 yes (bus number) */
/* lower level, cycle through the devices */
#if OW_MT

struct checkpresence_struct {
	struct connection_in *in;
	const struct parsedname *pn;
	int ret;
};

static void *CheckPresence_callback(void *v)
{
	struct checkpresence_struct *cps = (struct checkpresence_struct *) v;
	cps->ret = CheckPresence_low(cps->in, cps->pn);
	LEVEL_DEBUG("<%s> Normal exit\n",SAFESTRING(cps->in->name));
	pthread_exit(NULL);
	return NULL;
}

static int CheckPresence_low(struct connection_in *in, const struct parsedname *pn)
{
	int ret = 0;
	pthread_t thread;
	int threadbad = 1;
	struct parsedname pn2;
	struct checkpresence_struct cps = { in->next, pn, 0 };

	threadbad = (in->next == NULL)
		|| pthread_create(&thread, NULL, CheckPresence_callback, (void *) (&cps));

	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
	pn2.selected_connection = in;

	//printf("CheckPresence_low:\n");
	if (TestConnection(&pn2)) {	// reconnect successful?
		ret = -ECONNABORTED;
	} else if (BusIsServer(in)) {
		//printf("CheckPresence_low: call ServerPresence\n");
		if (ServerPresence(&pn2) >= 0) {
			/* Device was found on this in-device, return it's index */
			ret = in->index;
		} else {
			ret = -1;
		}
		//printf("CheckPresence_low: ServerPresence(%s) pn->selected_connection->index=%d ret=%d\n", pn->path, pn->selected_connection->index, ret);
	} else if ( (get_busmode(in) == bus_fake) || (get_busmode(in) == bus_tester) || (get_busmode(in) == bus_mock) ) {
		ret = (DirblobSearch(pn2.sn, &(in->main)) < 0) ? -1 : in->index;
	} else {
		struct transaction_log t[] = {
			TRXN_NVERIFY,
			TRXN_END,
		};
		/* this can only be done on local buses */
		if (BUS_transaction(t, &pn2)) {
			ret = -1;
		} else {
			/* Device was found on this in-device, return it's index */
			ret = in->index;
		}
	}
	if (threadbad == 0) {		/* was a thread created? */
		void *v;
		if (pthread_join(thread, &v)) {
			return ret;			/* wait for it (or return only this result) */
		}
		if (cps.ret >= 0) {
			return cps.in->index;
		}
	}
	//printf("Presence return = %d\n",ret) ;
	return ret;
}

#else							/* OW_MT */

static int CheckPresence_low(struct connection_in *in, const struct parsedname *pn)
{
	struct parsedname pn2;

	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
	pn2.selected_connection = in;

	if (TestConnection(&pn2)) {	// reconnect successful?
		return -ECONNABORTED;
	} else if (BusIsServer(in)) {
		if (ServerPresence(&pn2) >= 0) {
			/* Device was found on this in-device, return it's index */
			return in->index;
		}
	} else if ( (get_busmode(in) == bus_fake) || (get_busmode(in) == bus_tester) || (get_busmode(in) == bus_mock) ) {
		int ret = DirblobSearch(pn2.sn, &(in->main));
		if (ret >= 0) {
			return ret;
		}
	} else {
		struct transaction_log t[] = {
			TRXN_NVERIFY,
			TRXN_END,
		};
		/* this can only be done on local buses */
		if (BUS_transaction(t, &pn2) == 0) {
			/* Device was found on this in-device, return it's index */
			return in->index;
		}
	}

	/* recurse to next bus number */
	if (in->next) {
		return CheckPresence_low(in->next, pn);
	}
	return -ENOENT;				// no success
}
#endif							/* OW_MT */

int FS_present(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	if (NotRealDir(pn) || pn->selected_device == DeviceSimultaneous || pn->selected_device == DeviceThermostat) {
		OWQ_Y(owq) = 1;
	} else if (get_busmode(pn->selected_connection) == bus_fake) {
		OWQ_Y(owq) = 1;
	} else if (get_busmode(pn->selected_connection) == bus_tester) {
		OWQ_Y(owq) = 1;
	} else if (get_busmode(pn->selected_connection) == bus_mock) {
		OWQ_Y(owq) = 1;
	} else {
		struct transaction_log t[] = {
			TRXN_NVERIFY,
			TRXN_END,
		};
		OWQ_Y(owq) = BUS_transaction(t, pn) ? 0 : 1;
	}
	return 0;
}
