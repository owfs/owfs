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

/** BUS_send_data
    Send a data and expect response match
    puts into data mode if needed.
    return 0=good
    bad return sendback_data
    -EIO if memcpy
 */
int BUS_send_data(const BYTE * data, const size_t len,
				  const struct parsedname *pn)
{
	int ret;
    BYTE * resp ;
    
    if ( len == 0 ) return 0 ;

    resp = malloc(len) ;
    if ( resp==NULL) return -ENOMEM ;
    
    if ((ret = BUS_sendback_data(data, resp, len, pn)) == 0) {
        if ((ret = memcmp(data, resp, (size_t) len))) {
            ret = -EIO;		/* EPROTO not available for MacOSX */
            STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
        }
    }
    free(resp) ;
	return ret;
}

/** readin_data
  Send 0xFFs and return response block
  Returns 0=good
  Bad sendback_data
 */
int BUS_readin_data(BYTE * data, const size_t len,
					const struct parsedname *pn)
{
	int ret =
		BUS_sendback_data(memset(data, 0xFF, (size_t) len), data, len, pn);
	if (ret) {
		STAT_ADD1(BUS_readin_data_errors);
	}
	return ret;
}

// ----------------------------------------------------------------
// Low level default routines, often superceded by more capable adapters

/* Symmetric */
/* send bytes, and read back -- calls lower level bit routine */
int BUS_sendback_data(const BYTE * data, BYTE * resp, const size_t len,
					  const struct parsedname *pn)
{
	if (pn->selected_connection->iroutines.sendback_data) {
		return (pn->selected_connection->iroutines.sendback_data) (data, resp, len, pn);
	} else {
		UINT i, bits = len << 3;
		int ret;
        int combuffer_length_adjusted = MAX_FIFO_SIZE >> 3 ;
        int remain = len - combuffer_length_adjusted ;

		/* Split into smaller packets? */
		if (remain > 0)
            return BUS_sendback_data(data, resp, combuffer_length_adjusted, pn)
                    || BUS_sendback_data(&data[combuffer_length_adjusted],
									 resp ? (&resp[combuffer_length_adjusted]) : NULL,
                                     remain, pn);

		/* Encode bits */
		for (i = 0; i < bits; ++i)
			pn->selected_connection->combuffer[i] = UT_getbit(data, i) ? 0xFF : 0x00;

		/* Communication with DS9097 routine */
		if ((ret =
			 BUS_sendback_bits(pn->selected_connection->combuffer, pn->selected_connection->combuffer, bits,
							   pn))) {
			STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
			return ret;
		}

		/* Decode Bits */
		if (resp) {
			for (i = 0; i < bits; ++i)
				UT_setbit(resp, i, pn->selected_connection->combuffer[i] & 0x01);
		}

		return 0;
	}
}

// Send 8 bits of communication to the 1-Wire Net
// Delay delay msec and return to normal
//
// Power is supplied by normal pull-up resistor, nothing special
//
/* Returns 0=good
   bad = -EIO
 */
int BUS_PowerByte(BYTE data, BYTE * resp, UINT delay,
				  const struct parsedname *pn)
{
	int ret ;

	if (pn->selected_connection->iroutines.PowerByte) {
		ret = (pn->selected_connection->iroutines.PowerByte) (data, resp, delay, pn);
	} else {
		// send the packet
		ret = BUS_sendback_data(&data, resp, 1, pn) ;
		// delay
		UT_delay(delay);
	}
	if ( ret ) {
		STAT_ADD1_BUS( e_bus_pullup_errors, pn->selected_connection ) ;
	}
	return ret;
}

int BUS_ProgramPulse( const struct parsedname *pn )
{
	int ret ;

	if (pn->selected_connection->iroutines.ProgramPulse) {
		ret = (pn->selected_connection->iroutines.ProgramPulse)(pn);
	} else {
		ret = -ENOTSUP ;
	}
	if ( ret ) {
		STAT_ADD1_BUS( e_bus_program_errors, pn->selected_connection ) ;
	}
	return ret;
}

// RESET called with bus locked
int BUS_reset(const struct parsedname *pn)
{
	int ret = (pn->selected_connection->iroutines.reset) (pn);
	/* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
	if (ret == BUS_RESET_OK ) {
		pn->selected_connection->reconnect_state = reconnect_ok; // Flag as good!
	} else if (ret == BUS_RESET_SHORT) {
		pn->selected_connection->AnyDevices = 0;
		STAT_ADD1_BUS(e_bus_short_errors, pn->selected_connection);
		LEVEL_CONNECT("1-wire bus short circuit.\n");
		return 1;
	} else {
		pn->selected_connection->reconnect_state++;	// Flag for eventual reconnection
		STAT_ADD1_BUS(e_bus_reset_errors, pn->selected_connection);
	}
	STAT_ADD1_BUS(e_bus_resets, pn->selected_connection);

	return ret;
}
