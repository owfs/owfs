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
#include "ow_specialcase.h"

struct toENET {
	ASCII *command;
	ASCII address[16];
	const BYTE *data;
	size_t length;
};

//static void byteprint( const BYTE * b, int size ) ;
static int ENET_write(int file_descriptor, const ASCII * msg, size_t length, struct connection_in *in) ;
static int OWServer_Enet_reset(const struct parsedname *pn);
static void OWServer_Enet_close(struct connection_in *in);
#if 0
static void toENETinit(struct toENET *enet) ;
#endif
static int ENET_send_detail(int file_descriptor, struct connection_in *in) ;
static int ENET_get_detail(const struct parsedname * pn ) ;
static int OWServer_Enet_read(int file_descriptor, struct memblob *mb) ;
static int Add_a_property(const char * tag, const char * property, const char * romid, char ** buffer) ;
static int parse_detail_record(char * detail, const struct parsedname *pn) ;
static char * find_xml_string( const char * tag, size_t * length, char * buffer) ;

static int SpecialRead( struct one_wire_query * owq ) ;
static void ENET_SpecialCases( struct connection_in * in ) ;

static int xml_integer( const char * tag, char ** buffer) ;
static char * xml_string( const char * tag, char ** buffer) ;
static _FLOAT xml_float( const char * tag, char ** buffer) ;

static int OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn);
static int OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void OWServer_Enet_setroutines(struct connection_in *in);
static int OWServer_Enet_select( const struct parsedname * pn ) ;

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
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2409path | ADAP_FLAG_presence_from_dirblob ;
	in->bundling_length = HA7E_FIFO_SIZE;
}

int OWServer_Enet_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;

	/* Set up low-level routines */
	OWServer_Enet_setroutines(in);

	if (in->name == NULL) {
		return -1;
	}

	/* Add the port if it isn't there already */
	if (strchr(in->name, ':') == NULL) {
		ASCII *temp = owrealloc(in->name, strlen(in->name) + 3);
		if (temp == NULL) {
			return -ENOMEM;
		}
		in->name = temp;
		strcat(in->name, ":80");
	}

	if (ClientAddr(in->name, in)) {
		return -EIO;
	}

	in->Adapter = adapter_ENET;
	in->adapter_name = "OWServer_Enet";
	in->busmode = bus_enet;

	if (ENET_get_detail(&pn) == 0) {
		ENET_SpecialCases(in) ; // load the special access functions
		return 0;
	}
	return -ENODEV;
}

#define HA7_READ_BUFFER_LENGTH 500

