/**
 * Inferno Embedded Firmware Updater
 * Copyright 2018 Inferno Embedded
 */

#define _1W_FIRMWARE_BOOTLOADER_SIZE	0x00
#define _1W_FIRMWARE_ERASE_APP			0x01
#define _1W_FIRMWARE_WRITE				0x02
#define _1W_FIRMWARE_READ				0x03
#define _1W_FIRMWARE_APPLICATION_RANGE	0x04
#define _1W_FIRMWARE_BOOT_APPLICATION	0x05
#define _1W_FIRMWARE_WRITE_BLOCK		0x06
#define _1W_FIRMWARE_CRC				0x07

#define STATUS_OPERATION_FAIL		1
#define STATUS_OPERATION_SUCCESS	2

#define BLOCK_SIZE 1024

#define WRITE_BLOCK_DELAY 20 // msec
#define ERASE_DELAY 50

typedef struct firmware_updater {
	uint32_t bootloader_size;
	uint32_t application_start;
	uint32_t application_end;
	uint32_t application_size;
} firmware_updater;

static firmware_updater *new_firmware_updater(void)
{
	firmware_updater *device = owmalloc(sizeof(firmware_updater));
	if (device == NULL) {
		return NULL;
	}

	device->bootloader_size = 0;
	device->application_start = 0;
	device->application_end = 0;
	device->application_size = 0;

	return device;
}

/**
 * Populate the Firmware Updater information from the device on the bus
 * @param pn the parsed device name
 * @param device a struct to populate
 */
