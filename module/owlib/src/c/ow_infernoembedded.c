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

/* This should mirror SoftDeviceType in SoftDevice.h from Inferno Embedded */
typedef enum SoftDeviceType {
	UNKNOWN = 0x0000,
	RGBW_CONTROLLER = 0x0001,
	FIRMWARE_UPDATER = 0x0002,
} SoftDeviceType;

#if OW_UTHASH
#include "uthash.h"
#undef uthash_malloc
#undef uthash_free
#define uthash_malloc(sz) owmalloc(sz)
#define uthash_free(ptr,sz) owfree(ptr)

#endif


/* ------- Prototypes ----------- */

/* All Inferno Embedded Devices */
READ_FUNCTION(ie_get_device);
READ_FUNCTION(ie_get_version);
READ_FUNCTION(ie_get_status);
WRITE_FUNCTION(ie_boot_firmware_updater);

/* Inferno Embedded RGBW Controller */
VISIBLE_FUNCTION(is_visible_rgbw_device);
VISIBLE_FUNCTION(is_visible_rgbw_channel);
WRITE_FUNCTION(rgbw_all_off);
READ_FUNCTION(rgbw_count_channels);
READ_FUNCTION(rgbw_get_channel);
WRITE_FUNCTION(rgbw_set_channel);

/* Inferno Embedded Firmware Update */
VISIBLE_FUNCTION(is_visible_firmware_device);
READ_FUNCTION(firmware_range);
READ_FUNCTION(firmware_get_bootloader_size);
WRITE_FUNCTION(firmware_erase);
WRITE_FUNCTION(firmware_update_binary);
WRITE_FUNCTION(firmware_updater_exit);

/* ------- Structures ----------- */

static struct filetype InfernoEmbedded[] = {
	F_STANDARD,
	/* All Inferno Embedded devices */
	{"device", 64, NON_AGGREGATE, ft_vascii, fc_volatile, ie_get_device, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, ie_get_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"status", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, ie_get_status, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"enter_firmware_update", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, NO_READ_FUNCTION, ie_boot_firmware_updater, VISIBLE, NO_FILETYPE_DATA, },

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

