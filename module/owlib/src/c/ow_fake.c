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
static void Tester_setroutines(struct connection_in *in);
static void Mock_setroutines(struct connection_in *in);
static int Fake_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void GetNextByte( const ASCII ** strpointer, BYTE default_byte, BYTE * sn ) ;
static void GetDeviceName(const ASCII ** strpointer, struct connection_in * in) ;
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

static void Tester_setroutines(struct connection_in *in)
{
	Fake_setroutines(in);
	in->iroutines.detect = Tester_detect;
}

static void Mock_setroutines(struct connection_in *in)
{
	Fake_setroutines(in);
	in->iroutines.detect = Mock_detect;
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

static void GetDeviceName(const ASCII ** strpointer, struct connection_in * in)
{
	if ((isxdigit((*strpointer)[0])
		&& isxdigit((*strpointer)[1]))
		|| ((*strpointer) = namefind((*strpointer)))
	) {
		BYTE sn[8] ;
		BYTE sn1, sn2 ,sn3, sn4, sn5, sn6 ;

		sn[0] = string2num(*strpointer);
		*strpointer +=  2;
		
		switch (in->busmode) {
			case bus_fake:
			case bus_mock:
				sn1  = BYTE_MASK(rand()) ;
				sn2  = BYTE_MASK(rand()) ;
				sn3  = BYTE_MASK(rand()) ;
				sn4  = BYTE_MASK(rand()) ;
				sn5  = BYTE_MASK(rand()) ;
				sn6  = BYTE_MASK(rand()) ;
				break ;
			case bus_tester:
				// "bus number"
				sn1 = BYTE_MASK(in->connin.tester.index >> 0) ;
				sn2 = BYTE_MASK(in->connin.tester.index >> 8) ;
				// repeat family code
				sn3 = sn[0] ;
				// family code complement
				sn4 = BYTE_INVERSE(sn[0]) ;
				// "device" number
				sn5 = BYTE_MASK(DirblobElements(&(in->main)) >> 0) ;
				sn6 = BYTE_MASK(DirblobElements(&(in->main)) >> 8) ;
				break ;
			default: // should never occur
				return ;
		}

		GetNextByte(strpointer,sn1,&sn[1]);
		GetNextByte(strpointer,sn2,&sn[2]);
		GetNextByte(strpointer,sn3,&sn[3]);
		GetNextByte(strpointer,sn4,&sn[4]);
		GetNextByte(strpointer,sn5,&sn[5]);
		GetNextByte(strpointer,sn6,&sn[6]);
		sn[7] = CRC8compute(sn, 7, 0);
		DirblobAdd(sn, &(in->main));	// Ignore bad return
	}	
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
	struct parsedname pn;
	ASCII *oldname = in->name; // destructively parsed and deleted at the end.
	char newname[20] ;
	
	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;
	
	in->file_descriptor = index;
	in->connin.fake.index = index;
	in->connin.fake.templow = fromTemperature(Globals.templow,&pn);
	in->connin.fake.temphigh = fromTemperature(Globals.temphigh,&pn);
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
	Mock_setroutines(in);		// set up close, reconnect, reset, ...

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
	Tester_setroutines(in);		// set up close, reconnect, reset, ...
	
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

