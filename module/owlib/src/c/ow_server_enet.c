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

/* not our owserver, but a stand-alone device made by EDS called OW-SERVER-ENET */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

struct toENET {
	ASCII *command;
	ASCII address[16];
	const BYTE *data;
	size_t length;
};

#define BYTE_string(x)  ((BYTE *)(x))

//static void byteprint( const BYTE * b, int size ) ;
static RESET_TYPE OWServer_Enet_reset(const struct parsedname *pn);
static void OWServer_Enet_close(struct connection_in *in);

static enum search_status OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void OWServer_Enet_setroutines(struct connection_in *in);
static GOOD_OR_BAD OWServer_Enet_select( const struct parsedname * pn ) ;

static GOOD_OR_BAD OWServer_Enet_reopen(struct connection_in *in) ;
static GOOD_OR_BAD OWServer_Enet_read( BYTE * buf, size_t size, struct connection_in * in ) ;
static GOOD_OR_BAD OWServer_Enet_write(const BYTE * buf, size_t size, struct connection_in *in) ;
static GOOD_OR_BAD OWServer_Enet_write_string( char * buf, struct connection_in *in) ;
static GOOD_OR_BAD OWServer_Enet_command( char * cmd_string, struct connection_in * in ) ;
static RESET_TYPE OWServer_Enet_reset_in(struct connection_in * in);
static GOOD_OR_BAD OWServer_Enet_directory(struct device_search *ds, struct dirblob *db, struct connection_in * in) ;
static GOOD_OR_BAD OWServer_Enet_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;

static void OWServer_Enet_setroutines(struct connection_in *in)
{
	in->iroutines.detect = OWServer_Enet_detect;
	in->iroutines.reset = OWServer_Enet_reset;
	in->iroutines.next_both = OWServer_Enet_next_both;
	in->iroutines.PowerByte = NULL;
//    in->iroutines.ProgramPulse = ;
	in->iroutines.sendback_data = OWServer_Enet_sendback_data;
//    in->iroutines.sendback_bits = ;
	in->iroutines.select = OWServer_Enet_select ;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = OWServer_Enet_close;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_no2409path | ADAP_FLAG_overdrive | ADAP_FLAG_bundle ;
	in->bundling_length = HA7E_FIFO_SIZE;
}

GOOD_OR_BAD OWServer_Enet_detect(struct connection_in *in)
{
	/* Set up low-level routines */
	OWServer_Enet_setroutines(in);

	in->Adapter = adapter_ENET;
	in->adapter_name = "OWServer_Enet";
	in->busmode = bus_enet;
	in->timeout.tv_sec = 0 ;
	in->timeout.tv_usec = 600000 ;

	if (in->name == NULL) {
		return gbBAD;
	}

	RETURN_BAD_IF_BAD(ClientAddr(in->name, DEFAULT_ENET_PORT, in)) ;

	RETURN_GOOD_IF_GOOD( OWServer_Enet_reopen(in)) ;
	RETURN_GOOD_IF_GOOD( OWServer_Enet_reopen(in)) ;
	RETURN_GOOD_IF_GOOD( OWServer_Enet_reopen(in)) ;
	LEVEL_DEFAULT("Problem opening OW_SERVER_ENET on IP address=[%s] port=[%s] -- is the \"1-Wire Setup\" enabled?", SAFESTRING(in->master.tcp.host), in->master.tcp.service );
	return gbBAD ;
}

static GOOD_OR_BAD OWServer_Enet_reopen(struct connection_in *in)
{
	BYTE char_in ;
	int in_chars ;
	int index_0D = 0 ;
	GOOD_OR_BAD Qmark_got = gbBAD ;
	
	Test_and_Close( &(in->file_descriptor) ) ;
	
	in->master.serial.tcp.default_discard = 0 ;
	in->master.serial.tcp.CRLF_size = 0 ;
	in->master.link.tmode = e_link_t_unknown ;
	in->master.link.qmode = e_link_t_unknown ;

	memset( in->master.ha7e.sn, 0x00, SERIAL_NUMBER_SIZE ) ;

	in->file_descriptor = ClientConnect(in) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(in->file_descriptor) ) {
		return gbBAD;
	}

	// need to read 1 char at a time to get a short string
	for ( in_chars = 0 ; in_chars < 32 ; ++in_chars ) {
		if ( BAD( OWServer_Enet_read( &char_in, 1, in)) ) {
			LEVEL_DEBUG("Cannot read the initial prompt");
			return gbBAD ;
		}
		switch( char_in ) {
			case '?':
				Qmark_got = gbGOOD ;
				break ;
			case 0x0D:
				index_0D = in_chars ;
				break ;
			case 0x0A:
				in->master.serial.tcp.CRLF_size = in_chars - index_0D + 1 ;
				LEVEL_DEBUG("CRLF string length = %d",in->master.serial.tcp.CRLF_size) ;
				return Qmark_got ;
			default:
				break ;
		}
	}
	LEVEL_DEFAULT("OW_SERVER_ENET prompt string too long");
	return gbBAD ;
}

