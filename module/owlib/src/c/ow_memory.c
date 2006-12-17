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
#include "ow_connection.h"

/* No CRC -- 0xF0 code */
int OW_r_mem_simple(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	BYTE p[3] = { 0xF0, offset & 0xFF, (offset >> 8) & 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 3, trxn_match},
		{NULL, data, size, trxn_read},
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

/* read up to end of page to CRC16 -- 0xA5 code */
int OW_r_mem_crc16(BYTE * data, const size_t size, const off_t offset,
				   const struct parsedname *pn, size_t pagesize)
{
	BYTE p[3 + pagesize + 2];
	int rest = pagesize - (offset % pagesize);
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 3, trxn_match,},
		{NULL, &p[3], rest + 2, trxn_read,},
		TRXN_END,
	};

	p[0] = 0xA5;
	p[1] = offset & 0xFF;
	p[2] = (offset >> 8) & 0xFF;
	if (BUS_transaction(t, pn) || CRC16(p, 3 + rest + 2))
		return 1;
	memcpy(data, &p[3], size);
	return 0;
}

/* read up to end of page to CRC16 -- 0xA5 code */
/* Extra 8 bytes, too */
int OW_r_mem_p8_crc16(BYTE * data, const size_t size, const off_t offset,
					  const struct parsedname *pn, size_t pagesize,
					  BYTE * extra)
{
	BYTE p[3 + pagesize + 8 + 2];
	int rest = pagesize - (offset % pagesize);
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 3, trxn_match,},
		{NULL, &p[3], rest + 8 + 2, trxn_read,},
		TRXN_END,
	};

	p[0] = 0xA5;
	p[1] = offset & 0xFF;
	p[2] = (offset >> 8) & 0xFF;
	if (BUS_transaction(t, pn) || CRC16(p, 3 + rest + 8 + 2))
		return 1;
	if (data)
		memcpy(data, &p[3], size);
	memcpy(extra, &p[3 + rest], 8);
	return 0;
}
