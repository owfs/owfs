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

static int Turnoff(const struct parsedname *pn);
#if 0
static int BUS_reselect_branch(const struct parsedname *pn) ;
#endif
static int BUS_select_subbranch(const struct buspath *bp, const struct parsedname *pn);
static int BUS_Skip_Rom(const struct parsedname *pn);
static int BUS_select_opening(const struct parsedname *pn) ;
static int BUS_select_closing(const struct parsedname *pn) ;

/* DS2409 commands */
#define _1W_STATUS_READ_WRITE  0x5A
#define _1W_ALL_LINES_OFF      0x66
#define _1W_DISCHARGE_LINES    0x99
#define _1W_DIRECT_ON_MAIN     0xA5
#define _1W_SMART_ON_MAIN      0xCC
#define _1W_SMART_ON_AUX       0x33


//--------------------------------------------------------------------------
/** Select
   -- selects a 1-wire device to respond to further commands.
   First resets, then climbs down the branching tree,
    finally 'selects' the device.
   If no device is listed in the parsedname structure,
    only the reset and branching is done. This allows selective listing.
   Return 0=good, else
    reset, send_data, sendback_data
 */

/* Now you might wonder, why the low in BUS_select_low?
   There is a vague thought that higher level selection -- specifically
   for the DS9490 with it's intrinsic path commands might be implemented.
   Obviously not yet.
   Well, you asked
*/

int BUS_select(const struct parsedname *pn)
{
	int ret;
	// match Serial Number command 0x55
	BYTE sent[9] = { _1W_MATCH_ROM, };
	int pl = pn->pathlength;

	if (Globals.one_device) {
		return BUS_Skip_Rom(pn);
	}

	if (!RootNotBranch(pn) && !AdapterSupports2409(pn)) {
		LEVEL_CALL("Attempt to use a branched path (DS2409 main or aux) when bus master doesn't support it.");
		return -ENOTSUP;		/* cannot do branching with eg. LINK ascii */
	}
	/* Adapter-specific select routine? */
	if ( FunctionExists(pn->selected_connection->iroutines.select) ) {
		return (pn->selected_connection->iroutines.select) (pn);
	}

	LEVEL_DEBUG("Selecting a path (and device) path=%s SN=" SNformat
				" last path=" SNformat, SAFESTRING(pn->path), SNvar(pn->sn), SNvar(pn->selected_connection->branch.sn));

	/* Very messy, we may need to clear all the DS2409 couplers up the the current branch */
	if (RootNotBranch(pn)) {	/* no branches, overdrive possible */
		if (pn->selected_connection->branch.sn[0]) {	// need clear root branch */
			LEVEL_DEBUG("Clearing root branch");
			BUS_select_closing(pn) ;
		} else {
			LEVEL_DEBUG("Continuing root branch");
			//BUS_reselect_branch(pn) ;
		}
		pn->selected_connection->branch.sn[0] = 0x00;	// flag as no branches turned on
		if (pn->selected_connection->speed == bus_speed_overdrive) {	// overdrive?
			sent[0] = _1W_OVERDRIVE_MATCH_ROM;
		}
	} else {
		if ( (memcmp(pn->selected_connection->branch.sn, pn->bp[pl - 1].sn, SERIAL_NUMBER_SIZE) != 0)
			|| ( pn->selected_connection->branch.branch != pn->bp[pl - 1].branch) )
		{
			/* different path */
			LEVEL_DEBUG("Clearing all branches to level %d", pn->pathlength);
			BUS_select_closing(pn) ;
		} else {
			LEVEL_DEBUG("Reselecting branch at level %d", pn->pathlength);
			//BUS_reselect_branch(pn) ;
		}
		memcpy(pn->selected_connection->branch.sn, pn->bp[pl - 1].sn, SERIAL_NUMBER_SIZE);
		pn->selected_connection->branch.branch = pn->bp[pl - 1].branch;
	}

	if ( BUS_select_opening(pn) ) {
		pn->selected_connection->branch.sn[0] = BUSPATH_BAD ;
		return -EIO ;
	}
	
	/* proper path now "turned on" */
	if ((pn->selected_device != NULL)
		&& (pn->selected_device != DeviceThermostat)) {
		//printf("Really select %s\n",pn->selected_device->code);
		memcpy(&sent[1], pn->sn, SERIAL_NUMBER_SIZE);
		if ((ret = BUS_send_data(sent, 1, pn))) {
			STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
			LEVEL_CONNECT("Select error for %s on bus %s", pn->selected_device->readable_name, pn->selected_connection->name);
			return ret;
		}
		if ((ret = BUS_send_data(&sent[1], SERIAL_NUMBER_SIZE, pn))) {
			STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
			LEVEL_CONNECT("Select error for %s on bus %s", pn->selected_device->readable_name, pn->selected_connection->name);
			return ret;
		}
		return ret;
	}
	return 0;
}

