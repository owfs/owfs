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
#include "ow_devices.h"
#include "ow_external.h"

static int device_compare(const void *a, const void *b);
static int file_compare(const void *a, const void *b);
static void Device2Tree(const struct device *d, enum ePN_type type);
static void External_Process(void);

struct device *DeviceSimultaneous;
struct device *DeviceThermostat;

static int device_compare(const void *a, const void *b)
{
	return strcmp(((const struct device *) a)->family_code, ((const struct device *) b)->family_code);
}

static int file_compare(const void *a, const void *b)
{
	return strcmp(((const struct filetype *) a)->name, ((const struct filetype *) b)->name);
}

void *Tree[ePN_max_type];


#ifdef __FreeBSD__
static void Device2Tree(const struct device *d, enum ePN_type type)
{
	// FreeBSD fix from Robert Nilsson
	/*  In order for DeviceDestroy to work on FreeBSD we must copy the keys.
	    Otherwise, tdestroy will attempt to free implicitly allocated structures.
	*/
	// Note, I'm not using owmalloc since owfree is probably not calledby FreeBSD's tdestroy //
	struct device *d_copy ;
	if ((d_copy = (struct device *) malloc(sizeof(struct device)))) {
		memmove(d_copy, d, sizeof(struct device));
	} else {
		LEVEL_DATA("Could not allocate memory for device %s", d->readable_name);
		return ;
	}
	// Add in the device into the appropriate red/black tree
	tsearch(d_copy, &Tree[type], device_compare);
	// Sort all the file types alphabetically for search and listing
	if (d_copy->filetype_array != NULL) {
		qsort(d_copy->filetype_array, (size_t) d_copy->count_of_filetypes, sizeof(struct filetype), file_compare);
	}
}
#else    /* not FreeBSD */
static void Device2Tree(const struct device *d, enum ePN_type type)
{
	// Add in the device into the appropriate red/black tree
	tsearch(d, &Tree[type], device_compare);
	// Sort all the file types alphabetically for search and listing
	if (d->filetype_array != NULL) {
		qsort(d->filetype_array, (size_t) d->count_of_filetypes, sizeof(struct filetype), file_compare);
	}
}
#endif							/* __FreeBSD__ */

static void free_node(void *nodep)
{
	(void) nodep;
	/* nothing to free */
	return;
}

void DeviceDestroy(void)
{
	UINT i;

	// clear external trees
	tdestroy( sensor_tree, owfree_func ) ;
	tdestroy( family_tree, owfree_func ) ;
	tdestroy( property_tree, owfree_func ) ;

	// clear these trees -- have static data
	for (i = 0; i < (sizeof(Tree) / sizeof(void *)); i++) {
		/* ePN_structure is just a duplicate of ePN_real */
		if (i != ePN_structure) {
			SAFETDESTROY( Tree[i], free_node);
		} else {
			/* ePN_structure (will be cleared in ePN_real) */
			Tree[i] = NULL;
		}
	}
}

