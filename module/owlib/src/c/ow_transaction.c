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
#include <assert.h>

struct transaction_bundle {
	const struct transaction_log *start;
	int packets;
	size_t max_size;
	struct memblob mb;
	int select_first;
};

// static int BUS_transaction_length( const struct transaction_log * tl, const struct parsedname * pn ) ;
static GOOD_OR_BAD BUS_transaction_single(const struct transaction_log *t, const struct parsedname *pn);

static GOOD_OR_BAD Bundle_pack(const struct transaction_log *tl, const struct parsedname *pn);
static GOOD_OR_BAD Pack_item(const struct transaction_log *tl, struct transaction_bundle *tb);
static GOOD_OR_BAD Bundle_ship(struct transaction_bundle *tb, const struct parsedname *pn);
static GOOD_OR_BAD Bundle_enroute(struct transaction_bundle *tb, const struct parsedname *pn);
static GOOD_OR_BAD Bundle_unpack(struct transaction_bundle *tb);

static void Bundle_init(struct transaction_bundle *tb, const struct parsedname *pn);

#define TRANSACTION_INCREMENT 1000

/* Bus transaction */
/* Encapsulates communication with a device, including locking the bus, reset and selection */
/* Then a series of bytes is sent and returned, including sending data and reading the return data */
GOOD_OR_BAD BUS_transaction(const struct transaction_log *tl, const struct parsedname *pn)
{
	GOOD_OR_BAD ret ;

	if (tl == NULL) {
		return gbGOOD;
	}
	BUSLOCK(pn);
	ret = BUS_transaction_nolock(tl, pn);
	BUSUNLOCK(pn);

	return ret;
}

GOOD_OR_BAD BUS_transaction_nolock(const struct transaction_log *tl, const struct parsedname *pn)
{
	const struct transaction_log *t = tl;
	GOOD_OR_BAD ret = gbGOOD;

	if (pn->selected_connection->iroutines.flags & ADAP_FLAG_bundle) {
		return Bundle_pack(tl, pn);
	}

	do {
		//printf("Transact type=%d\n",t->type) ;
		ret = BUS_transaction_single(t, pn);
		if (ret == gbOTHER) {	// trxn_done flag
			ret = gbGOOD;			// restore no error code
			break;				// but stop looping anyways
		}
		++t;
	} while ( GOOD(ret) );
	return ret;
}

