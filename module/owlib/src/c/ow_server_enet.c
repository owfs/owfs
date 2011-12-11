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

#define BYTE_string(x)  ((BYTE *)(x))

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
static char OWServer_Enet_command( char * cmd_string, struct connection_in * in ) ;
static RESET_TYPE OWServer_Enet_reset_in(struct connection_in * in);
static GOOD_OR_BAD OWServer_Enet_directory(struct device_search *ds, struct connection_in * in) ;
static enum ENET_dir OWServer_Enet_directory_loop(struct device_search *ds, struct connection_in * in) ;
static GOOD_OR_BAD OWServer_Enet_sendback_part(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn) ;

#define BAD_CHAR ( (char) -1 )

static void OWServer_Enet_setroutines(struct connection_in *in)
{
	in->iroutines.detect = OWServer_Enet_detect;
	in->iroutines.reset = OWServer_Enet_reset;
	in->iroutines.next_both = OWServer_Enet_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = OWServer_Enet_sendback_data;
    in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = OWServer_Enet_select ;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE ;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = OWServer_Enet_close;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_no2409path | ADAP_FLAG_overdrive | ADAP_FLAG_bundle | ADAP_FLAG_no2404delay ;
	in->bundling_length = HA7E_FIFO_SIZE;
}

GOOD_OR_BAD OWServer_Enet_detect(struct connection_in *in)
{
	/* Set up low-level routines */
	OWServer_Enet_setroutines(in);

	in->Adapter = adapter_ENET;
	in->adapter_name = "OWServer_Enet";
	in->busmode = bus_enet;

	// A lot of telnet parameters, really only used
	// to goose the connection when reconnecting
	// The ENET tolerates telnet, but isn't really
	// into RFC2217
	COM_set_standard(in) ;
	SOC(in)->timeout.tv_sec = 0 ;
	SOC(in)->timeout.tv_usec = 600000 ;
	SOC(in)->flow = flow_none; // flow control
	SOC(in)->baud = B115200 ;

	if ( SOC(in)->devicename == NULL) {
		return gbBAD;
	}

	SOC(in)->type = ct_telnet ;
	RETURN_BAD_IF_BAD( COM_open(in) ) ;

	// Always returns 0D0A
	in->master.enet.reopening = 0 ;
	
	memset( in->master.enet.sn, 0x00, SERIAL_NUMBER_SIZE ) ;

	RETURN_GOOD_IF_GOOD( OWServer_Enet_reopen(in)) ;
	RETURN_GOOD_IF_GOOD( OWServer_Enet_reopen(in)) ;
	LEVEL_DEFAULT("Problem opening OW_SERVER_ENET on IP address=[%s] port=[%s] -- is the \"1-Wire Setup\" enabled?", SAFESTRING(SOC(in)->dev.tcp.host), SOC(in)->dev.tcp.service );
	return gbBAD ;
}

static GOOD_OR_BAD OWServer_Enet_reopen(struct connection_in *in)
{
	if ( in->master.enet.reopening ) {
		LEVEL_DEBUG("Attempt to double-reopen %s",SAFESTRING(SOC(in)->devicename)) ;
		return gbBAD ;
	}

	// clear "stored" Enet device
	memset( in->master.enet.sn, 0x00, SERIAL_NUMBER_SIZE ) ;

	RETURN_BAD_IF_BAD( COM_open(in) ) ;
	//printf("ENET: reopen com port\n");
	telnet_change( in ) ; // Really just to send a prompt

	in->master.enet.reopening = 1 ; // flag in reopening mode
	if ( OWServer_Enet_command( "", in ) == '?' ) {
		in->master.enet.reopening = 0 ; // out of reopening mode
		return gbGOOD ;
	}
	in->master.enet.reopening = 0 ; // out of reopening mode
	LEVEL_DEBUG("Cannot read the initial prompt");
	return gbBAD ;
}

static RESET_TYPE OWServer_Enet_reset(const struct parsedname *pn)
{
	return OWServer_Enet_reset_in( pn->selected_connection ) ;
}

static RESET_TYPE OWServer_Enet_reset_in(struct connection_in * in)
{
	char reset_response = OWServer_Enet_command( "R\r", in ) ;
	
	switch ( reset_response ) {
		case 'P':
			in->AnyDevices = anydevices_yes;
			return BUS_RESET_OK;
		case 'N':
			in->AnyDevices = anydevices_no;
			return BUS_RESET_OK;
		case BAD_CHAR:
			LEVEL_DEBUG("Problem sending reset character");
			return BUS_RESET_ERROR;
		default:
			LEVEL_DEBUG("Unrecognized BUS reset character <%c>",reset_response ) ;
			return BUS_RESET_ERROR;
	}
}