static int OWServer_Enet_read(int file_descriptor, struct memblob *mb)
{
	ASCII readin_area[HA7_READ_BUFFER_LENGTH + 1];
	int first_pass = 1 ;
	size_t read_size;
	struct timeval tvnet = { Globals.timeout_ha7, 0, };
	
	MemblobInit(mb, HA7_READ_BUFFER_LENGTH);

	while (	tcp_read(file_descriptor, readin_area, HA7_READ_BUFFER_LENGTH, &tvnet, &read_size) == 0 ) {
		// make sure null terminated (allocated extra byte in readin_area to always have room)
		//printf("read_size = %d\n",(int)read_size) ;
		readin_area[read_size] = '\0';
		//printf("%s\n",readin_area);
		if ( first_pass ) {
			first_pass = 0 ;
			if ( read_size < 38 ) {
				LEVEL_DEBUG("Got a very short read from details.xml") ;
				return -EIO ;
			}
			// Look for happy response
			if (strncmp("HTTP/1.1 200 OK", readin_area, 15)) {	//Bad HTTP return code
				LEVEL_DATA("Bad HTTP response from the ENET server");
				return -EINVAL;
			}
			if (strstr(readin_area,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>")==NULL) {	//Bad XML header
				LEVEL_DATA("response problem with XML: %s", readin_area);
				return -EINVAL;
			}
		}
		if (MemblobAdd((BYTE *) readin_area, read_size, mb)) {
			MemblobClear(mb);
			return -ENOMEM;
		}
		if (read_size < HA7_READ_BUFFER_LENGTH) {
			break;
		}
	}

	// Add trailing null
	if (MemblobAdd((BYTE *) "", 1, mb)) {
		MemblobClear(mb);
		return -ENOMEM;
	}
	LEVEL_DEBUG("READ FROM ENET:\n%s",MemblobData(mb));
	return 0;
}

static int ENET_get_detail(const struct parsedname *pn) {
	int file_descriptor;
	int ret = -EIO ;
	struct dirblob *db = &(pn->selected_connection->main) ;

	DirblobClear(db);
	if ((file_descriptor = ClientConnect(pn->selected_connection)) < 0) {
		return -EIO;
	}
	if (ENET_send_detail(file_descriptor,pn->selected_connection)==0) {
		struct memblob mb;
		if ( OWServer_Enet_read(file_descriptor,&mb) == 0 ) {
			ret = parse_detail_record( (char *) MemblobData(&mb), pn ) ;
			MemblobClear(&mb);
		}
	} else {
		LEVEL_DEBUG("Cannot get details.xml from OW-SERVER-ENET");
	}
	close(file_descriptor);
	return ret ;
}

static int parse_detail_record(char * detail, const struct parsedname *pn) 
{
	struct connection_in * in = pn->selected_connection ;
	struct dirblob *db = &(in->main) ;
	int d,devices ;
	
	devices = xml_integer( "DevicesConnected", &detail ) ;
	if ( detail==NULL) {
		return -EINVAL ;
	}
	LEVEL_DEBUG("devices=%d\n",devices) ;
		
	for ( d=0 ; d<devices ; ++d ) {
		char * romid = xml_string( "ROMId", &detail) ;
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		if (detail==NULL) {
			return -EINVAL ;
		}
		sn[0] = string2num(&romid[0]);
		sn[1] = string2num(&romid[2]);
		sn[2] = string2num(&romid[4]);
		sn[3] = string2num(&romid[6]);
		sn[4] = string2num(&romid[8]);
		sn[5] = string2num(&romid[10]);
		sn[6] = string2num(&romid[12]);
		sn[7] = string2num(&romid[14]);

		LEVEL_DEBUG("SN found: " SNformat, SNvar(sn));
		// CRC check
		if (CRC8(sn, SERIAL_NUMBER_SIZE) || (sn[0] == 0)) {
			/* A minor "error" and should perhaps only return -1 */
			/* to avoid reconnect */
			LEVEL_DEBUG("sn = %s\n", romid);
		}
		Cache_Add_Device(in->index,sn) ;
		DirblobAdd(sn, db);
		switch ( sn[0] ) {
		case 0x10: //DS18S20
			Add_a_property("Temperature", "temperature", romid, &detail ) ;
			Add_a_property("UserByte1", "templow", romid, &detail ) ;
			Add_a_property("UserByte2", "temphigh", romid, &detail ) ;
			break ;
		case 0x28: //DS18B20
			Add_a_property("Temperature", "temperature", romid, &detail ) ;
			Add_a_property("UserByte1", "templow", romid, &detail ) ;
			Add_a_property("UserByte2", "temphigh", romid, &detail ) ;
			Add_a_property("PwrSupply", "power", romid, &detail ) ;
			break ;
		case 0x1D: //DS2423
			Add_a_property("CounterA", "counters.A", romid, &detail ) ;
			Add_a_property("CounterB", "counters.B", romid, &detail ) ;
			break ;
		case 0x26: //DS2438
			Add_a_property("Temperature", "temperature", romid, &detail ) ;
			Add_a_property("SupplyVoltage", "VDD", romid, &detail ) ;
			Add_a_property("Current", "vis", romid, &detail ) ;
			Add_a_property("PinVoltage", "VAD", romid, &detail ) ;
			break ;
		case 0x29: //DS2408
			Add_a_property("PIOLogicState", "sensed", romid, &detail ) ;
			Add_a_property("PIOOutputLatchState", "PIO", romid, &detail ) ;
			Add_a_property("PIOActivityLatchState", "latch", romid, &detail ) ;
			Add_a_property("RSTZConfiguration", "strobe", romid, &detail ) ;
			Add_a_property("PowerOnResetLatch", "por", romid, &detail ) ;
			Add_a_property("VCCPowerStatus", "power", romid, &detail ) ;
			break ;
		case 0x20: //DS2450
			Add_a_property("ChannelAConversionValue", "volt.A", romid, &detail ) ;
			Add_a_property("ChannelBConversionValue", "volt.B", romid, &detail ) ;
			Add_a_property("ChannelCConversionValue", "volt.C", romid, &detail ) ;
			Add_a_property("ChannelDConversionValue", "volt.D", romid, &detail ) ;
			Add_a_property("ChannelAOutputControl", "PIO.A", romid, &detail ) ;
			Add_a_property("ChannelBOutputControl", "PIO.B", romid, &detail ) ;
			Add_a_property("ChannelCOutputControl", "PIO.C", romid, &detail ) ;
			Add_a_property("ChannelDOutputControl", "PIO.D", romid, &detail ) ;
			Add_a_property("VCCControl", "power", romid, &detail ) ;
			break ;
		default:
			break ;
		}
			
	}
	return 0 ;
}

static int SpecialRead( struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	enum ePS_state state = pn->state ;
	if ( ENET_get_detail(pn) != 0 ) {
		return -EINVAL ;
	}
	pn->state |= ~ePS_uncached ; // turn off uncached
	if (OWQ_Cache_Get(owq)) {
		// Not found even after re-reading detail
		pn->state = state ;
		return -EINVAL ;
	}
	pn->state = state ;
	return 0 ;
}

static void ENET_SpecialCases( struct connection_in * in )
{
	SpecialCase_add( in, 0x10, "temperature", SpecialRead, NO_WRITE_FUNCTION ) ;
	SpecialCase_add( in, 0x10, "templow",     SpecialRead, NO_WRITE_FUNCTION ) ;
	SpecialCase_add( in, 0x10, "temphigh",    SpecialRead, NO_WRITE_FUNCTION ) ;
	
	SpecialCase_add( in, 0x28, "temperature", SpecialRead, NO_WRITE_FUNCTION ) ;
	SpecialCase_add( in, 0x28, "templow",     SpecialRead, NO_WRITE_FUNCTION ) ;
	SpecialCase_add( in, 0x28, "temphigh",    SpecialRead, NO_WRITE_FUNCTION ) ;
	SpecialCase_add( in, 0x28, "power",       SpecialRead, NO_WRITE_FUNCTION ) ;
	
	SpecialCase_add( in, 0x1D, "counters.A",  SpecialRead, NO_WRITE_FUNCTION ) ;
	SpecialCase_add( in, 0x1D, "counters.B",  SpecialRead, NO_WRITE_FUNCTION ) ;	

	SpecialCase_add( in, 0x26, "temperature", SpecialRead, NO_WRITE_FUNCTION ) ;	
	SpecialCase_add( in, 0x26, "VAD",         SpecialRead, NO_WRITE_FUNCTION ) ;	
	SpecialCase_add( in, 0x26, "VDD",         SpecialRead, NO_WRITE_FUNCTION ) ;	
	SpecialCase_add( in, 0x26, "vis",         SpecialRead, NO_WRITE_FUNCTION ) ;	

}

#define enet_data_length 1000
static int Add_a_property(const char * tag, const char * property, const char * romid, char ** buffer)
{
	char path[PATH_MAX] ;
	struct one_wire_query * owq ;
	char * buffer_pointer = buffer[0] ;
	int ret = 0 ;
	
	if ( buffer[0] == NULL ) {
		return -EINVAL ;
	}
	//printf("About to create %s from %s\n",property,tag);
	strncpy( path, romid, 16 ) ;
	path[16] = '\0' ;
	strcat(path,"/");
	strcat(path,property) ;
	
	owq = OWQ_create_from_path(path) ; // for write
	//printf("Created OWQ for %s from %s",path,tag);
	
	if ( owq==NULL ) {
		LEVEL_DEBUG("Couldn't understand path %s",path);
		return -EINVAL ;
	}
	
	switch (PN(owq)->selected_filetype->format) {
	case ft_ascii:
	case ft_vascii:
	case ft_binary:
	{
		char * data = xml_string(tag,&buffer_pointer) ;
		if ( data == NULL ) {
			ret = -EINVAL ;
		} else if ( OWQ_allocate_write_buffer(data,enet_data_length,owq) != 0 ) {
			ret = -EINVAL ;
		}
		LEVEL_DEBUG("%s given value <%s> from tag %s",path,data,tag) ;
		break ;
	}
	case ft_integer:
		OWQ_I(owq) = xml_integer(tag,&buffer_pointer) ;
		LEVEL_DEBUG("%s given value %d from tag %s",path,(int)OWQ_I(owq),tag) ;
		break ;
	case ft_unsigned:
		OWQ_U(owq) = xml_integer(tag,&buffer_pointer) ;
		LEVEL_DEBUG("%s given value %d from tag %s",path,(int)OWQ_U(owq),tag) ;
		break ;
	case ft_yesno:
		OWQ_Y(owq) = (xml_integer(tag,&buffer_pointer) != 0) ;
		LEVEL_DEBUG("%s given value %d from tag %s",path,(int)OWQ_Y(owq),tag) ;
		break ;
	case ft_date:
	case ft_float:
		break ;
	case ft_pressure:
	case ft_temperature:
	case ft_tempgap:
		//printf("get the float\n");
		OWQ_F(owq) = xml_float(tag,&buffer_pointer) ;
		LEVEL_DEBUG("%s given value %lg from tag %s",path,OWQ_F(owq),tag) ;
		break ;
	default:
		ret = -EINVAL ;
		break;
	}
	if ( buffer_pointer != NULL ) {
		OWQ_Cache_Add(owq) ;
	} else {
		ret = -EINVAL ;
	}
	OWQ_destroy(owq) ;
	return ret ;
}

static int xml_integer( const char * tag, char ** buffer)
{
	size_t length ;
	buffer[0] = find_xml_string(tag,&length, buffer[0]) ;
	if ( buffer[0] != NULL ){
		return strtol( buffer[0], buffer, 0) ;
	}
	return 0 ;
}

static _FLOAT xml_float( const char * tag, char ** buffer)
{
	size_t length ;
	buffer[0] = find_xml_string(tag,&length, buffer[0]) ;
	//printf("found the string %s, length %lu\n",buffer[0],length);
	if ( buffer[0] != NULL ){
		_FLOAT F ;
		if ( sscanf(buffer[0],"%lg",&F) == 1 ) {
			//printf("Value %lg\n",F) ;
			return F ;
		}
	}
	return 0. ;
}

static char * xml_string( const char * tag, char ** buffer)
{
	size_t length ;
	buffer[0] = find_xml_string(tag,&length, buffer[0]) ;
	if ( buffer[0] != NULL ){
		char * ret = buffer[0] ;
		ret[length] = '\0' ;
		buffer[0] = ret + length + 1 ;
		return ret ;
	}
	return NULL ;
}

static char * find_xml_string( const char * tag, size_t * length, char * buffer)
{
	char * tag1 = owmalloc(strlen(tag)+2) ;
	char * tag2 = owmalloc(strlen(tag)+4) ;
	char * found1 ;
	char * data ;
	char * found2 ;

	strcpy(tag1,"<") ;
	strcat(tag1,tag);
	found1 = strstr( buffer, tag1 ) ;
	if ( found1 == NULL ) {
		LEVEL_DEBUG("No opening XML tag %s in %s",tag1,buffer) ;
		return NULL ;
	}
	data = found1 + strlen(tag1) ;
	switch (data[0]) {
	case ' ':
		data = strchr(data,'>') ;
		if ( data == NULL ) {
			LEVEL_DEBUG("Bad opening XML tag %s in %s",tag1,buffer) ;
			return NULL ;
		}
		// fall through
	case '>':
		++data ;
		break ;
	default :
		LEVEL_DEBUG("No opening XML tag %s in %s",tag1,buffer) ;
		return NULL ;
	}
	
	strcpy(tag2,"</") ;
	strcat(tag2,tag);
	strcat(tag2,">") ;
	found2 = strstr( data, tag2 ) ;
	if ( found2 == NULL) {
		LEVEL_DEBUG("No closing XML tag %s in %s",tag2,buffer) ;
		return NULL ;
	}
	*length = found2 - data ;
	return data ;
}

static int OWServer_Enet_reset(const struct parsedname *pn)
{
	(void) pn ;
	return BUS_RESET_OK;
}

static int OWServer_Enet_next_both(struct device_search *ds, const struct parsedname *pn)
{
	int ret = 0;
	struct dirblob *db = &(pn->selected_connection->main) ;

	if ( ds->search == _1W_CONDITIONAL_SEARCH_ROM ) {
		return -ENOTSUP ;
	}

	if (ds->LastDevice) {
		return -ENODEV;
	}

	if (ds->index == -1) {
		if (ENET_get_detail(pn)!=0) {
			return -EIO;
		}
	}
	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d", ds->index);

	ret = DirblobGet(ds->index, ds->sn, db);
	LEVEL_DEBUG("DirblobGet %d\n", ret);
	switch (ret) {
	case 0:
		break;
	case -ENODEV:
		ds->LastDevice = 1;
		break;
	}

	LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));
	return ret;
}

