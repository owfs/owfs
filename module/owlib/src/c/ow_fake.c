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
static int Fake_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void GetNextByte( const ASCII ** strpointer, BYTE default_byte, BYTE * sn ) ;

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
	in->iroutines.detect = Fake_detect;
}

static void GetNextByte( const ASCII ** strpointer, BYTE default_byte, BYTE * sn )
{
	if ( (*strpointer)[0] == '.' ) {
		++strpointer ;
	}
	if ( isxdigit((*strpointer)[0]) && isxdigit((*strpointer)[1]) ) {
		*sn = string2num(*strpointer) ;
		strpointer += 2 ;
	} else {
		*sn = default_byte ;
	}
}

/* Device-specific functions */
/* Note, the "Bad"adapter" ha not function, and returns "-ENOTSUP" (not supported) for most functions */
/* It does call lower level functions for higher ones, which of course is pointless since the lower ones don't work either */
int Fake_detect(struct connection_in *in)
{
	ASCII *newname;
	ASCII *oldname = in->name;
	
	in->file_descriptor = Inbound_Control.next_fake;
	Fake_setroutines(in);		// set up close, reconnect, reset, ...

	in->adapter_name = "Simulated-Random";
	in->Adapter = adapter_fake;
	in->connin.fake.bus_number_this_type = Inbound_Control.next_fake;
	LEVEL_CONNECT("Setting up Simulated (Fake) Bus Master (%d)\n", Inbound_Control.next_fake);
	if ((newname = (ASCII *) malloc(20))) {
		const ASCII *current_device_start;
		ASCII *remaining_device_list = in->name;

		UCLIBCLOCK ;
		snprintf(newname, 18, "fake.%d", Inbound_Control.next_fake);
		UCLIBCUNLOCK ;
		in->name = newname;
		
		while (remaining_device_list != NULL) {
			BYTE sn[8];
			for (current_device_start = strsep(&remaining_device_list, " ,"); current_device_start[0] != '\0'; ++current_device_start) {
				// note that strsep updates "remaining_device_list" pointer
				if (current_device_start[0] != ' ' && current_device_start[0] != ',') {
					break;
				}
			}
			if ((isxdigit(current_device_start[0])
				 && isxdigit(current_device_start[1]))
				|| (current_device_start = namefind(current_device_start))) {
				sn[0] = string2num(current_device_start);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[1]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[2]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[3]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[4]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[5]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[6]);
				sn[7] = CRC8compute(sn, 7, 0);
				DirblobAdd(sn, &(in->main));	// Ignore bad return
			}
		}
		in->AnyDevices = (DirblobElements(&(in->main)) > 0);
		if (oldname) {
			free(oldname);
		}
	}
	++Inbound_Control.next_fake;
	return 0;
}

/* Device-specific functions */
/* Note, the "Bad"adapter" ha not function, and returns "-ENOTSUP" (not supported) for most functions */
/* It does call lower level functions for higher ones, which of course is pointless since the lower ones don't work either */
int Mock_detect(struct connection_in *in)
{
	ASCII *newname;
	ASCII *oldname = in->name;
	
	in->file_descriptor = Inbound_Control.next_mock;
	Fake_setroutines(in);		// set up close, reconnect, reset, ...

	in->adapter_name = "Simulated-Mock";
	in->Adapter = adapter_mock;
	in->connin.mock.bus_number_this_type = Inbound_Control.next_mock;
	LEVEL_CONNECT("Setting up Simulated (Mock) Bus Master (%d)\n", Inbound_Control.next_mock);
	if ((newname = (ASCII *) malloc(20))) {
		const ASCII *current_device_start;
		ASCII *remaining_device_list = in->name;

		UCLIBCLOCK ;
		snprintf(newname, 18, "mock.%d", Inbound_Control.next_mock);
		UCLIBCUNLOCK ;
		in->name = newname;
		
		while (remaining_device_list != NULL) {
			BYTE sn[8];
			for (current_device_start = strsep(&remaining_device_list, " ,"); current_device_start[0] != '\0'; ++current_device_start) {
				// note that strsep updates "remaining_device_list" pointer
				if (current_device_start[0] != ' ' && current_device_start[0] != ',') {
					break;
				}
			}
			if ((isxdigit(current_device_start[0])
				 && isxdigit(current_device_start[1]))
				|| (current_device_start = namefind(current_device_start))) {
				sn[0] = string2num(current_device_start);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[1]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[2]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[3]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[4]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[5]);
				GetNextByte(&current_device_start,BYTE_MASK(rand()),&sn[6]);
				sn[7] = CRC8compute(sn, 7, 0);
				DirblobAdd(sn, &(in->main));	// Ignore bad return
			}
		}
		in->AnyDevices = (DirblobElements(&(in->main)) > 0);
		if (oldname) {
			free(oldname);
		}
	}
	++Inbound_Control.next_mock;
	return 0;
}

int Tester_detect(struct connection_in *in)
{
	ASCII *newname;
	ASCII *oldname = in->name;

	in->file_descriptor = Inbound_Control.next_tester;
	Tester_setroutines(in);		// set up close, reconnect, reset, ...

	in->adapter_name = "Simulated-Computed";
	in->Adapter = adapter_tester;
	in->connin.tester.bus_number_this_type = Inbound_Control.next_tester;
	LEVEL_CONNECT("Setting up Simulated (Testing) Bus Master (%d)\n", Inbound_Control.next_tester);
	if ((newname = (ASCII *) malloc(20))) {
		const ASCII *current_device_start;
		ASCII *remaining_device_list = in->name;

		UCLIBCLOCK ;
		snprintf(newname, 18, "tester.%d", Inbound_Control.next_tester);
		UCLIBCUNLOCK ;
		in->name = newname;

		while (remaining_device_list) {
			BYTE sn[8];
			for (current_device_start = strsep(&remaining_device_list, " ,"); current_device_start[0]; ++current_device_start) {
				if (current_device_start[0] != ' ' && current_device_start[0] != ',') {
					break;
				}
			}
			if ((isxdigit(current_device_start[0])
				 && isxdigit(current_device_start[1]))
				|| (current_device_start = namefind(current_device_start))) {
				unsigned int device_number = DirblobElements(&(in->main));
				// family code
				sn[0] = string2num(current_device_start);
				// "bus number"
				GetNextByte(&current_device_start, BYTE_MASK(Inbound_Control.next_tester >> 0), &sn[1]);
				GetNextByte(&current_device_start, BYTE_MASK(Inbound_Control.next_tester >> 8), &sn[2]);
				// repeat family code
				GetNextByte(&current_device_start, sn[0], &sn[3]);
				// family code complement
				GetNextByte(&current_device_start, BYTE_INVERSE(sn[0]), &sn[4]);
				// "device" number
				GetNextByte(&current_device_start, BYTE_MASK(device_number >> 0), &sn[5]);
				GetNextByte(&current_device_start, BYTE_MASK(device_number >> 8), &sn[6]);
				// CRC8
				sn[7] = CRC8compute(sn, 7, 0);
				DirblobAdd(sn, &(in->main));	// Ignore bad return
			}
		}
		in->AnyDevices = (DirblobElements(&(in->main)) > 0);
		if (oldname) {
			free(oldname);
		}
	}
	++Inbound_Control.next_tester;
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