static RESET_TYPE OWServer_Enet_reset(const struct parsedname *pn)
{
	return OWServer_Enet_reset_in( pn->selected_connection ) ;
}

static RESET_TYPE OWServer_Enet_reset_in(struct connection_in * in)
{
	char reset_char[1+in->master.serial.tcp.CRLF_size] ;

	if ( BAD( OWServer_Enet_write_string("R\r",in )) ) {
		return BUS_RESET_ERROR ;
	}
	
	if ( BAD( OWServer_Enet_read( BYTE_string( reset_char ), 1, in )) ) {
		return BUS_RESET_ERROR ;
	}
	
	switch ( reset_char[0] ) {
		case 'P':
			in->AnyDevices = anydevices_yes;
			return BUS_RESET_OK;
		case 'N':
			in->AnyDevices = anydevices_no;
			return BUS_RESET_OK;
		default:
			LEVEL_DEBUG("Unrecognized BUS reset character <%c>",reset_char[0] ) ;
			TCP_slurp( in->file_descriptor ) ;
		return BUS_RESET_ERROR;
	}
}

static enum search_status OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(in->alarm) : &(in->main);

	//Special case for DS2409 hub, use low-level code
	if ( pn->pathlength>0 ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		if ( BAD(OWServer_Enet_directory(ds, db, in)) ) {
			return search_error;
		}
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, db) ) {
		case 0:
			if ((ds->sn[0] & 0x7F) == 0x04) {
				/* We found a DS1994/DS2404 which require longer delays */
				pn->selected_connection->ds2404_compliance = 1;
			}
			LEVEL_DEBUG("SN found: " SNformat "", SNvar(ds->sn));
			return search_good;
		case -ENODEV:
		default:
			ds->LastDevice = 1;
			LEVEL_DEBUG("SN finished");
			return search_done;
	}
}

#define DEVICE_LENGTH  16
#define EXCLAIM_LENGTH 1

static GOOD_OR_BAD OWServer_Enet_directory(struct device_search *ds, struct dirblob *db, struct connection_in * in)
{
	char resp[DEVICE_LENGTH+in->master.serial.tcp.CRLF_size];
	char * search_first ;
	char * search_next ;

	if ( ds->search == _1W_CONDITIONAL_SEARCH_ROM ) {
		search_first = "C\r" ;
		search_next = "c\r" ;
	} else {
		search_first = "S\r" ;
		search_next = "s\r" ;
	}
	
	DirblobClear(db);

	if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
		in->AnyDevices = anydevices_no;
	}

	// send the first search
	RETURN_BAD_IF_BAD( OWServer_Enet_write_string( search_first, in)) ;

	do {
		size_t read_so_far = 0 ;
		// read first character
		RETURN_BAD_IF_BAD(OWServer_Enet_read(BYTE_string(resp), EXCLAIM_LENGTH, in)) ;
		
		switch (resp[0]) {
			case '!':
				return gbGOOD ;
			case '?':
				read_so_far = 0 ;
				break ;
			default:
				read_so_far = EXCLAIM_LENGTH+in->master.serial.tcp.CRLF_size ;
				break ;
		}
		
		// read rest of the characters
		if ( BAD(OWServer_Enet_read(BYTE_string(&resp[read_so_far]), DEVICE_LENGTH-read_so_far, in)) ) {
			return gbBAD;
		}

		BYTE sn[SERIAL_NUMBER_SIZE];

		sn[7] = string2num(&resp[0]);
		sn[6] = string2num(&resp[2]);
		sn[5] = string2num(&resp[4]);
		sn[4] = string2num(&resp[6]);
		sn[3] = string2num(&resp[8]);
		sn[2] = string2num(&resp[10]);
		sn[1] = string2num(&resp[12]);
		sn[0] = string2num(&resp[14]);
		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));
		// Set as current "Address" for adapter
		memcpy( in->master.enet.sn, sn, SERIAL_NUMBER_SIZE) ;

		// CRC check
		if (CRC8(sn, SERIAL_NUMBER_SIZE) || (sn[0] == 0x00)) {
			LEVEL_DEBUG("BAD family or CRC8");
			return gbBAD;
		}

		DirblobAdd(sn, db);

		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			in->AnyDevices = anydevices_yes;
		}

		// send the subsequent search
		RETURN_BAD_IF_BAD(OWServer_Enet_write_string( search_next, in)) ;
	} while (1) ;
}

