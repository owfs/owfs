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
#include "ow_2436.h"

/* ------- Prototypes ----------- */

/* DS2436 Battery */
bREAD_FUNCTION(FS_r_page);
bWRITE_FUNCTION(FS_w_page);
fREAD_FUNCTION(FS_temp);
fREAD_FUNCTION(FS_volts);

/* ------- Structures ----------- */

struct aggregate A2436 = { 3, ag_numbers, ag_separate, };
struct filetype DS2436[] = {
    F_STANDARD,
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2436, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
  {"volts", 12, NULL, ft_float, fc_volatile, {f: FS_volts}, {v: NULL}, {v:NULL},},
  {"temperature", 12, NULL, ft_temperature, fc_volatile, {f: FS_temp}, {v: NULL}, {v:NULL},},
};

DeviceEntry(1 B, DS2436);

/* ------- Functions ------------ */

/* DS2436 */
static int OW_r_page(BYTE * p, const size_t size, const off_t offset,
		     const struct parsedname *pn);
static int OW_w_page(const BYTE * p, const size_t size, const off_t offset,
		     const struct parsedname *pn);
static int OW_temp(_FLOAT * T, const struct parsedname *pn);
static int OW_volts(_FLOAT * V, const struct parsedname *pn);

static size_t Asize[] = { 24, 8, 8, };

/* 2436 A/D */
static int FS_r_page(BYTE * buf, const size_t size, const off_t offset,
		     const struct parsedname *pn)
{
    if (pn->extension > 2)
	return -ERANGE;
    if (OW_r_page(buf, size, offset + ((pn->extension) << 5), pn))
	return -EINVAL;
    return size;
}

static int FS_w_page(const BYTE * buf, const size_t size,
		     const off_t offset, const struct parsedname *pn)
{
    if (pn->extension > 2)
	return -ERANGE;
    if (OW_w_page(buf, size, offset + ((pn->extension) << 5), pn))
	return -EINVAL;
    return 0;
}

static int FS_temp(_FLOAT * T, const struct parsedname *pn)
{
    if (OW_temp(T, pn))
	return -EINVAL;
    return 0;
}

static int FS_volts(_FLOAT * V, const struct parsedname *pn)
{
    if (OW_volts(V, pn))
	return -EINVAL;
    return 0;
}

/* DS2436 simple battery */
/* only called for a single page, and that page is 0,1,2 only*/
static int OW_r_page(BYTE * p, const size_t size, const off_t offset,
		     const struct parsedname *pn)
{
    BYTE data[32];
    int page = offset >> 5;
    size_t s;
    BYTE scratchin[] = { 0x11, offset, };
    static BYTE copyin[] = { 0x71, 0x77, 0x7A, };
    BYTE *copy = &copyin[page];
    struct transaction_log tcopy[] = {
	TRXN_START,
	{copy, NULL, 1, trxn_match,},
	TRXN_END,
    };
    struct transaction_log tscratch[] = {
	TRXN_START,
	{scratchin, NULL, 2, trxn_match,},
	{NULL, data, size, trxn_read},
	TRXN_END,
    };


    s = Asize[page] - (offset & 0x1F);
    if (s > size)
	s = size;
    tscratch[2].size = s;

    memset(p, 0xFF, size);

    if (BUS_transaction(tcopy, pn))
	return 1;

    UT_delay(10);

    if (BUS_transaction(tscratch, pn))
	return 1;

    // copy to buffer
    memcpy(p, data, s);

    return 0;
}

/* only called for a single page, and that page is 0,1,2 only*/
static int OW_w_page(const BYTE * p, const size_t size, const off_t offset,
		     const struct parsedname *pn)
{
    int page = offset >> 5;
    size_t s;
    BYTE scratchin[] = { 0x11, offset, };
    BYTE scratchout[] = { 0x17, offset, };
    BYTE data[32];
    static BYTE copyout[] = { 0x22, 0x25, 0x27, };
    BYTE *copy = &copyout[page];
    struct transaction_log tscratch[] = {
	TRXN_START,
	{scratchout, NULL, 2, trxn_match,},
	{p, NULL, size, trxn_match,},
	TRXN_START,
	{scratchin, NULL, 2, trxn_match,},
	{data, NULL, size, trxn_match,},
	TRXN_END,
    };
    struct transaction_log tcopy[] = {
	TRXN_START,
	{copy, NULL, 1, trxn_match,},
	TRXN_END,
    };

    s = Asize[page] - (offset & 0x1F);
    if (s > size)
	s = size;
    tscratch[2].size = tscratch[5].size = s;

    if (BUS_transaction(tscratch, pn) || memcmp(data, p, s)
	|| BUS_transaction(tcopy, pn))
	return 1;

    UT_delay(10);

    return 0;
}

static int OW_temp(_FLOAT * T, const struct parsedname *pn)
{
    BYTE d2[] = { 0xD2, };
    BYTE b2[] = { 0xB2, 0x60, };
    BYTE t[2];
    struct transaction_log tconvert[] = {
	TRXN_START,
	{d2, NULL, 1, trxn_match},
	TRXN_END,
    };
    struct transaction_log tdata[] = {
	TRXN_START,
	{b2, NULL, 2, trxn_match},
	{NULL, t, 3, trxn_read},
	TRXN_END,
    };

    // initiate conversion
    if (BUS_transaction(tconvert, pn))
	return 1;
    UT_delay(10);


    /* Get data */
    if (BUS_transaction(tdata, pn))
	return 1;

    // success
    //printf("Temp bytes %0.2X %0.2X\n",t[0],t[1]);
    //printf("temp int=%d\n",((int)((int8_t)t[1])));

    //T[0] = ((int)((int8_t)t[1])) + .00390625*t[0] ;
    T[0] = UT_int16(t) / 256.;
    return 0;
}

static int OW_volts(_FLOAT * V, const struct parsedname *pn)
{
    BYTE b4[] = { 0xB4, };
    BYTE b2[] = { 0xB2, 0x77, };
    BYTE v[2];
    struct transaction_log tconvert[] = {
	TRXN_START,
	{b4, NULL, 1, trxn_match},
	TRXN_END,
    };
    struct transaction_log tdata[] = {
	TRXN_START,
	{b2, NULL, 2, trxn_match},
	{NULL, v, 2, trxn_read},
	TRXN_END,
    };

    // initiate conversion
    if (BUS_transaction(tconvert, pn))
	return 1;
    UT_delay(10);

    /* Get data */
    if (BUS_transaction(tdata, pn))
	return 1;

    // success
    //V[0] = .01 * (_FLOAT)( ( ((uint32_t)v[1]) <<8 )|v[0] ) ;
    V[0] = .01 * (_FLOAT) (UT_uint16(v));
    return 0;
}