static int BUS_Skip_Rom(const struct parsedname *pn)
{
	BYTE skip[1];
	struct transaction_log t[] = {
		TRXN_WRITE1(skip),
		TRXN_END,
	};

	if ((BUS_reset(pn))) {
		return 1;
	}
	skip[0] = (pn->selected_connection->speed == bus_speed_overdrive) ? _1W_OVERDRIVE_SKIP_ROM : _1W_SKIP_ROM;
	return BUS_transaction_nolock(t, pn);
}

/* All the railroad switches need to be opened in order */
static int BUS_select_opening(const struct parsedname *pn)
{
	int level ;

	if ((BUS_reset(pn))) {
		return 1;
	}
	for ( level=0 ; level<(int)pn->pathlength ; ++level ) {
		if ( BUS_select_subbranch(&(pn->bp[level]), pn) ) {
			return 1 ;
		}
	}
	return 0 ;
}

/* All need to be closed at every level */
static int BUS_select_closing(const struct parsedname *pn)
{
	int turnoff_level ;
	
	// step through turning off levels
	for ( turnoff_level=0 ; turnoff_level<=(int)pn->pathlength ; ++turnoff_level ) {
		int level ;
		if ((BUS_reset(pn))) {
			return 1;
		}
		if ( Turnoff(pn) ) {
			return 1 ;
		}
		for ( level=0 ; level<turnoff_level ; ++level ) {
			if ( BUS_select_subbranch(&(pn->bp[level]), pn) ) {
				return 1 ;
			}
		}
	}
	return 0 ;
}

#if 0
static int BUS_reselect_branch(const struct parsedname *pn)
{
	if ((BUS_reset(pn))) {
		return 1;
	}
	if ( pn->pathlength != 0 && BUS_select_subbranch(&(pn->bp[pn->pathlength-1]), pn) ) {
		return 1 ;
	}
	return 0 ;
}
#endif

/* Select the specific branch */
static int BUS_select_subbranch(const struct buspath *bp, const struct parsedname *pn)
{
	BYTE sent[11] = { _1W_MATCH_ROM, };
	BYTE branch[2] = { _1W_SMART_ON_MAIN, _1W_SMART_ON_AUX, };	/* Main, Aux */
	BYTE resp[2];
	struct transaction_log t[] = {
		TRXN_WRITE(sent, 11),
		TRXN_READ2(resp),
		TRXN_END,
	};

	memcpy(&sent[1], bp->sn, SERIAL_NUMBER_SIZE);
	sent[SERIAL_NUMBER_SIZE+1] = branch[bp->branch];
	sent[SERIAL_NUMBER_SIZE+2] = 0xFF;
	LEVEL_DEBUG("Selecting subbranch " SNformat, SNvar(bp->sn));
	if (BUS_transaction_nolock(t, pn) || (resp[1] != branch[bp->branch])) {
		STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
		LEVEL_CONNECT("Select subbranch error on bus %s", pn->selected_connection->name);
		return 1;
	}
	//printf("subbranch stop\n");
	return 0;
}

/* find every DS2409 (family code 1F) and switch off, at this depth */
static int Turnoff(const struct parsedname *pn)
{
	BYTE sent[2] = { _1W_SKIP_ROM, _1W_ALL_LINES_OFF, };
	struct transaction_log t[] = {
		TRXN_WRITE2(sent),
		TRXN_END,
	};
	//printf("Attempting turnoff\n");
	return BUS_transaction_nolock(t, pn);
}
