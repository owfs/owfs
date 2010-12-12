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

static GOOD_OR_BAD BUS_sendback_data_bitbang(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn) ;

/** BUS_send_data
    Send a data and expect response match
 */
GOOD_OR_BAD BUS_send_data(const BYTE * data, const size_t len, const struct parsedname *pn)
{
	BYTE resp[len];

	if (len == 0) {
		return gbGOOD;
	}

	if ( BAD( BUS_sendback_data(data, resp, len, pn) ) ) {
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
		return gbBAD ;
	}

	if ( memcmp(data, resp, len) != 0 ) {
		LEVEL_DEBUG("Response doesn't match data sent");
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
		return gbBAD ;
	}
	return gbGOOD;
}

/** readin_data
  Send 0xFFs and return response block
 */
GOOD_OR_BAD BUS_readin_data(BYTE * data, const size_t len, const struct parsedname *pn)
{
	memset(data, 0xFF, (size_t) len) ;
	RETURN_GOOD_IF_GOOD( BUS_sendback_data( data, data, len, pn) ) ;

	STAT_ADD1(BUS_readin_data_errors);
	return gbBAD;
}

// ----------------------------------------------------------------
// Low level default routines, often superceded by more capable adapters

/* Symmetric */
/* send bytes, and read back -- calls lower level bit routine */
GOOD_OR_BAD BUS_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	GOOD_OR_BAD (*select_and_sendback) (const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn) = pn->selected_connection->iroutines.select_and_sendback ;
	if ( select_and_sendback != NO_SELECTANDSENDBACK_ROUTINE ) {
		return (select_and_sendback) (data, resp, len, pn);
	} else {
		RETURN_BAD_IF_BAD( BUS_select(pn) );
		return BUS_sendback_data(data, resp, len, pn);
	}
}

/* Symmetric */
/* send bytes, and read back -- calls lower level bit routine */
GOOD_OR_BAD BUS_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	GOOD_OR_BAD (*sendback_data) (const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn) = pn->selected_connection->iroutines.sendback_data ;
	
	/* Empty is ok */
	if (len == 0) {
		return gbGOOD;
	}
	
	/* Native function for this bus master? */
	if ( sendback_data != NO_SENDBACKDATA_ROUTINE ) {
		return (sendback_data) (data, resp, len, pn);
	}

	return BUS_sendback_data_bitbang(data, resp, len, pn);
}

/* Symmetric */
/* send bytes, and read back -- calls lower level bit routine */
static GOOD_OR_BAD BUS_sendback_data_bitbang(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	/* Empty is ok */
	if (len == 0) {
		return gbGOOD;
	} else {
		int max_split_bytes = MAX_FIFO_SIZE / 8 ;
		int remain = len - max_split_bytes;
	
		/* Possibly split into smaller packets? */
		if (remain > 0) {
			RETURN_BAD_IF_BAD( BUS_sendback_data_bitbang(data, resp, max_split_bytes, pn) );
			RETURN_BAD_IF_BAD( BUS_sendback_data_bitbang(&data[max_split_bytes], resp ? (&resp[max_split_bytes]) : NULL, remain, pn) );
		}
	}

	{
		UINT i, bits = len * 8;
		BYTE bit_buffer[ bits ] ;

		/* Encode bits */
		for (i = 0; i < bits; ++i) {
#if OW_DEBUG
			if (Globals.error_level>=e_err_debug) {
				if ( (i&0x7)==0 ) {
					int b = i >>3 ;
					LEVEL_DEBUG("Splitting byte %d of %d = %.2X",b,len,data[b]) ;
				}
			}
#endif /* OW_DEBUG */
			bit_buffer[i] = UT_getbit(data, i) ? 0xFF : 0x00;
		}

		/* Communication with bit-level (e.g. DS9097) routine */
		if ( BAD( BUS_sendback_bits(bit_buffer, bit_buffer, bits, pn)) ) {
			STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
			return gbBAD;
		}

		/* Decode Bits */
		if (resp) {
			for (i = 0; i < bits; ++i) {
				UT_setbit(resp, i, bit_buffer[i] & 0x01);
#if OW_DEBUG
				if (Globals.error_level>=e_err_debug) {
					if ( (i&0x7)==7 ) {
						int b = i >>3 ;
						LEVEL_DEBUG("Consolidating byte %d of %d = %.2X",b,len,resp[b]);
					}
				}
#endif /* OW_DEBUG */
			}
		}
	}

	return gbGOOD;
}
