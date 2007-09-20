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

int global_count_fake_busses = 0;
int global_count_tester_busses = 0 ;

/* All the remaining_device_list of the program sees is the Fake_detect and the entry in iroutines */

static int Fake_reset(const struct parsedname *pn);
static int Fake_overdrive(const UINT ov, const struct parsedname *pn);
static int Fake_testoverdrive(const struct parsedname *pn);
static int Fake_ProgramPulse(const struct parsedname *pn);
static int Fake_sendback_bits(const BYTE * data, BYTE * resp,
							  const size_t len,
							  const struct parsedname *pn);
static void Fake_close(struct connection_in *in);
static int Fake_next_both(struct device_search *ds,
						  const struct parsedname *pn);
static const ASCII *namefind(const char *name);
static void Fake_setroutines(struct interface_routines *f) ;
static void Tester_setroutines(struct interface_routines *f) ;

static void Fake_setroutines(struct interface_routines *f)
{
    f->detect = Fake_detect;
    f->reset = Fake_reset;
    f->next_both = Fake_next_both;
    f->overdrive = Fake_overdrive;
    f->testoverdrive = Fake_testoverdrive;
    f->PowerByte = NULL;
    f->ProgramPulse = Fake_ProgramPulse;
    f->sendback_data = NULL;
    f->sendback_bits = Fake_sendback_bits;
    f->select = NULL;
    f->reconnect = NULL;
    f->close = Fake_close;
    f->transaction = NULL;
    f->flags = ADAP_FLAG_2409path;
}

static void Tester_setroutines(struct interface_routines *f)
{
    Fake_setroutines(f) ;
    f->detect = Fake_detect;
}

/* Device-specific functions */
/* Note, the "Bad"adapter" ha not function, and returns "-ENOTSUP" (not supported) for most functions */
/* It does call lower level functions for higher ones, which of course is pointless since the lower ones don't work either */
int Fake_detect(struct connection_in *in)
{
    ASCII *newname;
    ASCII *oldname = in->name;

    in->file_descriptor = global_count_fake_busses;
    Fake_setroutines(&in->iroutines); // set up close, reconnect, reset, ...

    DirblobInit(&(in->connin.fake.db));
    in->adapter_name = "Simulated";
    in->Adapter = adapter_fake;
    in->connin.fake.bus_number_this_type = global_count_fake_busses ;
    LEVEL_CONNECT("Setting up Simulated (Fake) Bus Master (%d)\n", global_count_fake_busses);
    if ((newname = (ASCII *) malloc(20))) {
        const ASCII *current_device_start;
        ASCII *remaining_device_list = in->name;

        snprintf(newname, 18, "fake.%d", global_count_fake_busses);
        in->name = newname;

        while (remaining_device_list!=NULL) {
            BYTE sn[8];
            for (current_device_start = strsep(&remaining_device_list, " ,"); current_device_start[0]!='\0'; ++current_device_start) {
                // note that strsep updates "remaining_device_list" pointer
                if (current_device_start[0] != ' ' && current_device_start[0] != ',')
                    break;
            }
            if ((isxdigit(current_device_start[0]) && isxdigit(current_device_start[1]))
                 || (current_device_start = namefind(current_device_start))) {
                sn[0] = string2num(current_device_start);
                sn[1] = BYTE_MASK(rand()) ;
                sn[2] = BYTE_MASK(rand()) ;
                sn[3] = BYTE_MASK(rand()) ;
                sn[4] = BYTE_MASK(rand()) ;
                sn[5] = BYTE_MASK(rand()) ;
                sn[6] = BYTE_MASK(rand()) ;
                sn[7] = CRC8compute(sn, 7, 0);
                DirblobAdd(sn, &(in->connin.fake.db));  // Ignore bad return
                 }
        }
        in->AnyDevices = (in->connin.fake.db.devices > 0);
        if (oldname) {
            free(oldname);
        }
    }
    ++global_count_fake_busses;
    return 0;
}

