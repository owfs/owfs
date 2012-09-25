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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

#if OW_HA7
struct toHA7 {
	ASCII *command;
	ASCII lock[10];
	ASCII conditional[1];
	ASCII address[16];
	const BYTE *data;
	size_t length;
};

//static void byteprint( const BYTE * b, int size ) ;
static GOOD_OR_BAD HA7_write(const ASCII * msg, size_t size, struct connection_in *in);
static void toHA7init(struct toHA7 *ha7);
static void setHA7address(struct toHA7 *ha7, const BYTE * sn);
static GOOD_OR_BAD HA7_toHA7( const struct toHA7 *ha7, struct connection_in *in);
static GOOD_OR_BAD HA7_read( struct memblob *mb, struct connection_in * in );
static RESET_TYPE HA7_reset(const struct parsedname *pn);
static enum search_status HA7_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD HA7_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD HA7_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD HA7_sendback_block(const BYTE * data, BYTE * resp, const size_t size, int also_address, const struct parsedname *pn);
static GOOD_OR_BAD HA7_select(const struct parsedname *pn);
static void HA7_setroutines(struct connection_in *in);
static void HA7_close(struct connection_in *in);
static GOOD_OR_BAD HA7_directory( struct device_search *ds, const struct parsedname *pn);

static void HA7_setroutines(struct connection_in *in)
{
	in->iroutines.detect = HA7_detect;
	in->iroutines.reset = HA7_reset;
	in->iroutines.next_both = HA7_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.select_and_sendback = HA7_select_and_sendback;
	in->iroutines.sendback_data = HA7_sendback_data;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = HA7_select;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = HA7_close;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2404delay ;
	in->bundling_length = HA7_FIFO_SIZE;	// arbitrary number
}

GOOD_OR_BAD HA7_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct parsedname pn;
	struct toHA7 ha7;

	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	HA7_setroutines(in);

	in->master.ha7.locked = 0;

	if (pin->init_data == NULL) {
		return gbBAD;
	} else {
		DEVICENAME(in) = owstrdup(pin->init_data) ;
	}

	pin->type = ct_tcp ;
	pin->timeout.tv_sec = Globals.timeout_ha7 ;
	pin->timeout.tv_usec = 0 ;
	RETURN_BAD_IF_BAD( COM_open(in) ) ;

	in->Adapter = adapter_HA7NET;

	toHA7init(&ha7);
	ha7.command = "ReleaseLock";
	if (GOOD( HA7_toHA7( &ha7, in)) ) {
		struct memblob mb;
		if ( GOOD( HA7_read( &mb, in )) ) {
			in->adapter_name = "HA7Net";
			pin->busmode = bus_ha7net;
			in->AnyDevices = anydevices_yes;
			MemblobClear(&mb);
			return gbGOOD;
		}
	}
	COM_close(in) ;
	return gbBAD;
}

static RESET_TYPE HA7_reset(const struct parsedname *pn)
{
	struct memblob mb;
	RESET_TYPE ret = BUS_RESET_OK;
	struct toHA7 ha7;
	struct connection_in * in = pn->selected_connection ;

	toHA7init(&ha7);
	ha7.command = "Reset";
	if ( BAD(HA7_toHA7( &ha7, in)) ) {
		LEVEL_DEBUG("Trouble sending reset command");
		ret = BUS_RESET_ERROR;
	} else if ( BAD(HA7_read( &mb, in )) ) {
		LEVEL_DEBUG("Trouble with reset command response");
		ret = BUS_RESET_ERROR;
	}
	MemblobClear(&mb);
	return ret;
}

static GOOD_OR_BAD HA7_directory( struct device_search *ds, const struct parsedname *pn)
{
	GOOD_OR_BAD ret = gbGOOD;
	struct toHA7 ha7;
	struct memblob mb;
	struct connection_in * in = pn->selected_connection ;

	DirblobClear(&(ds->gulp));

	toHA7init(&ha7);
	ha7.command = "Search";
	if ( ds->search == _1W_CONDITIONAL_SEARCH_ROM) {
		ha7.conditional[0] = '1';
	}

	if ( BAD(HA7_toHA7( &ha7, in)) ) {
		ret = gbBAD;
	} else if ( BAD(HA7_read( &mb, in )) ) {
		STAT_ADD1_BUS(e_bus_read_errors, in);
		ret = gbBAD;
	} else {
		BYTE sn[SERIAL_NUMBER_SIZE];
		ASCII *p = (ASCII *) MemblobData(&mb);
		while ((p = strstr(p, "<INPUT CLASS=\"HA7Value\" NAME=\"Address_"))
			   && (p = strstr(p, "VALUE=\""))) {
			p += 7;
			if (strspn(p, "0123456789ABCDEF") < 16) {
				ret = gbBAD;
				break;
			}
			sn[7] = string2num(&p[0]);
			sn[6] = string2num(&p[2]);
			sn[5] = string2num(&p[4]);
			sn[4] = string2num(&p[6]);
			sn[3] = string2num(&p[8]);
			sn[2] = string2num(&p[10]);
			sn[1] = string2num(&p[12]);
			sn[0] = string2num(&p[14]);
			if (CRC8(sn, SERIAL_NUMBER_SIZE)) {
				ret = gbBAD;
				break;
			}
			DirblobAdd(sn, &(ds->gulp));
		}
		MemblobClear(&mb);
	}
	return ret;
}

