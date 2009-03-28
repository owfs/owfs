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
#include "ow_devices.h"

static int device_compare(const void *a, const void *b);
static int file_compare(const void *a, const void *b);
static void Device2Tree(const struct device *d, enum ePN_type type);

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


// FreeBSD fix from Robert Nilsson
/*  In order for DeviceDestroy to work on FreeBSD we must copy the keys.
    Otherwise, tdestroy will attempt to free implicitly allocated structures.
*/
static void Device2Tree(const struct device *d, enum ePN_type type)
{
#ifdef __FreeBSD__
	struct device *d_copy;

	if ((d_copy = (struct device *) owmalloc(sizeof(struct device)))) {
		memmove(d_copy, d, sizeof(struct device));
		tsearch(d_copy, &Tree[type], device_compare);
		if (d_copy->filetype_array) {
			qsort(d_copy->filetype_array, (size_t) d_copy->count_of_filetypes, sizeof(struct filetype), file_compare);
		}
	} else {
		LEVEL_DATA("Could not allocate memory for device %s\n", d->readable_name);
	}
#else							/* __FreeBSD__ */
	tsearch(d, &Tree[type], device_compare);
	if (d->filetype_array) {
		qsort(d->filetype_array, (size_t) d->count_of_filetypes, sizeof(struct filetype), file_compare);
	}
#endif							/* __FreeBSD__ */
/*
{
int i ;
printf("Device(%d) %s: ",type,d->name);
for(i=0;i<d->nft;++i){ printf( "%s ",d->selected_filetype[i].name);}
printf("\n");
}
*/
}

static void free_node(void *nodep)
{
	(void) nodep;
	/* nothing to free */
	return;
}

void DeviceDestroy(void)
{
	UINT i;
	for (i = 0; i < (sizeof(Tree) / sizeof(void *)); i++) {
		if (Tree[i]) {
			/* ePN_structure is just a duplicate of ePN_real */
			if (i != ePN_structure) {
				tdestroy(Tree[i], free_node);
			}
			Tree[i] = NULL;
		}
	}
}

void DeviceSort(void)
{
	memset(Tree, 0, sizeof(void *) * ePN_max_type);

	/* Sort the filetypes for the unrecognized device */
	qsort(NoDevice.filetype_array, (size_t) NoDevice.count_of_filetypes, sizeof(struct filetype), file_compare);

	Device2Tree(&d_DS1420, ePN_real);
	Device2Tree(&d_DS1425, ePN_real);
	Device2Tree(&d_DS18S20, ePN_real);
	Device2Tree(&d_DS18B20, ePN_real);
	Device2Tree(&d_DS1821, ePN_real);
	Device2Tree(&d_DS1822, ePN_real);
	Device2Tree(&d_DS1825, ePN_real);
	Device2Tree(&d_DS1921, ePN_real);
	Device2Tree(&d_DS1923, ePN_real);
	Device2Tree(&d_DS1977, ePN_real);
	Device2Tree(&d_DS1963S, ePN_real);
	Device2Tree(&d_DS1963L, ePN_real);
	Device2Tree(&d_DS1982U, ePN_real);
	Device2Tree(&d_DS1985U, ePN_real);
	Device2Tree(&d_DS1986U, ePN_real);
	Device2Tree(&d_DS1991, ePN_real);
	Device2Tree(&d_DS1992, ePN_real);
	Device2Tree(&d_DS1993, ePN_real);
	Device2Tree(&d_DS1995, ePN_real);
	Device2Tree(&d_DS1996, ePN_real);
	Device2Tree(&d_DS2401, ePN_real);
	Device2Tree(&d_DS2404, ePN_real);
	Device2Tree(&d_DS2404S, ePN_real);
	Device2Tree(&d_DS2405, ePN_real);
	Device2Tree(&d_DS2406, ePN_real);
	Device2Tree(&d_DS2408, ePN_real);
	Device2Tree(&d_DS2409, ePN_real);
	Device2Tree(&d_DS2413, ePN_real);
	Device2Tree(&d_DS2415, ePN_real);
	Device2Tree(&d_DS2417, ePN_real);
	Device2Tree(&d_DS2423, ePN_real);
	Device2Tree(&d_DS2430A, ePN_real);
	Device2Tree(&d_DS2431, ePN_real);
	Device2Tree(&d_DS2433, ePN_real);
	Device2Tree(&d_DS2436, ePN_real);
	Device2Tree(&d_DS2437, ePN_real);
	Device2Tree(&d_DS2438, ePN_real);
	Device2Tree(&d_DS2450, ePN_real);
	Device2Tree(&d_DS2502, ePN_real);
	Device2Tree(&d_DS2505, ePN_real);
	Device2Tree(&d_DS2506, ePN_real);
	Device2Tree(&d_DS2720, ePN_real);
	Device2Tree(&d_DS2740, ePN_real);
	Device2Tree(&d_DS2751, ePN_real);
	Device2Tree(&d_DS2755, ePN_real);
	Device2Tree(&d_DS2760, ePN_real);
	Device2Tree(&d_DS2770, ePN_real);
	Device2Tree(&d_DS2780, ePN_real);
	Device2Tree(&d_DS2781, ePN_real);
	Device2Tree(&d_DS2890, ePN_real);
	Device2Tree(&d_DS28EA00, ePN_real);
	Device2Tree(&d_DS28EC20, ePN_real);
	Device2Tree(&d_DS28E04, ePN_real);
	Device2Tree(&d_LCD, ePN_real);
	Device2Tree(&d_stats_cache, ePN_statistics);
	Device2Tree(&d_stats_directory, ePN_statistics);
	Device2Tree(&d_stats_errors, ePN_statistics);
	Device2Tree(&d_stats_read, ePN_statistics);
	Device2Tree(&d_stats_thread, ePN_statistics);
	Device2Tree(&d_stats_write, ePN_statistics);
	Device2Tree(&d_set_cache, ePN_settings);
	Device2Tree(&d_set_units, ePN_settings);
	Device2Tree(&d_sys_process, ePN_system);
	Device2Tree(&d_sys_connections, ePN_system);
	Device2Tree(&d_sys_configure, ePN_system);
	Device2Tree(&d_simultaneous, ePN_real);
	Device2Tree(&d_interface_settings, ePN_interface);
	Device2Tree(&d_interface_statistics, ePN_interface);
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

void FS_devicefindhex(BYTE f, struct parsedname *pn)
{
	char ID[] = "XX";
	const struct device d = { ID, NULL, 0, 0, NULL };
	struct device_opaque *p;

	pn->selected_device = &NoDevice;
	num2string(ID, f);
	if ((p = tfind(&d, &Tree[pn->type], device_compare))) {
		pn->selected_device = p->key;
	} else {
		num2string(ID, f ^ 0x80);
		if ((p = tfind(&d, &Tree[pn->type], device_compare))) {
			pn->selected_device = p->key;
		}
	}
}

void FS_devicefind(const char *code, struct parsedname *pn)
{
	const struct device d = { code, NULL, 0, 0, NULL };
	struct device_opaque *p = tfind(&d, &Tree[pn->type], device_compare);
	if (p) {
		pn->selected_device = p->key;
	} else {
		pn->selected_device = &NoDevice;
	}
}
