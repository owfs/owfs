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
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"
#include "ow_external.h"

static enum search_status External_next_both(struct device_search *ds, const struct parsedname *pn);
static void External_setroutines(struct connection_in *in);
static enum search_status External_directory(struct device_search *ds,const struct parsedname *pn);

#define HA5MUTEX_INIT(in)		_MUTEX_INIT(in->master.ha5.all_channel_lock)
#define HA5MUTEX_LOCK(in)		_MUTEX_LOCK(in->master.ha5.all_channel_lock ) ;
#define HA5MUTEX_UNLOCK(in)		_MUTEX_UNLOCK(in->master.ha5.all_channel_lock);
#define HA5MUTEX_DESTROY(in)	_MUTEX_DESTROY(in->master.ha5.all_channel_lock);

#define CR_char		(0x0D)

static void External_setroutines(struct connection_in *in)
{
	in->iroutines.detect = External_detect;
	in->iroutines.reset = NO_RESET_ROUTINE ;
	in->iroutines.next_both = External_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE ;
	in->iroutines.sendback_bits = HNO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE ;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = NO_CLOSE_ROUTINE;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2404delay | ADAP_FLAG_presence_from_dirblob ;
	in->bundling_length = 1;
}

GOOD_OR_BAD External_detect(struct connection_in *in)
{
	External_setroutines(in);
	return gbGood ;
}

static enum search_status External_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		enum search_status ret ;
		ret = HA5_directory(ds, pn) ;

		if ( ret != search_good ) {
			return search_error;
		}
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, &(ds->gulp) ) ) {
		case 0:
			LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));
			return search_good;
		case -ENODEV:
		default:
			ds->LastDevice = 1;
			LEVEL_DEBUG("SN finished");
			return search_done;
	}
}

/************************************************************************/
/*	HA5_directory: searches the Directory stores it in a dirblob	     */
/*			& stores in in a dirblob object depending if it              */
/*			Supports conditional searches of the bus for 	             */
/*			/alarm branch					                             */
/*                                                                       */
/* Only called for the first element, everything else comes from dirblob */
/* returns 0 even if no elements, errors only on communication errors    */
/************************************************************************/
struct {
	struct dirblob * db ;
} global_externaldir_struct;

static void External_dir_action(const void *nodep, const VISIT which, const int depth)
{
	const struct sensor_node *p = *(struct sensor_node * const *) nodep;
	(void) depth;

	switch (which) {
	case leaf:
	case postorder:
		DirblobAdd( &(p->name), global_externaldir_struct.cb );
	case preorder:
	case endorder:
		break;
	}
}

static enum search_status External_directory(struct device_search *ds, const struct parsedname *pn)
{
	unsigned char resp[20];
	struct connection_in * in = pn->selected_connection ;

	EXTERNALCOUNTLOCK;
	global_externaldir_struct.cb = (struct charblob *) &(ds->gulp) ;
	DirblobClear(global_externaldir_struct.db );
	twalk( sensor_tree, External_dir_action);
	EXTERNALCOUNTUNLOCK;

	return search_good ;
}