/* select a device for reference */
static GOOD_OR_BAD OWServer_Enet_select( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	char send_address[] = "A7766554433221100\r" ;

	// Apparently need to reset before select (Unlike the HA7S)
	RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn)) ) ;
	
	if ( (pn->selected_device==NULL) || (pn->selected_device==DeviceThermostat) ) {
		return gbGOOD ;
	}

	if ( memcmp( pn->sn, in->master.ha7e.sn, SERIAL_NUMBER_SIZE ) != 0 ) {
		num2string( &send_address[ 1], pn->sn[7] ) ;
		num2string( &send_address[ 3], pn->sn[6] ) ;
		num2string( &send_address[ 5], pn->sn[5] ) ;
		num2string( &send_address[ 7], pn->sn[4] ) ;
		num2string( &send_address[ 9], pn->sn[3] ) ;
		num2string( &send_address[11], pn->sn[2] ) ;
		num2string( &send_address[13], pn->sn[1] ) ;
		num2string( &send_address[15], pn->sn[0] ) ;
		// Set as current "Address" for adapter
		memcpy( in->master.enet.sn, pn->sn, SERIAL_NUMBER_SIZE) ;
	} else { // Serial number already loaded in adapter
		strcpy( send_address, "M\r" ) ;
	}

	return OWServer_Enet_command( send_address, in ) ;
}

/* send a command and get the '+' */
static GOOD_OR_BAD OWServer_Enet_command( char * cmd_string, struct connection_in * in )
{
	char cmd_response[1+in->master.serial.tcp.CRLF_size] ;

	if ( BAD( OWServer_Enet_write_string( cmd_string, in )) ) {
		LEVEL_DEBUG("Error sending string <%s>", cmd_string ) ;
		return gbBAD ;
	}

	if ( BAD( OWServer_Enet_read( BYTE_string(cmd_response), 1, in )) ) {
		LEVEL_DEBUG("Error reading response to <%s>", cmd_string ) ;
		return gbBAD ;
	}

	switch ( cmd_response[0] ) {
		case '+':
			return gbGOOD ;
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD OWServer_Enet_read( BYTE * buf, size_t size, struct connection_in * in )
{
	// Only need to add an extra byte sometimes
	return telnet_read( buf, size+in->master.serial.tcp.CRLF_size, in ) ;
}

static GOOD_OR_BAD OWServer_Enet_write_string( char * buf, struct connection_in *in)
{
	RETURN_GOOD_IF_GOOD( OWServer_Enet_write( BYTE_string(buf), strlen(buf), in ) );
	LEVEL_DEBUG("Let's try to reopen the bus");
	tcp_read_flush( in->file_descriptor ) ;
	RETURN_BAD_IF_BAD( OWServer_Enet_reopen( in )) ;
	return OWServer_Enet_write( BYTE_string(buf), strlen(buf), in ) ;
}

static GOOD_OR_BAD OWServer_Enet_write(const BYTE * buf, size_t size, struct connection_in *in)
{
	return COM_write( buf, size, in ) ;
}

#define MAX_ENET_MEMORY_GULP   64

//  Send data and return response block -- up to 32 bytes
static GOOD_OR_BAD OWServer_Enet_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	char send_data[1+MAX_ENET_MEMORY_GULP*2+3] ;
	char get_data[MAX_ENET_MEMORY_GULP*2+in->master.serial.tcp.CRLF_size] ;

	send_data[0] = 'W' ;
	bytes2string(&send_data[1], data, size) ;
	send_data[1+2*size] = '\r' ;
	send_data[1+2*size+1] = '\0' ;

	if ( BAD( OWServer_Enet_write_string( send_data, in)) ) {
		LEVEL_DEBUG("Error with sending ENET block") ;
		return gbBAD ;
	}
	if ( BAD(OWServer_Enet_read( BYTE_string(get_data), size*2, in)) ) {
		LEVEL_DEBUG("Error with reading ENET block") ;
		return gbBAD ;
	}

	string2bytes(get_data, resp, size) ;
	return gbGOOD ;
}

static GOOD_OR_BAD OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int left;

	for ( left=size ; left>0 ; left -= MAX_ENET_MEMORY_GULP ) {
		size_t pass_start = size - left ;
		size_t pass_size = (left>MAX_ENET_MEMORY_GULP) ? MAX_ENET_MEMORY_GULP : left ;
		RETURN_BAD_IF_BAD( OWServer_Enet_sendback_part( &data[pass_start], &resp[pass_start], pass_size, pn ) ) ;
	}
	return gbGOOD;
}


static void OWServer_Enet_close(struct connection_in *in)
{
	FreeClientAddr(in);
}