/* select a device for reference */
/* Don't do much in this case */
static int OWServer_Enet_select( const struct parsedname * pn )
{
	// Set as current "Address" for adapter
	memcpy( pn->selected_connection->connin.enet.sn, pn->sn, SERIAL_NUMBER_SIZE) ;

	return 0 ;
}

static int OWServer_Enet_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	(void) data;
	(void) resp;
	(void) size;
	(void) pn ;
	return -ENOTSUP;
}

static void OWServer_Enet_close(struct connection_in *in)
{
	FreeClientAddr(in);
}

#if 0
static void toENETinit(struct toENET *enet)
{
	memset(enet, 0, sizeof(struct toENET));
}
#endif

static int ENET_write(int file_descriptor, const ASCII * msg, size_t length, struct connection_in *in)
{
	ssize_t r, sl = length;
	ssize_t size = sl;
	while (sl > 0) {
		r = write(file_descriptor, &msg[size - sl], sl);
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			ERROR_CONNECT("Trouble writing data to OW-SERVER-ENET: %s", SAFESTRING(in->name));
			break;
		}
		sl -= r;
	}
	gettimeofday(&(in->bus_write_time), NULL);
	if (sl > 0) {
		STAT_ADD1_BUS(e_bus_write_errors, in);
		return -EIO;
	}
	return 0;
}

static int ENET_send_detail(int file_descriptor, struct connection_in *in)
{
	char * full_command = "GET /details.xml HTTP/1.0\x0D\x0A\x0D\x0A" ;
	LEVEL_DEBUG("To ENET %s", full_command);

	return ENET_write(file_descriptor, full_command, strlen(full_command)+1, in);
}
