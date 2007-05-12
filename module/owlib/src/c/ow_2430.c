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
#include "ow_2430.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_lock);
READ_FUNCTION(FS_r_application);
WRITE_FUNCTION(FS_w_application);

/* ------- Structures ----------- */

struct filetype DS2430A[] = {
	F_STANDARD,
  {"memory", 16, NULL, ft_binary, fc_stable, {o: FS_r_memory}, {o: FS_w_memory}, {v:NULL},},
  {"application", 8, NULL, ft_binary, fc_stable, {o: FS_r_application}, {o: FS_w_application}, {v:NULL},},
  {"lock",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_lock}, {o: NULL}, {v:NULL},},
};

DeviceEntry(14, DS2430A);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_COPY_SCRATCHPAD_VALIDATION_KEY 0xA5
#define _1W_READ_MEMORY 0xF0
#define _1W_READ_STATUS_REGISTER 0x66
#define _1W_WRITE_APPLICATION_REGISTER 0x99
#define _1W_READ_APPLICATION_REGISTER 0xC3
#define _1W_COPY_AND_LOCK_APPLICATION_REGISTER 0x5A

/* ------- Functions ------------ */

/* DS2502 */
static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn);
static int OW_w_app(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn);
static int OW_r_app(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn);
static int OW_r_status(BYTE * data, const struct parsedname *pn);

/* DS2430A memory */
static int FS_r_memory(struct one_wire_query * owq)
{
    if (OW_r_mem_simple( owq, 0, 0 ))
		return -EINVAL;
    return 0 ;
}

/* DS2430A memory */
static int FS_r_application(struct one_wire_query * owq)
{
    if (OW_r_app((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_memory(struct one_wire_query * owq)
{
    if (OW_w_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_application(struct one_wire_query * owq)
{
    if (OW_w_app((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_lock(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_status(&data, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = data & 0x01;
	return 0;
}

/* Byte-oriented write */
static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
    BYTE fo[] = { _1W_READ_MEMORY, };
	struct transaction_log tread[] = {
		TRXN_START,
		{fo, NULL, 1, trxn_match},
		TRXN_END,
	};
    BYTE of[] = { _1W_WRITE_SCRATCHPAD, (BYTE) (offset & 0x1F), };
	struct transaction_log twrite[] = {
		TRXN_START,
		{of, NULL, 2, trxn_match},
		{data, NULL, size, trxn_match},
		TRXN_END,
	};
	BYTE ver[16];
    BYTE vr[] = { _1W_READ_SCRATCHPAD, (BYTE) (offset & 0x1F), };
	struct transaction_log tver[] = {
		TRXN_START,
		{vr, NULL, 2, trxn_match},
		{NULL, ver, size, trxn_read},
		TRXN_END,
	};
    BYTE cp[] = { _1W_COPY_SCRATCHPAD, };
    BYTE cf[] = { _1W_COPY_SCRATCHPAD_VALIDATION_KEY, };
	struct transaction_log tcopy[] = {
		TRXN_START,
		{cp, NULL, 1, trxn_match},
		{cf, cf, 10, trxn_power},
		TRXN_END,
	};

	/* load scratch pad if incomplete write */
	if ((size != 16) && BUS_transaction(tread, pn))
		return 1;
	/* write data to scratchpad */
	if (BUS_transaction(twrite, pn))
		return 1;
	/* read back the scratchpad */
	if (BUS_transaction(tver, pn))
		return 1;
	if (memcmp(data, ver, size))
		return 1;
	/* copy scratchpad to memory */
	return BUS_transaction(tcopy, pn);
}

/* Byte-oriented write */
static int OW_w_app(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
    BYTE fo[] = { _1W_READ_APPLICATION_REGISTER, };
	struct transaction_log tread[] = {
		TRXN_START,
		{fo, NULL, 1, trxn_match},
		TRXN_END,
	};
    BYTE of[] = { _1W_WRITE_APPLICATION_REGISTER, (BYTE) (offset & 0x0F), };
	struct transaction_log twrite[] = {
		TRXN_START,
		{of, NULL, 2, trxn_match},
		{data, NULL, size, trxn_match},
		TRXN_END,
	};
	BYTE ver[9];
    BYTE vr[] = { _1W_READ_SCRATCHPAD, (BYTE) (offset & 0x1F), };
	struct transaction_log tver[] = {
		TRXN_START,
		{vr, NULL, 2, trxn_match},
		{NULL, ver, size, trxn_read},
		TRXN_END,
	};
    BYTE cp[] = { _1W_COPY_AND_LOCK_APPLICATION_REGISTER, };
    BYTE cf[] = { _1W_COPY_SCRATCHPAD_VALIDATION_KEY, };
	struct transaction_log tcopy[] = {
		TRXN_START,
		{cp, NULL, 1, trxn_match},
		{cf, cf, 10, trxn_power},
		TRXN_END,
	};

	/* load scratch pad if incomplete write */
	if ((size != 8) && BUS_transaction(tread, pn))
		return 1;
	/* write data to scratchpad */
	if (BUS_transaction(twrite, pn))
		return 1;
	/* read back the scratchpad */
	if (BUS_transaction(tver, pn))
		return 1;
	if (memcmp(data, ver, size))
		return 1;
	/* copy scratchpad to memory */
	return BUS_transaction(tcopy, pn);
}

static int OW_r_app(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
    BYTE fo[] = { _1W_READ_APPLICATION_REGISTER, (BYTE) (offset & 0x0F), };
	struct transaction_log tread[] = {
		TRXN_START,
		{fo, NULL, 2, trxn_match},
		{NULL, data, size, trxn_read},
		TRXN_END,
	};
	if (BUS_transaction(tread, pn))
		return 1;
	return 0;
}

static int OW_r_status(BYTE * data, const struct parsedname *pn)
{
    BYTE ss[] = { _1W_READ_STATUS_REGISTER, 0x00 };
	struct transaction_log tread[] = {
		TRXN_START,
		{ss, NULL, 2, trxn_match},
		{NULL, data, 1, trxn_read},
		TRXN_END,
	};
	return BUS_transaction(tread, pn);
}
