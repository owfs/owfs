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
#include "ow_devices.h"

/* device display format */
void FS_devicename(char *buffer, const size_t length, const BYTE * sn, const struct parsedname *pn)
{
	UCLIBCLOCK;
	//printf("dev format sg=%X DeviceFormat = %d\n",pn->si->sg,DeviceFormat(pn)) ;
	switch (DeviceFormat(pn)) {
	case fdi:
		snprintf(buffer, length, "%02X.%02X%02X%02X%02X%02X%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6]);
		break;
	case fi:
		snprintf(buffer, length, "%02X%02X%02X%02X%02X%02X%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6]);
		break;
	case fdidc:
		snprintf(buffer, length, "%02X.%02X%02X%02X%02X%02X%02X.%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
		break;
	case fdic:
		snprintf(buffer, length, "%02X.%02X%02X%02X%02X%02X%02X%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
		break;
	case fidc:
		snprintf(buffer, length, "%02X%02X%02X%02X%02X%02X%02X.%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
		break;
	case fic:
		snprintf(buffer, length, "%02X%02X%02X%02X%02X%02X%02X%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7]);
		break;
	}
	UCLIBCUNLOCK;
}

static const char dirname_state_uncached[] = "uncached";
static const char dirname_state_alarm[] = "alarm";
static const char dirname_state_text[] = "text";

/* copy state into buffer (for constructing path) return number of chars added */
int FS_dirname_state(char *buffer, const size_t length, const struct parsedname *pn)
{
	const char *p;
	size_t len;
	//printf("dirname state on %.2X\n", pn->state);
	if (IsAlarmDir(pn)) {
		p = dirname_state_alarm;
#if 0
	} else if (pn->state & ePS_text) {
		/* should never return text in a directory listing, since it's a
		 * hidden feature. Uncached should perhaps be the same... */
		strncpy(buffer, dirname_state_text, length);
#endif
	} else if (IsUncachedDir(pn)) {
		p = dirname_state_uncached;
	} else if (pn->terminal_bus_number > -1) {
		int ret;
		//printf("Called FS_dirname_state on %s bus number %d\n",pn->path,pn->bus_nr) ;
		UCLIBCLOCK;
		ret = snprintf(buffer, length, "bus.%d", pn->terminal_bus_number);
		UCLIBCUNLOCK;
		return ret;
	} else {
		return 0;
	}
	//printf("FS_dirname_state: unknown state %.2X on %s\n", pn->state, pn->path);
	strncpy(buffer, p, length);
	len = strlen(p);
	if (len < length)
		return len;
	return length;
}

static const char dirname_type_statistics[] = "statistics";
static const char dirname_type_system[] = "system";
static const char dirname_type_settings[] = "settings";
static const char dirname_type_structure[] = "structure";
static const char dirname_type_interface[] = "interface";

/* copy type into buffer (for constructing path) return number of chars added */
int FS_dirname_type(char *buffer, const size_t length, const struct parsedname *pn)
{
	const char *p;
	size_t len;
	switch (pn->type) {
	case ePN_statistics:
		p = dirname_type_statistics;
		break;
	case ePN_system:
		p = dirname_type_system;
		break;
	case ePN_settings:
		p = dirname_type_settings;
		break;
	case ePN_structure:
		p = dirname_type_structure;
		break;
	case ePN_interface:
		p = dirname_type_interface;
		break;
	default:
		return 0;
	}
	strncpy(buffer, p, length);
	len = strlen(p);
	if (len < length)
		return len;
	return length;
}

/* name of file from filetype structure -- includes extension */
int FS_FileName(char *name, const size_t size, const struct parsedname *pn)
{
	int s;
	if (pn->selected_filetype == NULL)
		return -ENOENT;
	UCLIBCLOCK;
	if (pn->selected_filetype->ag == NULL) {
		s = snprintf(name, size, "%s", pn->selected_filetype->name);
	} else if (pn->extension == EXTENSION_ALL) {
		s = snprintf(name, size, "%s.ALL", pn->selected_filetype->name);
	} else if (pn->extension == EXTENSION_BYTE) {
		s = snprintf(name, size, "%s.BYTE", pn->selected_filetype->name);
	} else if (pn->selected_filetype->ag->letters == ag_letters) {
		s = snprintf(name, size, "%s.%c", pn->selected_filetype->name, pn->extension + 'A');
	} else {
		s = snprintf(name, size, "%s.%-d", pn->selected_filetype->name, pn->extension);
	}
	UCLIBCUNLOCK;
	return (s < 0) ? -ENOBUFS : 0;
}

/* Return the last part of the file name specified by pn */
/* This can be a device, directory, subdiirectory, if property file */
/* Prints this directory element (not the whole path) */
/* Suggest that size = OW_FULLNAME_MAX */
void FS_DirName(char *buffer, const size_t size, const struct parsedname *pn)
{
	if (pn->selected_filetype) {	/* A real file! */
		char *pname = strchr(pn->selected_filetype->name, '/');	// for subdirectories
		if (pname) {
			++pname;
		} else {
			pname = pn->selected_filetype->name;
		}

		UCLIBCLOCK;
		if (pn->selected_filetype->ag == NULL) {
			snprintf(buffer, size, "%s", pname);
		} else if (pn->extension == EXTENSION_ALL) {
			snprintf(buffer, size, "%s.ALL", pname);
		} else if (pn->extension == EXTENSION_BYTE) {
			snprintf(buffer, size, "%s.BYTE", pname);
		} else if (pn->selected_filetype->ag->letters == ag_letters) {
			snprintf(buffer, size, "%s.%c", pname, pn->extension + 'A');
		} else {
			snprintf(buffer, size, "%s.%-d", pname, pn->extension);
		}
		UCLIBCUNLOCK;
	} else if (pn->subdir) {	/* in-device subdirectory */
		strncpy(buffer, pn->subdir->name, size);
	} else if (pn->selected_device == NULL) {	/* root-type directory */
		if (NotRealDir(pn)) {
			FS_dirname_type(buffer, size, pn);
		} else {
			FS_dirname_state(buffer, size, pn);
		}
	} else if (pn->selected_device == DeviceSimultaneous) {
		strncpy(buffer, DeviceSimultaneous->family_code, size);
	} else if (IsRealDir(pn)) {	/* real device */
		FS_devicename(buffer, size, pn->sn, pn);
	} else {					/* pseudo device */
		strncpy(buffer, pn->selected_device->family_code, size);
	}
}
