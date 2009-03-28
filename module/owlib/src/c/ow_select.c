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

static int Turnoff(int depth, const struct parsedname *pn);
static int BUS_select_branch(const struct parsedname *pn);
static int BUS_select_subbranch(const struct buspath *bp, const struct parsedname *pn);
static int BUS_Skip_Rom(const struct parsedname *pn);

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
	//printf("SELECT WORK: pathlength=%d path=%s\n",pn->pathlength,pn->path);
	// if declared only a single device, we can use faster SKIP ROM command
	if (Globals.one_device) {
		return BUS_Skip_Rom(pn);
	}

	if (!RootNotBranch(pn) && AdapterSupports2409(pn)) {
		LEVEL_CALL("Attempt to use a branched path (DS2409 main or aux) when adapter doesn't support it.\n");
		return -ENOTSUP;		/* cannot do branching with LINK ascii */
	}
	/* Adapter-specific select routine? */
	if (pn->selected_connection->iroutines.select) {
		return (pn->selected_connection->iroutines.select) (pn);
	}

	LEVEL_DEBUG("Selecting a path (and device) path=%s SN=" SNformat
				" last path=" SNformat "\n", SAFESTRING(pn->path), SNvar(pn->sn), SNvar(pn->selected_connection->branch.sn));

	/* Very messy, we may need to clear all the DS2409 couplers up the the current branch */
	if (RootNotBranch(pn)) {	/* no branches, overdrive possible */
		//printf("SELECT_LOW root path\n") ;
		if (pn->selected_connection->branch.sn[0] || pn->selected_connection->buspath_bad) {	// need clear root branch */
			//printf("SELECT_LOW root path will be cleared\n") ;
			LEVEL_DEBUG("Clearing root branch\n");
			if (Turnoff(0, pn)) {
				return 1;
			}
			pn->selected_connection->branch.sn[0] = 0x00;	// flag as no branches turned on
		}
		if (pn->selected_connection->speed == bus_speed_overdrive) {	// overdrive?
			sent[0] = _1W_OVERDRIVE_MATCH_ROM;
		}
	} else if (memcmp(pn->selected_connection->branch.sn, pn->bp[pl - 1].sn, 8) || pn->selected_connection->buspath_bad) {	/* different path */
		int iclear;
		LEVEL_DEBUG("Clearing all branches to level %d\n", pl);
		for (iclear = 0; iclear <= pl; ++iclear) {
			// All lines off
			if (Turnoff(iclear, pn)) {
				return 1;
			}
		}
		memcpy(pn->selected_connection->branch.sn, pn->bp[pl - 1].sn, 8);
		pn->selected_connection->branch.branch = pn->bp[pl - 1].branch;
	} else if (pn->selected_connection->branch.branch != pn->bp[pl - 1].branch) {	/* different branch */
		LEVEL_DEBUG("Clearing last branches (level %d)\n", pl);
		if (Turnoff(pl, pn)) {
			return 1;			// clear just last level
		}
		pn->selected_connection->branch.branch = pn->bp[pl - 1].branch;
	}
	pn->selected_connection->buspath_bad = 0;

	/* proper path now "turned on" */
	/* Now select */
	if (BUS_reset(pn) || BUS_select_branch(pn)) {
		return 1;
	}

	if ((pn->selected_device != NULL)
		&& (pn->selected_device != DeviceThermostat)) {
		//printf("Really select %s\n",pn->selected_device->code);
		memcpy(&sent[1], pn->sn, 8);
		if ((ret = BUS_send_data(sent, 1, pn))) {
			STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
			LEVEL_CONNECT("Select error for %s on bus %s\n", pn->selected_device->readable_name, pn->selected_connection->name);
			return ret;
		}
		if ((ret = BUS_send_data(&sent[1], 8, pn))) {
			STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
			LEVEL_CONNECT("Select error for %s on bus %s\n", pn->selected_device->readable_name, pn->selected_connection->name);
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

/* All the railroad switches are correctly set, just isolate the last segment */
static int BUS_select_branch(const struct parsedname *pn)
{
	if (RootNotBranch(pn)) {
		return 0;
	}
	return BUS_select_subbranch(&(pn->bp[pn->pathlength - 1]), pn);
}

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

	memcpy(&sent[1], bp->sn, 8);
	sent[9] = branch[bp->branch];
	sent[10] = 0xFF;
	LEVEL_DEBUG("Selecting subbranch " SNformat "\n", SNvar(bp->sn));
	if (BUS_transaction_nolock(t, pn) || (resp[1] != branch[bp->branch])) {
		STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
		LEVEL_CONNECT("Select subbranch error for %s on bus %s\n", pn->selected_device->readable_name, pn->selected_connection->name);
		return 1;
	}
	//printf("subbranch stop\n");
	return 0;
}

/* find every DS2409 (family code 1F) and switch off, at this depth */
static int Turnoff(int depth, const struct parsedname *pn)
{
	BYTE sent[2] = { _1W_SKIP_ROM, _1W_ALL_LINES_OFF, };
	struct transaction_log t[] = {
		TRXN_WRITE2(sent),
		TRXN_END,
	};

	//printf("TURNOFF entry depth=%d\n",depth) ;

	if ((BUS_reset(pn))) {
		return 1;
	}

	if ((pn->selected_connection->Adapter == adapter_fake)
		|| (pn->selected_connection->Adapter == adapter_tester)) {
		LEVEL_DEBUG("return on fake adapter");
		return 0;
	}

	if (depth && BUS_select_subbranch(&(pn->bp[depth - 1]), pn)) {
		return 1;
	}
	return BUS_transaction_nolock(t, pn);
}
