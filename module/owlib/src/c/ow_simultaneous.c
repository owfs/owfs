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

/* Simultaneous is a trigger to do a mass conversion on all the devices in the specified path */

/* Added "present" From Jan Kandziora to search for any devices */

#include <config.h>
#include "owfs_config.h"
#include "ow_simultaneous.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
yREAD_FUNCTION(FS_r_convert);
yWRITE_FUNCTION(FS_w_convert);
yREAD_FUNCTION(FS_r_present);
aREAD_FUNCTION(FS_r_single);

/* -------- Structures ---------- */
struct filetype simultaneous[] = {
  {"temperature", 1, NULL, ft_yesno, fc_volatile, {y: FS_r_convert}, {y: FS_w_convert}, {i:simul_temp},},
  {"voltage", 1, NULL, ft_yesno, fc_volatile, {y: FS_r_convert}, {y: FS_w_convert}, {i:simul_volt},},
  {"present", 1, NULL, ft_yesno, fc_volatile, {y: FS_r_present}, {v: NULL}, {i:0},},
  {"present_ds2400", 1, NULL, ft_yesno, fc_volatile, {y: FS_r_present}, {v: NULL}, {i:1},},
  {"single", 18, NULL, ft_ascii, fc_volatile, {a: FS_r_single}, {v: NULL}, {i:0},},
  {"single_ds2400", 18, NULL, ft_ascii, fc_volatile, {a: FS_r_single}, {v: NULL}, {i:1},},
};

DeviceEntry(simultaneous, simultaneous);

/* ------- Functions ------------ */
static void OW_single2cache(BYTE * sn, const struct parsedname *pn2);

struct internal_prop ipSimul[] = {
	{"temperature", fc_volatile},
	{"voltage", fc_volatile},
};

/* returns 0 if valid conversion exists */
int Simul_Test(const enum simul_type type, const struct parsedname *pn)
{
	struct parsedname pn2;
	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
	FS_LoadPath(pn2.sn, &pn2);
	if (Cache_Get_Internal_Strict(NULL, 0, &ipSimul[type], &pn2)) {
		LEVEL_DEBUG("No simultaneous conversion valid.\n");
		return 1;
	}
	LEVEL_DEBUG("Simultaneous conversion IS valid.\n");
	return 0;
}

static int FS_w_convert(const int *y, const struct parsedname *pn)
{
	struct parsedname pn2;
	enum simul_type type = (enum simul_type) pn->ft->data.i;
	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
	FS_LoadPath(pn2.sn, &pn2);
	pn2.dev = NULL;				/* only branch select done, not actual device */
	/* Since writing to /simultaneous/temperature is done recursive to all
	 * adapters, we have to fake a successful write even if it's detected
	 * as a bad adapter. */
	Cache_Del_Internal(&ipSimul[type], &pn2);	// remove existing entry
	CookTheCache();				// make sure all volatile entries are invalidated
	if (y[0] == 0)
		return 0;				// don't send convert
	if (pn->in->Adapter != adapter_Bad) {
		int ret = 1;			// set just to block compiler errors
		switch (type) {
		case simul_temp:{
				const BYTE cmd_temp[] = { 0xCC, 0x44 };
				struct transaction_log t[] = {
					TRXN_START,
					{cmd_temp, NULL, 2, trxn_match,},
					TRXN_END,
				};
				BUSLOCK(&pn2);
				ret = BUS_transaction_nolock(t, &pn2)
					|| FS_poll_convert(&pn2);
				BUSUNLOCK(&pn2);
				//printf("CONVERT (simultaneous temp) ret=%d\n",ret) ;
			}
			break;
		case simul_volt:{
				BYTE cmd_volt[] = { 0xCC, 0x3C, 0x0F, 0x00, 0xFF, 0xFF };
				struct transaction_log t[] = {
					TRXN_START,
					{cmd_volt, NULL, 4, trxn_match,},
					{NULL, &cmd_volt[4], 2, trxn_read,},
					{&cmd_volt[1], NULL, 5, trxn_crc16,},
					{NULL, NULL, 5, trxn_delay},
					TRXN_END,
				};
				ret = BUS_transaction(t, &pn2);
				//printf("CONVERT (simultaneous volt) ret=%d\n",ret) ;
			}
			break;
		}
		if (ret == 0)
			Cache_Add_Internal(NULL, 0, &ipSimul[type], &pn2);
	}
	return 0;
}

