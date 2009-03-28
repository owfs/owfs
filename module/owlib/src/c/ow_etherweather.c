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

/*
    Written by Jacob Potter 2007
    for the "EtherWeather":

    It started out as a school-related project, then grew into a
    hobby/for-fun thing, and now I'm polishing it off to be able to sell.
    The main components are an AVR microcontroller and Ethernet chip; I
    wrote all the interface and application code (using the uIP TCP/IP
    stack). I'm planning to keep the firmware closed, but the
    communication protocols and support code will all be open.

    The system listens on a TCP socket and acts as a low-level bus master
    (reset/bit/byte, strong pullup, and search acceleration), so it should
    be able to talk to any device supported by the code talking to it.
    Besides waiting for connections from PC-side software such as OWFS, it
    can also periodically connect to a remote system over the Internet to
    receive commands, and I've written some code to handle that as well.

    At the moment, I've built a few prototypes and am waiting for the next
    round of components and boards to come in. The majority of the
    software (firmware w/DHCP, bootloader, OWFS patch, and service
    daemon/Web interface) is working, although not polished off yet. The
    board is 2" x 3", runs off of 7-9v DC, and should cost around $50
    (assembled/programmed, but without case and power supply).
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

#define EtherWeather_COMMAND_RESET	'R'
#define EtherWeather_COMMAND_ACCEL	'A'
#define EtherWeather_COMMAND_BYTES	'B'
#define EtherWeather_COMMAND_BITS		'b'
#define EtherWeather_COMMAND_POWER	'P'

static int EtherWeather_command(struct connection_in *in, char command, int datalen, const BYTE * idata, BYTE * odata)
{
	ssize_t res;
	ssize_t left = datalen;
	BYTE *packet;

	struct timeval tvnet = { 0, 200000, };

	// The packet's length field includes the command byte.
	packet = malloc(datalen + 2);
	packet[0] = datalen + 1;
	packet[1] = command;
	memcpy(packet + 2, idata, datalen);

	left = datalen + 2;
/*
	char prologue[2];
	prologue[0] = datalen + 1;
	prologue[1] = command;

	res = write(in->file_descriptor, prologue, 2);
	if (res < 1) {
		ERROR_CONNECT("Trouble writing data to EtherWeather: %s\n",
			SAFESTRING(in->name));
		return -EIO;
	}
*/
	while (left > 0) {
		res = write(in->file_descriptor, &packet[datalen + 2 - left], left);
		if (res < 0) {
			if (errno == EINTR) {
				continue;
			}
			ERROR_CONNECT("Trouble writing data to EtherWeather: %s\n", SAFESTRING(in->name));
			break;
		}
		left -= res;
	}

	tcdrain(in->file_descriptor);
	gettimeofday(&(in->bus_write_time), NULL);

	if (left > 0) {
		STAT_ADD1_BUS(e_bus_write_errors, in);
		free(packet);
		return -EIO;
	}
	// Allow extra time for powered bytes
	if (command == 'P') {
		tvnet.tv_sec += 2;
	}
	// Read the response header
	if (tcp_read(in->file_descriptor, packet, 2, &tvnet) != 2) {
		LEVEL_CONNECT("header read error\n");
		free(packet);
		return -EIO;
	}
	// Make sure it was echoed properly
	if (packet[0] != (datalen + 1) || packet[1] != command) {
		LEVEL_CONNECT("invalid header\n");
		free(packet);
		return -EIO;
	}
	// Then read any data
	if (datalen > 0) {
		if (tcp_read(in->file_descriptor, odata, datalen, &tvnet) != (ssize_t) datalen) {
			LEVEL_CONNECT("data read error\n");
			free(packet);
			return -EIO;
		}
	}

	free(packet);
	return 0;
}

static int EtherWeather_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{

	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_BYTES, size, data, resp)) {
		return -EIO;
	}

	return 0;
}

static int EtherWeather_sendback_bits(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{

	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_BITS, size, data, resp)) {
		return -EIO;
	}

	return 0;
}

