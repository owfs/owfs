/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_codes.h"

/* All the remaining_device_list of the program sees is the Fake_detect and the entry in iroutines */

static RESET_TYPE Fake_reset(const struct parsedname *pn);
static GOOD_OR_BAD Fake_ProgramPulse(const struct parsedname *pn);
static GOOD_OR_BAD Fake_sendback_bits(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void Fake_close(struct connection_in *in);
static enum search_status Fake_next_both(struct device_search *ds, const struct parsedname *pn);
static const ASCII *namefind(const char *name);
static void Fake_setroutines(struct connection_in *in);
static GOOD_OR_BAD Fake_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void GetNextByte( const ASCII ** strpointer, BYTE default_byte, BYTE * sn ) ;
static void GetDeviceName(const ASCII ** strpointer, struct connection_in * in) ;
static void GetDefaultDeviceName(BYTE * dn, const BYTE * sn, const struct connection_in * in) ;
static void GetAllDeviceNames( struct port_in * pin ) ;
static void SetConninData( int indx, const char * type, struct port_in *pin ) ;

static void Fake_setroutines(struct connection_in *in)
{
	in->iroutines.detect = Fake_detect;
	in->iroutines.reset = Fake_reset;
	in->iroutines.next_both = Fake_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = Fake_ProgramPulse;
	in->iroutines.sendback_data = Fake_sendback_data;
	in->iroutines.sendback_bits = Fake_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = Fake_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	in->iroutines.flags = ADAP_FLAG_no2409path | ADAP_FLAG_presence_from_dirblob | ADAP_FLAG_no2404delay ;

	DirblobInit( &(in->master.fake.main) );
	DirblobInit( &(in->master.fake.alarm) );
}

static GOOD_OR_BAD Fake_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	(void) pn;
	(void) data;
	(void) resp;
	(void) len;
	return gbGOOD;
}

static void GetNextByte( const ASCII ** strpointer, BYTE default_byte, BYTE * sn )
{
	if ( (*strpointer)[0] == '.' ) {
		++*strpointer ;
	}
	if ( isxdigit((*strpointer)[0]) && isxdigit((*strpointer)[1]) ) {
		*sn = string2num(*strpointer) ;
		*strpointer += 2 ;
	} else {
		*sn = default_byte ;
	}
}

// set up default device ID
static void GetDefaultDeviceName(BYTE * dn, const BYTE * sn, const struct connection_in * in)
{
	switch (get_busmode(in)) {
		case bus_tester:
			// "bus number"
			dn[1] = BYTE_MASK(in->master.tester.index >> 0) ;
			dn[2] = BYTE_MASK(in->master.tester.index >> 8) ;
			// repeat family code
			dn[3] = sn[0] ;
			// family code complement
			dn[4] = BYTE_INVERSE(sn[0]) ;
			// "device" number
			dn[5] = BYTE_MASK(DirblobElements(&(in->master.fake.main)) >> 0) ;
			dn[6] = BYTE_MASK(DirblobElements(&(in->master.fake.main)) >> 8) ;
			break ;
		case bus_fake:
		case bus_mock:
		default: // only for compiler warning
			dn[1]  = BYTE_MASK(rand()) ;
			dn[2]  = BYTE_MASK(rand()) ;
			dn[3]  = BYTE_MASK(rand()) ;
			dn[4]  = BYTE_MASK(rand()) ;
			dn[5]  = BYTE_MASK(rand()) ;
			dn[6]  = BYTE_MASK(rand()) ;
			break ;
	}
}

static void GetDeviceName(const ASCII ** strpointer, struct connection_in * in)
{
	BYTE sn[SERIAL_NUMBER_SIZE] ;
	BYTE dn[SERIAL_NUMBER_SIZE] ;

	if ( isxdigit((*strpointer)[0])	&& isxdigit((*strpointer)[1]) ) {
		// family code specified
		sn[0] = string2num(*strpointer);
		*strpointer +=  2;

		GetDefaultDeviceName( dn, sn, in ) ;
		// Choice of default or specified ID
		GetNextByte(strpointer,dn[1],&sn[1]);
		GetNextByte(strpointer,dn[2],&sn[2]);
		GetNextByte(strpointer,dn[3],&sn[3]);
		GetNextByte(strpointer,dn[4],&sn[4]);
		GetNextByte(strpointer,dn[5],&sn[5]);
		GetNextByte(strpointer,dn[6],&sn[6]);
	} else {
		const ASCII * name_to_familycode = namefind((*strpointer)) ;
		if (  name_to_familycode != NULL) {
			// device name specified (e.g. DS2401)
			sn[0] = string2num(name_to_familycode);
			GetDefaultDeviceName( dn, sn, in ) ;
			sn[1] = dn[1] ;
			sn[2] = dn[2] ;
			sn[3] = dn[3] ;
			sn[4] = dn[4] ;
			sn[5] = dn[5] ;
			sn[6] = dn[6] ;
		} else {
			// Bad device name
			LEVEL_DEFAULT("Device %d <%s> not recognized for %s %d -- ignored",DirblobElements(&(in->master.fake.main))+1,*strpointer,in->adapter_name,in->master.fake.index);
			return ;
		}
	}
	sn[SERIAL_NUMBER_SIZE-1] = CRC8compute(sn, SERIAL_NUMBER_SIZE-1, 0);
	DirblobAdd(sn, &(in->master.fake.main));	// Ignore bad return
}