static GOOD_OR_BAD BUS_transaction_single(const struct transaction_log *t, const struct parsedname *pn)
{
	GOOD_OR_BAD ret = gbGOOD;
	switch (t->type) {
	case trxn_select:			// select a 1-wire device (by unique ID)
		ret = BUS_select(pn);
		LEVEL_DEBUG("select = %d", ret);
		break;
	case trxn_compare:			// match two strings -- no actual 1-wire
		if ((t->in == NULL) || (t->out == NULL)
			|| (memcmp(t->in, t->out, t->size) != 0)) {
			ret = gbBAD;
		}
		LEVEL_DEBUG("compare = %d", ret);
		break;
	case trxn_bitcompare:			// match two strings -- no actual 1-wire
		if ((t->in == NULL) || (t->out == NULL)
			|| BAD( BUS_compare_bits( t->in, t->out, t->size)) ) {
			ret = gbBAD;
		}
		LEVEL_DEBUG("compare = %d", ret);
		break;
	case trxn_match:			// send data and compare response
		assert(t->in == NULL);	// else use trxn_compare
		if (t->size == 0) {		/* ignore for both cases */
			break;
		} else {
			ret = BUS_send_data(t->out, t->size, pn);
			LEVEL_DEBUG("send = %d", ret);
		}
		break;
	case trxn_bitmatch:			// send data and compare response
		assert(t->in == NULL);	// else use trxn_compare
		if (t->size == 0) {		/* ignore for both cases */
			break;
		} else {
			ret = BUS_send_bits(t->out, t->size, pn);
			LEVEL_DEBUG("send bits = %d", ret);
		}
		break;
	case trxn_read:
		assert(t->out == NULL);	// pure read
		if (t->size == 0) {		/* ignore for all cases */
			break;
		} else if (t->out == NULL) {
			ret = BUS_readin_data(t->in, t->size, pn);
			LEVEL_DEBUG("readin = %d", ret);
		}
		break;
	case trxn_bitread:
		assert(t->out == NULL);	// pure read
		if (t->size == 0) {		/* ignore for all cases */
			break;
		} else if (t->out == NULL) {
			ret = BUS_readin_bits(t->in, t->size, pn);
			LEVEL_DEBUG("readin bits = %d", ret);
		}
		break;
	case trxn_modify:			// write data and read response. No match needed
		ret = BUS_sendback_data(t->out, t->in, t->size, pn);
		LEVEL_DEBUG("modify = %d", ret);
		break;
	case trxn_bitmodify:			// write data and read response. No match needed
		ret = BUS_sendback_bits(t->out, t->in, t->size, pn);
		LEVEL_DEBUG("bit modify = %d", ret);
		break;
	case trxn_blind:			// write data ignore response
		{
			BYTE *dummy = owmalloc(t->size);
			if (dummy != NULL) {
				ret = BUS_sendback_data(t->out, dummy, t->size, pn);
				owfree(dummy);
			} else {
				ret = gbBAD;
			}
		}
		LEVEL_DEBUG("blind = %d", ret);
		break;
	case trxn_power:
		ret = BUS_PowerByte(t->out[0], t->in, t->size, pn);
		LEVEL_DEBUG("power (%d usec) = %d", t->size, ret);
		break;
	case trxn_bitpower:
		ret = BUS_PowerBit(t->out[0], t->in, t->size, pn);
		LEVEL_DEBUG("power bit (%d usec) = %d", t->size, ret);
		break;
	case trxn_program:
		ret = BUS_ProgramPulse(pn);
		LEVEL_DEBUG("program pulse = %d", ret);
		break;
	case trxn_crc8:
		ret = CRC8(t->out, t->size);
		LEVEL_DEBUG("CRC8 = %d", ret);
		break;
	case trxn_crc8seeded:
		ret = CRC8seeded(t->out, t->size, ((UINT *) (t->in))[0]);
		LEVEL_DEBUG("seeded CRC8 = %d", ret);
		break;
	case trxn_crc16:
		ret = CRC16(t->out, t->size);
		LEVEL_DEBUG("CRC16 = %d", ret);
		break;
	case trxn_crc16seeded:
		ret = CRC16seeded(t->out, t->size, ((UINT *) (t->in))[0]);
		LEVEL_DEBUG("seeded CRC16 = %d", ret);
		break;
	case trxn_delay:
		if (t->size > 0) {
			UT_delay(t->size);
		}
		LEVEL_DEBUG("Delay %d", t->size);
		break;
	case trxn_udelay:
		if (t->size > 0) {
			UT_delay_us(t->size);
		}
		LEVEL_DEBUG("Micro Delay %d", t->size);
		break;
	case trxn_reset:
		ret = BUS_reset(pn)==BUS_RESET_OK ? gbGOOD : gbBAD;
		LEVEL_DEBUG("reset = %d", ret);
		break;
	case trxn_end:
		LEVEL_DEBUG("end = %d", ret);
		return gbOTHER;			// special "end" flag
	case trxn_verify:
		{
			struct parsedname pn2;
			memcpy(&pn2, pn, sizeof(struct parsedname));	//shallow copy
			pn2.selected_device = NO_DEVICE;
			if ( BAD( BUS_select(&pn2) )) {
				ret = gbBAD ;
			} else if ( BAD(BUS_verify(t->size, pn) )) {
				ret = gbBAD ;
			} else {
				ret = gbGOOD;
			}
			LEVEL_DEBUG("verify = %d", ret);
		}
		break;
	case trxn_nop:
		ret = gbGOOD;
		break;
	}
	return ret;
}

// initialize the bundle
static void Bundle_init(struct transaction_bundle *tb, const struct parsedname *pn)
{
	memset(tb, 0, sizeof(struct transaction_bundle));
	MemblobInit(&tb->mb, TRANSACTION_INCREMENT);
	tb->max_size = pn->selected_connection->bundling_length;
}

static GOOD_OR_BAD Bundle_pack(const struct transaction_log *tl, const struct parsedname *pn)
{
	const struct transaction_log *t_index;
	struct transaction_bundle s_tb;
	struct transaction_bundle *tb = &s_tb;

	Bundle_init(tb, pn);

	for (t_index = tl; t_index->type != trxn_end; ++t_index) {
		switch (Pack_item(t_index, tb)) {
		case gbGOOD:
			LEVEL_DEBUG("Item added");
			break;
		case gbBAD:
			LEVEL_DEBUG("Item cannot be bundled");
			RETURN_BAD_IF_BAD(Bundle_ship(tb, pn)) ;
			RETURN_BAD_IF_BAD(BUS_transaction_single(t_index, pn)) ;
			break;
		case gbOTHER:
			LEVEL_DEBUG("Item too big");
			RETURN_BAD_IF_BAD(Bundle_ship(tb, pn)) ;
			if ( GOOD( Pack_item(t_index, tb) ) ) {
				break;
			}
			RETURN_BAD_IF_BAD(BUS_transaction_single(t_index, pn)) ;
			break;
		}
	}
	return Bundle_ship(tb, pn);
}

