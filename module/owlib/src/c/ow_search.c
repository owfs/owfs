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

static void BUS_first_both(struct device_search *ds);
static enum search_status BUS_next_3try(struct device_search *ds, const struct parsedname *pn) ;

//--------------------------------------------------------------------------
/** The 'owFirst' doesn't find the first device on the 1-Wire Net.
 instead, it sets up for BUS_next interator.

 serialnumber -- 8byte (64 bit) serial number found

 Returns:   0-device found 1-no dev or error
*/
enum search_status BUS_first(struct device_search *ds, const struct parsedname *pn)
{
	LEVEL_DEBUG("Start of directory path=%s device=" SNformat, SAFESTRING(pn->path), SNvar(pn->sn));
	// reset the search state
	BUS_first_both(ds);
	ds->search = _1W_SEARCH_ROM;
	return BUS_next(ds, pn);
}

enum search_status BUS_first_alarm(struct device_search *ds, const struct parsedname *pn)
{
	LEVEL_DEBUG("Start of directory path=%s device=" SNformat, SAFESTRING(pn->path), SNvar(pn->sn));
	// reset the search state
	BUS_first_both(ds);
	ds->search = _1W_CONDITIONAL_SEARCH_ROM;
	return BUS_next(ds, pn);
}

static void BUS_first_both(struct device_search *ds)
{
	// reset the search state
	memset(ds->sn, 0, 8);		// clear the serial number
	ds->LastDiscrepancy = -1;
	ds->LastDevice = 0;
	ds->index = -1;				// true place in dirblob
}

//--------------------------------------------------------------------------
/** The BUS_next function does a general search.  This function
 continues from the previous search state (held in struct device_search). The search state
 can be reset by using the BUS_first function.

 Returns:  0=No problems, 1=Problems

 Sets LastDevice=1 if no more
*/
enum search_status BUS_next(struct device_search *ds, const struct parsedname *pn)
{
	switch ( BUS_next_3try(ds, pn) ) {
		case search_good:
			// found a device in a directory search, add to "presence" cache
			LEVEL_DEBUG("Device found: " SNformat, SNvar(ds->sn));
			Cache_Add_Device(pn->selected_connection->index,ds->sn) ;
			return search_good ;
		case search_done:
			return search_done;
		case search_error:
		default:
			return search_error;
	}
}

/* try the directory search 3 times.
 * Since ds->LastDescrepancy is alertered only on success a repeat is legal
 * */
static enum search_status BUS_next_3try(struct device_search *ds, const struct parsedname *pn)
{
	switch (BUS_next_both(ds, pn) ) {
		case search_good:
			return search_good ;
		case search_done:
			return search_done;
		case search_error:
			break ;
	}
	STAT_ADD1_BUS(e_bus_search_errors1, pn->selected_connection);
	
	switch (BUS_next_both(ds, pn) ) {
		case search_good:
			return search_good ;
		case search_done:
			return search_done;
		case search_error:
			break ;
	}
	STAT_ADD1_BUS(e_bus_search_errors2, pn->selected_connection);

	switch (BUS_next_both(ds, pn) ) {
		case search_good:
			return search_good ;
		case search_done:
			return search_done;
		case search_error:
			break ;
	}
	STAT_ADD1_BUS(e_bus_search_errors3, pn->selected_connection);

	return search_error ;
}

/* Call either master-specific search routine, or the bit-banging one */
enum search_status BUS_next_both(struct device_search *ds, const struct parsedname *pn)
{
	enum search_status next_both ;
	if ( pn->selected_connection->iroutines.next_both != NO_NEXT_BOTH_ROUTINE ) {
		next_both = (pn->selected_connection->iroutines.next_both) (ds, pn);
	} else {
		next_both = BUS_next_both_bitbang( ds, pn ) ;
	}
	switch ( next_both ) {
		case search_good:
			if ((ds->sn[0] & 0x7F) == 0x04) {
				/* We found a DS1994/DS2404 which requires longer delays */
				pn->selected_connection->ds2404_found = 1;
			}
			break ;
		default :
			break ;
	}
	return next_both ;
}

/* Low level search routines -- bit banging */
/* Not used by more advanced adapters */
enum search_status BUS_next_both_bitbang(struct device_search *ds, const struct parsedname *pn)
{
	if ( BAD( BUS_select(pn) ) ) {
		return search_error ;
	} else {
		int search_direction = 0;	/* initialization just to forestall incorrect compiler warning */
		int bit_number;
		int last_zero = -1;
		BYTE bits[3];
		
		// initialize for search
		// if the last call was not the last one
		if (ds->LastDevice) {
			return search_done;
		}
		
		/* Appropriate search command */
		if ( BAD( BUS_send_data(&(ds->search), 1, pn)) ) {
			return search_error ;
		}

		// Need data from a reset for AnyDevices -- obtained from BUS_data_send above
		if (pn->selected_connection->AnyDevices == anydevices_no) {
			ds->LastDevice = 1;
			return search_done;
		}

		// loop to do the search
		for (bit_number = 0;; ++bit_number) {
			bits[1] = bits[2] = 0xFF;
			if (bit_number == 0) {	/* First bit */
				/* get two bits (AND'ed bit and AND'ed complement) */
				if ( BAD( BUS_sendback_bits(&bits[1], &bits[1], 2, pn) ) ) {
					return search_error;
				}
			} else {
				bits[0] = search_direction;
				if (bit_number < 64) {
					/* Send chosen bit path, then check match on next two */
					if ( BAD( BUS_sendback_bits(bits, bits, 3, pn) ) ) {
						return search_error;
					}
				} else {		/* last bit */
					if ( BAD( BUS_sendback_bits(bits, bits, 1, pn) ) ) {
						return search_error;
					}
					break;
				}
			}
			if (bits[1]) {
				if (bits[2]) {	/* 1,1 */
					/* No devices respond */
					ds->LastDevice = 1;
					return search_done;
				} else {		/* 1,0 */
					search_direction = 1;	// bit write value for search
				}
			} else if (bits[2]) {	/* 0,1 */
				search_direction = 0;	// bit write value for search
			} else if (bit_number > ds->LastDiscrepancy) {	/* 0,0 looking for last discrepancy in this new branch */
				// Past branch, select zeros for now
				search_direction = 0;
				last_zero = bit_number;
			} else if (bit_number == ds->LastDiscrepancy) {	/* 0,0 -- new branch */
				// at branch (again), select 1 this time
				search_direction = 1;	// if equal to last pick 1, if not then pick 0
			} else if (UT_getbit(ds->sn, bit_number)) {	/* 0,0 -- old news, use previous "1" bit */
				// this discrepancy is before the Last Discrepancy
				search_direction = 1;
			} else {			/* 0,0 -- old news, use previous "0" bit */
				// this discrepancy is before the Last Discrepancy
				search_direction = 0;
				last_zero = bit_number;
			}
			UT_setbit(ds->sn, bit_number, search_direction);
			
		}	// loop until through serial number bits
		
		if ( (CRC8(ds->sn, SERIAL_NUMBER_SIZE)!=0) || (bit_number < 64) || (ds->sn[0] == 0)) {
			/* A minor "error" */
			return search_error;
		}
		// if the search was successful then
		
		ds->LastDiscrepancy = last_zero;
		//    printf("Post, lastdiscrep=%d\n",si->LastDiscrepancy) ;
		ds->LastDevice = (last_zero < 0);
		return search_good;
	}
}
