/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
static GOOD_OR_BAD BUS_select_branch(const struct ds2409_hubs *bp, const struct parsedname *pn);
static GOOD_OR_BAD BUS_Skip_Rom(const struct parsedname *pn);
static GOOD_OR_BAD BUS_select_branched_path(const struct parsedname *pn) ;
static GOOD_OR_BAD BUS_select_device(BYTE select_byte, const struct parsedname *pn);
static GOOD_OR_BAD BUS_clear_this_path(const struct parsedname *pn) ;

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
	BYTE select_byte = _1W_MATCH_ROM ;
	int ds2409_depth = pn->ds2409_depth;
	struct connection_in * in = pn->selected_connection ;

	// Select only applicable to local bus -- remote selects for themselves
	if ( BusIsServer(in) ) {
		LEVEL_DEBUG("Calling local select on remote bus for <%s>",SAFESTRING(pn->path) ) ;
		return gbBAD ;
	}

	// Single slave device -- use faster routines
	if (Globals.one_device) {
		return BUS_Skip_Rom(pn);
	}

	// Check generic bus-master support for branching
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
		if (in->branch.branch != eBranch_cleared ) {	// need clear root branch */
			LEVEL_DEBUG("Clearing root branch");
			RETURN_BAD_IF_BAD( BUS_clear_this_path(pn) ) ;
		} else {
			LEVEL_DEBUG("Continuing root branch");
		}

		if (in->overdrive) {	// overdrive?
			select_byte = _1W_OVERDRIVE_MATCH_ROM;
		}
	} else { // a branch requested
		if ( (memcmp(in->branch.sn, pn->bp[ds2409_depth - 1].sn, SERIAL_NUMBER_SIZE) != 0)
			|| ( in->branch.branch != pn->bp[ds2409_depth - 1].branch) )
		{
			/* different path */
			LEVEL_DEBUG("Clearing all branches to level %d", ds2409_depth);
			BUS_clear_this_path(pn) ;

			// Load the branch into the "last branch" space to ease addressing next time
			memcpy(in->branch.sn, pn->bp[ds2409_depth - 1].sn, SERIAL_NUMBER_SIZE);
			in->branch.branch = pn->bp[ds2409_depth - 1].branch;
		} else {
			LEVEL_DEBUG("Reselecting branch at level %d", ds2409_depth);
		}
	}

	// Everything cleared and ready
	if ( BAD( BUS_select_branched_path(pn) ) ) {
		// Mark this branch as needing a reset
		in->branch.branch = eBranch_bad ;
		return gbBAD ;
	}

	/* proper path now "turned on" */
	if ((pn->selected_device != NO_DEVICE) && (pn->selected_device != DeviceThermostat)) {
		// select a particular slave as well
		RETURN_BAD_IF_BAD( BUS_select_device( select_byte, pn ) ) ;
	}

	return gbGOOD;
}

static GOOD_OR_BAD BUS_Skip_Rom(const struct parsedname *pn)
{
	BYTE skip[1];
	struct transaction_log t[] = {
		TRXN_RESET,
		TRXN_WRITE1(skip),
		TRXN_END,
	};

	skip[0] = pn->selected_connection->overdrive ? _1W_OVERDRIVE_SKIP_ROM : _1W_SKIP_ROM;
	return BUS_transaction_nolock(t, pn);
}

/* All the railroad switches need to be opened in order */
static GOOD_OR_BAD BUS_select_branched_path(const struct parsedname *pn)
{
	int level ;
	int branch_depth = pn->ds2409_depth ;

	// initial reset
	RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn) ) ) ;
	for ( level=0 ; level<branch_depth ; ++level ) {
		LEVEL_DEBUG("XXXX select bp path %d of %d =" SNformat, level, pn->ds2409_depth, SNvar((pn->bp[level].sn))) ;
		RETURN_BAD_IF_BAD( BUS_select_branch(&(pn->bp[level]), pn) ) ;
	}
	return gbGOOD ;
}

