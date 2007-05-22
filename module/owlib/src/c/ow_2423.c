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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2423.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_counter);
READ_FUNCTION(FS_pagecount);
#if OW_CACHE
READ_FUNCTION(FS_r_mincount);
WRITE_FUNCTION(FS_w_mincount);
#endif							/* OW_CACHE */

/* ------- Structures ----------- */

struct aggregate A2423 = { 16, ag_numbers, ag_separate, };
struct aggregate A2423c = { 2, ag_letters, ag_separate, };
struct filetype DS2423[] = {
	F_STANDARD,
  {"memory", 512, NULL, ft_binary, fc_stable,   FS_r_mem, FS_w_mem, {v:NULL},} ,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"pages/page", 32, &A2423, ft_binary, fc_stable,   FS_r_page, FS_w_page, {v:NULL},} ,
  {"counters",PROPERTY_LENGTH_UNSIGNED, &A2423c, ft_unsigned, fc_volatile,   FS_counter, NO_WRITE_FUNCTION, {v:NULL},} ,
#if OW_CACHE
  {"mincount",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,   FS_r_mincount, FS_w_mincount, {v:NULL},} ,
#endif							/*OW_CACHE */
  {"pages/count",PROPERTY_LENGTH_UNSIGNED, &A2423, ft_unsigned, fc_volatile,   FS_pagecount, NO_WRITE_FUNCTION, {v:NULL},} ,
};

DeviceEntryExtended(1D, DS2423, DEV_ovdr);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x5A
#define _1W_READ_MEMORY 0xF0
#define _1W_READ_MEMORY_PLUS_COUNTER 0xA5

/* Persistent storage */
//static struct internal_prop ip_cum = { "CUM", fc_persistent };
MakeInternalProp(cum,fc_persistent) ; // cumulative

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn);
static int OW_r_mem_counter(struct one_wire_query * owq, size_t page, size_t pagesize) ;

/* 2423A/D Counter */
static int FS_r_page(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if ( OWQ_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_r_mem_counter ) )
		return -EINVAL;
    return 0;
}

static int FS_w_page(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
	if ( OW_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_w_mem ) )
		return -EINVAL;
    return 0;
}

static int FS_r_mem(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if ( OWQ_readwrite_paged( owq, 0, pagesize, OW_r_mem_counter ) )
		return -EINVAL;
    return 0;
}

static int FS_w_mem(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
	if ( OW_readwrite_paged( owq, 0, pagesize, OW_w_mem ) )
		return -EINVAL;
    return 0;
}

static int FS_counter(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if (OW_r_mem_counter(owq, OWQ_pn(owq).extension + 14, pagesize ))
        return -EINVAL;
    return 0;
}

static int FS_pagecount(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if (OW_r_mem_counter(owq, OWQ_pn(owq).extension, pagesize ))
        return -EINVAL;
    return 0;
}

#if OW_CACHE
/* Special code for cumulative counters -- read/write -- uses the caching system for storage */
/* Different from LCD system, counters are NOT reset with each read */
static int FS_r_mincount(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
	UINT st[3], ct[2];			// stored and current counter values
    
    if ( OW_r_mem_counter( owq, 0, 32 ) ) return -EINVAL ;
    ct[0] = OWQ_U(owq) ;
    
    if ( OW_r_mem_counter( owq, 1, 32 ) ) return -EINVAL ;
    ct[1] = OWQ_U(owq) ;
    
    if (Cache_Get_Internal_Strict((void *) st, 3 * sizeof(UINT), InternalProp(cum), pn)) {	// record doesn't (yet) exist
		st[2] = ct[0] < ct[1] ? ct[0] : ct[1];
	} else {
		UINT d0 = ct[0] - st[0];	//delta counter.A
		UINT d1 = ct[1] - st[1];	// delta counter.B
		st[2] += d0 < d1 ? d0 : d1;	// add minimum delta
	}
	st[0] = ct[0];
	st[1] = ct[1];
    OWQ_U(owq) = st[2];
    
    if ( Cache_Add_Internal((void *) st, 3 * sizeof(UINT), InternalProp(cum), pn) )
        return -EINVAL ;
    return 0 ;
}

static int FS_w_mincount(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    UINT st[3];					// stored and current counter values

    st[2] = OWQ_U(owq) ;

    if ( OW_r_mem_counter( owq, 0, 32 ) ) return -EINVAL ;
    st[0] = OWQ_U(owq) ;
    
    if ( OW_r_mem_counter( owq, 1, 32 ) ) return -EINVAL ;
    st[1] = OWQ_U(owq) ;
    
    if ( Cache_Add_Internal((void *) st, 3 * sizeof(UINT), InternalProp(cum), pn) )
        return -EINVAL ;
    return 0 ;
}
#endif							/*OW_CACHE */

static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn)
{
    BYTE p[1 + 2 + 32 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log tcopy[] = {
		TRXN_START,
		{p, NULL, size + 3, trxn_match,},
		{NULL, &p[size + 3], 2, trxn_read,},
		{p, NULL, 1 + 2 + size + 2, trxn_crc16,},
		TRXN_END,
	};
	struct transaction_log treread[] = {
		TRXN_START,
		{p, NULL, 1, trxn_match,},
		{NULL, &p[1], 3 + size, trxn_read,},
		TRXN_END,
	};
	struct transaction_log twrite[] = {
		TRXN_START,
		{p, NULL, 4, trxn_match,},
		TRXN_END,
	};

	/* Copy to scratchpad */
	memcpy(&p[3], data, size);

	if (((offset + size) & 0x1F)) {	// doesn't end on page boundary, no crc16
		tcopy[2].type = tcopy[3].type = trxn_nop;
	}

	if (BUS_transaction(tcopy, pn))
		return 1;

	/* Re-read scratchpad and compare */
	/* Note that we tacitly shift the data one byte down for the E/S byte */
    p[0] = _1W_READ_SCRATCHPAD ;
	if (BUS_transaction(treread, pn) || memcmp(&p[4], data, size))
		return 1;

	/* Copy Scratchpad to SRAM */
    p[0] = _1W_COPY_SCRATCHPAD;
	if (BUS_transaction(twrite, pn))
		return 1;

	UT_delay(32);
	return 0;
}

/* read memory area and counter (just past memory) */
/* Nathan Holmes help troubleshoot this one! */
static int OW_r_mem_counter(struct one_wire_query * owq, size_t page, size_t pagesize)
{
    /* read in (after command and location) 'rest' memory bytes, 4 counter bytes, 4 zero bytes, 2 CRC16 bytes */
    switch( OWQ_pn(owq).ft->format ) {
        case ft_binary:
        case ft_ascii:
        case ft_vascii:
            return OW_r_mem_p8_crc16( owq, page, pagesize, NULL ) ;
        case ft_unsigned:
        {
            BYTE extra[8];
            OWQ_make( owq_read ) ;
            OWQ_create_temporary( owq_read, NULL, 1, 31, PN(owq) ) ;
            if ( OW_r_mem_p8_crc16( owq_read, page, pagesize, extra ) ) return 1 ;
            if (extra[4] != 0x00 || extra[5] != 0x00 || extra[6] != 0x00
                || extra[7] != 0x00)
                return 1;
            /* counter is held in the 4 bytes after the data */
            OWQ_U(owq) = UT_uint32(extra);
            return 0 ;
        }
        default:
            return 1 ;
    }
}
