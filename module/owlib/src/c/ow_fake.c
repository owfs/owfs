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
#include "ow_connection.h"
#include "ow_codes.h"

/* All the remaining_device_list of the program sees is the Fake_detect and the entry in iroutines */

static int Fake_reset(const struct parsedname *pn);
static int Fake_ProgramPulse(const struct parsedname *pn);
static int Fake_sendback_bits(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void Fake_close(struct connection_in *in);
static int Fake_next_both(struct device_search *ds, const struct parsedname *pn);
static const ASCII *namefind(const char *name);
static void Fake_setroutines(struct connection_in *in);
static int Fake_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void GetNextByte( const ASCII ** strpointer, BYTE default_byte, BYTE * sn ) ;
static void GetDeviceName(const ASCII ** strpointer, struct connection_in * in) ;
static void GetDefaultDeviceName(BYTE * dn, const BYTE * sn, const struct connection_in * in) ;
static void GetAllDeviceNames( ASCII * remaining_device_list, struct connection_in * in ) ;
static void SetConninData( int index, const char * name, struct connection_in *in ) ;

static void Fake_setroutines(struct connection_in *in)
{
	in->iroutines.detect = Fake_detect;
	in->iroutines.reset = Fake_reset;
	in->iroutines.next_both = Fake_next_both;
	in->iroutines.PowerByte = NULL;
	in->iroutines.ProgramPulse = Fake_ProgramPulse;
	in->iroutines.sendback_data = Fake_sendback_data;
	in->iroutines.sendback_bits = Fake_sendback_bits;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = Fake_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_2409path;
}

static int Fake_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn)
{
	(void) pn;
	(void) data;
	(void) resp;
	(void) len;
	return 0;
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
	switch (in->busmode) {
		case bus_fake:
		case bus_mock:
			dn[1]  = BYTE_MASK(rand()) ;
			dn[2]  = BYTE_MASK(rand()) ;
			dn[3]  = BYTE_MASK(rand()) ;
			dn[4]  = BYTE_MASK(rand()) ;
			dn[5]  = BYTE_MASK(rand()) ;
			dn[6]  = BYTE_MASK(rand()) ;
			break ;
		case bus_tester:
			// "bus number"
			dn[1] = BYTE_MASK(in->connin.tester.index >> 0) ;
			dn[2] = BYTE_MASK(in->connin.tester.index >> 8) ;
			// repeat family code
			dn[3] = sn[0] ;
			// family code complement
			dn[4] = BYTE_INVERSE(sn[0]) ;
			// "device" number
			dn[5] = BYTE_MASK(DirblobElements(&(in->main)) >> 0) ;
			dn[6] = BYTE_MASK(DirblobElements(&(in->main)) >> 8) ;
			break ;
	}
}

static void GetDeviceName(const ASCII ** strpointer, struct connection_in * in)
{
	BYTE sn[8] ;
	BYTE dn[8] ;

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
		ASCII * name_to_familycode = namefind((*strpointer)) ;
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
			LEVEL_DEFAULT("Device %d <%s> not recognized for %s %d -- ignored\n",DirblobElements(&(in->main))+1,*strpointer,in->adapter_name,in->connin.fake.index);
			return ;
		}
	}
	sn[7] = CRC8compute(sn, 7, 0);
	DirblobAdd(sn, &(in->main));	// Ignore bad return
}

static void GetAllDeviceNames( ASCII * remaining_device_list, struct connection_in * in )
{
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
	in->AnyDevices = (DirblobElements(&(in->main)) > 0);
}

static void SetConninData( int index, const char * name, struct connection_in *in )
{
	ASCII *oldname = in->name; // destructively parsed and deleted at the end.
	char newname[20] ;
	
	in->file_descriptor = index;
	in->connin.fake.index = index;
	in->connin.fake.templow = Globals.templow;
	in->connin.fake.temphigh = Globals.temphigh;
	LEVEL_CONNECT("Setting up %s Bus Master (%d)\n", in->adapter_name, index);

	UCLIBCLOCK ;
	snprintf(newname, 18, "%s.%d", name, index);
	UCLIBCUNLOCK ;

	GetAllDeviceNames( oldname, in ) ;
	if (oldname) {
		free(oldname);
	}
	
	in->name = strdup(newname);
}

