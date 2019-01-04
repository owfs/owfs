/**
 * Inferno Embedded Switch Master
 * Copyright 2018 Inferno Embedded
 */

#define _1W_SWITCH_MASTER_COUNT_SWITCHES		0x01
#define _1W_SWITCH_MASTER_COUNT_LEDS			0x02
#define _1W_SWITCH_MASTER_COUNT_RELAYS			0x03
#define _1W_SWITCH_MASTER_SWITCH_TYPE			0x04
#define _1W_SWITCH_MASTER_GET_SWITCH_STATE		0x05
#define _1W_SWITCH_MASTER_GET_ACTIVATED_STATE	0x06
#define _1W_SWITCH_MASTER_SET_LED				0x07
#define _1W_SWITCH_MASTER_GET_LED				0x08
#define _1W_SWITCH_MASTER_SET_RELAY_MODE		0x09
#define _1W_SWITCH_MASTER_SET_RELAY_TIMEOUT		0x0A
#define _1W_SWITCH_MASTER_SET_RELAY_STATE		0x0B
#define _1W_SWITCH_MASTER_GET_RELAY_STATES		0x0C
#define _1W_SWITCH_MASTER_GET_RELAY_MODES		0x0D
#define _1W_SWITCH_MASTER_GET_RELAY_TIMEOUTS	0x0E

#define STATUS_HAS_ACTIVATED_SWITCHES	1

#define MAX_PORTS 8
#define MAX_CHANNELS 64

typedef struct relay {
	uint8_t mode;
	uint8_t state;
	uint8_t timeout;
} relay;


typedef struct switch_master {
	uint8_t switch_port_count;
	uint8_t switch_port_channels;
	uint8_t led_port_count;
	uint8_t led_port_channels;
	uint8_t relay_port_count;
	uint8_t relay_port_channels;

	uint64_t switch_activations[MAX_PORTS];
	uint64_t last_led_state[MAX_PORTS];
	relay relays[MAX_PORTS][MAX_CHANNELS];
} switch_master;

static switch_master *new_switch_master(void)
{
	switch_master *device = owmalloc(sizeof(switch_master));
	if (device == NULL) {
		return NULL;
	}

	device->switch_port_count = 0;
	device->switch_port_channels = 0;
	device->led_port_count = 0;
	device->led_port_channels = 0;
	device->relay_port_count = 0;
	device->relay_port_channels = 0;

	for (uint8_t led_port = 0; led_port < MAX_PORTS; led_port++) {
		device->last_led_state[led_port] = 0;
	}

	for (uint8_t switch_port = 0; switch_port < MAX_PORTS; switch_port++) {
		device->switch_activations[switch_port] = 0;
	}

	for (uint8_t relay_port = 0; relay_port < MAX_PORTS; relay_port++) {
		for (uint8_t relay_channel = 0; relay_channel < MAX_PORTS;
				relay_channel++) {
			device->relays[relay_port][relay_channel].mode = 0;
			device->relays[relay_port][relay_channel].state = 0;
			device->relays[relay_port][relay_channel].timeout = 0;
		}
	}

	return device;
}

/**
 * Populate the Switch Master information from the device on the bus
 * @param pn the parsed device name
 * @param device a struct to populate
 */