static enum search_status HA7_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (ds->LastDevice) {
		return search_done;
	}

	if (++(ds->index) == 0) {
		if ( BAD(HA7_directory( ds, pn )) ) {
			return search_error;
		}
	}
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

#define HA7_READ_BUFFER_LENGTH 2000

static GOOD_OR_BAD HA7_read( struct memblob *mb, struct connection_in * in )
{
	struct port_in * pin = in->head ;
	
	ASCII readin_area[HA7_READ_BUFFER_LENGTH + 1];
	ASCII *start;
	size_t read_size;

	MemblobInit(mb, HA7_READ_BUFFER_LENGTH);
	pin->timeout.tv_sec = 2 ;
	pin->timeout.tv_usec = 0 ;

	// Read first block of data from HA7
	read_size = COM_read_with_timeout( (BYTE*) readin_area, HA7_READ_BUFFER_LENGTH, in) ;
	if ( read_size <= 0 ) {
		LEVEL_CONNECT("Read error");
		return gbBAD;
	}
	// make sure null terminated (allocated extra byte in readin_area to always have room)
	readin_area[read_size] = '\0';

	// Look for happy response
	if (strncmp("HTTP/1.1 200 OK", readin_area, 15)) {	//Bad HTTP return code
		ASCII *p = strchr(&readin_area[15], '\n');
		if (p == NULL) {
			p = &readin_area[15 + 32];
		}
		LEVEL_DATA("response problem:%.*s", p - readin_area - 15, &readin_area[15]);
		return gbBAD;
	}
	// Look for "<body>"
	if ((start = strstr(readin_area, "<body>")) == NULL) {
		LEVEL_DATA("response: No HTTP body to parse");
		MemblobClear(mb);
		return gbBAD;
	}
	// HTML body found, dump header
	if (MemblobAdd((BYTE *) start, read_size - (start - readin_area), mb)) {
		MemblobClear(mb);
		return gbBAD;
	}
	// loop through reading in HA7_READ_BUFFER_LENGTH blocks
	while (read_size == HA7_READ_BUFFER_LENGTH) {	// full read, so presume more waiting
		read_size = COM_read_with_timeout( (BYTE*) readin_area, HA7_READ_BUFFER_LENGTH, in) ;
		if (read_size <= 0) {
			LEVEL_DATA("Couldn't get rest of HA7 data (err=%d)", read_size);
			MemblobClear(mb);
			return gbBAD;
		} else if (MemblobAdd((BYTE *) readin_area, read_size, mb)) {
			MemblobClear(mb);
			return gbBAD;
		}
	}

	// Add trailing null
	if (MemblobAdd((BYTE *) "", 1, mb)) {
		MemblobClear(mb);
		return gbBAD;
	}
	LEVEL_DEBUG("Successful read of data");
	//printf("READ FROM HA7:\n%s\n",MemblobData(mb));
	return gbGOOD;
}

static GOOD_OR_BAD HA7_write( const ASCII * msg, size_t length, struct connection_in *in )
{
	return COM_write( (const BYTE *) msg, length, in) ;
}

static GOOD_OR_BAD HA7_toHA7( const struct toHA7 *ha7, struct connection_in *in)
{
	int first = 1;
	int probable_length;
	char *full_command;
	LEVEL_DEBUG
		("To HA7 command=%s address=%.16s conditional=%.1s lock=%.10s",
		 SAFESTRING(ha7->command), SAFESTRING(ha7->address), SAFESTRING(ha7->conditional), SAFESTRING(ha7->lock));

	if (ha7->command == NULL) {
		return gbBAD;
	}

	probable_length = 11 + strlen(ha7->command) + 5 + ((ha7->address[0]) ? 1 + 8 + 16 : 0)
		+ ((ha7->conditional[0]) ? 1 + 12 + 1 : 0)
		+ ((ha7->data) ? 1 + 5 + ha7->length * 2 : 0)
		+ ((ha7->lock[0]) ? 1 + 7 + 10 : 0)
		+ 11 + 1;

	full_command = owmalloc(probable_length);
	if (full_command == NULL) {
		return gbBAD;
	}
	memset(full_command, 0, probable_length);

	strcpy(full_command, "GET /1Wire/");
	strcat(full_command, ha7->command);
	strcat(full_command, ".html");

	if (ha7->address[0]) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "Address=");
		strcat(full_command, ha7->address);
		first = 0;
	}

	if (ha7->conditional[0]) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "Conditional=");
		strcat(full_command, ha7->conditional);
		first = 0;
	}

	if (ha7->data) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "Data=");
		bytes2string(&full_command[strlen(full_command)], ha7->data, ha7->length);
	}

	if (ha7->lock[0]) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "LockID=");
		strcat(full_command, ha7->lock);
		first = 0;
	}

	strcat(full_command, " HTTP/1.0\n\n");

	LEVEL_DEBUG("To HA7 %s", full_command);

	if ( BAD( COM_open(in) ) ) {
		// force reopen
		owfree(full_command);
		return gbBAD ;
	}

	if ( BAD( HA7_write( full_command, probable_length, in) ) ) {
		owfree(full_command);
		return gbBAD ;
	}
		
	owfree(full_command);
	return gbGOOD;
}