// Take a bundle, execute the transaction, unpack, and clear the memoblob
static GOOD_OR_BAD Bundle_ship(struct transaction_bundle *tb, const struct parsedname *pn)
{
	LEVEL_DEBUG("Ship Packets=%d", tb->packets);
	if (tb->packets == 0) {
		return gbGOOD;
	}

	if ( BAD( Bundle_enroute(tb, pn) ) ) {
		// clear the bundle
		MemblobClear(&tb->mb);
		tb->packets = 0;
		tb->select_first = 0;
		return gbBAD;
	}

	return Bundle_unpack(tb);
}

// Execute a bundle transaction (actual bytes on 1-wire bus)
static GOOD_OR_BAD Bundle_enroute(struct transaction_bundle *tb, const struct parsedname *pn)
{
	if (tb->select_first) {
		return BUS_select_and_sendback(MemblobData(&(tb->mb)), MemblobData(&(tb->mb)), MemblobLength(&(tb->mb)), pn);
	} else {
		return BUS_sendback_data(MemblobData(&(tb->mb)), MemblobData(&(tb->mb)), MemblobLength(&(tb->mb)), pn);
	}
}

/* See if the item can be packed
   return gbBAD -- cannot be packed at all
   return gbOTHER -- should be at start
   return gbGOOD       -- added successfully
*/
static GOOD_OR_BAD Pack_item(const struct transaction_log *tl, struct transaction_bundle *tb)
{
	GOOD_OR_BAD ret = 0;				//default return value for good packets;
	//printf("PACK_ITEM used=%d size=%d max=%d\n",MemblobLength(&(tl>mb)),tl->size,tb->max_size);
	switch (tl->type) {
	case trxn_select:			// select a 1-wire device (by unique ID)
		LEVEL_DEBUG("pack=SELECT");
		if (tb->packets != 0) {
			return gbOTHER;		// select must be first
		}
		tb->select_first = 1;
		break;
	case trxn_compare:			// match two strings -- no actual 1-wire
	case trxn_bitcompare:			// match two strings -- no actual 1-wire
		LEVEL_DEBUG("pack=COMPARE");
		break;
	case trxn_read:
	case trxn_bitread:
		LEVEL_DEBUG(" pack=READ");
		if (tl->size > tb->max_size) {
			return gbBAD;		// too big for any bundle
		}
		if (tl->size + MemblobLength(&(tb->mb)) > tb->max_size) {
			return gbOTHER;		// too big for this partial bundle
		}
		if (MemblobAddChar(0xFF, tl->size, &tb->mb)) {
			return gbBAD;
		}
		break;
	case trxn_match:			// write data and match response
	case trxn_bitmatch:			// write data and match response
	case trxn_modify:			// write data and read response. No match needed
	case trxn_bitmodify:			// write data and read response. No match needed
	case trxn_blind:			// write data and ignore response
		LEVEL_DEBUG("pack=MATCH MODIFY BLIND");
		if (tl->size > tb->max_size) {
			return gbBAD;		// too big for any bundle
		}
		if (tl->size + MemblobLength(&(tb->mb)) > tb->max_size) {
			return gbOTHER;		// too big for this partial bundle
		}
		if (MemblobAdd(tl->out, tl->size, &tb->mb)) {
			return gbBAD;
		}
		break;
	case trxn_power:
	case trxn_bitpower:
	case trxn_program:
		LEVEL_DEBUG("pack=POWER PROGRAM");
		if (1 > tb->max_size) {
			return gbBAD;		// too big for any bundle
		}
		if (1 + MemblobLength(&(tb->mb)) > tb->max_size) {
			return gbOTHER;		// too big for this partial bundle
		}
		if (MemblobAdd(tl->out, 1, &tb->mb)) {
			return gbBAD;
		}
		ret = gbGOOD;			// needs delay
		break;
	case trxn_crc8:
	case trxn_crc8seeded:
	case trxn_crc16:
	case trxn_crc16seeded:
		LEVEL_DEBUG("pack=CRC*");
		break;
	case trxn_delay:
	case trxn_udelay:
		LEVEL_DEBUG("pack=(U)DELAYS");
		ret = gbGOOD;
		break;
	case trxn_reset:
	case trxn_end:
	case trxn_verify:
		LEVEL_DEBUG("pack=RESET END VERIFY");
		return gbBAD;
	case trxn_nop:
		LEVEL_DEBUG("pack=NOP");
		break;
	}
	if (tb->packets == 0) {
		tb->start = tl;
	}
	++tb->packets;
	return ret;
}

