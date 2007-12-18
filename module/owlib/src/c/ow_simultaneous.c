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
READ_FUNCTION(FS_r_convert);
WRITE_FUNCTION(FS_w_convert);
READ_FUNCTION(FS_r_present);
READ_FUNCTION(FS_r_single);

/* -------- Structures ---------- */
struct filetype simultaneous[] = {
  {"temperature",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_r_convert, FS_w_convert, {i:simul_temp},} ,
  {"voltage",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_r_convert, FS_w_convert, {i:simul_volt},} ,
  {"present",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_r_present, NO_WRITE_FUNCTION, {i:0},} ,
  {"present_ds2400",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,   FS_r_present, NO_WRITE_FUNCTION, {i:1},} ,
  {"single", 18, NULL, ft_ascii, fc_volatile,   FS_r_single, NO_WRITE_FUNCTION, {i:0},} ,
  {"single_ds2400", 18, NULL, ft_ascii, fc_volatile,   FS_r_single, NO_WRITE_FUNCTION, {i:1},} ,
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

static int FS_w_convert(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    struct parsedname struct_pn2 ;
    struct parsedname * pn2 = & struct_pn2 ;
    
    enum simul_type type = (enum simul_type) pn->selected_filetype->data.i;
	memcpy(pn2, pn, sizeof(struct parsedname));	// shallow copy
	FS_LoadPath(pn2->sn, pn2);
	pn2->selected_device = NULL;				/* only branch select done, not actual device */
	/* Since writing to /simultaneous/temperature is done recursive to all
	 * adapters, we have to fake a successful write even if it's detected
	 * as a bad adapter. */
	Cache_Del_Internal(&ipSimul[type], pn2);	// remove existing entry
	CookTheCache();				// make sure all volatile entries are invalidated
    if (OWQ_Y(owq) == 0)
		return 0;				// don't send convert
	if (pn->selected_connection->Adapter != adapter_Bad) {
		int ret = 1;			// set just to block compiler errors
		switch (type) {
		case simul_temp:{
            const BYTE cmd_temp[] = { _1W_SKIP_ROM, 0x44 };
				struct transaction_log t[] = {
					TRXN_START,
                    TRXN_WRITE2( cmd_temp ),
					TRXN_END,
				};
				BUSLOCK(pn2);
				ret = BUS_transaction_nolock(t, pn2)
					|| FS_poll_convert(pn2);
				BUSUNLOCK(pn2);
				//printf("CONVERT (simultaneous temp) ret=%d\n",ret) ;
			}
			break;
		case simul_volt:{
            BYTE cmd_volt[] = { _1W_SKIP_ROM, 0x3C, 0x0F, 0x00, 0xFF, 0xFF };
				struct transaction_log t[] = {
					TRXN_START,
                    TRXN_WR_CRC16( cmd_volt, 4, 0 ),
                    TRXN_DELAY( 5 ),
					TRXN_END,
				};
				ret = BUS_transaction(t, pn2);
				//printf("CONVERT (simultaneous volt) ret=%d\n",ret) ;
			}
			break;
		}
		if (ret == 0)
			Cache_Add_Internal(NULL, 0, &ipSimul[type], pn2);
	}
	return 0;
}

static int FS_r_convert(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    struct parsedname struct_pn2 ;
    struct parsedname * pn2 = & struct_pn2 ;
    
	memcpy(pn2, pn, sizeof(struct parsedname));	// shallow copy
	FS_LoadPath(pn2->sn, pn2);
    OWQ_Y(owq) =
		(Cache_Get_Internal_Strict(NULL, 0, &ipSimul[pn->selected_filetype->data.i], pn2)
		 == 0);
	return 0;
}

static int FS_r_present(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    if (pn->selected_connection->Adapter == adapter_fake) {	// fake adapter -- simple memory look
        OWQ_Y(owq) = (pn->selected_connection->connin.fake.db.devices > 0);
	} else {					// real adapter
        struct parsedname struct_pn2 ;
        struct parsedname * pn2 = & struct_pn2 ;
        BYTE read_ROM[] = { _1W_READ_ROM, };
		BYTE resp[8];
		BYTE match[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
		struct transaction_log t[] = {
			TRXN_START,
            TRXN_WRITE1( read_ROM),
            TRXN_READ( resp, 8 ),
            TRXN_END,
		};

		/* check if DS2400 compatibility is needed */
		if (pn->selected_filetype->data.i)
			read_ROM[0] = 0x0F;

		memcpy(pn2, pn, sizeof(struct parsedname));	// shallow copy
		FS_LoadPath(pn2->sn, pn2);
		pn2->selected_device = NULL;			// directory only
		if (BUS_transaction(t, pn2))
			return -EINVAL;
		if (memcmp(resp, match, 8)) {	// some device(s) complained
            OWQ_Y(owq) = 1;			// YES present
			if (CRC8(resp, 8))
				return 0;		// crc8 error -- more than one device
			OW_single2cache(resp, pn2);
		} else {				// no devices
            OWQ_Y(owq) = 0;
		}
	}
	return 0;
}

static int FS_r_single(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    ASCII ad[30] = { 0x00, };	// long enough -- default "blank"
	BYTE resp[8];
	if (pn->selected_connection->Adapter == adapter_fake) {	// fake adapter -- look in memory
		if (pn->selected_connection->connin.fake.db.devices == 1) {
			DirblobGet(0, resp, &(pn->selected_connection->connin.fake.db));
			FS_devicename(ad, sizeof(ad), resp, pn);
		}
	} else {					// real adapter
        struct parsedname struct_pn2 ;
        struct parsedname * pn2 = & struct_pn2 ;
        BYTE read_ROM[] = { _1W_READ_ROM, };
		BYTE match[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
		struct transaction_log t[] = {
			TRXN_START,
            TRXN_WRITE1( read_ROM),
            TRXN_READ( resp, 8 ),
			TRXN_END,
		};

		/* check if DS2400 compatibility is needed */
		if (pn->selected_filetype->data.i)
			read_ROM[0] = 0x0F;

		memcpy(pn2, pn, sizeof(struct parsedname));	// shallow copy
		FS_LoadPath(pn2->sn, pn2);
		pn2->selected_device = NULL;			// directory only
		if (BUS_transaction(t, pn2))
			return -EINVAL;
		LEVEL_DEBUG("FS_r_single (simultaneous) dat=" SNformat
					" crc8c=%02x\n", SNvar(resp), CRC8(resp, 7));
		if ((memcmp(resp, match, 8) != 0) && (CRC8(resp, 8) == 0)) {	// non-empty, and no CRC error
			OW_single2cache(resp, pn2);
			/* Return device id. */
			FS_devicename(ad, sizeof(ad), resp, pn);
		}
	}
    Fowq_output_offset_and_size_z(ad, owq) ;
    return 0 ;
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
	Cache_Add_Device(pn2->selected_connection->index, pn2);	// Device cache
}