int Tester_detect(struct connection_in *in)
{
    ASCII *newname;
    ASCII *oldname = in->name;

    in->file_descriptor = global_count_tester_busses;
    Tester_setroutines(&in->iroutines); // set up close, reconnect, reset, ...

    DirblobInit(&(in->connin.tester.db));
    in->adapter_name = "Simulated";
    in->Adapter = adapter_tester;
    in->connin.tester.bus_number_this_type = global_count_tester_busses ;
    LEVEL_CONNECT("Setting up Simulated (Testing) Bus Master (%d)\n", global_count_tester_busses);
    if ((newname = (ASCII *) malloc(20))) {
        const ASCII *current_device_start;
        ASCII *remaining_device_list = in->name;

        snprintf(newname, 18, "tester.%d", global_count_fake_busses);
        in->name = newname;

        while (remaining_device_list) {
            BYTE sn[8];
            for (current_device_start = strsep(&remaining_device_list, " ,"); current_device_start[0]; ++current_device_start) {
                if (current_device_start[0] != ' ' && current_device_start[0] != ',')
                    break;
            }
            if ((isxdigit(current_device_start[0]) && isxdigit(current_device_start[1]))
                 || (current_device_start = namefind(current_device_start))) {
                unsigned int device_number = in->connin.tester.db.devices ;
                sn[0] = string2num(current_device_start); // family code
                sn[1] = BYTE_MASK(global_count_tester_busses>>0) ; // "bus" number
                sn[2] = BYTE_MASK(global_count_tester_busses>>8) ; // "bus" number
                sn[3] = sn[0] ; // repeat family code
                sn[4] = BYTE_INVERSE(sn[0]) ; // family code complement
                sn[5] = BYTE_MASK(device_number>>0) ; // "device" number
                sn[6] = BYTE_MASK(device_number>>8) ; // "device" number
                sn[7] = CRC8compute(sn, 7, 0); // CRC
                DirblobAdd(sn, &(in->connin.tester.db));  // Ignore bad return
            }
        }
        in->AnyDevices = (in->connin.tester.db.devices > 0);
        if (oldname) {
            free(oldname);
        }
    }
    ++global_count_tester_busses;
    return 0;
}

static int Fake_reset(const struct parsedname *pn)
{
	(void) pn;
	return BUS_RESET_OK;
}
static int Fake_overdrive(const UINT ov, const struct parsedname *pn)
{
	(void) ov;
	(void) pn;
	return 0;
}
static int Fake_testoverdrive(const struct parsedname *pn)
{
	(void) pn;
	return 0;
}
static int Fake_ProgramPulse(const struct parsedname *pn)
{
	(void) pn;
	return 0;
}
static int Fake_sendback_bits(const BYTE * data, BYTE * resp,
							  const size_t length,
							  const struct parsedname *pn)
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

static int Fake_next_both(struct device_search *ds,
						  const struct parsedname *pn)
{
	//printf("Fake_next_both LastDiscrepancy=%d, devices=%d, LastDevice=%d, AnyDevice=%d\n",ds->LastDiscrepancy,pn->in->connin.fake.devices,ds->LastDevice,pn->in->AnyDevices);
    if (ds->search == _1W_CONDITIONAL_SEARCH_ROM) {	// alarm not supported
		ds->LastDevice = 1;
		return -ENODEV;
	}
	if (DirblobGet
		(++ds->LastDiscrepancy, ds->sn, &(pn->in->connin.fake.db))) {
		ds->LastDevice = 1;
		return -ENODEV;
	}
	return 0;
}

/* Need to lock struct global_namefind_struct since twalk requires global data -- can't pass void pointer */
#if OW_MT
pthread_mutex_t Namefindmutex = PTHREAD_MUTEX_INITIALIZER;
#define NAMEFINDMUTEXLOCK pthread_mutex_lock(&Namefindmutex)
#define NAMEFINDMUTEXUNLOCK pthread_mutex_unlock(&Namefindmutex)
#else                          /* OW_MT */
#define NAMEFINDMUTEXLOCK
#define NAMEFINDMUTEXUNLOCK
#endif                          /* OW_MT */

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
		if (strcasecmp(p->readable_name, global_namefind_struct.readable_name) == 0
			|| strcasecmp(p->family_code, global_namefind_struct.readable_name) == 0) {
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
    
    NAMEFINDMUTEXLOCK ;

	global_namefind_struct.readable_name = name;
	global_namefind_struct.ret = NULL;
	twalk(Tree[pn_real], Namefindaction);
	ret = global_namefind_struct.ret;

    NAMEFINDMUTEXUNLOCK ;

    return ret;
}
