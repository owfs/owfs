/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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

/* Alfille 2014: 
 * Etherweather seems to never have taken off commercially. 
 * Use this as a reference driver */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

#define EtherWeather_COMMAND_RESET	'R'
#define EtherWeather_COMMAND_ACCEL	'A'
#define EtherWeather_COMMAND_BYTES	'B'
#define EtherWeather_COMMAND_BITS	'b'
#define EtherWeather_COMMAND_POWER	'P'

static RESET_TYPE EtherWeather_reset(const struct parsedname *pn) ;
static int EtherWeather_command(struct connection_in *in, char command, int datalen, const BYTE * idata, BYTE * odata) ;
static void EtherWeather_close(struct connection_in *in);
static GOOD_OR_BAD EtherWeather_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn);
static enum search_status EtherWeather_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD EtherWeather_sendback_bits(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn);
static GOOD_OR_BAD EtherWeather_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn);
static void EtherWeather_setroutines(struct connection_in *in);

static int EtherWeather_command(struct connection_in *in, char command, int datalen, const BYTE * idata, BYTE * odata)
{
	struct port_in * pin = in->pown ;
	BYTE *packet;

	// The packet's length field includes the command byte.
	packet = owmalloc(datalen + 2);
	if ( packet== NULL ) {
		return -ENOMEM ;
	}
	packet[0] = datalen + 1;
	packet[1] = command;
	memcpy( &packet[2], idata, datalen);

	pin->timeout.tv_sec = 0 ;
	pin->timeout.tv_usec = 200000 ;

	if ( BAD(COM_write( packet, datalen+2, in) ) ) {
		ERROR_CONNECT("Trouble writing data to EtherWeather: %s", SAFESTRING(DEVICENAME(in)));
		STAT_ADD1_BUS(e_bus_write_errors, in);
		owfree(packet) ;
		return -EIO ;
	}

	// Allow extra time for powered bytes
	if (command == 'P') {
		pin->timeout.tv_sec += 2 ;
	}
	// Read the response header
	if ( BAD(COM_read( packet, 2, in )) ) {
		LEVEL_CONNECT("header read error");
		owfree(packet);
		return -EIO;
	}
	// Make sure it was echoed properly
	if (packet[0] != (datalen + 1) || packet[1] != command) {
		LEVEL_CONNECT("invalid header");
		owfree(packet);
		return -EIO;
	}
	// Then read any data
	if (datalen > 0) {
		if ( BAD(COM_read( odata, datalen, in )) ) {
			LEVEL_CONNECT("data read error");
			owfree(packet);
			return -EIO;
		}
	}

	owfree(packet);
	return 0;
}

static GOOD_OR_BAD EtherWeather_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{

	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_BYTES, size, data, resp)) {
		return gbBAD;
	}

	return gbGOOD;
}

static GOOD_OR_BAD EtherWeather_sendback_bits(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{

	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_BITS, size, data, resp)) {
		return gbBAD;
	}

	return gbGOOD;
}

static enum search_status EtherWeather_next_both(struct device_search *ds, const struct parsedname *pn)
{
	BYTE sendbuf[9];

	// if the last call was not the last one
	if (ds->LastDevice) {
		return search_done;
	}

	if ( BAD( BUS_select(pn) ) ) {
		return search_error ;
	}

	memcpy(sendbuf, ds->sn, SERIAL_NUMBER_SIZE);
	if (ds->LastDiscrepancy == -1) {
		sendbuf[8] = 0x40;
	} else {
		sendbuf[8] = ds->LastDiscrepancy;
	}
	if (ds->search == 0xEC) {
		sendbuf[8] |= 0x80;
	}

	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_ACCEL, 9, sendbuf, sendbuf)) {
		return search_error;
	}

	if (sendbuf[8] == 0xFF) {
		/* No devices */
		return search_done;
	}

	memcpy(ds->sn, sendbuf, 8);

	if (CRC8(ds->sn, 8) || (ds->sn[0] == 0)) {
		/* Bus error */
		return search_error;
	}

	/* 0xFE indicates no discrepancies */
	ds->LastDiscrepancy = sendbuf[8];
	if (ds->LastDiscrepancy == 0xFE) {
		ds->LastDiscrepancy = -1;
	}

	ds->LastDevice = (sendbuf[8] == 0xFE);

	LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));

	return search_good;
}

static GOOD_OR_BAD EtherWeather_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname *pn)
{
	BYTE pbbuf[2];

	/* PowerByte command specifies delay in 500ms ticks, not milliseconds */
	pbbuf[0] = (delay + 499) / 500;
	pbbuf[1] = byte;
	LEVEL_DEBUG("SPU: %d %d", pbbuf[0], pbbuf[1]);
	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_POWER, 2, pbbuf, pbbuf)) {
		return gbBAD;
	}

	*resp = pbbuf[1];
	return gbGOOD;
}


static void EtherWeather_close(struct connection_in *in)
{
	// the standard COM-free cleans up the connection
	(void) in ;
}

static RESET_TYPE EtherWeather_reset(const struct parsedname *pn)
{
	if (EtherWeather_command(pn->selected_connection, EtherWeather_COMMAND_RESET, 0, NULL, NULL)) {
		return BUS_RESET_ERROR;
	}

	return BUS_RESET_OK;
}

static void EtherWeather_setroutines(struct connection_in *in)
{
	in->iroutines.detect = EtherWeather_detect;
	in->iroutines.reset = EtherWeather_reset;
	in->iroutines.next_both = EtherWeather_next_both;
	in->iroutines.PowerByte = EtherWeather_PowerByte;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = EtherWeather_sendback_data;
	in->iroutines.sendback_bits = EtherWeather_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = EtherWeather_close;
	in->iroutines.flags = ADAP_FLAG_overdrive | ADAP_FLAG_dirgulp | ADAP_FLAG_no2409path | ADAP_FLAG_no2404delay ;
}

GOOD_OR_BAD EtherWeather_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct parsedname pn;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	LEVEL_CONNECT("Connecting to EtherWeather");

	/* Set up low-level routines */
	EtherWeather_setroutines(in);

	if (pin->init_data == NULL) {
		LEVEL_DEFAULT("Etherweather bus master requires a port name");
		return gbBAD;
	}

	pin->type = ct_tcp ;
	RETURN_BAD_IF_BAD( COM_open(in) ) ;

	/* TODO: probe version, and confirm that it's actually an EtherWeather */

	LEVEL_CONNECT("Connected to EtherWeather at %s", DEVICENAME(in));

	in->Adapter = adapter_EtherWeather;

	in->adapter_name = "EtherWeather";
	pin->busmode = bus_etherweather;

	return gbGOOD;
}
