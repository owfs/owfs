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
#include "ow_2433.h"

/* ------- Prototypes ----------- */

/* DS2433 EEPROM */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
WRITE_FUNCTION(FS_w_page2D);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
WRITE_FUNCTION(FS_w_memory2D);

 /* ------- Structures ----------- */

struct aggregate A2431 = { 4, ag_numbers, ag_separate, };
struct filetype DS2431[] = {
	F_STANDARD,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2431, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page2D}, {v:NULL},},
  {"memory", 128, NULL, ft_binary, fc_stable, {o: FS_r_memory}, {o: FS_w_memory2D}, {v:NULL},},
};

DeviceEntryExtended(2D, DS2431, DEV_ovdr | DEV_resume);

struct aggregate A2433 = { 16, ag_numbers, ag_separate, };
struct filetype DS2433[] = {
	F_STANDARD,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2433, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
  {"memory", 512, NULL, ft_binary, fc_stable, {o: FS_r_memory}, {o: FS_w_memory}, {v:NULL},},
};

DeviceEntryExtended(23, DS2433, DEV_ovdr);

/* ------- Functions ------------ */

/* DS2433 */

static int OW_w_23page( BYTE * data,  size_t size,
					    off_t offset,  struct parsedname *pn);
static int OW_w_2Dpage( BYTE * data,  size_t size,
					    off_t offset,  struct parsedname *pn);

static int FS_r_memory(struct one_wire_query * owq)
{
	/* read is not page-limited */
    if (OW_r_mem_simple(owq, 0, 0 ))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_memory(struct one_wire_query * owq)
{
	/* paged access */
	size_t pagesize = 32 ;
    if (OW_readwrite_paged(owq, 0, pagesize, OW_w_23page))
		return -EFAULT;
	return 0;
}

/* Although externally it's 32 byte pages, internally it acts as 8 byte pages */
static int FS_w_memory2D(struct one_wire_query * owq)
{
	/* paged access */
	size_t pagesize = 8 ;
    if (OW_readwrite_paged(owq, 0, pagesize, OW_w_2Dpage))
		return -EFAULT;
	return 0;
}

static int FS_r_page(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
    if (OW_r_mem_simple( owq, OWQ_pn(owq).extension, pagesize ))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query * owq)
{
	/* paged access */
	size_t pagesize = 32 ;
    if (OW_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_23page))
		return -EFAULT;
	return 0;
}

static int FS_w_page2D(struct one_wire_query * owq)
{
	/* paged access */
	size_t pagesize = 8 ;
    if (OW_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_2Dpage))
		return -EFAULT;
	return 0;
}

/* paged, and pre-screened */
static int OW_w_23page( BYTE * data,  size_t size,
					    off_t offset,  struct parsedname *pn)
{
	BYTE p[1 + 2 + 32 + 2] = { 0x0F, offset & 0xFF, offset >> 8, };
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
	p[0] = 0xAA;
	if (BUS_transaction(treread, pn) || memcmp(&p[4], data, size))
		return 1;

	/* Copy Scratchpad to SRAM */
	p[0] = 0x5A;
	if (BUS_transaction(twrite, pn))
		return 1;

	UT_delay(5);
	return 0;
}

/* paged, and pre-screened */
/* read REAL DS2431 pages -- 8 bytes. */
static int OW_w_2Dpage( BYTE * data,  size_t size,
					    off_t offset,  struct parsedname *pn)
{
	off_t pageoff = offset & 0x07;
	BYTE p[4 + 8 + 2] = { 0x0F, (offset - pageoff) & 0xFF,
		((offset - pageoff) >> 8) & 0xFF,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		{p, NULL, 3 + 8, trxn_match},
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		{p, NULL, 1, trxn_match},
		{NULL, &p[1], 8 + 3 + 2, trxn_read},
		TRXN_END,
	};
	struct transaction_log tsram[] = {
		TRXN_START,
		{p, NULL, 4, trxn_match},
		TRXN_END,
	};

	if (size != 8) { // incomplete page
		OWQ_make( owq_old ) ;
        OWQ_create_temporary( owq_old, (char *) &p[3], 8, offset - pageoff, pn ) ;
		if (OW_r_mem_simple( owq_old, 0 ,0 ))
			return 1;
	}

	memcpy(&p[3 + pageoff], data, size);

	/* Copy to scratchpad */
	if (BUS_transaction(tcopy, pn))
		return 1;

	/* Re-read scratchpad and compare */
	p[0] = 0xAA;
	if (BUS_transaction(tread, pn))
		return 1;
	if (memcmp(&p[4 + pageoff], data, size))
		return 1;
	if (CRC16(p, 4 + 8 + 2))
		return 1;

	/* Copy Scratchpad to SRAM */
	p[0] = 0x55;
	if (BUS_transaction(tsram, pn))
		return 1;

	UT_delay(13);
	return 0;
}