/* All DS2409's need to be closed at every level */
static GOOD_OR_BAD BUS_clear_this_path(const struct parsedname *pn)
{
	int turnoff_level ;
	int branch_depth = pn->ds2409_depth ;
	
	// step through turning off levels
	for ( turnoff_level=0 ; turnoff_level<=branch_depth ; ++turnoff_level ) {
		int level ; // for second level selection
		RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn) ) ) ; // reset to start
		for ( level=0 ; level<turnoff_level ; ++level ) {
			RETURN_BAD_IF_BAD( BUS_select_branch(&(pn->bp[level]), pn) ) ;
		}
		RETURN_BAD_IF_BAD( Turnoff(pn) ) ; // does subbranch turn off
	}
	pn->selected_connection->branch.branch = eBranch_cleared;	// flag as branches cleared
	return gbGOOD ;
}

/* Select the specific branch */
/* Already reset has been called */
static GOOD_OR_BAD BUS_select_branch(const struct ds2409_hubs *bp, const struct parsedname *pn)
{
	BYTE sent[1+SERIAL_NUMBER_SIZE+2] = { _1W_MATCH_ROM, };
	BYTE resp[2];
	struct transaction_log t[] = {
		TRXN_WRITE(sent, 1+SERIAL_NUMBER_SIZE+2),
		TRXN_READ2(resp),
		TRXN_COMPARE(&sent[1+SERIAL_NUMBER_SIZE],&resp[1],1),
		TRXN_END,
	};

	// bp->branch has an index into the branch (main=0, aux=1) set in Parsename from the DS2409 file data field.

	memcpy(&sent[1], bp->sn, SERIAL_NUMBER_SIZE);
	//We'll test the range here for safety
	switch( bp->branch ) {
		case eBranch_main:
			sent[1+SERIAL_NUMBER_SIZE] = _1W_SMART_ON_MAIN;
			break ;
		case eBranch_aux:
			sent[1+SERIAL_NUMBER_SIZE] = _1W_SMART_ON_AUX;
			break;
		default:
			LEVEL_DEBUG("Calling illegal branch path");
			return gbBAD ;
	}
	sent[1+SERIAL_NUMBER_SIZE+1] = 0xFF; // "Reset stimulus"

	// Response to "Smart-ON" commands:
	// first byte reset status (which we ignore)
	// second byte, echo of smart-on command (which we test)
	LEVEL_DEBUG("Selecting subbranch " SNformat, SNvar(bp->sn));
	if ( BAD(BUS_transaction_nolock(t, pn)) ) {
		STAT_ADD1_BUS(e_bus_select_errors, pn->selected_connection);
		LEVEL_CONNECT("Select subbranch error on bus %s", DEVICENAME(pn->selected_connection));
		return gbBAD;
	}

	return gbGOOD;
}

/* Select the specific branch */
/* Already reset has been called */
static GOOD_OR_BAD BUS_select_device(BYTE select_byte, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	BYTE sent[1+SERIAL_NUMBER_SIZE] ;
	struct transaction_log t[] = {
		TRXN_WRITE(sent, 1+SERIAL_NUMBER_SIZE),
		TRXN_END,
	};

	sent[0] = select_byte ;
	memcpy(&sent[1], pn->sn, SERIAL_NUMBER_SIZE);

	LEVEL_DEBUG("Selecting device " SNformat, SNvar(pn->sn));
	if ( BAD(BUS_transaction_nolock(t, pn)) ) {
		STAT_ADD1_BUS(e_bus_select_errors, in);
		LEVEL_CONNECT("Select error for %s on bus %s", pn->selected_device->readable_name, DEVICENAME(in));
		return gbBAD;
	}
	return gbGOOD;
}

/* find every DS2409 (family code 1F) and switch off, at this depth */
static GOOD_OR_BAD Turnoff(const struct parsedname *pn)
{
	BYTE sent[2] = { _1W_SKIP_ROM, _1W_ALL_LINES_OFF, };
	BYTE get[1] ;
	struct transaction_log t[] = {
		TRXN_WRITE2(sent),
		TRXN_READ1(get),
		TRXN_COMPARE(&get[0],&sent[1],1),
		TRXN_END,
	};

	return BUS_transaction_nolock(t, pn);
}