static void GetAllDeviceNames( struct port_in * pin )
{
	struct connection_in * in = pin->first ;
	ASCII * remaining_device_list = owstrdup( pin->init_data ) ;
	ASCII * remember_location = remaining_device_list ;

	while (remaining_device_list != NULL) {
		const ASCII *current_device_start;
		for (current_device_start = strsep(&remaining_device_list, " ,"); current_device_start[0] != '\0'; ++current_device_start) {
			// note that strsep updates "remaining_device_list" pointer
			if (current_device_start[0] != ' ' && current_device_start[0] != ',') {
				break;
			}
		}
		GetDeviceName( &current_device_start, in ) ;
	}
	SAFEFREE( remember_location ) ;
	in->AnyDevices = (DirblobElements(&(in->master.fake.main)) > 0) ? anydevices_yes : anydevices_no ;
}

static void SetConninData( int indx, const char * type, struct port_in *pin )
{
	struct connection_in * in = pin->first ;
	char name[20] ;

	pin->file_descriptor = indx;
	pin->type = ct_none ;
	in->master.fake.index = indx;
	in->master.fake.templow = Globals.templow;
	in->master.fake.temphigh = Globals.temphigh;
	LEVEL_CONNECT("Setting up %s Bus Master (%d)", type, indx);

	UCLIBCLOCK ;
	snprintf(name, 18, "%s.%d", type, indx);
	UCLIBCUNLOCK ;

	GetAllDeviceNames( pin ) ;

	// Device name and init_data diverge now
	SAFEFREE(DEVICENAME(in)) ;
	DEVICENAME(in) = owstrdup(name);
}

/* Device-specific functions */
/* Since this is simulated bus master, it's creation cannot fail */
/* in->name starts with a list of devices which is destructovely parsed and freed */
/* in->name end with a name-index format */
GOOD_OR_BAD Fake_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	Fake_setroutines(in);		// set up close, reconnect, reset, ...
	in->iroutines.detect = Fake_detect;

	in->adapter_name = "Simulated-Random";
	in->Adapter = adapter_fake;

	SetConninData( Inbound_Control.next_fake++, "fake", pin  );

	return gbGOOD;
}

/* Device-specific functions */
/* Since this is simulated bus master, it's creation cannot fail */
/* in->name starts with a list of devices which is destructovely parsed and freed */
/* in->name end with a name-index format */
GOOD_OR_BAD Mock_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	Fake_setroutines(in);		// set up close, reconnect, reset, ...
	in->iroutines.detect = Mock_detect;

	in->adapter_name = "Simulated-Mock";
	in->Adapter = adapter_mock;
	SetConninData( Inbound_Control.next_mock++, "mock", pin  );

	return gbGOOD;
}

/* Device-specific functions */
/* Since this is simulated bus master, it's creation cannot fail */
/* in->name starts with a list of devices which is destructovely parsed and freed */
/* in->name end with a name-index format */
GOOD_OR_BAD Tester_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	Fake_setroutines(in);		// set up close, reconnect, reset, ...
	in->iroutines.detect = Tester_detect;

	in->adapter_name = "Simulated-Computed";
	in->Adapter = adapter_tester;
	SetConninData( Inbound_Control.next_tester++, "tester", pin  );

	return gbGOOD;
}

static RESET_TYPE Fake_reset(const struct parsedname *pn)
{
	(void) pn;
	return BUS_RESET_OK;
}

static GOOD_OR_BAD Fake_ProgramPulse(const struct parsedname *pn)
{
	(void) pn;
	return gbGOOD;
}

static GOOD_OR_BAD Fake_sendback_bits(const BYTE * data, BYTE * resp, const size_t length, const struct parsedname *pn)
{
	(void) pn;
	(void) data;
	(void) resp;
	(void) length;
	return gbGOOD;
}

static void Fake_close(struct connection_in *in)
{
	DirblobClear( &(in->master.fake.main) );
	DirblobClear( &(in->master.fake.alarm) );
}

static enum search_status Fake_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {	// alarm not supported
		ds->LastDevice = 1;
		return search_done;
	}
	if (DirblobGet(++ds->index, ds->sn, &(pn->selected_connection->master.fake.main))) {
		ds->LastDevice = 1;
		return search_done;
	}
	return search_good;
}

/* Need to lock struct global_namefind_struct since twalk requires global data -- can't pass void pointer */
/* Except all *_detect routines are done sequentially, not concurrently */
struct {
	const ASCII *readable_name;
	const ASCII *ret;
} global_namefind_struct;

static void Namefindaction(const void *nodep, const VISIT which, const int depth)
{
	const struct device *p = *(struct device * const *) nodep;
	(void) depth;
	//printf("Comparing %s|%s with %s\n",p->name ,p->code , Namefindname ) ;
	switch (which) {
	case leaf:
	case postorder:
		if (strcasecmp(p->readable_name, global_namefind_struct.readable_name) == 0 ||
			strcasecmp(p->family_code, global_namefind_struct.readable_name) == 0) {
			global_namefind_struct.ret = p->family_code;
		}
	case preorder:
	case endorder:
		break;
	}
}

static const ASCII *namefind(const char *name)
{
	const ASCII *ret;

	NAMEFINDLOCK;

	global_namefind_struct.readable_name = name;
	global_namefind_struct.ret = NULL;
	twalk(Tree[ePN_real], Namefindaction);
	ret = global_namefind_struct.ret;

	NAMEFINDUNLOCK;

	return ret;
}
