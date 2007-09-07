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
	if (len > 16) {
		int dlen = len - (len >> 1);
		if ((ret = BUS_send_data(data, dlen, pn)))
			return ret;
		ret = BUS_send_data(&data[dlen], len >> 1, pn);
	} else {
		BYTE resp[16];
		if ((ret = BUS_sendback_data(data, resp, len, pn)) == 0) {
			if ((ret = memcmp(data, resp, (size_t) len))) {
				ret = -EIO;		/* EPROTO not available for MacOSX */
				STAT_ADD1_BUS(BUS_echo_errors, pn->in);
			}
		}
	}
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
	if (pn->in->iroutines.sendback_data) {
		return (pn->in->iroutines.sendback_data) (data, resp, len, pn);
	} else {
		UINT i, bits = len << 3;
		int ret;
		int remain = len - (UART_FIFO_SIZE >> 3);

		/* Split into smaller packets? */
		if (remain > 0)
			return BUS_sendback_data(data, resp, UART_FIFO_SIZE >> 3, pn)
				|| BUS_sendback_data(&data[UART_FIFO_SIZE >> 3],
									 resp ? (&resp[UART_FIFO_SIZE >> 3]) :
									 NULL, remain, pn);

		/* Encode bits */
		for (i = 0; i < bits; ++i)
			pn->in->combuffer[i] = UT_getbit(data, i) ? 0xFF : 0x00;

		/* Communication with DS9097 routine */
		if ((ret =
			 BUS_sendback_bits(pn->in->combuffer, pn->in->combuffer, bits,
							   pn))) {
			STAT_ADD1_BUS(BUS_byte_errors, pn->in);
			return ret;
		}

		/* Decode Bits */
		if (resp) {
			for (i = 0; i < bits; ++i)
				UT_setbit(resp, i, pn->in->combuffer[i] & 0x01);
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
	if (pn->in->iroutines.PowerByte) {
		return (pn->in->iroutines.PowerByte) (data, resp, delay, pn);
	} else {
		int ret;
		// send the packet
		if ((ret = BUS_sendback_data(&data, resp, 1, pn))) {
			STAT_ADD1_BUS(BUS_PowerByte_errors, pn->in);
			return ret;
		}
		// delay
		UT_delay(delay);

		return ret;
	}
}

// RESET called with bus locked
int BUS_reset(const struct parsedname *pn)
{
	int ret = (pn->in->iroutines.reset) (pn);
	/* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
	if (ret == BUS_RESET_OK ) {
		pn->in->reconnect_state = reconnect_ok; // Flag as good!
	} else if (ret == BUS_RESET_SHORT) {
		pn->in->AnyDevices = 0;
		STAT_ADD1_BUS(BUS_short_errors, pn->in);
		LEVEL_CONNECT("1-wire bus short circuit.\n");
		return 1;
	} else {
		pn->in->reconnect_state++;	// Flag for eventual reconnection
		STAT_ADD1_BUS(BUS_reset_errors, pn->in);
	}
	return ret;
}
