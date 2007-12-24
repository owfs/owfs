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
    const struct transaction_log *start ;
    int packets ;
    size_t bytes ;
    struct memblob mb ;
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
            LEVEL_DEBUG("  Transaction compare = %d\n", ret);
            break ;
        case trxn_match: // send data and compare response
            assert( t->in == NULL ) ; // else use trxn_compare
            if (t->size == 0) { /* ignore for both cases */
                break;
            } else {
                ret = BUS_send_data(t->out, t->size, pn);
                LEVEL_DEBUG("  Transaction send = %d\n", ret);
            }
            break;
        case trxn_read:
            assert( t->out == NULL ) ; // pure read
            if (t->size == 0) { /* ignore for all cases */
                break;
            } else if (t->out == NULL) {
                ret = BUS_readin_data(t->in, t->size, pn);
                LEVEL_DEBUG("  Transaction readin = %d\n", ret);
            }
            break;
        case trxn_modify: // write data and read response. No match needed
            ret = BUS_sendback_data(t->out, t->in, t->size, pn);
            LEVEL_DEBUG("  Transaction modify = %d\n", ret);
            break ;
        case trxn_blind: // write data ignore response
        {
            BYTE * dummy = malloc( t->size ) ;
            if ( dummy != NULL ) {
                ret = BUS_sendback_data(t->out, dummy, t->size, pn);
                free( dummy) ;
            } else {
                ret = -ENOMEM ;
            }
        }
            LEVEL_DEBUG("  Transaction blind = %d\n", ret);
            break ;
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
static int Bundle_execute( struct transaction_bundle * tb, struct parsedname * pn )
{
    int packet_index ;
    struct transaction_log * tl ;
    size_t bytes_used = 0 ;
    BYTE * data = MemblobGet( &tb->mb ) ;


    for ( packet_index = 0, tl = tb->start ; packet_index < tb->packets ; ++packet_index, ++tl ) {
        switch (tl->type) {
            case trxn_select: // select a 1-wire device (by unique ID)
                break;
            case trxn_compare: // match two strings -- no actual 1-wire
                if ( (tl->in==NULL) || (tl->out==NULL) || (memcmp(tl->in,tl->out,tl->size)!=0) ) {
                        return -EINVAL ;
                    }
                break ;
            case trxn_match: // send data and compare response
                if ( memcmp( tl->out, &data[bytes_used], tl->size) != 0 ) return -EINVAL ;
                bytes_used += tl->size ;
                break;
            case trxn_read:
            case trxn_modify: // write data and read response. No match needed
                memmove(tl->in, &data[bytes_used], tl->size);
                bytes_used += tl->size ;
                break;
            case trxn_power:
            case trxn_program:
                break;
            case trxn_crc8:
                ret = CRC8(tl->out, tl->size);
                LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
                break;
            case trxn_crc8seeded:
                ret = CRC8seeded(tl->out, tl->size, ((UINT *) (tl->in))[0]);
                LEVEL_DEBUG("  Transaction CRC8 = %d\n", ret);
                break;
            case trxn_crc16:
                ret = CRC16(tl->out, tl->size);
                LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
                break;
            case trxn_crc16seeded:
                ret = CRC16seeded(tl->out, tl->size, ((UINT *) (tl->in))[0]);
                LEVEL_DEBUG("  Transaction CRC16 = %d\n", ret);
                break;
            case trxn_delay:
                if ( tl->size > 0 ) {
                    UT_delay(tl->size);
                }
                LEVEL_DEBUG("  Transaction Delay %d\n", tl->size);
                break;
            case trxn_udelay:
                if ( tl->size > 0 ) {
                    UT_delay_us(tl->size);
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
                ret = BUS_select(&pn2) || BUS_verify(tl->size, pn);
                LEVEL_DEBUG("  Transaction verify = %d\n", ret);
            }
            break;
            case trxn_nop:
                ret = 0;
                break;
        }
    }
}
#endif
