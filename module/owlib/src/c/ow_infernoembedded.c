/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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
#include "ow_infernoembedded.h"

// This should mirror SoftDeviceType in SoftDevice.h from Inferno Embedded
typedef enum SoftDeviceType {
	UNKNOWN = 0x0000,
	RGBW_CONTROLLER = 0x0001,
} SoftDeviceType;

#if OW_UTHASH
#include "uthash.h"
#undef uthash_malloc
#undef uthash_free
#define uthash_malloc(sz) owmalloc(sz)
#define uthash_free(ptr,sz) owfree(ptr)

#endif


/* ------- Prototypes ----------- */

/* Inferno Embedded RGBW Controller */
VISIBLE_FUNCTION(is_visible_rgbw_device);
VISIBLE_FUNCTION(is_visible_rgbw_channel);
WRITE_FUNCTION(rgbw_all_off);
READ_FUNCTION(rgbw_count_channels);
READ_FUNCTION(rgbw_get_channel);
WRITE_FUNCTION(rgbw_set_channel);


/* ------- Structures ----------- */

static struct filetype InfernoEmbedded[] = {
	F_STANDARD,
	/* Inferno Embedded RGBW Controller */
	{"rgbw_all_off", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, NO_READ_FUNCTION, rgbw_all_off, is_visible_rgbw_device, NO_FILETYPE_DATA, },
	{"rgbw_channels", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_unsigned, fc_static, rgbw_count_channels, NO_WRITE_FUNCTION, is_visible_rgbw_device, NO_FILETYPE_DATA, },
	{"rgbw_channel0", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 0}, },
	{"rgbw_channel1", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 1}, },
	{"rgbw_channel2", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 2}, },
	{"rgbw_channel3", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 3}, },
	{"rgbw_channel4", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 4}, },
	{"rgbw_channel5", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 5}, },
	{"rgbw_channel6", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 6}, },
	{"rgbw_channel7", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 7}, },
	{"rgbw_channel8", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 8}, },
	{"rgbw_channel9", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 9}, },
	{"rgbw_channel10", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 10}, },
	{"rgbw_channel11", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 11}, },
	{"rgbw_channel12", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 12}, },
	{"rgbw_channel13", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 13}, },
	{"rgbw_channel14", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 14}, },
	{"rgbw_channel15", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 15}, },
	{"rgbw_channel16", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 16}, },
	{"rgbw_channel17", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 17}, },
	{"rgbw_channel18", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 18}, },
	{"rgbw_channel19", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 19}, },
	{"rgbw_channel20", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 20}, },
	{"rgbw_channel21", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 21}, },
	{"rgbw_channel22", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 22}, },
	{"rgbw_channel23", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 23}, },
	{"rgbw_channel24", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 24}, },
	{"rgbw_channel25", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 25}, },
	{"rgbw_channel26", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 26}, },
	{"rgbw_channel27", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 27}, },
	{"rgbw_channel28", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 28}, },
	{"rgbw_channel29", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 29}, },
	{"rgbw_channel30", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 30}, },
	{"rgbw_channel31", 3+1+3+1+3+1+3+1+8+1 /* "r,g,b,w,time" */, NON_AGGREGATE, ft_vascii, fc_volatile, rgbw_get_channel, rgbw_set_channel, is_visible_rgbw_channel, {.u = 31}, },
};

DeviceEntryExtended(ED, InfernoEmbedded, DEV_resume, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_VERSION 0xFE
#define _1W_DEVICE 0xFF

typedef struct ie_device {
	BYTE address[SERIAL_NUMBER_SIZE]; /* key */
    SoftDeviceType device;
    uint64_t version;

    void *info; /* device specific info */
#if OW_UTHASH
    UT_hash_handle hh; /* makes this structure hashable */
#endif
} ie_device;

#if OW_UTHASH
ie_device *device_cache = NULL;
#endif

/**
 * Allocate space for a new device
 * @return a pointer to the device, or NULL if allocation failed
 */
static ie_device *new_device(void) {
	ie_device *device = owmalloc(sizeof(ie_device));
	if (device == NULL) {
		return NULL;
	}

	memset(device->address, 0, sizeof(device->address));
	device->device = UNKNOWN;
	device->info = NULL;

	return device;
}

/**
 * Free a device (and any device specific info)
 * @param device a pointer to the device pointer, we will clear the pointer
 */
static void free_device(ie_device **device) {
	owfree((*device)->info);
	owfree(*device);
	*device = NULL;
}


/**
 * A caller is done with a device (that has been malloced)
 * If we have a hash caching the data, don't do anything, otherwise we leave dangling pointers in the hash
 * If we don't have a hash, then free the device
 */
#if OW_UTHASH
#define DEVICE_TERM(__dev)
#else
#define DEVICE_TERM(__dev) free_device(&__dev)
#endif

static GOOD_OR_BAD OW_rgbw_controller_info(const struct parsedname *pn, ie_device *device);

/**
 * Populate the device information from the device on the bus
 * @param pn the parsed device name
 * @param device a struct to populate
 */
static GOOD_OR_BAD OW_device_info(const struct parsedname *pn, ie_device *device)
{
	BYTE write_string[] = { _1W_DEVICE};
	BYTE buf[4];

	memcpy(device->address, pn->sn, SERIAL_NUMBER_SIZE);

	// Get the device type
	struct transaction_log txn[] = {
		TRXN_START,
		TRXN_WRITE1(write_string),
		TRXN_READ(buf, 4),
		TRXN_END,
	};

	if (BAD(BUS_transaction(txn, pn))) {
		return gbBAD;
	}

	uint64_t val = 0;

	for (int i = 0; i < 4; i++) {
		LEVEL_DEBUG("buf[%d]=%d", i, buf[i]);
		val >>= 8;
		val |= buf[i] << 24;
	}
	LEVEL_DEBUG("val='%l'", val);

	device->device = val;

	// Get the device version
	write_string[0] = _1W_VERSION;

	if (BAD(BUS_transaction(txn, pn))) {
		return gbBAD;
	}

	for (int i = 0; i < 4; i++) {
		val >>= 8;
		val |= buf[i] << 24;
	}

	device->version = val;

	// Populate device specific info
	switch (device->device) {
	case RGBW_CONTROLLER:
		return OW_rgbw_controller_info(pn, device);
		break;

	default:
		LEVEL_DEBUG("Unknown device type %ld", device->device);
		return gbBAD;
	}
}

/**
 * Get metadata for a device
 * @param pn the parsed device name
 * @param device a pointer to a device that we will set (must be terminated with DEVICE_TERM)
 * @return bad on error
 */
static GOOD_OR_BAD device_info(const struct parsedname *pn, ie_device **device)
{
	*device = NULL;

	// Check the hash first
#if OW_UTHASH
	HASH_FIND(hh, device_cache, pn->sn, SERIAL_NUMBER_SIZE, *device);
#endif

	if (*device) {
		LEVEL_DEBUG("Found device in hash");
		// Found, all good
		return gbGOOD;
	}


	// Not found, better fetch it from the bus
	*device = new_device();
	if (*device == NULL) {
		LEVEL_DEBUG("Could not allocate new Inferno Embedded device");
	}

	if (BAD(OW_device_info(pn, *device))) {
		DEVICE_TERM(*device);
		return gbBAD;
	}

#if OW_UTHASH
	LEVEL_DEBUG("Storing device in hash");
	HASH_ADD( hh, device_cache, address, SERIAL_NUMBER_SIZE, *device);
#endif

	return gbGOOD;
}


#include "ow_ie_rgbw_controller.c"