static GOOD_OR_BAD Bundle_unpack(struct transaction_bundle *tb)
{
	int packet_index;
	const struct transaction_log *tl;
	BYTE *data = MemblobData(&(tb->mb));
	GOOD_OR_BAD ret = gbGOOD;

	LEVEL_DEBUG("unpacking");

	for (packet_index = 0, tl = tb->start; packet_index < tb->packets; ++packet_index, ++tl) {
		switch (tl->type) {
		case trxn_compare:		// match two strings -- no actual 1-wire
			LEVEL_DEBUG("unpacking #%d COMPARE", packet_index);
			if ((tl->in == NULL) || (tl->out == NULL)
				|| (memcmp(tl->in, tl->out, tl->size) != 0)) {
				ret = gbBAD;
			}
			break;
		case trxn_bitcompare:			// match two strings -- no actual 1-wire
			if ((tl->in == NULL) || (tl->out == NULL)
				|| BAD( BUS_compare_bits( tl->in, tl->out, tl->size)) ) {
				ret = gbBAD;
			}
			break ;
		case trxn_match:		// send data and compare response
			LEVEL_DEBUG("unpacking #%d MATCH", packet_index);
			if (memcmp(tl->out, data, tl->size) != 0) {
				ret = gbBAD;
			}
			data += tl->size;
			break;
		case trxn_read:
		case trxn_modify:		// write data and read response. No match needed
			LEVEL_DEBUG("unpacking #%d READ MODIFY", packet_index);
			memmove(tl->in, data, tl->size);
			data += tl->size;
			break;
		case trxn_blind:
			LEVEL_DEBUG("unpacking #%d BLIND", packet_index);
			data += tl->size;
			break;
		case trxn_power:
		case trxn_program:
			LEVEL_DEBUG("unpacking #%d POWER PROGRAM", packet_index);
			memmove(tl->in, data, 1);
			data += 1;
			UT_delay(tl->size);
			break;
		case trxn_crc8:
			LEVEL_DEBUG("unpacking #%d CRC8", packet_index);
			if (CRC8(tl->out, tl->size) != 0) {
				ret = gbBAD;
			}
			break;
		case trxn_crc8seeded:
			LEVEL_DEBUG("unpacking #%d CRC8 SEEDED", packet_index);
			if (CRC8seeded(tl->out, tl->size, ((UINT *) (tl->in))[0]) != 0) {
				ret = gbBAD;
			}
			break;
		case trxn_crc16:
			LEVEL_DEBUG("npacking #%d CRC16", packet_index);
			if (CRC16(tl->out, tl->size) != 0) {
				ret = gbBAD;
			}
			break;
		case trxn_crc16seeded:
			LEVEL_DEBUG("unpacking #%d CRC16 SEEDED", packet_index);
			if (CRC16seeded(tl->out, tl->size, ((UINT *) (tl->in))[0]) != 0) {
				ret = gbBAD;
			}
			break;
		case trxn_delay:
			LEVEL_DEBUG("unpacking #%d DELAY", packet_index);
			UT_delay(tl->size);
			break;
		case trxn_udelay:
			LEVEL_DEBUG("unpacking #%d UDELAY", packet_index);
			UT_delay_us(tl->size);
			break;
		case trxn_reset:
		case trxn_end:
		case trxn_verify:
			// should never get here
			LEVEL_DEBUG("unpacking #%d RESET END VERIFY", packet_index);
			ret = gbBAD;
			break;
		case trxn_nop:
		case trxn_select:
			LEVEL_DEBUG("unpacking #%d NOP or SELECT", packet_index);
			break;
		}
		if ( BAD(ret) ) {
			break;
		}
	}

	// clear the bundle
	MemblobClear(&tb->mb);
	tb->packets = 0;
	tb->select_first = 0;

	return ret;
}