void DeviceSort(void)
{
	memset(Tree, 0, sizeof(void *) * ePN_max_type);

	/* Sort the filetypes for the unrecognized device */
	qsort(UnknownDevice.filetype_array, (size_t) UnknownDevice.count_of_filetypes, sizeof(struct filetype), file_compare);

	Device2Tree( & d_Example_slave,  ePN_real);

	Device2Tree( & d_BAE,            ePN_real);
	Device2Tree( & d_DS1420,         ePN_real);
	Device2Tree( & d_DS1425,         ePN_real);
	Device2Tree( & d_DS18S20,        ePN_real);
	Device2Tree( & d_DS18B20,        ePN_real);
	Device2Tree( & d_DS1821,         ePN_real);
	Device2Tree( & d_DS1822,         ePN_real);
	Device2Tree( & d_DS1825,         ePN_real);
	Device2Tree( & d_DS1921,         ePN_real);
	Device2Tree( & d_DS1923,         ePN_real);
	Device2Tree( & d_DS1977,         ePN_real);
	Device2Tree( & d_DS1963S,        ePN_real);
	Device2Tree( & d_DS1963L,        ePN_real);
	Device2Tree( & d_DS1982U,        ePN_real);
	Device2Tree( & d_DS1985U,        ePN_real);
	Device2Tree( & d_DS1986U,        ePN_real);
	Device2Tree( & d_DS1991,         ePN_real);
	Device2Tree( & d_DS1992,         ePN_real);
	Device2Tree( & d_DS1993,         ePN_real);
	Device2Tree( & d_DS1995,         ePN_real);
	Device2Tree( & d_DS1996,         ePN_real);
	Device2Tree( & d_DS2401,         ePN_real);
	Device2Tree( & d_DS2404,         ePN_real);
	Device2Tree( & d_DS2405,         ePN_real);
	Device2Tree( & d_DS2406,         ePN_real);
	Device2Tree( & d_DS2408,         ePN_real);
	Device2Tree( & d_DS2409,         ePN_real);
	Device2Tree( & d_DS2413,         ePN_real);
	Device2Tree( & d_DS2413_85,      ePN_real);
	Device2Tree( & d_DS2415,         ePN_real);
	Device2Tree( & d_DS2417,         ePN_real);
	Device2Tree( & d_DS2423,         ePN_real);
	Device2Tree( & d_DS2430A,        ePN_real);
	Device2Tree( & d_DS2431,         ePN_real);
	Device2Tree( & d_DS2433,         ePN_real);
	Device2Tree( & d_DS2436,         ePN_real);
	Device2Tree( & d_DS2437,         ePN_real);
	Device2Tree( & d_DS2438,         ePN_real);
	Device2Tree( & d_DS2438_A6,      ePN_real);
	Device2Tree( & d_DS2450,         ePN_real);
	Device2Tree( & d_DS2502,         ePN_real);
	Device2Tree( & d_DS2505,         ePN_real);
	Device2Tree( & d_DS2506,         ePN_real);
	Device2Tree( & d_DS2720,         ePN_real);
	Device2Tree( & d_DS2740,         ePN_real);
	Device2Tree( & d_DS2751,         ePN_real);
	Device2Tree( & d_DS2755,         ePN_real);
	Device2Tree( & d_DS2760,         ePN_real);
	Device2Tree( & d_DS2770,         ePN_real);
	Device2Tree( & d_DS2780,         ePN_real);
	Device2Tree( & d_DS2781,         ePN_real);
	Device2Tree( & d_DS2890,         ePN_real);
	Device2Tree( & d_DS28EA00,       ePN_real);
	Device2Tree( & d_DS28EC20,       ePN_real);
	Device2Tree( & d_DS28E04,        ePN_real);
	Device2Tree( & d_EDS,            ePN_real);
	Device2Tree( & d_HobbyBoards_EE, ePN_real);
	Device2Tree( & d_HobbyBoards_EF, ePN_real);
	Device2Tree( & d_InfernoEmbedded,ePN_real);
	Device2Tree( & d_LCD,            ePN_real);
	Device2Tree( & d_mAM001,         ePN_real);
	Device2Tree( & d_mCM001,         ePN_real);
	Device2Tree( & d_mRS001,         ePN_real);
	Device2Tree( & d_mTS017,         ePN_real);
	Device2Tree( & d_mDI001,         ePN_real);
	Device2Tree( & d_IBLSS,          ePN_real);
	Device2Tree( & d_simultaneous,   ePN_real);

	Device2Tree( & d_stats_cache,          ePN_statistics);
	Device2Tree( & d_stats_directory,      ePN_statistics);
	Device2Tree( & d_stats_errors,         ePN_statistics);
	Device2Tree( & d_stats_read,           ePN_statistics);
	Device2Tree( & d_stats_thread,         ePN_statistics);
	Device2Tree( & d_stats_write,          ePN_statistics);
	Device2Tree( & d_stats_return_code,    ePN_statistics);

	Device2Tree( & d_set_timeout,          ePN_settings);
	Device2Tree( & d_set_units,            ePN_settings);
	Device2Tree( & d_set_alias,            ePN_settings);
	Device2Tree( & d_set_return_code,      ePN_settings);

	Device2Tree( & d_sys_process,          ePN_system);
	Device2Tree( & d_sys_connections,      ePN_system);
	Device2Tree( & d_sys_configure,        ePN_system);

	Device2Tree( & d_interface_settings,   ePN_interface);
	Device2Tree( & d_interface_statistics, ePN_interface);

	/* Add external devices */
	External_Process() ;

	/* Match simultaneous for special processing */
	{
		struct parsedname pn;
		pn.type = ePN_real;
		FS_devicefind("simultaneous", &pn);
		DeviceSimultaneous = pn.selected_device;
	}
	/* Match thermostat for special processing */
	{
		struct parsedname pn;
		pn.type = ePN_real;
		FS_devicefind("thermostat", &pn);
		DeviceThermostat = pn.selected_device;
	}

	/* structure uses same tree as real */
	Tree[ePN_structure] = Tree[ePN_real];
}