	/* Inferno Embedded Firmware Updater */
	{"firmware_bootloader_size", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_unsigned, fc_volatile, firmware_get_bootloader_size, NO_WRITE_FUNCTION, is_visible_firmware_device, NO_FILETYPE_DATA, },
	{"firmware_range", 2+8+1+2+8+1 /* 0xval,0xval */, NON_AGGREGATE, ft_vascii, fc_volatile, firmware_range, NO_WRITE_FUNCTION, is_visible_firmware_device, NO_FILETYPE_DATA, },
	{"erase_firmware", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, firmware_erase, is_visible_firmware_device, NO_FILETYPE_DATA, },
	{"update_firmware", 64*1024, NON_AGGREGATE, ft_binary, fc_volatile, NO_READ_FUNCTION, firmware_update_binary, is_visible_firmware_device, NO_FILETYPE_DATA, },
	{"exit_firmware_update", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, firmware_updater_exit, is_visible_firmware_device, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(ED, InfernoEmbedded, DEV_resume, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_BOOT_FIRMWARE_UPDATER	0xFC
#define _1W_STATUS_REGISTER			0xFD
#define _1W_VERSION 				0xFE
#define _1W_DEVICE 					0xFF

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
 * A caller is done with a device (that has been malloced)
 * If we have a hash caching the data, don't do anything, otherwise we leave dangling pointers in the hash
 * If we don't have a hash, then free the device
 */
#if OW_UTHASH
#define DEVICE_TERM(__dev)
#else
#define DEVICE_TERM(__dev) free_device(&__dev)

/**
 * Free a device (and any device specific info)
 * @param device a pointer to the device pointer, we will clear the pointer
 */
static void free_device(ie_device **device) {
	owfree((*device)->info);
	owfree(*device);
	*device = NULL;
}
#endif

#if OW_UTHASH
/**
 * Invalidate a device (eg. if the device has been rebooted and will present as a different device)
 * @param device a pointer to the device pointer
 */
static void invalidate_device(ie_device **device) {
	HASH_DEL(device_cache, *device);
	free_device(device);
}
#endif

/**
 * Convert a little endian byte array to a uint16_t
 * @param buf the byte array to convert (must be at least 2 bytes long)
 * @return the converted value
 */
static inline uint16_t read_uint16(BYTE *buf) {
	uint16_t val = 0;

	for (int i = 0; i < 2; i++) {
		val >>= 8;
		val |= buf[i] << 8;
	}

	return val;
}

/**
 * Convert a little endian byte array to a uint32_t
 * @param buf the byte array to convert (must be at least 4 bytes long)
 * @return the converted value
 */
static inline uint32_t read_uint32(BYTE *buf) {
	uint32_t val = 0;

	for (int i = 0; i < 4; i++) {
		val >>= 8;
		val |= buf[i] << 24;
	}

	return val;
}

/**
 * Convert a little endian byte array to a uint64_t
 * @param buf the byte array to convert (must be at least 8 bytes long)
 * @return the converted value
 */
static inline uint64_t read_uint64(BYTE *buf) {
	uint64_t val = 0;

	for (int i = 0; i < 8; i++) {
		val >>= 8;
		val |= ((uint64_t)buf[i]) << 56;
	}

	return val;
}

/**
 * Convert a uint16_t to a little endian byte array
 * @param val the value to convert
 * @param buf the buffer to write to (must be at least 2 bytes long)
 */
static inline void write_uint16(uint16_t val, BYTE *data) {
	data[0] = (val & 0x000000FF);
	data[1] = (val & 0x0000FF00) >> 8;
}

/**
 * Convert a uint32_t to a little endian byte array
 * @param val the value to convert
 * @param buf the buffer to write to (must be at least 4 bytes long)
 */
static inline void write_uint32(uint32_t val, BYTE *data) {
	data[0] = (val & 0x000000FF);
	data[1] = (val & 0x0000FF00) >> 8;
	data[2] = (val & 0x00FF0000) >> 16;
	data[3] = (val & 0xFF000000) >> 24;
}

static GOOD_OR_BAD OW_rgbw_controller_info(const struct parsedname *pn, ie_device *device);
static GOOD_OR_BAD OW_firmware_updater_info(const struct parsedname *pn, ie_device *device);

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

	device->device = read_uint32(buf);

	// Get the device version
	write_string[0] = _1W_VERSION;

	if (BAD(BUS_transaction(txn, pn))) {
		return gbBAD;
	}

	device->version = read_uint32(buf);

	// Populate device specific info
	switch (device->device) {
	case RGBW_CONTROLLER:
		return OW_rgbw_controller_info(pn, device);
		break;

	case FIRMWARE_UPDATER:
		return OW_firmware_updater_info(pn, device);
		break;

	default:
		LEVEL_DEBUG("Unknown device type %ld", device->device);
		return gbBAD;
	}
}

static GOOD_OR_BAD OW_ie_get_status(const struct parsedname *pn, uint64_t *status)
{
	BYTE write_string[] = { _1W_STATUS_REGISTER};
	BYTE buf[8];

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(write_string),
		TRXN_READ(buf, 8),
		TRXN_END,
	};

	if (BAD(BUS_transaction(t, pn))) {
		LEVEL_DEBUG("Status transaction failed");
		return gbBAD;
	}

	*status = read_uint64(buf);

	return gbGOOD;
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

/**
 * Reboot the device into the firmware updater
 * @param pn the parsed device name
 */
static GOOD_OR_BAD OW_ie_boot_firmware_updater(const struct parsedname *pn)
{
	BYTE write_string[] = { _1W_BOOT_FIRMWARE_UPDATER,  0};
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_END,
	};

	if (BAD(BUS_transaction(t, pn))) {
		return gbBAD;
	}

#if OW_UTHASH
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		return gbBAD;
	}
	invalidate_device(&device);
#endif

	return gbGOOD;
}

/**
 * Get the name of the device
 * @param owq the query
 */
static ZERO_OR_ERROR ie_get_device(struct one_wire_query *owq)
{
	ie_device *device;
	int ret = 0;
	int len = 0;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch (device->device) {
	case RGBW_CONTROLLER:
		len = snprintf(OWQ_buffer(owq), OWQ_size(owq), "Inferno Embedded RGBW Controller");
		break;
	case FIRMWARE_UPDATER:
		len = snprintf(OWQ_buffer(owq), OWQ_size(owq), "Inferno Embedded Firmware Updater");
		break;
	default:
		len = snprintf(OWQ_buffer(owq), OWQ_size(owq), "Unknown - is your OWFS install up to date?");
		ret = 1;
	}

	memset(OWQ_buffer(owq)+ len, '\0', OWQ_size(owq) - len);
	DEVICE_TERM(device);

	return ret;
}

/**
 * Get the version of the device
 * @param owq the query
 */
static ZERO_OR_ERROR ie_get_version(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U(owq) = device->version;
	return 0;
}

/**
 * Get the 64bit status register from the device
 * @param owq the query
 */
static ZERO_OR_ERROR ie_get_status(struct one_wire_query *owq)
{
	uint64_t status;
	RETURN_ERROR_IF_BAD(OW_ie_get_status(PN(owq), &status));

	OWQ_U(owq) = status;
	LEVEL_DEBUG("Device status is 0x%016llx", status);

	return 0;
}

/**
 * Reboot the device into the firmware updater
 * @param owq the query
 */
static ZERO_OR_ERROR ie_boot_firmware_updater(struct one_wire_query *owq)
{
	if (OWQ_Y(owq)) {
		RETURN_ERROR_IF_BAD(OW_ie_boot_firmware_updater(PN(owq)));
	}
	return 0;
}

#include "ow_ie_rgbw_controller.c"
#include "ow_ie_firmware_updater.c"
#include "ow_ie_switch_master.c"