// Reset, select, and read/write data
/* return 0=good
   sendout_data, readin
 */
static GOOD_OR_BAD HA7_select_and_sendback(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t location = 0;
	int also_address = 1;

	while (location < size) {
		size_t block = size - location;
		if (block > 32) {
			block = 32;
		}
		// Don't add address (that's the "0")
		RETURN_BAD_IF_BAD(HA7_sendback_block(&data[location], &resp[location], block, also_address, pn)) ;
		location += block;
		also_address = 0;		//for subsequent blocks
	}
	return gbGOOD ;
}

//  Send data and return response block
/* return 0=good
   sendout_data, readin
 */
#define HA7_CONSERVATIVE_LENGTH 32
static GOOD_OR_BAD HA7_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t location = 0;

	while (location < size) {
		size_t block = size - location;
		if (block > HA7_CONSERVATIVE_LENGTH) {
			block = HA7_CONSERVATIVE_LENGTH;
		}
		// Don't add address (that's the "0")
		RETURN_BAD_IF_BAD(HA7_sendback_block(&data[location], &resp[location], block, 0, pn)) ;
		location += block;
	}
	return gbGOOD;
}

// HA7 only allows WriteBlock of 32 bytes
// This routine assumes that larger writes have already been broken up
static GOOD_OR_BAD HA7_sendback_block(const BYTE * data, BYTE * resp, const size_t size, int also_address, const struct parsedname *pn)
{
	struct memblob mb;
	struct toHA7 ha7;
	GOOD_OR_BAD ret = gbBAD;
	struct connection_in * in =  pn->selected_connection ;

	toHA7init(&ha7);
	ha7.command = "WriteBlock";
	ha7.data = data;
	ha7.length = size;
	if (also_address) {
		setHA7address(&ha7, pn->sn);
	}

	if ( GOOD( HA7_toHA7( &ha7, in)) ) {
		if ( GOOD( HA7_read( &mb, in )) ) {
			ASCII *p = (ASCII *) MemblobData(&mb);
			if ((p = strstr(p, "<INPUT TYPE=\"TEXT\" NAME=\"ResultData_0\""))
				&& (p = strstr(p, "VALUE=\""))) {
				p += 7;
				LEVEL_DEBUG("HA7_sendback_data received(%d): %.*s", size * 2, size * 2, p);
				if (strspn(p, "0123456789ABCDEF") >= size << 1) {
					string2bytes(p, resp, size);
					ret = gbGOOD;
				}
			}
			MemblobClear(&mb);
		} else {
			STAT_ADD1_BUS(e_bus_read_errors, in);
		}
		return ret ;
	}
	return gbGOOD;
}

static void setHA7address(struct toHA7 *ha7, const BYTE * sn)
{
	num2string(&(ha7->address[0]), sn[7]);
	num2string(&(ha7->address[2]), sn[6]);
	num2string(&(ha7->address[4]), sn[5]);
	num2string(&(ha7->address[6]), sn[4]);
	num2string(&(ha7->address[8]), sn[3]);
	num2string(&(ha7->address[10]), sn[2]);
	num2string(&(ha7->address[12]), sn[1]);
	num2string(&(ha7->address[14]), sn[0]);
}

static GOOD_OR_BAD HA7_select(const struct parsedname *pn)
{
	GOOD_OR_BAD ret = gbBAD;
	struct connection_in * in =  pn->selected_connection ;

	if (pn->selected_device) {
		struct toHA7 ha7;
		toHA7init(&ha7);
		ha7.command = "AddressDevice";
		setHA7address(&ha7, pn->sn);
		if ( GOOD( HA7_toHA7( &ha7, in)) ) {
			struct memblob mb;
			if ( GOOD( HA7_read( &mb, in )) ) {
				MemblobClear(&mb);
				ret = gbGOOD;
			}
		}
	} else {
		return HA7_reset(pn)==BUS_RESET_OK ? gbGOOD : gbBAD ;
	}
	return ret;
}

static void HA7_close(struct connection_in *in)
{
	// that standard COM_free cleans up the connection
	(void) in ;
}

static void toHA7init(struct toHA7 *ha7)
{
	memset(ha7, 0, sizeof(struct toHA7));
}

#endif							/* OW_HA7 */