static int EtherWeather_next_both(struct device_search *ds, const struct parsedname *pn)
{
	BYTE sendbuf[9];

	// if the last call was not the last one
	if (!pn->selected_connection->AnyDevices) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}

	memcpy(sendbuf, ds->sn, 8);
	if (ds->LastDiscrepancy == -1) {
		sendbuf[8] = 0x40;
	} else {
		sendbuf[8] = ds->LastDiscrepancy;
	}
	if (ds->search == 0xEC) {
		sendbuf[8] |= 0x80;
	}

	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_ACCEL, 9, sendbuf, sendbuf)) {
		return -EIO;
	}

	if (sendbuf[8] == 0xFF) {
		/* No devices */
		return -ENODEV;
	}

	memcpy(ds->sn, sendbuf, 8);

	if (CRC8(ds->sn, 8) || (ds->sn[0] == 0)) {
		/* Bus error */
		return -EIO;
	}

	if ((ds->sn[0] & 0x7F) == 0x04) {
		/* We found a DS1994/DS2404 which require longer delays */
		pn->selected_connection->ds2404_compliance = 1;
	}

	/* 0xFE indicates no discrepancies */
	ds->LastDiscrepancy = sendbuf[8];
	if (ds->LastDiscrepancy == 0xFE) {
		ds->LastDiscrepancy = -1;
	}

	ds->LastDevice = (sendbuf[8] == 0xFE);

	LEVEL_DEBUG("SN found: " SNformat "\n", SNvar(ds->sn));

	return 0;
}

static int EtherWeather_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	BYTE pbbuf[2];

	/* PowerByte command specifies delay in 500ms ticks, not milliseconds */
	pbbuf[0] = (delay + 499) / 500;
	pbbuf[1] = byte;
	LEVEL_DEBUG("SPU: %d %d\n", pbbuf[0], pbbuf[1]);
	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_POWER, 2, pbbuf, pbbuf)) {
		return -EIO;
	}

	*resp = pbbuf[1];
	return 0;
}


static void EtherWeather_close(struct connection_in *in)
{
	Test_and_Close( &(in->file_descriptor) ) ;
	FreeClientAddr(in);
}

static int EtherWeather_reset(const struct parsedname *pn)
{
	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_RESET, 0, NULL, NULL)) {
		return -EIO;
	}

	return 0;
}

static void EtherWeather_setroutines(struct connection_in *in)
{
	in->iroutines.detect = EtherWeather_detect;
	in->iroutines.reset = EtherWeather_reset;
	in->iroutines.next_both = EtherWeather_next_both;
	in->iroutines.PowerByte = EtherWeather_PowerByte;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = EtherWeather_sendback_data;
	in->iroutines.sendback_bits = EtherWeather_sendback_bits;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = EtherWeather_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_overdrive | ADAP_FLAG_dirgulp | ADAP_FLAG_2409path;
}

int EtherWeather_detect(struct connection_in *in)
{

	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	LEVEL_CONNECT("Connecting to EtherWeather\n");

	/* Set up low-level routines */
	EtherWeather_setroutines(in);

	if (in->name == NULL) {
		return -1;
	}

	/* Add the port if it isn't there already */
	if (strchr(in->name, ':') == NULL) {
		ASCII *temp = realloc(in->name, strlen(in->name) + 3);
		if (temp == NULL) {
			return -ENOMEM;
		}
		in->name = temp;
		strcat(in->name, ":15862");
	}

	if (ClientAddr(in->name, in)) {
		return -1;
	}
	if ((pn.selected_connection->file_descriptor = ClientConnect(in)) < 0) {
		return -EIO;
	}


	/* TODO: probe version, and confirm that it's actually an EtherWeather */

	LEVEL_CONNECT("Connected to EtherWeather at %s\n", in->name);

	in->Adapter = adapter_EtherWeather;

	in->adapter_name = "EtherWeather";
	in->busmode = bus_etherweather;
	in->AnyDevices = 1;

	return 0;
}