static enum search_status OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;

	//Special case for DS2409 hub, use low-level code
	if ( pn->ds2409_depth>0 ) {
		return search_error ;
	}

	if (ds->LastDevice) {
		return search_done;
	}

	if (ds->index == -1) {
		if ( BAD(OWServer_Enet_directory(ds, in)) ) {
			return search_error;
		}
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("Index %d", ds->index);

	switch ( DirblobGet(ds->index, ds->sn, &(ds->gulp) ) ) {
		case 0:
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

enum ENET_dir { ENET_dir_ok, ENET_dir_repeat, ENET_dir_bad } ;

static GOOD_OR_BAD OWServer_Enet_directory(struct device_search *ds, struct connection_in * in) {
	int i ;
	for ( i=0 ; i<10 ; ++i ) {
		switch ( OWServer_Enet_directory_loop( ds, in ) ) {
			case ENET_dir_ok:
				return gbGOOD ;
			case ENET_dir_repeat:
				LEVEL_DEBUG("Repeating directory loop because of communication error");
				continue ;
			case ENET_dir_bad:
				LEVEL_DEBUG("directory error");
				return gbBAD;
		}
	}
	LEVEL_DEBUG("Too many attempts at a directory listing");
	return gbBAD ;
}

static enum ENET_dir OWServer_Enet_directory_loop(struct device_search *ds, struct connection_in * in)
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
	
	DirblobClear( &(ds->gulp) );

	if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
		in->AnyDevices = anydevices_no;
	}

	// send the first search
	if ( BAD( OWServer_Enet_write_string( search_first, in)) ) {
		return ENET_dir_bad ;
	}

	do {
		size_t read_so_far = 0 ;
		// read first character
		if ( BAD(OWServer_Enet_read(BYTE_string(resp), EXCLAIM_LENGTH, in)) ) {
			return ENET_dir_bad ;
		}
		
		switch (resp[0]) {
			case '!':
				return ENET_dir_ok ;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				read_so_far = EXCLAIM_LENGTH+in->master.serial.tcp.CRLF_size ;
				break ;
			default:
				return ENET_dir_repeat ;
		}
		
		// read rest of the characters
		if ( BAD(OWServer_Enet_read(BYTE_string(&resp[read_so_far]), DEVICE_LENGTH-read_so_far, in)) ) {
			return ENET_dir_bad;
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
			return ENET_dir_repeat;
		}

		DirblobAdd(sn, &(ds->gulp) );

		if (ds->search != _1W_CONDITIONAL_SEARCH_ROM) {
			in->AnyDevices = anydevices_yes;
		}

		// send the subsequent search
		if ( BAD(OWServer_Enet_write_string( search_next, in)) ) {
			return ENET_dir_bad ;
		}
	} while (1) ;
}

/* select a device for reference */
static GOOD_OR_BAD OWServer_Enet_select( const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	char send_address[] = "A7766554433221100\r" ;

	// Apparently need to reset before select (Unlike the HA7S)
	RETURN_BAD_IF_BAD( gbRESET( BUS_reset(pn)) ) ;
	
	if ( (pn->selected_device==NO_DEVICE) || (pn->selected_device==DeviceThermostat) ) {
		return gbGOOD ;
	}

	if ( memcmp( pn->sn, in->master.enet.sn, SERIAL_NUMBER_SIZE ) != 0 ) {
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

	return (OWServer_Enet_command( send_address, in ) == '+') ? gbGOOD : gbBAD ;
}

/* send a command and get the '+' */
static char OWServer_Enet_command( char * cmd_string, struct connection_in * in )
{
	char cmd_response[1+in->master.serial.tcp.CRLF_size] ;
	//printf("ENET: Send command\n");
	if ( BAD( OWServer_Enet_write_string( cmd_string, in )) ) {
		LEVEL_DEBUG("Error sending string <%s>", cmd_string ) ;
		return BAD_CHAR ;
	}

	//printf("ENET: Get Response\n");
	if ( BAD( OWServer_Enet_read( BYTE_string(cmd_response), 1, in )) ) {
		LEVEL_DEBUG("Error reading response to <%s>", cmd_string ) ;
		return BAD_CHAR ;
	}

	return cmd_response[0] ;
}

static GOOD_OR_BAD OWServer_Enet_read( BYTE * buf, size_t size, struct connection_in * in )
{
	return COM_read( buf, size+in->master.serial.tcp.CRLF_size, in ) ;
}

static GOOD_OR_BAD OWServer_Enet_write_string( char * buf, struct connection_in *in)
{
	int length = strlen(buf) ;
	//printf("ENET: Sending %d length string\n", length);
	if ( length == 0 ) {
		return gbGOOD ;
	}
	return OWServer_Enet_write( BYTE_string(buf), length, in ) ;
}

static GOOD_OR_BAD OWServer_Enet_write(const BYTE * buf, size_t size, struct connection_in *in)
{
	RETURN_GOOD_IF_GOOD( COM_write_simple( buf, size, in ) ) ;
	//printf("ENET: Error in sending data\n");

	if ( SOC(in)->file_descriptor == FILE_DESCRIPTOR_BAD ) {
		//printf("ENET: Connection closed\n");
		RETURN_BAD_IF_BAD( OWServer_Enet_reopen( in ) ) ;
		//printf("ENET: Connection reopened\n");
		RETURN_GOOD_IF_GOOD( COM_write_simple( buf, size, in ) ) ;
		//printf("ENET: Error sending data second time\n");
	}
	//printf("ENET: Bad sending experience\n");
	return gbBAD ;
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
	//printf("ENET sendback %d bytes\n",(int) size) ;
	for ( left=size ; left>0 ; left -= MAX_ENET_MEMORY_GULP ) {
		size_t pass_start = size - left ;
		size_t pass_size = (left>MAX_ENET_MEMORY_GULP) ? MAX_ENET_MEMORY_GULP : left ;
		RETURN_BAD_IF_BAD( OWServer_Enet_sendback_part( &data[pass_start], &resp[pass_start], pass_size, pn ) ) ;
	}
	return gbGOOD;
}

static void OWServer_Enet_close(struct connection_in *in)
{
	// the statndard COM_free cleans up the connection
	(void) in ;
}