static GOOD_OR_BAD OW_firmware_updater_info(const struct parsedname *pn, ie_device *device)
{
	BYTE write_string[] = {_1W_FIRMWARE_BOOTLOADER_SIZE, 0};
	BYTE buf[8];
	BYTE command_ok[1];
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log get_bootloader_size[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_READ1(command_ok),
		TRXN_READ4(buf),
		TRXN_END,
	};

	if (BAD(BUS_transaction(get_bootloader_size, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	uint32_t bootloader_size = read_uint32(buf);

	write_string[0] = _1W_FIRMWARE_APPLICATION_RANGE;
	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log get_application_range[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_READ1(command_ok),
		TRXN_READ8(buf),
		TRXN_END,
	};

	if (BAD(BUS_transaction(get_application_range, pn))) {
		return gbBAD;
	}

	uint32_t app_start = read_uint32(buf);
	uint32_t app_end = read_uint32(buf + 4);

	device->info = new_firmware_updater();

	if (NULL == device->info) {
		return gbBAD;
	}

	((firmware_updater *)(device->info))->bootloader_size = bootloader_size;
	((firmware_updater *)(device->info))->application_start = app_start;
	((firmware_updater *)(device->info))->application_end = app_end;
	((firmware_updater *)(device->info))->application_size = app_end - app_start;

	LEVEL_DEBUG("Bootloader size=%u app_start=0x%08x app_end=0x%08x app_size=%d",
			bootloader_size, app_start, app_end, app_end - app_start);

	return gbGOOD;
}

/**
 * Allow a path to be visible if we have an firmware updater device
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_firmware_device(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != FIRMWARE_UPDATER) {
		LEVEL_DEBUG("Not a a Firmware Updater (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	DEVICE_TERM(device);
	return visible_now;
}

/**
 * Get the start & end addresses of the firmware
 * @param owq the query
 */
static ZERO_OR_ERROR firmware_range(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	int len = snprintf(OWQ_buffer(owq), OWQ_size(owq), "0x%08x,0x%08x",
			((firmware_updater *)(device->info))->application_start,
			((firmware_updater *)(device->info))->application_end);

	LEVEL_DEBUG("Application range: %s", OWQ_buffer(owq));

	DEVICE_TERM(device);

	return len;
}

/**
 * Get the size of the bootloader
 * @param owq the query
 */
ZERO_OR_ERROR firmware_get_bootloader_size(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U(owq) = ((firmware_updater *)(device->info))->bootloader_size;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Erase the firmware (raw)
 * @param owq the query
 */
static GOOD_OR_BAD OW_firmware_erase(struct one_wire_query *owq)
{
	BYTE write_string[] = { _1W_FIRMWARE_ERASE_APP,  0};
	BYTE command_ok[1];
	BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_READ1(command_ok),
		TRXN_END,
	};

	RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	for (int retry = 0; retry < 32; retry++) {
		uint64_t status;

		sleep(1);

		RETURN_ERROR_IF_BAD(OW_ie_get_status(PN(owq), &status));

		LEVEL_DEBUG("Status=0x%016llx", status);

		if (status & (1 << STATUS_OPERATION_SUCCESS)) {
			LEVEL_DEBUG("Erase was successful");
			return gbGOOD;
		} else if (status & (1 << STATUS_OPERATION_FAIL)) {
			LEVEL_DEBUG("Erase failed");
			return gbBAD;
		}
	}

	LEVEL_DEBUG("Timeout waiting for firmware erase");

	return gbBAD;
}

/**
 * Erase the firmware if requested
 * @param owq the query
 */
static ZERO_OR_ERROR firmware_erase(struct one_wire_query *owq)
{
	if (OWQ_Y(owq)) {
		if (BAD(OW_firmware_erase(owq))) {
			return 1;
		}
	}
	return 0;
}

/**
 * Exit the bootloader and boot the application
 * @param owq the query
 */
static ZERO_OR_ERROR firmware_updater_exit(struct one_wire_query *owq)
{
	if (OWQ_Y(owq)) {
		BYTE write_string[] = { _1W_FIRMWARE_BOOT_APPLICATION,  0};
		BYTE command_ok[1];
		BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
		crc = CRC8compute(write_string, 1, crc);
		write_string[1] = crc;

		struct transaction_log t[] = {
			TRXN_START,
			TRXN_WRITE2(write_string),
			TRXN_READ1(command_ok),
			TRXN_END,
		};

		RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

		if (command_ok[0]) {
			LEVEL_DEBUG("Device reported bad command");
			return 1;
		}

#if OW_UTHASH
	ie_device *device;
	if (BAD(device_info(PN(owq), &device))) {
		return gbBAD;
	}
	invalidate_device(&device);
#endif
	}
	return 0;
}

/**
 * Update the application firmware
 * @param owq the query
 */
static ZERO_OR_ERROR firmware_update_binary(struct one_wire_query *owq)
{
	ie_device *device;
	BYTE write_string[] = { _1W_FIRMWARE_WRITE_BLOCK, 0, 0, 0, 0, /* 32 bit addr */ 0, 0, 0, 0, /* 32 bit size */ 0 /* CRC */};
	BYTE crc_string[] = { _1W_FIRMWARE_CRC, 0, 0, 0, 0, /* 32 bit addr */ 0, 0, 0, 0, /* 32 bit size */ 0 /* CRC */};
	BYTE read_back[1];
	BYTE crc_ok[1];
	BYTE device_firmware_crc[2];

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return -1;
	}


	BYTE *data = (BYTE *)OWQ_buffer(owq);
	uint32_t size = OWQ_size(owq);

	if (size > ((firmware_updater *)(device->info))->application_size) {
		LEVEL_DEBUG("Binary size %d exceeds available firmware space %d",
				size, ((firmware_updater *)(device->info))->application_size);
		DEVICE_TERM(device);
		return -1;
	}

	LEVEL_DEBUG("Erasing firmware");

	if (OW_firmware_erase(owq)) {
		DEVICE_TERM(device);
		return -1;
	}

	uint32_t start_address = ((firmware_updater *)(device->info))->application_start;

	LEVEL_DEBUG("Writing %d bytes to 0x%08x", size, start_address);

	for (uint32_t offset = 0; offset < size; offset += BLOCK_SIZE) {
		uint32_t increment = size - offset;
		if (increment > BLOCK_SIZE) {
			increment = BLOCK_SIZE;
		}

		LEVEL_DEBUG("Writing block of %d bytes to 0x%08x", increment, start_address + offset);

		write_uint32(start_address + offset, write_string + 1);
		write_uint32(increment, write_string + 1 + 4);

		BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
		crc = CRC8compute(write_string, 1 + 4 + 4, crc);
		write_string[1 + 4 + 4] = crc;

		uint16_t dataCRC16 = CRC16compute(data + offset, increment, 0);
		BYTE dataCRC[4];
		write_uint16(dataCRC16, dataCRC);
		dataCRC[2] = 0;
		dataCRC[3] = 0;

		struct transaction_log t[] = {
				TRXN_START,
				TRXN_WRITE(write_string, 1 + 4 + 4 + 1),
				TRXN_READ1(read_back),
				TRXN_WRITE(data + offset, increment),
				TRXN_WRITE4(dataCRC),
				TRXN_DELAY(1000),
				TRXN_READ1(crc_ok),
				TRXN_END,
		};

		RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

		if (read_back[0]) {
			LEVEL_DEBUG("Device reported bad command when flashing");
			DEVICE_TERM(device);
			return -1;
		}

		if (crc_ok[0]) {
			LEVEL_DEBUG("Device reported data CRC mismatch");
			DEVICE_TERM(device);
			return -1;
		}


		for (int retry = 0; retry < 32; retry++) {
			uint64_t status;

			usleep(100000);

			RETURN_ERROR_IF_BAD(OW_ie_get_status(PN(owq), &status));

			LEVEL_DEBUG("Status=0x%016llx", status);

			if (status & (1 << STATUS_OPERATION_SUCCESS)) {
				LEVEL_DEBUG("Write was successful");
				break;
			} else if (status & (1 << STATUS_OPERATION_FAIL)) {
				LEVEL_DEBUG("Write failed");
				DEVICE_TERM(device);
				return -1;
			}
		}

		sleep(1);
	}

	// Validate the CRC of the data written to the device
	uint16_t firmware_crc = CRC16compute(data, size, 0);

	write_uint32(start_address, crc_string + 1);
	write_uint32(size, crc_string + 1 + 4);
	BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
	crc = CRC8compute(crc_string, 1 + 4 + 4, crc);
	crc_string[1 + 4 + 4] = crc;

	struct transaction_log t[] = {
			TRXN_START,
			TRXN_WRITE(crc_string, 1 + 4 + 4 + 1),
			TRXN_READ1(read_back),
			TRXN_DELAY(1000),
			TRXN_READ2(device_firmware_crc),
			TRXN_END,
	};

	RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

	if (read_back[0]) {
		LEVEL_DEBUG("Device reported bad command");
		DEVICE_TERM(device);
		return -1;
	}

	uint16_t device_crc = read_uint16(device_firmware_crc);

	if (device_crc != firmware_crc) {
		LEVEL_DEBUG("Firmware CRC mismatch, %u != %u", firmware_crc, device_crc);
		DEVICE_TERM(device);
		return -1;
	}

	LEVEL_DEBUG("Firmware updated successfully");

	DEVICE_TERM(device);

	return 0;
}




