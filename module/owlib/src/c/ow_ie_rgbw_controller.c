/**
 * Inferno Embedded RGBW LED Controller
 * Copyright 2018 Inferno Embedded
 */


// RGBW Controller
typedef struct rgbw_channel {
	BYTE channel;
	BYTE red;
	BYTE green;
	BYTE blue;
	BYTE white;
	uint32_t fade_time;
} RGBW_CHANNEL;

typedef struct rgbw_controller {
	uint8_t channel_count;
} rgbw_controller;


#define _1W_RGBW_ALL_OFF 0x01
#define _1W_RGBW_COUNT_CHANNELS 0x02
#define _1W_RGBW_SET_CHANNEL 0x03
#define _1W_RGBW_GET_CHANNEL 0x04
#define _1W_RGBW_COMMIT 0x05

static rgbw_controller *new_rgbw_controller(void) {
	rgbw_controller *device = owmalloc(sizeof(rgbw_controller));
	if (device == NULL) {
		return NULL;
	}

	device->channel_count = 0;

	return device;
}

/**
 * Populate the RGBW Controller information from the device on the bus
 * @param pn the parsed device name
 * @param device a struct to populate
 */
static GOOD_OR_BAD OW_rgbw_controller_info(const struct parsedname *pn, ie_device *device)
{
	BYTE write_string[] = { _1W_RGBW_COUNT_CHANNELS,  0};
	BYTE read_back[2];
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_READ2(read_back),
		TRXN_END,
	};

	if (BAD(BUS_transaction(t, pn))) {
		return gbBAD;
	}

	if (read_back[0]) {
		return gbBAD;
	}

	uint8_t channel_count = read_back[1];
	device->info = new_rgbw_controller();

	if (NULL == device->info) {
		return gbBAD;
	}

	((rgbw_controller *)(device->info))->channel_count = channel_count;

	return gbGOOD;
}