/* Device-specific functions */
/* Since this is simulated bus master, it's creation cannot fail */
/* in->name starts with a list of devices which is destructovely parsed and freed */
/* in->name end with a name-index format */
int Fake_detect(struct connection_in *in)
{
	Fake_setroutines(in);		// set up close, reconnect, reset, ...
	in->iroutines.detect = Fake_detect;
	
	in->adapter_name = "Simulated-Random";
	in->Adapter = adapter_fake;

	SetConninData( Inbound_Control.next_fake++, "fake", in  );
	
	return 0;
}

/* Device-specific functions */
/* Since this is simulated bus master, it's creation cannot fail */
/* in->name starts with a list of devices which is destructovely parsed and freed */
/* in->name end with a name-index format */
int Mock_detect(struct connection_in *in)
{
	Fake_setroutines(in);		// set up close, reconnect, reset, ...
	in->iroutines.detect = Mock_detect;
	
	in->adapter_name = "Simulated-Mock";
	in->Adapter = adapter_mock;
	SetConninData( Inbound_Control.next_mock++, "mock", in  );

	return 0;
}

/* Device-specific functions */
/* Since this is simulated bus master, it's creation cannot fail */
/* in->name starts with a list of devices which is destructovely parsed and freed */
/* in->name end with a name-index format */
int Tester_detect(struct connection_in *in)
{
	Fake_setroutines(in);		// set up close, reconnect, reset, ...
	in->iroutines.detect = Tester_detect;
	
	in->adapter_name = "Simulated-Computed";
	in->Adapter = adapter_tester;
	SetConninData( Inbound_Control.next_tester++, "tester", in  );
	
	return 0;
}

static int Fake_reset(const struct parsedname *pn)
{
	(void) pn;
	return BUS_RESET_OK;
}

static int Fake_ProgramPulse(const struct parsedname *pn)
{
	(void) pn;
	return 0;
}

static int Fake_sendback_bits(const BYTE * data, BYTE * resp, const size_t length, const struct parsedname *pn)
{
	(void) pn;
	(void) data;
	(void) resp;
	(void) length;
	return 0;
}

static void Fake_close(struct connection_in *in)
{
	(void) in;
}

static int Fake_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {	// alarm not supported
		ds->LastDevice = 1;
		return -ENODEV;
	}
	if (DirblobGet(++ds->index, ds->sn, &(pn->selected_connection->main))) {
		ds->LastDevice = 1;
		return -ENODEV;
	}
	return 0;
}

/* Need to lock struct global_namefind_struct since twalk requires global data -- can't pass void pointer */
/* Except all *_detect routines are done sequentially, not concurrently */
#if OW_MT
pthread_mutex_t Namefindmutex = PTHREAD_MUTEX_INITIALIZER;
#define NAMEFINDMUTEXLOCK		my_pthread_mutex_lock(&Namefindmutex)
#define NAMEFINDMUTEXUNLOCK		my_pthread_mutex_unlock(&Namefindmutex)
#else							/* OW_MT */
#define NAMEFINDMUTEXLOCK		return_ok()
#define NAMEFINDMUTEXUNLOCK		return_ok()
#endif							/* OW_MT */

struct {
	const ASCII *readable_name;
	const ASCII *ret;
} global_namefind_struct;
void Namefindaction(const void *nodep, const VISIT which, const int depth)
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

	NAMEFINDMUTEXLOCK;

	global_namefind_struct.readable_name = name;
	global_namefind_struct.ret = NULL;
	twalk(Tree[ePN_real], Namefindaction);
	ret = global_namefind_struct.ret;

	NAMEFINDMUTEXUNLOCK;

	return ret;
}
