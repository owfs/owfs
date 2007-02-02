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

static int Turnoff(int depth, const struct parsedname *pn);
static int BUS_selection_error(int ret);
static int BUS_select_branch(const struct parsedname *pn);
static int BUS_select_subbranch(const struct buspath *bp,
								const struct parsedname *pn);

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
	if (pn->pathlength > 0
		&& (pn->in->iroutines.flags & ADAP_FLAG_2409path)) {
		LEVEL_CALL
			("Attempt to use a branched path (DS2409 main or aux) when adapter doesn't support it.\n");
		return -ENOTSUP;		/* cannot do branching with LINK ascii */
	}
	if (pn->in->iroutines.select) {
		return (pn->in->iroutines.select) (pn);
	} else {
		int ret;
		// match Serial Number command 0x55
		BYTE sent[9] = { 0x55, };
		int pl = pn->pathlength;

		LEVEL_DEBUG("Selecting a path (and device) path=%s SN=" SNformat
					" last path=" SNformat "\n", SAFESTRING(pn->path),
					SNvar(pn->sn), SNvar(pn->in->branch.sn));

		/* Very messy, we may need to clear all the DS2409 couplers up the the current branch */
		if (pl == 0) {			/* no branches, overdrive possible */
			//printf("SELECT_LOW root path\n") ;
			if (pn->in->branch.sn[0] || pn->in->buspath_bad) {	// need clear root branch */
				//printf("SELECT_LOW root path will be cleared\n") ;
				LEVEL_DEBUG("Clearing root branch\n");
				if (Turnoff(0, pn))
					return 1;
				pn->in->branch.sn[0] = 0x00;	// flag as no branches turned on
			}
			if (pn->in->use_overdrive_speed) {	// overdrive?
				if ((ret = BUS_testoverdrive(pn)) < 0) {
					BUS_selection_error(ret);
					return ret;
				} else {
					//printf("use overdrive speed\n");
					sent[0] = 0x69;
				}
			}
		} else if (memcmp(pn->in->branch.sn, pn->bp[pl - 1].sn, 8) || pn->in->buspath_bad) {	/* different path */
			int iclear;
			LEVEL_DEBUG("Clearing all branches to level %d\n", pl);
			//printf("SELECT_LOW clear pathes to level %d \n",pl) ;
			for (iclear = 0; iclear <= pl; ++iclear) {
				// All lines off
				if (Turnoff(iclear, pn))
					return 1;
			}
			memcpy(pn->in->branch.sn, pn->bp[pl - 1].sn, 8);
			pn->in->branch.branch = pn->bp[pl - 1].branch;
		} else if (pn->in->branch.branch != pn->bp[pl - 1].branch) {	/* different branch */
			//printf("SELECT_LOW clear path just level %d \n",pl) ;
			LEVEL_DEBUG("Clearing last branches (level %d)\n", pl);
			if (Turnoff(pl, pn))
				return 1;		// clear just last level
			pn->in->branch.branch = pn->bp[pl - 1].branch;
		}
		pn->in->buspath_bad = 0;

		/* proper path now "turned on" */
		/* Now select */
		if (BUS_reset(pn) || BUS_select_branch(pn))
			return 1;

		if (pn->dev && (pn->dev != DeviceThermostat)) {
			//printf("Really select %s\n",pn->dev->code);
			memcpy(&sent[1], pn->sn, 8);
			if ((ret = BUS_send_data(sent, 1, pn))) {
				BUS_selection_error(ret);
				return ret;
			}
			if (sent[0] == 0x69) {
				if ((ret =
					 BUS_overdrive(ONEWIREBUSSPEED_OVERDRIVE, pn)) < 0) {
					BUS_selection_error(ret);
					return ret;
				}
			}
			if ((ret = BUS_send_data(&sent[1], 8, pn))) {
				BUS_selection_error(ret);
				return ret;
			}
			return ret;
		}
		return 0;
	}
}

/* All the railroad switches are correctly set, just isolate the last segment */
static int BUS_select_branch(const struct parsedname *pn)
{
	if (pn->pathlength == 0)
		return 0;
	return BUS_select_subbranch(&(pn->bp[pn->pathlength - 1]), pn);
}

/* Select the specific branch */
static int BUS_select_subbranch(const struct buspath *bp,
								const struct parsedname *pn)
{
	BYTE sent[10] = { 0x55, };
	BYTE branch[2] = { 0xCC, 0x33, };	/* Main, Aux */
	BYTE resp[3];
	struct transaction_log t[] = {
		{sent, NULL, 10, trxn_match,},
		{NULL, resp, 3, trxn_read,},
		TRXN_END,
	};

	memcpy(&sent[1], bp->sn, 8);
	sent[9] = branch[bp->branch];
	//printf("subbranch start\n");
	if (BUS_transaction_nolock(t, pn) || (resp[2] != branch[bp->branch])) {
		STAT_ADD1(BUS_select_low_branch_errors);
		//printf("SELECT error3\n");
		//printf("subbranch error\n");
		return 1;
	}
	//printf("subbranch stop\n");
	return 0;
}

/* find every DS2409 (family code 1F) and switch off, at this depth */
static int Turnoff(int depth, const struct parsedname *pn)
{
	BYTE sent[2] = { 0xCC, 0x66, };
	struct transaction_log t[] = {
		{sent, NULL, 2, trxn_match},
		TRXN_END,
	};

	//printf("TURNOFF entry depth=%d\n",depth) ;

	if ((BUS_reset(pn)))
		return 1;
	if (depth && BUS_select_subbranch(&(pn->bp[depth - 1]), pn))
		return 1;
	return BUS_transaction_nolock(t, pn);
}

static int BUS_selection_error(int ret)
{
	STAT_ADD1(BUS_select_low_errors);
	LEVEL_CONNECT("SELECTION ERROR\n");
	return ret;
}
