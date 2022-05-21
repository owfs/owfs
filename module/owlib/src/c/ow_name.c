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
#include "ow_devices.h"

char *ePN_name[] = {
	"",
	"",
	"statistics",
	"system",
	"settings",
	"structure",
	"interface",
	0,
};

/* device display format */
void FS_devicename(char *buffer, const size_t length, const BYTE * sn, const struct parsedname *pn)
{
	UCLIBCLOCK;
	switch (DeviceFormat(pn)) {
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
	case fdi:
	default:
		snprintf(buffer, length, "%02X.%02X%02X%02X%02X%02X%02X", sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6]);
		break;
	}
	UCLIBCUNLOCK;
}

/* Return the last part of the file name specified by pn */
/* This can be a device, directory, subdiirectory, if property file */
/* Prints this directory element (not the whole path) */
/* Suggest that size = OW_FULLNAME_MAX */
const char *FS_DirName(const struct parsedname *pn)
{
	char *slash;
	if (pn == NULL) {
		return "";
	}
	slash = strrchr(pn->path, '/');
	if (slash == NULL) {
		return "";
	}
	return slash + 1;
}
