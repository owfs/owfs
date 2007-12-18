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

struct transaction_bundle {
    const struct transaction_log *start ;
    int packets ;
    size_t bytes ;
    int select_first ;
} ;

// static int BUS_transaction_length( const struct transaction_log * tl, const struct parsedname * pn ) ;
static int BUS_transaction_single(const struct transaction_log *t,
                           const struct parsedname *pn) ;


/* Bus transaction */
/* Encapsulates communication with a device, including locking the bus, reset and selection */
/* Then a series of bytes is sent and returned, including sending data and reading the return data */
int BUS_transaction(const struct transaction_log *tl,
					const struct parsedname *pn)
{
	int ret = 0;

	if (tl == NULL)
		return 0;

	BUSLOCK(pn);
	ret = BUS_transaction_nolock(tl, pn);
	BUSUNLOCK(pn);

	return ret;
}
int BUS_transaction_nolock(const struct transaction_log *tl,
                           const struct parsedname *pn)
{
    const struct transaction_log *t = tl;
    int ret = 0;

    do {
        //printf("Transact type=%d\n",t->type) ;
        ret = BUS_transaction_single(t,pn) ;
        if ( ret == -ESRCH ) { // trxn_done flag
            ret = 0 ; // restore no error code
            break ; // but stop looping anyways
        }
        ++t;
    } while (ret == 0);
    return ret;
}