struct device * FS_devicefindhex(BYTE f, struct parsedname *pn)
{
	char ID[] = "XX";
	const struct device d = { ID, NULL, 0, 0, NULL, NO_GENERIC_READ, NO_GENERIC_WRITE };
	struct device_opaque *p;

	num2string(ID, f);
	if ((p = tfind(&d, &Tree[pn->type], device_compare))) {
		return p->key;
	} else {
		num2string(ID, f ^ 0x80);
		if ((p = tfind(&d, &Tree[pn->type], device_compare))) {
			return p->key;
		}
	}
	return &UnknownDevice ;
}

void FS_devicefind(const char *code, struct parsedname *pn)
{
	const struct device d = { code, NULL, 0, 0, NULL, NO_GENERIC_READ, NO_GENERIC_WRITE };
	struct device_opaque *p = tfind(&d, &Tree[pn->type], device_compare);
	if (p) {
		pn->selected_device = p->key;
	} else {
		pn->selected_device = &UnknownDevice;
	}
}

/* Need to lock struct global_namefind_struct since twalk requires global data -- can't pass void pointer */
/* Except all *_detect routines are done sequentially, not concurrently */
struct {
	const struct family_node * f ;
	int count ;
} global_externalcount_struct;

static void External_propertycount_action(const void *nodep, const VISIT which, const int depth)
{
	const struct property_node *p = *(struct property_node * const *) nodep;
	(void) depth;

	switch (which) {
	case leaf:
	case postorder:
		if (strcmp(p->family, global_externalcount_struct.f->family) == 0 ) {
			++global_externalcount_struct.count;
		}
	case preorder:
	case endorder:
		break;
	}
}

static void External_propertycopy_action(const void *nodep, const VISIT which, const int depth)
{
	const struct property_node *p = *(struct property_node * const *) nodep;
	(void) depth;

	switch (which) {
	case leaf:
	case postorder:
		if (strcmp(p->family, global_externalcount_struct.f->family) == 0 ) {
			memcpy(
				& (global_externalcount_struct.f->dev.filetype_array[global_externalcount_struct.count]),
				&(p->ft),
				sizeof(struct filetype)
			) ;
			++global_externalcount_struct.count;
		}
	case preorder:
	case endorder:
		break;
	}
}

// First loop through families -- to count properties and allocate filetype array.
static void External_family_action(const void *nodep, const VISIT which, const int depth)
{
	const struct family_node *p = *(struct family_node * const *) nodep;
	struct family_node * non_const_f ; // to allow assignments
	(void) depth;

 	switch (which) {
	case leaf:
	case postorder:
		// First count
		global_externalcount_struct.f = p ;
		global_externalcount_struct.count = 0 ;
		twalk( property_tree, External_propertycount_action);

		// Refind this node to allow assignment
		non_const_f = Find_External_Family( p->family ) ;
		non_const_f->dev.filetype_array = owcalloc(
			global_externalcount_struct.count,
			sizeof( struct filetype)
		) ;
		non_const_f->dev.count_of_filetypes = global_externalcount_struct.count ;

		// Next copy
		global_externalcount_struct.count = 0 ;
		twalk( property_tree, External_propertycopy_action);

		// Finally add to tree
		Device2Tree( & (p->dev), ePN_real);
		break ;
	case preorder:
	case endorder:
		break;
	}
}

static void External_Process(void)
{
	EXTERNALCOUNTLOCK;

	twalk( family_tree, External_family_action);

	EXTERNALCOUNTUNLOCK;
}