/**
 * Allow a path to be visible if we have an RGBW device
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_rgbw_device(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != RGBW_CONTROLLER) {
		LEVEL_DEBUG("Not an RGBW Controller (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	DEVICE_TERM(device);
	return visible_now;
}

/**
 * Allow a path to be visible if we have an RGBW device and the channel is implemented
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_rgbw_channel(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != RGBW_CONTROLLER) {
		LEVEL_DEBUG("Not an RGBW Controller (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	uint8_t channel = pn->selected_filetype->data.u;
	if (channel < ((rgbw_controller *)(device->info))->channel_count) {
		DEVICE_TERM(device);
		return visible_now;
	}

	DEVICE_TERM(device);
	return visible_not_now;
}


static GOOD_OR_BAD OW_rgbw_all_off(const struct parsedname *pn)
{
	BYTE write_string[] = { _1W_RGBW_ALL_OFF,  0};
	BYTE read_back[1];
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_READ1(read_back),
		TRXN_END,
	};

	if (BAD(BUS_transaction(t, pn))) {
		return gbBAD;
	}

	if (read_back[0]) {
		return gbBAD;
	}

	return gbGOOD;
}

static GOOD_OR_BAD OW_rgbw_count_channels(BYTE *channel_count, const struct parsedname *pn)
{
	BYTE write_string[] = { _1W_RGBW_COUNT_CHANNELS,  0};
	BYTE read_back[2];
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(write_string),
		TRXN_READ2(read_back),
		TRXN_END,
	};

	if (BAD(BUS_transaction(t, pn))) {
		return gbBAD;
	}

	if (read_back[0]) {
		return gbBAD;
	}

	*channel_count = read_back[1];

	return gbGOOD;
}

static GOOD_OR_BAD OW_rgbw_get_channel(BYTE channel, char *buf, int buf_len, const struct parsedname *pn)
{
	BYTE write_string[] = { _1W_RGBW_GET_CHANNEL, channel, 0};
	BYTE read_back[8];
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 2, crc);
	write_string[2] = crc;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(write_string),
		TRXN_READ(read_back, 7),
		TRXN_END,
	};

	if (BAD(BUS_transaction(t, pn))) {
		return gbBAD;
	}

	if (read_back[0]) {
		return gbBAD;
	}

	{
		BYTE red = read_back[1];
		BYTE green = read_back[2];
		BYTE blue = read_back[3];
		BYTE white = read_back[4];
		uint32_t fade_time = read_back[5] + (read_back[6] << 8) + (read_back[7] << 16);

		snprintf(buf, buf_len, "%3d,%3d,%3d,%3d,%8d", red, green, blue, white, fade_time);
	}

	return gbGOOD;
}

static GOOD_OR_BAD OW_w_set_channel(RGBW_CHANNEL *channel, const struct parsedname *pn)
{
	BYTE write_string[] = { _1W_RGBW_SET_CHANNEL, channel->channel, channel->red, channel->green, channel->blue, channel->white,
			channel->fade_time & 0xFF, (channel->fade_time & 0xFF00) >> 8, (channel->fade_time & 0xFF0000) >> 16, 0};
	BYTE commit_string[] = {_1W_RGBW_COMMIT, 0};
	BYTE result[1];

	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 9, crc);
	write_string[9] = crc;

	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(commit_string, 1, crc);
	commit_string[1] = crc;

	struct transaction_log set_channel[] = {
		TRXN_START,
		TRXN_WRITE(write_string, 10),
		TRXN_READ1(result),
		TRXN_END,
	};

	if (BAD(BUS_transaction(set_channel, pn))) {
		LEVEL_DEBUG("Bad SET_CHANNEL transaction");
		return gbBAD;
	}

	if (result[0]) {
		LEVEL_DEBUG("Slave reports CRC error on SET_CHANNEL");
		return gbBAD;
	}

	struct transaction_log commit[] = {
		TRXN_START,
		TRXN_WRITE2(commit_string),
		TRXN_READ1(result),
		TRXN_END,
	};

	if (BAD(BUS_transaction(commit, pn))) {
		LEVEL_DEBUG("Bad COMMIT transaction");
		return gbBAD;
	}

	if (result[0]) {
		LEVEL_DEBUG("Slave reports CRC error on COMMIT");
		return gbBAD;
	}

	return gbGOOD;
}


static ZERO_OR_ERROR rgbw_all_off(struct one_wire_query *owq)
{
	if (OWQ_Y(owq)) {
		RETURN_ERROR_IF_BAD(OW_rgbw_all_off(PN(owq)));
	}
	return 0;
}

static ZERO_OR_ERROR rgbw_count_channels(struct one_wire_query *owq)
{
	BYTE channel_count;
	RETURN_ERROR_IF_BAD(OW_rgbw_count_channels(&channel_count, (PN(owq))));

	OWQ_U(owq) = channel_count;

	return 0;
}

static ZERO_OR_ERROR rgbw_get_channel(struct one_wire_query *owq)
{
	uint8_t channel = PN(owq)->selected_filetype->data.u;
	RETURN_ERROR_IF_BAD(OW_rgbw_get_channel(channel, OWQ_buffer(owq), OWQ_size(owq), (PN(owq))));

	return 0;
}

static GOOD_OR_BAD parse_rgbw_string(char *buf, RGBW_CHANNEL *channel) {

	if (sscanf(buf, "%hhu,%hhu,%hhu,%hhu,%d", &channel->red, &channel->green, &channel->blue, &channel->white, &channel->fade_time) != 5) {
		LEVEL_DEBUG("Parsing failed");
		return gbBAD;
	}

	if (channel->fade_time >= 16777216) {
		LEVEL_DEBUG("Excessive fade time, %d >= %d", channel->fade_time, 16777216);
		return gbBAD;
	}

	return gbGOOD;
}

static ZERO_OR_ERROR rgbw_set_channel(struct one_wire_query *owq)
{
	RGBW_CHANNEL channelData;

	channelData.channel = PN(owq)->selected_filetype->data.u;
	LEVEL_DEBUG("Channel=%d string='%s'", channelData.channel, OWQ_buffer(owq));
	RETURN_ERROR_IF_BAD(parse_rgbw_string(OWQ_buffer(owq), &channelData));
	RETURN_ERROR_IF_BAD(OW_w_set_channel(&channelData, (PN(owq))));

	return 0;
}