static GOOD_OR_BAD OW_switch_master_info(const struct parsedname *pn, ie_device *device)
{
	device->info = new_switch_master();

	if (NULL == device->info) {
		return gbBAD;
	}

	BYTE write_string[] = { _1W_SWITCH_MASTER_COUNT_SWITCHES, 0, 0 };
	BYTE buf[MAX_PORTS * MAX_CHANNELS];
	BYTE command_ok[1];
	BYTE crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	struct transaction_log count[] = {
			TRXN_START,
			TRXN_WRITE2(write_string),
			TRXN_READ1(command_ok),
			TRXN_READ2(buf),
			TRXN_END,
	};

	if (BAD(BUS_transaction(count, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	uint8_t switch_port_count = buf[0];
	uint8_t switch_channel_count = buf[1];

	write_string[0] = _1W_SWITCH_MASTER_COUNT_LEDS;
	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	if (BAD(BUS_transaction(count, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	uint8_t led_port_count = buf[0];
	uint8_t led_channel_count = buf[1];

	write_string[0] = _1W_SWITCH_MASTER_COUNT_RELAYS;
	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	if (BAD(BUS_transaction(count, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	uint8_t relay_port_count = buf[0];
	uint8_t relay_channel_count = buf[1];

	if (switch_port_count > MAX_PORTS) {
		switch_port_count = MAX_PORTS;
	}

	if (led_port_count > MAX_PORTS) {
		led_port_count = MAX_PORTS;
	}

	if (relay_port_count > MAX_PORTS) {
		relay_port_count = MAX_PORTS;
	}

	uint8_t led_bytes = (led_channel_count + 7) / 8;

	((switch_master *) (device->info))->switch_port_count = switch_port_count;
	((switch_master *) (device->info))->switch_port_channels =
			switch_channel_count;
	((switch_master *) (device->info))->led_port_count = led_port_count;
	((switch_master *) (device->info))->led_port_channels = led_channel_count;
	((switch_master *) (device->info))->relay_port_count = relay_port_count;
	((switch_master *) (device->info))->relay_port_channels =
			relay_channel_count;

	for (uint8_t led_port = 0; led_port < led_port_count; led_port++) {
		write_string[0] = _1W_SWITCH_MASTER_GET_LED;
		write_string[1] = led_port;
		crc = CRC8compute(pn->sn, 8, 0);
		crc = CRC8compute(write_string, 2, crc);
		write_string[2] = crc;

		memset(buf, 0, sizeof(buf));

		struct transaction_log txn_led_port[] = {
				TRXN_START,
				TRXN_WRITE3(write_string),
				TRXN_READ1(command_ok),
				TRXN_READ(buf, led_bytes),
				TRXN_END,
		};

		if (BAD(BUS_transaction(txn_led_port, pn))) {
			return gbBAD;
		}

		if (command_ok[0]) {
			LEVEL_DEBUG("Device reported bad command");
			return gbBAD;
		}

		((switch_master *) (device->info))->last_led_state[led_port] =
				read_uint64(buf);

	}

	write_string[0] = _1W_SWITCH_MASTER_GET_RELAY_STATES;
	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	memset(buf, 0, sizeof(buf));
	int relay_bytes = relay_port_count * relay_channel_count;
	LEVEL_DEBUG("Reading %d bytes for relays, buffer size is %d",
			relay_bytes, sizeof(buf));

	struct transaction_log txn_relay_state[] = {
			TRXN_START,
			TRXN_WRITE2(write_string),
			TRXN_READ1(command_ok),
			TRXN_READ(buf, relay_bytes),
			TRXN_END,
	};

	if (BAD(BUS_transaction(txn_relay_state, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	uint16_t offset = 0;
	for (uint8_t relay_port = 0; relay_port < relay_port_count; relay_port++) {
		for (uint8_t relay_channel = 0; relay_channel < relay_channel_count; relay_channel++) {
			((switch_master *) (device->info))->relays[relay_port][relay_channel].state = buf[offset++];
			LEVEL_DEBUG("Relay %d:%d initial state is %d", relay_port, relay_channel,
					((switch_master *) (device->info))->relays[relay_port][relay_channel].state);
		}
	}

	write_string[0] = _1W_SWITCH_MASTER_GET_RELAY_MODES;
	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	memset(buf, 0, sizeof(buf));

	struct transaction_log txn_relay_mode[] = {
			TRXN_START,
			TRXN_WRITE2(write_string),
			TRXN_READ1(command_ok),
			TRXN_READ(buf, relay_bytes),
			TRXN_END,
	};

	if (BAD(BUS_transaction(txn_relay_mode, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	offset = 0;
	for (uint8_t relay_port = 0; relay_port < relay_port_count; relay_port++) {
		for (uint8_t relay_channel = 0; relay_channel < relay_channel_count; relay_channel++) {
			((switch_master *) (device->info))->relays[relay_port][relay_channel].mode = buf[offset++];
			LEVEL_DEBUG("Relay %d:%d initial mode is %d", relay_port, relay_channel,
					((switch_master *) (device->info))->relays[relay_port][relay_channel].mode);
		}
	}

	write_string[0] = _1W_SWITCH_MASTER_GET_RELAY_TIMEOUTS;
	crc = CRC8compute(pn->sn, 8, 0);
	crc = CRC8compute(write_string, 1, crc);
	write_string[1] = crc;

	memset(buf, 0, sizeof(buf));

	struct transaction_log txn_relay_timeout[] = {
			TRXN_START,
			TRXN_WRITE2(write_string),
			TRXN_READ1(command_ok),
			TRXN_READ(buf, relay_bytes),
			TRXN_END,
	};

	if (BAD(BUS_transaction(txn_relay_timeout, pn))) {
		return gbBAD;
	}

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		return gbBAD;
	}

	offset = 0;
	for (uint8_t relay_port = 0; relay_port < relay_port_count; relay_port++) {
		for (uint8_t relay_channel = 0; relay_channel < relay_channel_count; relay_channel++) {
			((switch_master *) (device->info))->relays[relay_port][relay_channel].timeout = buf[offset++];
			LEVEL_DEBUG("Relay %d:%d initial timeout %d", relay_port, relay_channel,
					((switch_master *) (device->info))->relays[relay_port][relay_channel].timeout);
		}
	}


	LEVEL_DEBUG(
			"Switches: %d ports of %d channels, LEDs: %d ports of %d channels, Relays %d ports of %d channels",
			switch_port_count, switch_channel_count, led_port_count,
			led_channel_count, relay_port_count, relay_channel_count);

	return gbGOOD;
}

/**
 * Allow a path to be visible if we have an switch master device
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_switch_master_device(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != SWITCH_MASTER) {
		LEVEL_DEBUG("Not a a Switch Master (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	DEVICE_TERM(device);
	return visible_now;
}

/**
 * Allow a path to be visible if we have a switch master device and the switch port is implemented
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_switch_master_switch(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != SWITCH_MASTER) {
		LEVEL_DEBUG("Not a Switch Master (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	uint8_t port = pn->selected_filetype->data.u;
	if (port < ((switch_master *) (device->info))->switch_port_count) {
		DEVICE_TERM(device);
		return visible_now;
	}

	DEVICE_TERM(device);
	return visible_not_now;
}

/**
 * Allow a path to be visible if we have a switch master device and the led port is implemented
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_switch_master_led(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != SWITCH_MASTER) {
		LEVEL_DEBUG("Not a Switch Master (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	uint8_t port = pn->selected_filetype->data.u;
	if (port < ((switch_master *) (device->info))->led_port_count) {
		DEVICE_TERM(device);
		return visible_now;
	}

	DEVICE_TERM(device);
	return visible_not_now;
}

/**
 * Allow a path to be visible if we have a switch master device and the relay port is implemented
 * @param pn the parsed name of the device
 * @return the visibility of the path
 */
static enum e_visibility is_visible_switch_master_relay(const struct parsedname *pn)
{
	ie_device *device;
	if (BAD(device_info(pn, &device))) {
		LEVEL_DEBUG("Could not get device info");
		return visible_not_now;
	}

	if (device->device != SWITCH_MASTER) {
		LEVEL_DEBUG("Not a Switch Master (have %d instead)", device->device);
		DEVICE_TERM(device);
		return visible_not_now;
	}

	uint8_t port = pn->selected_filetype->data.u;
	if (port < ((switch_master *) (device->info))->relay_port_count) {
		DEVICE_TERM(device);
		return visible_now;
	}

	DEVICE_TERM(device);
	return visible_now;
}

/**
 * Count the number of switch ports
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_count_ports(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U (owq) = ((switch_master *) (device->info))->switch_port_count;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Count the number of switch channels
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_count_channels(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U (owq) = ((switch_master *) (device->info))->switch_port_channels;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Count the number of LED ports
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_count_led_ports(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U (owq) = ((switch_master *) (device->info))->led_port_count;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Count the number of LED channels
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_count_led_channels(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U (owq) = ((switch_master *) (device->info))->led_port_channels;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Count the number of relay ports
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_count_relay_ports(struct one_wire_query *owq) {
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U (owq) = ((switch_master *) (device->info))->relay_port_count;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Count the number of relay channels
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_count_relay_channels(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	OWQ_U (owq) = ((switch_master *) (device->info))->relay_port_channels;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Refresh the state of the switch activations
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_refresh_activations(struct one_wire_query *owq)
{
	if (OWQ_Y(owq)) {
		ie_device *device;

		if (BAD(device_info(PN(owq), &device))) {
			LEVEL_DEBUG("Could not get device info");
			return 1;
		}

		switch_master *master = (switch_master *) (device->info);

		BYTE write_string[] = { _1W_SWITCH_MASTER_GET_ACTIVATED_STATE, 0 };
		BYTE command_ok[1];
		BYTE data[MAX_PORTS * sizeof(uint64_t)];
		BYTE temp[sizeof(uint64_t)];
		BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
		crc = CRC8compute(write_string, 1, crc);
		write_string[1] = crc;

		uint8_t bytes_per_port = (master->switch_port_channels + 7) / 8;

		struct transaction_log t[] = {
				TRXN_START,
				TRXN_WRITE2(write_string),
				TRXN_READ1(command_ok),
				TRXN_READ(data, bytes_per_port * master->switch_port_count),
				TRXN_END,
		};

		RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

		if (command_ok[0]) {
			LEVEL_DEBUG("Device reported bad command");
			return 1;
		}

		for (uint8_t port = 0; port < master->switch_port_count; port++) {
			memset(temp, 0, sizeof(temp));
			memcpy(temp, data + port * bytes_per_port, bytes_per_port);
			master->switch_activations[port] = read_uint64(temp);
		}
	}
	return 0;
}

/**
 * Get the most recent activations for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_read_switch_port(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	int len = 0;
	uint8_t port = PN(owq)->selected_filetype->data.u;

	for (uint8_t channel = 0; channel < master->switch_port_channels; channel++) {
		if (channel != 0) {
			OWQ_buffer(owq)[len++] = ',';
		}
		OWQ_buffer(owq)[len++] = (master->switch_activations[port] & (1 << channel)) ? '1' : '0';
	}
	OWQ_buffer(owq)[len] = '\0';

	DEVICE_TERM(device);

	return 0;
}

/**
 * Get the current LED state for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_read_led_port(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	int len = 0;
	uint8_t port = PN(owq)->selected_filetype->data.u;

	for (uint8_t channel = 0; channel < master->led_port_channels; channel++) {
		if (channel != 0) {
			OWQ_buffer(owq)[len++] = ',';
		}
		OWQ_buffer(owq)[len++] = (master->last_led_state[port] & (1 << channel)) ? '1' : '0';
	}
	OWQ_buffer(owq)[len] = '\0';

	DEVICE_TERM(device);

	return 0;
}

/**
 * Set the current LED state for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_write_led_port(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	uint8_t port = PN(owq)->selected_filetype->data.u;

	uint8_t bit = 0;
	uint64_t data = 0;
	for (uint8_t i = 0; i < OWQ_size(owq); i++) {
		switch (OWQ_buffer(owq)[i]) {
		case '0':
			bit++;
			break;
		case '1':
			data |= 1 << bit;
			bit++;
			break;
		}
	}

	uint64_t mask = data ^ master->last_led_state[port];
	for (uint8_t channel = 0; channel < master->led_port_channels; channel++) {
		if (mask & (1 << channel)) {
			BYTE write_string[] = { _1W_SWITCH_MASTER_SET_LED, 0, 0, 0, 0 };
			BYTE command_ok[1];

			write_string[1] = port;
			write_string[2] = channel;
			write_string[3] = (data & (1 << channel)) ? 1 : 0;

			BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
			crc = CRC8compute(write_string, 4, crc);
			write_string[4] = crc;

			struct transaction_log t[] = { TRXN_START, TRXN_WRITE5(
					write_string), TRXN_READ1(command_ok), TRXN_END, };

			RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

			if (command_ok[0]) {
				LEVEL_DEBUG("Device reported bad command");
				return 1;
			}

		}
	}

	master->last_led_state[port] = data;

	DEVICE_TERM(device);

	return 0;
}

/**
 * Get the current relay state for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_read_relay_state(struct one_wire_query *owq) {
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	int len = 0;
	uint8_t port = PN(owq)->selected_filetype->data.u;

	for (uint8_t channel = 0; channel < master->relay_port_channels;
			channel++) {
		if (channel != 0) {
			OWQ_buffer(owq)[len++] = ',';
		}
		len += snprintf(OWQ_buffer(owq) + len, OWQ_size(owq) - len, "%d",
				master->relays[port][channel].state);
	}

	OWQ_buffer(owq)[len] = '\0';

	DEVICE_TERM(device);

	return 0;
}

/**
 * Set the current relay state for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_write_relay_state(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	uint8_t port = PN(owq)->selected_filetype->data.u;
	char *next = OWQ_buffer(owq);
	for (uint8_t channel = 0;
			(channel < master->relay_port_channels) && (next != NULL);
			channel++) {
		long val = strtol(next, &next, 10);
		if (val > 255) {
			val = 255;
		}

		if (val != master->relays[port][channel].state) {
			LEVEL_DEBUG("Setting relay port %d channel %d to %d", port, channel, val);

			BYTE write_string[] = { _1W_SWITCH_MASTER_SET_RELAY_STATE, 0, 0, 0, 0};
			BYTE command_ok[1];

			write_string[1] = port;
			write_string[2] = channel;
			write_string[3] = val;

			BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
			crc = CRC8compute(write_string, 4, crc);
			write_string[4] = crc;

			struct transaction_log t[] = {
					TRXN_START,
					TRXN_WRITE5(write_string),
					TRXN_READ1(command_ok),
					TRXN_END,
			};

			RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

			if (command_ok[0]) {
				LEVEL_DEBUG("Device reported bad command");
				return 1;
			}

			master->relays[port][channel].state = val;
		}

		while (!isdigit(*next) && (*next != '\0')) {
			next++;
		}
	}

	DEVICE_TERM(device);

	return 0;
}


/**
 * Get the current relay mode for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_read_relay_mode(struct one_wire_query *owq) {
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	int len = 0;
	uint8_t port = PN(owq)->selected_filetype->data.u;

	for (uint8_t channel = 0; channel < master->relay_port_channels;
			channel++) {
		if (channel != 0) {
			OWQ_buffer(owq)[len++] = ',';
		}
		len += snprintf(OWQ_buffer(owq) + len, OWQ_size(owq) - len, "%d",
				master->relays[port][channel].mode);
	}

	OWQ_buffer(owq)[len] = '\0';

	DEVICE_TERM(device);

	return 0;
}

/**
 * Set the current relay mode for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_write_relay_mode(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	uint8_t port = PN(owq)->selected_filetype->data.u;
	char *next = OWQ_buffer(owq);
	for (uint8_t channel = 0;
			(channel < master->relay_port_channels) && (next != NULL);
			channel++) {
		long val = strtol(next, &next, 10);
		if (val > 255) {
			val = 255;
		}

		if (val != master->relays[port][channel].state) {
			LEVEL_DEBUG("Setting relay mode for port %d channel %d to %d", port, channel, val);

			BYTE write_string[] = { _1W_SWITCH_MASTER_SET_RELAY_MODE, 0, 0, 0, 0};
			BYTE command_ok[1];

			write_string[1] = port;
			write_string[2] = channel;
			write_string[3] = val;

			BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
			crc = CRC8compute(write_string, 4, crc);
			write_string[4] = crc;

			struct transaction_log t[] = {
					TRXN_START,
					TRXN_WRITE5(write_string),
					TRXN_READ1(command_ok),
					TRXN_END,
			};

			RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

			if (command_ok[0]) {
				LEVEL_DEBUG("Device reported bad command");
				return 1;
			}

			master->relays[port][channel].state = val;
		}

		while (!isdigit(*next) && (*next != '\0')) {
			next++;
		}
	}

	DEVICE_TERM(device);

	return 0;
}

/**
 * Get the current relay timeout for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_read_relay_timeout(struct one_wire_query *owq) {
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	int len = 0;
	uint8_t port = PN(owq)->selected_filetype->data.u;

	for (uint8_t channel = 0; channel < master->relay_port_channels;
			channel++) {
		if (channel != 0) {
			OWQ_buffer(owq)[len++] = ',';
		}
		len += snprintf(OWQ_buffer(owq) + len, OWQ_size(owq) - len, "%d",
				master->relays[port][channel].timeout);
	}

	OWQ_buffer(owq)[len] = '\0';

	DEVICE_TERM(device);

	return 0;
}

/**
 * Set the current relay timeout for a port
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_write_relay_timeout(struct one_wire_query *owq)
{
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	uint8_t port = PN(owq)->selected_filetype->data.u;
	char *next = OWQ_buffer(owq);
	for (uint8_t channel = 0;
			(channel < master->relay_port_channels) && (next != NULL);
			channel++) {
		long val = strtol(next, &next, 10);
		if (val > 255) {
			val = 255;
		}

		if (val != master->relays[port][channel].timeout) {
			LEVEL_DEBUG("Setting relay timeout for port %d channel %d to %d", port, channel, val);

			BYTE write_string[] = { _1W_SWITCH_MASTER_SET_RELAY_TIMEOUT, 0, 0, 0, 0};
			BYTE command_ok[1];

			write_string[1] = port;
			write_string[2] = channel;
			write_string[3] = val;

			BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
			crc = CRC8compute(write_string, 4, crc);
			write_string[4] = crc;

			struct transaction_log t[] = {
					TRXN_START,
					TRXN_WRITE5(write_string),
					TRXN_READ1(command_ok),
					TRXN_END,
			};

			RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

			if (command_ok[0]) {
				LEVEL_DEBUG("Device reported bad command");
				return 1;
			}

			master->relays[port][channel].timeout = val;
		}

		while (!isdigit(*next) && (*next != '\0')) {
			next++;
		}
	}

	DEVICE_TERM(device);

	return 0;
}


/**
 * Set the type of a switch - port,channel,type where type is:
 * 	TOGGLE_PULL_UP = 0
 *	TOGGLE_PULL_DOWN = 1
 *	MOMENTARY_PULL_UP = 2
 *	MOMENTARY_PULL_DOWN = 3
 * @param owq the query
 */
static ZERO_OR_ERROR switch_master_set_switch_type(struct one_wire_query *owq) {
	ie_device *device;

	if (BAD(device_info(PN(owq), &device))) {
		LEVEL_DEBUG("Could not get device info");
		return 1;
	}

	switch_master *master = (switch_master *) (device->info);

	uint32_t port = 0;
	uint32_t channel = 0;
	uint32_t type = 0;

	if (sscanf(OWQ_buffer(owq), "%u,%u,%u", &port, &channel, &type)) {
		LEVEL_DEBUG("Parsing failed");
		DEVICE_TERM(device);
		return 1;
	}

	if (port >= master->switch_port_count) {
		LEVEL_DEBUG("Invalid port %d >= %d", port, master->switch_port_count);
		DEVICE_TERM(device);
		return 1;
	}

	if (channel >= master->switch_port_channels) {
		LEVEL_DEBUG("Invalid channel %d >= %d", channel,
				master->switch_port_channels);
		DEVICE_TERM(device);
		return 1;
	}

	if (type >= 4) {
		LEVEL_DEBUG("Invalid channel %d >= 4", type);
		DEVICE_TERM(device);
		return 1;
	}

	BYTE write_string[] = { _1W_SWITCH_MASTER_SWITCH_TYPE, 0, 0, 0, 0 };
	BYTE command_ok[1];

	write_string[1] = port;
	write_string[2] = channel;
	write_string[3] = type;

	BYTE crc = CRC8compute(PN(owq)->sn, 8, 0);
	crc = CRC8compute(write_string, 4, crc);
	write_string[4] = crc;

	struct transaction_log t[] = {
			TRXN_START,
			TRXN_WRITE5(write_string),
			TRXN_READ1(command_ok),
			TRXN_END,
	};

	RETURN_ERROR_IF_BAD(BUS_transaction(t, PN(owq)));

	if (command_ok[0]) {
		LEVEL_DEBUG("Device reported bad command");
		DEVICE_TERM(device);
		return 1;
	}

	DEVICE_TERM(device);

	return 0;
}