static int BUS_transaction_single(const struct transaction_log *t,
                           const struct parsedname *pn)
{
    int ret = 0;
    switch (t->type) {
        case trxn_select: // select a 1-wire device (by unique ID)
            ret = BUS_select(pn);
            LEVEL_DEBUG("  Transaction select = %d\n", ret);
            break;
        case trxn_compare: // match two strings -- no actual 1-wire
            if ( (t->in==NULL) || (t->out==NULL) || (memcmp(t->in,t->out,t->size)!=0) ) {
                ret = -EINVAL ;
            }
            break ;
        case trxn_match: // send data and compare response
            if (t->size == 0) { /* ignore for both cases */
                break;
            } else if (t->in) { /* just compare out and in */
                ret = memcmp(t->out, t->in, t->size);
                LEVEL_DEBUG("  Transaction match = %d\n", ret);
            } else {
                ret = BUS_send_data(t->out, t->size, pn);
                LEVEL_DEBUG("  Transaction send = %d\n", ret);
            }
            break;
        case trxn_read:
            if (t->size == 0) { /* ignore for all cases */
                break;
            } else if (t->out == NULL) {
                ret = BUS_readin_data(t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction readin = %d\n", ret);
            } else if (t->in == NULL) {
                BYTE dummy[64];
                size_t s = t->size;
                while (s > 0) {
                    size_t ss = (s > 64) ? 64 : s;
                    s -= ss;
                    ret = BUS_sendback_data(t->out, dummy, ss, pn);
                    LEVEL_DEBUG
                            ("  Transaction null sendback (%lu of %lu)= %d\n",
                                ss, t->size, ret);
                    if (ret)
                        break;
                }
            } else {
                ret = BUS_sendback_data(t->out, t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction sendback = %d\n", ret);
            }
            break;
        case trxn_power:
            ret = BUS_PowerByte(t->out[0], t->in, t->size, pn);
            LEVEL_DEBUG("  Transaction power (%d usec) = %d\n", t->size, ret);
            break;
        case trxn_program:
            ret = BUS_ProgramPulse(pn);
            LEVEL_DEBUG("  Transaction program pulse = %d\n", ret);
            break;
        case trxn_crc8:
            ret = CRC8(t->out, t->size);
            LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
            break;
        case trxn_crc8seeded:
            ret = CRC8seeded(t->out, t->size, ((UINT *) (t->in))[0]);
            LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
            break;
        case trxn_crc16:
            ret = CRC16(t->out, t->size);
            LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
            break;
        case trxn_crc16seeded:
            ret = CRC16seeded(t->out, t->size, ((UINT *) (t->in))[0]);
            LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
            break;
        case trxn_delay:
            if ( t->size > 0 ) {
                UT_delay(t->size);
            }
            LEVEL_DEBUG("  Transaction Delay %d\n", t->size);
            break;
        case trxn_udelay:
            if ( t->size > 0 ) {
                UT_delay_us(t->size);
            }
            LEVEL_DEBUG("  Transaction Micro Delay %d\n", t->size);
            break;
        case trxn_reset:
            ret = BUS_reset(pn);
            LEVEL_DEBUG("  Transaction reset = %d\n", ret);
            break ;
        case trxn_end:
            LEVEL_DEBUG("  Transaction end = %d\n", ret);
            return -ESRCH ; // special "end" flag
        case trxn_verify:
        {
            struct parsedname pn2;
            memcpy(&pn2, pn, sizeof(struct parsedname));    //shallow copy
            pn2.selected_device = NULL;
            ret = BUS_select(&pn2) || BUS_verify(t->size, pn);
            LEVEL_DEBUG("  Transaction verify = %d\n", ret);
        }
        break;
        case trxn_nop:
            ret = 0;
            break;
    }
    return ret;
}

#if 0
/* start of bundled transactions */

int bundle_add( const struct transaction_log * tl, struct transaction_bundle * tb )
{
    if ( tb->start == NULL ) {
        tb->start = tl ;
    }
    switch (tl->type) {
        case trxn_select:
            select_first = 1 ;
            break;
        case trxn_match:
            if ( (tl->size>0) && (tl->in==NULL) {
                 tb->bytes += tl->size ;
            }
            break;
        case trxn_read:
                tb->bytes += tl->size ;
                break;
} else if (t->out == NULL) {
                ret = BUS_readin_data(t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction readin = %d\n", ret);
} else if (t->in == NULL) {
                BYTE dummy[64];
                size_t s = t->size;
                while (s > 0) {
                    size_t ss = (s > 64) ? 64 : s;
                    s -= ss;
                    ret = BUS_sendback_data(t->out, dummy, ss, pn);
                    LEVEL_DEBUG
                        ("  Transaction null sendback (%lu of %lu)= %d\n",
                         ss, t->size, ret);
                    if (ret)
                        break;
}
} else {
                ret = BUS_sendback_data(t->out, t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction sendback = %d\n", ret);
}
            break;
    case trxn_power:
            ret = BUS_PowerByte(t->out[0], t->in, t->size, pn);
            LEVEL_DEBUG("  Transaction power (%d usec) = %d\n", t->size, ret);
            break;
    case trxn_program:
            ret = BUS_ProgramPulse(pn);
            LEVEL_DEBUG("  Transaction program pulse = %d\n", ret);
            break;
    case trxn_crc8:
            ret = CRC8(t->out, t->size);
            LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
            break;
    case trxn_crc8seeded:
            ret = CRC8seeded(t->out, t->size, ((UINT *) (t->in))[0]);
            LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
            break;
    case trxn_crc16:
            ret = CRC16(t->out, t->size);
            LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
            break;
    case trxn_crc16seeded:
            ret = CRC16seeded(t->out, t->size, ((UINT *) (t->in))[0]);
            LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
            break;
    case trxn_delay:
            if ( t->size > 0 ) {
                UT_delay(t->size);
}
            LEVEL_DEBUG("  Transaction Delay %d\n", t->size);
            break;
    case trxn_udelay:
            if ( t->size > 0 ) {
                UT_delay_us(t->size);
}
            LEVEL_DEBUG("  Transaction Micro Delay %d\n", t->size);
            break;
    case trxn_reset:
            ret = BUS_reset(pn);
            LEVEL_DEBUG("  Transaction reset = %d\n", ret);
            return 0;
    case trxn_end:
            LEVEL_DEBUG("  Transaction end = %d\n", ret);
            return 0;
    case trxn_verify:
{
                struct parsedname pn2;
                memcpy(&pn2, pn, sizeof(struct parsedname));    //shallow copy
                pn2.selected_device = NULL;
                ret = BUS_select(&pn2) || BUS_verify(t->size, pn);
                LEVEL_DEBUG("  Transaction verify = %d\n", ret);
}
            break;
    case trxn_nop:
            ret = 0;
            break;
}
    ++ tb->packets ;
    
    
    
#endif