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

//#define CALC_NLINK

#ifdef CALC_NLINK
static int FS_nr_subdirs(struct parsedname *pn2);

static int FS_nr_subdirs(struct parsedname *pn2)
{
	BYTE sn[8];
	int dindex = 0;

	FS_LoadDirectoryOnly(sn, pn2);
	//printf("FS_nr_subdirs: sn="SNformat" pn2->path=%s\n", SNarg(sn), pn2->path);
	if (Cache_Get_Dir(sn, 0, pn2)) {
		// no entries in directory are cached
		return 0;
	}
	do {
        FS_LoadDirectoryOnly(sn, pn2);
		++dindex;
	} while (Cache_Get_Dir(sn, dindex, pn2) == 0);
	return dindex;
}
#endif

int FS_fstat(const char *path, struct stat *stbuf)
{
	struct parsedname pn;
	int ret = 0;

	LEVEL_CALL("FSTAT path=%s\n", SAFESTRING(path));

	/* Bad path */
	if (FS_ParsedName(path, &pn)) {
		ret = -ENOENT;
	} else {
		ret = FS_fstat_postparse(stbuf, &pn);
	}
	FS_ParsedName_destroy(&pn);
	return ret;
}

/* Fstat with parsedname already done */
int FS_fstat_postparse(struct stat *stbuf, const struct parsedname *pn)
{
	memset(stbuf, 0, sizeof(struct stat));

	LEVEL_CALL("ATTRIBUTES path=%s\n", SAFESTRING(pn->path));
	if (KnownBus(pn) && pn->known_bus==NULL) {
		/* check for presence of first in-device at least since FS_ParsedName
		 * doesn't do it yet. */
		return -ENOENT;
	} else if (pn->selected_device == NULL) {	/* root directory */
		int nr = 0;
		//printf("FS_fstat root\n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;	// plus number of sub-directories
#ifdef CALC_NLINK
		nr = FS_nr_subdirs(pn);
		//printf("FS_fstat: FS_nr_subdirs1 returned %d\n", nr);
#else							/* CALC_NLINK */
		nr = -1;				// make it 1
		/*
		   If calculating NSUB is hard, the filesystem can set st_nlink of
		   directories to 1, and find will still work.  This is not documented
		   behavior of find, and it's not clear whether this is intended or just
		   by accident.  But for example the NTFS filesysem relies on this, so
		   it's unlikely that this "feature" will go away.
		 */
#endif							/* CALC_NLINK */
		stbuf->st_nlink += nr;
		stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = start_time;
	} else if (pn->selected_filetype == NULL) {
		int nr = 0;
		//printf("FS_fstat pn.selected_filetype == NULL  (1-wire device)\n");
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;	// plus number of sub-directories

#ifdef CALC_NLINK
		{
			int i;
			for (i = 0; i < pn->selected_device->nft; i++) {
				if ((pn->selected_device->selected_filetype[i].format == ft_directory) ||
					(pn->selected_device->selected_filetype[i].format == ft_subdir)) {
					nr++;
				}
			}
		}
#else							/* CALC_NLINK */
		nr = -1;				// make it 1
#endif							/* CALC_NLINK */
//printf("FS_fstat seem to be %d entries (%d dirs) in device\n", pn.selected_device->nft, nr);
		stbuf->st_nlink += nr;
		FSTATLOCK;
		stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = dir_time;
		FSTATUNLOCK;
	} else if (pn->selected_filetype->format == ft_directory || pn->selected_filetype->format == ft_subdir) {	/* other directory */
		int nr = 0;
//printf("FS_fstat other dir inside device\n");
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;	// plus number of sub-directories
#ifdef CALC_NLINK
		nr = FS_nr_subdirs(pn);
#else							/* CALC_NLINK */
		nr = -1;				// make it 1
#endif							/* CALC_NLINK */
//printf("FS_fstat seem to be %d entries (%d dirs) in device\n", NFT(pn.selected_filetype));
		stbuf->st_nlink += nr;

		FSTATLOCK;
		stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = dir_time;
		FSTATUNLOCK;
	} else {					/* known 1-wire filetype */
		stbuf->st_mode = S_IFREG;
		if (pn->selected_filetype->read != NO_READ_FUNCTION)
			stbuf->st_mode |= 0444;
		if (!Global.readonly && (pn->selected_filetype->write != NO_WRITE_FUNCTION) )
			stbuf->st_mode |= 0222;
		stbuf->st_nlink = 1;

		switch (pn->selected_filetype->change) {
		case fc_volatile:
		case fc_Avolatile:
		case fc_second:
		case fc_statistic:
			stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime =
				time(NULL);
			break;
		case fc_stable:
		case fc_Astable:
			FSTATLOCK;
			stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = dir_time;
			FSTATUNLOCK;
			break;
		default:
			stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime =
				start_time;
			break;
		}
//printf("FS_fstat file\n");
	}
	stbuf->st_size = FullFileLength(pn);
	return 0;
}
