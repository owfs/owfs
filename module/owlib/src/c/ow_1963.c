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
#include "ow_1963.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
WRITE_FUNCTION(FS_w_password);
READ_FUNCTION(FS_counter);

/* ------- Structures ----------- */

struct aggregate A1963S = { 16, ag_numbers, ag_separate, };
struct filetype DS1963S[] = {
    F_STANDARD,
    {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
    {"pages/page", 32, &A1963S, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
    {"pages/count",PROPERTY_LENGTH_UNSIGNED, &A1963S, ft_unsigned, fc_volatile, {o: FS_counter}, {v: NULL}, {v:NULL},},
    {"pages/password", 8, NULL, ft_binary, fc_stable, {v: NULL}, {o: FS_w_password}, {v:NULL},},
    {"memory", 512, NULL, ft_binary, fc_stable, {o: FS_r_memory}, {o: FS_w_memory}, {v:NULL},},
    {"password", 8, NULL, ft_binary, fc_stable, {v: NULL}, {o: FS_w_password}, {v:NULL},},
};

DeviceEntryExtended(18, DS1963S, DEV_resume | DEV_ovdr);

struct aggregate A1963L = { 16, ag_numbers, ag_separate, };
struct filetype DS1963L[] = {
    F_STANDARD,
    {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
    {"pages/page", 32, &A1963L, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
    {"pages/count",PROPERTY_LENGTH_UNSIGNED, &A1963L, ft_unsigned, fc_volatile, {o: FS_counter}, {v: NULL}, {v:NULL},},
    {"memory", 512, NULL, ft_binary, fc_stable, {o: FS_r_memory}, {o: FS_w_memory}, {v:NULL},},
};

DeviceEntryExtended(1A, DS1963L, DEV_ovdr);

/* ------- Functions ------------ */

static int OW_w_mem( BYTE * data,  size_t size,
                     off_t offset,  struct parsedname *pn);
static int OW_r_mem_counter(struct one_wire_query * owq, size_t page, size_t pagesize) ;

static int FS_w_password(struct one_wire_query * owq)
{
    (void) owq ;
    return -EINVAL;
}

static int FS_r_page(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if (OWQ_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_r_mem_counter ) )
        return -EINVAL;
    return 0;
}

static int FS_r_memory(struct one_wire_query * owq)
{
    /* read is not page-limited */
    size_t pagesize = 32 ;
    if (OWQ_readwrite_paged( owq, 0, pagesize, OW_r_mem_counter ) )
        return -EINVAL;
    return 0;
}

static int FS_counter(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if (OW_r_mem_counter(owq, OWQ_pn(owq).extension, pagesize ))
        return -EINVAL;
    return 0;
}

static int FS_w_page(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if (OW_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem))
        return -EINVAL;
    return 0;
}

static int FS_w_memory(struct one_wire_query * owq)
{
    size_t pagesize = 32 ;
    if (OW_readwrite_paged(owq, 0, pagesize, OW_w_mem))
        return -EINVAL;
    return 0;
}

/* paged, and pre-screened */
static int OW_w_mem( BYTE * data,  size_t size,
                     off_t offset,  struct parsedname *pn)
{
    BYTE p[1 + 2 + 32 + 2] = { 0x0F, offset & 0xFF, offset >> 8, };
    struct transaction_log tcopy[] = {
        TRXN_START,
        {p, NULL, size + 3, trxn_match},
        {data, NULL, size, trxn_match},
        {NULL, &p[size + 3], 2, trxn_read},
        TRXN_END,
    };
    struct transaction_log tread[] = {
        TRXN_START,
        {p, NULL, 1, trxn_match},
        {NULL, &p[1], size + 3, trxn_read},
        TRXN_END,
    };
    struct transaction_log tsram[] = {
        TRXN_START,
        {p, NULL, 4, trxn_match},
        TRXN_END,
    };

    /* Copy to scratchpad */
    memcpy(&p[3], data, size);

    if (((offset + size) & 0x1F) != 0)
        tcopy[3].type = trxn_end;   // not at page boundary at end
    if (BUS_transaction(tcopy, pn))
        return 1;
    if (((offset + size) & 0x1F) == 0 && CRC16(p, 1 + 2 + size + 2))
        return 1;

    /* Re-read scratchpad and compare */
    /* Note that we tacitly shift the data one byte down for the E/S byte */
    p[0] = 0xAA;
    if (BUS_transaction(tread, pn))
        return 1;
    if (memcmp(&p[4], data, size))
        return 1;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x5A;
    if (BUS_transaction(tsram, pn))
        return 1;

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
            if (extra[4] != 0x55 || extra[5] != 0x55 || extra[6] != 0x55
                || extra[7] != 0x55)
                return 1;
            /* counter is held in the 4 bytes after the data */
            OWQ_U(owq) = UT_uint32(extra);
            return 0 ;
        }
        default:
            return 1 ;
    }
}
