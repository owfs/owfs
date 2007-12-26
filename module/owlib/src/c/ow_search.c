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

static void BUS_first_both(struct device_search *ds) ;

//--------------------------------------------------------------------------
/** The 'owFirst' doesn't find the first device on the 1-Wire Net.
 instead, it sets up for BUS_next interator.

 serialnumber -- 8byte (64 bit) serial number found

 Returns:   0-device found 1-no dev or error
*/
int BUS_first(struct device_search *ds, const struct parsedname *pn)
{
	// reset the search state
	LEVEL_DEBUG("Start of directory path=%s device=" SNformat "\n",
				SAFESTRING(pn->path), SNvar(pn->sn));
	BUS_first_both(ds) ;
	pn->selected_connection->ExtraReset = 0;
	ds->search = _1W_SEARCH_ROM;

	if (!pn->selected_connection->AnyDevices) {
		LEVEL_DATA("BUS_first: Empty bus -- no presence pulse\n");
	}

	return BUS_next(ds, pn);
}

int BUS_first_alarm(struct device_search *ds, const struct parsedname *pn)
{
	// reset the search state
	BUS_first_both(ds) ;
	ds->search = _1W_CONDITIONAL_SEARCH_ROM;
	//BUS_reset(pn);
	return BUS_next(ds, pn);
}

static void BUS_first_both(struct device_search *ds)
{
    // reset the search state
    memset(ds->sn, 0, 8);       // clear the serial number
    ds->LastDiscrepancy = -1;
    ds->LastDevice = 0;
    ds->index = -1 ; // true place in dirblob
}

//--------------------------------------------------------------------------
/** The BUS_next function does a general search.  This function
 continues from the previous search state (held in struct device_search). The search state
 can be reset by using the BUS_first function.

 Returns:  0=No problems, 1=Problems

 Sets LastDevice=1 if no more
*/
int BUS_next(struct device_search *ds, const struct parsedname *pn)
{
	int ret;

	if ( !RootNotBranch(pn) // branch directory
		|| (pn->selected_connection->iroutines.flags & ADAP_FLAG_dir_auto_reset)==0 // needs this flag
	) {
		if (BUS_select(pn)) {
			return 1;
		}
	}
	
	ret = BUS_next_both(ds, pn);
	LEVEL_DEBUG("BUS_next return = %d " SNformat "\n", ret, SNvar(ds->sn));
	if (ret && ret != -ENODEV) { // true error
		STAT_ADD1_BUS(e_bus_search_errors, pn->selected_connection);
	}
	return ret;
}

/* Low level search routines -- bit banging */
/* Not used by more advanced adapters */
int BUS_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (pn->selected_connection->iroutines.next_both) {
		return (pn->selected_connection->iroutines.next_both) (ds, pn);
	} else {
		int search_direction = 0;	/* initialization just to forestall incorrect compiler warning */
		int bit_number;
		int last_zero = -1;
		BYTE bits[3];
		int ret;

		// initialize for search
		// if the last call was not the last one
		if (!pn->selected_connection->AnyDevices)
			ds->LastDevice = 1;
		if (ds->LastDevice)
			return -ENODEV;

		/* Appropriate search command */
		if ((ret = BUS_send_data(&(ds->search), 1, pn)))
			return ret;
		// loop to do the search
		for (bit_number = 0;; ++bit_number) {
			bits[1] = bits[2] = 0xFF;
			if (bit_number == 0) {	/* First bit */
				/* get two bits (AND'ed bit and AND'ed complement) */
				if ((ret = BUS_sendback_bits(&bits[1], &bits[1], 2, pn)))
					return ret;
			} else {
				bits[0] = search_direction;
				if (bit_number < 64) {
					/* Send chosen bit path, then check match on next two */
					if ((ret = BUS_sendback_bits(bits, bits, 3, pn)))
						return ret;
				} else {		/* last bit */
					if ((ret = BUS_sendback_bits(bits, bits, 1, pn)))
						return ret;
					break;
				}
			}
			if (bits[1]) {
				if (bits[2]) {	/* 1,1 */
					/* No devices respond */
					ds->LastDevice = 1;
					return -ENODEV;
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
			// check for Last discrepancy in family
			//if (last_zero < 9) si->LastFamilyDiscrepancy = last_zero;
			UT_setbit(ds->sn, bit_number, search_direction);

			// serial number search direction write bit
			//if ( (ret=DS9097_sendback_bits(&search_direction,bits,1)) ) return ret ;
		}						// loop until through serial number bits

		if (CRC8(ds->sn, 8) || (bit_number < 64) || (ds->sn[0] == 0)) {
			/* A minor "error" */
			return -EIO;
		}
		if ((ds->sn[0] & 0x7F) == 0x04) {
			/* We found a DS1994/DS2404 which require longer delays */
			pn->selected_connection->ds2404_compliance = 1;
		}
		// if the search was successful then

		ds->LastDiscrepancy = last_zero;
		//    printf("Post, lastdiscrep=%d\n",si->LastDiscrepancy) ;
		ds->LastDevice = (last_zero < 0);
		LEVEL_DEBUG("Generic_next_both SN found: " SNformat "\n",
					SNvar(ds->sn));
		return 0;
	}
}
