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

static GOOD_OR_BAD Turnoff(const struct parsedname *pn);
static GOOD_OR_BAD BUS_select_subbranch(const struct ds2409_hubs *bp, const struct parsedname *pn);
static GOOD_OR_BAD BUS_Skip_Rom(const struct parsedname *pn);
static GOOD_OR_BAD BUS_select_opening(const struct parsedname *pn) ;
static GOOD_OR_BAD BUS_select_closing(const struct parsedname *pn) ;

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

GOOD_OR_BAD BUS_select(const struct parsedname *pn)
{
	// match Serial Number command 0x55
	BYTE sent[9] = { _1W_MATCH_ROM, };
	int ds2409_depth = pn->ds2409_depth;
	struct connection_in * in = pn->selected_connection ;

	if ( BusIsServer(in) ) {
		LEVEL_DEBUG("Calling local select on remote bus for <%s>",SAFESTRING(pn->path) ) ;
		return gbBAD ;
	}

	if (Globals.one_device) {
		return BUS_Skip_Rom(pn);
	}
	if (!RootNotBranch(pn) && !AdapterSupports2409(pn)) {
		LEVEL_CALL("Attempt to use a branched path (DS2409 main or aux) when bus master doesn't support it.");
		return gbBAD;		/* cannot do branching with eg. LINK ascii */
	}

	LEVEL_DEBUG("Selecting a path (and device) path=%s SN=" SNformat
				" last path=" SNformat, SAFESTRING(pn->path), SNvar(pn->sn), SNvar(in->branch.sn));
	/* Adapter-specific select routine? */
	if ( in->iroutines.select != NO_SELECT_ROUTINE ) {
		LEVEL_DEBUG("Use adapter-specific select routine");
		return (in->iroutines.select) (pn);
	}

	/* Very messy, we may need to clear all the DS2409 couplers up the the current branch */
	if (RootNotBranch(pn)) {	/* no branches, overdrive possible */
		if (in->branch.branch != eBranch_bad ) {	// need clear root branch */
			LEVEL_DEBUG("Clearing root branch");
			BUS_select_closing(pn) ;
		} else {
			LEVEL_DEBUG("Continuing root branch");
			//BUS_reselect_branch(pn) ;
		}
		in->branch.branch = eBranch_bad;	// flag as no branches turned on
		if (in->overdrive) {	// overdrive?
			sent[0] = _1W_OVERDRIVE_MATCH_ROM;
		}
	} else {
		if ( (memcmp(in->branch.sn, pn->bp[ds2409_depth - 1].sn, SERIAL_NUMBER_SIZE) != 0)
			|| ( in->branch.branch != pn->bp[ds2409_depth - 1].branch) )
		{
			/* different path */
			LEVEL_DEBUG("Clearing all branches to level %d", ds2409_depth);
			BUS_select_closing(pn) ;
		} else {
			LEVEL_DEBUG("Reselecting branch at level %d", ds2409_depth);
			//BUS_reselect_branch(pn) ;
		}
		memcpy(in->branch.sn, pn->bp[ds2409_depth - 1].sn, SERIAL_NUMBER_SIZE);
		in->branch.branch = pn->bp[ds2409_depth - 1].branch;
	}

	if ( BAD( BUS_select_opening(pn) ) ) {
		in->branch.branch = eBranch_bad ;
		return gbBAD ;
	}

	/* proper path now "turned on" */
	if ((pn->selected_device != NO_DEVICE) && (pn->selected_device != DeviceThermostat)) {
		memcpy(&sent[1], pn->sn, SERIAL_NUMBER_SIZE);
		if ( BAD(BUS_send_data(sent, 1+SERIAL_NUMBER_SIZE, pn))) {
			STAT_ADD1_BUS(e_bus_select_errors, in);
			LEVEL_CONNECT("Select error for %s on bus %s", pn->selected_device->readable_name, DEVICENAME(in));
			return gbBAD;
		}
	}
	return gbGOOD;
}

static GOOD_OR_BAD BUS_Skip_Rom(const struct parsedname *pn)
{
	BYTE skip[1];
	struct transaction_log t[] = {
		TRXN_WRITE1(skip),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn) ) ) ;

	skip[0] = pn->selected_connection->overdrive ? _1W_OVERDRIVE_SKIP_ROM : _1W_SKIP_ROM;
	return BUS_transaction_nolock(t, pn);
}

/* All the railroad switches need to be opened in order */
static GOOD_OR_BAD BUS_select_opening(const struct parsedname *pn)
{
	int level ;

	RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn) ) ) ;

	for ( level=0 ; level<(int)pn->ds2409_depth ; ++level ) {
		RETURN_BAD_IF_BAD( BUS_select_subbranch(&(pn->bp[level]), pn) ) ;
	}
	return gbGOOD ;
}

/* All need to be closed at every level */
static GOOD_OR_BAD BUS_select_closing(const struct parsedname *pn)
{
	int turnoff_level ;
	
	// step through turning off levels
	for ( turnoff_level=0 ; turnoff_level<=(int)pn->ds2409_depth ; ++turnoff_level ) {
		int level ;
		RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn) ) ) ;
		RETURN_BAD_IF_BAD(Turnoff(pn) ) ;
		for ( level=0 ; level<turnoff_level ; ++level ) {
			RETURN_BAD_IF_BAD( BUS_select_subbranch(&(pn->bp[level]), pn) ) ;
		}
	}
	return gbGOOD ;
}

/* Select the specific branch */
static GOOD_OR_BAD BUS_select_subbranch(const struct ds2409_hubs *bp, const struct parsedname *pn)
{
	BYTE sent[11] = { _1W_MATCH_ROM, };
	BYTE resp[2];
	struct transaction_log t[] = {
		TRXN_WRITE(sent, 11),
		TRXN_READ2(resp),
		TRXN_END,
	};

	// bp->branch has an index into the branch (main=0, aux=1) set in Parsename from the DS2409 file data field.

	memcpy(&sent[1], bp->sn, SERIAL_NUMBER_SIZE);
	//We'll test the range here for safety
	switch( bp->branch ) {
		case eBranch_main:
			sent[SERIAL_NUMBER_SIZE+1] = _1W_SMART_ON_MAIN;
			break ;
		case eBranch_aux:
			sent[SERIAL_NUMBER_SIZE+1] = _1W_SMART_ON_AUX;
			break;
		default:
			LEVEL_DEBUG("Calling illegal branch path");
			return gbBAD ;
	}
	sent[SERIAL_NUMBER_SIZE+2] = 0xFF;

	// Response to "Smart-ON" commands:
	// first byte reset status (which we ignore)
	// second byte, echo of smart-on command (which we test)
	LEVEL_DEBUG("Selecting subbranch " SNformat, SNvar(bp->sn));
	if ( BAD(BUS_transaction_nolock(t, pn)) || (resp[1] != sent[SERIAL_NUMBER_SIZE+1])) {
		STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
		LEVEL_CONNECT("Select subbranch error on bus %s", DEVICENAME(pn->selected_connection));
		return gbBAD;
	}
	//printf("subbranch stop\n");
	return gbGOOD;
}

/* find every DS2409 (family code 1F) and switch off, at this depth */
static GOOD_OR_BAD Turnoff(const struct parsedname *pn)
{
	BYTE sent[2] = { _1W_SKIP_ROM, _1W_ALL_LINES_OFF, };
	struct transaction_log t[] = {
		TRXN_WRITE2(sent),
		TRXN_END,
	};
	//printf("Attempting turnoff\n");
	return BUS_transaction_nolock(t, pn);
}