static int FS_r_convert(int *y, const struct parsedname *pn)
{
	struct parsedname pn2;
	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
	FS_LoadPath(pn2.sn, &pn2);
	y[0] =
		(Cache_Get_Internal_Strict(NULL, 0, &ipSimul[pn->ft->data.i], &pn2)
		 == 0);
	return 0;
}

static int FS_r_present(int *y, const struct parsedname *pn)
{
	if (pn->in->Adapter == adapter_fake) {	// fake adapter -- simple memory look
		y[0] = (pn->in->connin.fake.db.devices > 0);
	} else {					// real adapter
		struct parsedname pn2;
		BYTE read_ROM[] = { 0x33, };
		BYTE resp[8];
		BYTE match[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
		struct transaction_log t[] = {
			TRXN_START,
			{read_ROM, NULL, 1, trxn_match,},
			{NULL, resp, 8, trxn_read,},
			TRXN_END,
		};

		/* check if DS2400 compatibility is needed */
		if (pn->ft->data.i)
			read_ROM[0] = 0x0F;

		memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
		FS_LoadPath(pn2.sn, &pn2);
		pn2.dev = NULL;			// directory only
		if (BUS_transaction(t, &pn2))
			return -EINVAL;
		if (memcmp(resp, match, 8)) {	// some device(s) complained
			y[0] = 1;			// YES present
			if (CRC8(resp, 8))
				return 0;		// crc8 error -- more than one device
			OW_single2cache(resp, &pn2);
		} else {				// no devices
			y[0] = 0;
		}
	}
	return 0;
}

static int FS_r_single(char *buf, const size_t size, const off_t offset,
					   const struct parsedname *pn)
{
	ASCII ad[30] = { 0x00, };	// long enough -- default "blank"
	BYTE resp[8];
	if (pn->in->Adapter == adapter_fake) {	// fake adapter -- look in memory
		if (pn->in->connin.fake.db.devices == 1) {
			DirblobGet(0, resp, &(pn->in->connin.fake.db));
			FS_devicename(ad, sizeof(ad), resp, pn);
		}
	} else {					// real adapter
		struct parsedname pn2;
		BYTE read_ROM[] = { 0x33, };
		BYTE match[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
		struct transaction_log t[] = {
			TRXN_START,
			{read_ROM, NULL, 1, trxn_match,},
			{NULL, resp, 8, trxn_read,},
			TRXN_END,
		};

		/* check if DS2400 compatibility is needed */
		if (pn->ft->data.i)
			read_ROM[0] = 0x0F;

		memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
		FS_LoadPath(pn2.sn, &pn2);
		pn2.dev = NULL;			// directory only
		if (BUS_transaction(t, &pn2))
			return -EINVAL;
		LEVEL_DEBUG("FS_r_single (simultaneous) dat=" SNformat
					" crc8c=%02x\n", SNvar(resp), CRC8(resp, 7));
		if ((memcmp(resp, match, 8) != 0) && (CRC8(resp, 8) == 0)) {	// non-empty, and no CRC error
			OW_single2cache(resp, &pn2);
			/* Return device id. */
			FS_devicename(ad, sizeof(ad), resp, pn);
		}
	}
	return FS_output_ascii_z(buf, size, offset, ad);
}

// called with pn copy
// Do cache for single item
static void OW_single2cache(BYTE * sn, const struct parsedname *pn2)
{
	struct dirblob db;
	DirblobInit(&db);
	DirblobAdd(sn, &db);
	if (DirblobPure(&db))
		Cache_Add_Dir(&db, pn2);	// Directory cache
	memcpy(pn2->sn, sn, 8);
	Cache_Add_Device(pn2->in->index, pn2);	// Device cache
}
