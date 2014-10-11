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

ZERO_OR_ERROR FS_fstat(const char *path, struct stat *stbuf)
{
	struct parsedname pn;
	ZERO_OR_ERROR ret = 0;

	LEVEL_CALL("path=%s", SAFESTRING(path));

	/* Bad path */
	if (FS_ParsedName(path, &pn) != 0 ) {
		return -ENOENT;
	}

	ret = FS_fstat_postparse(stbuf, &pn);
	FS_ParsedName_destroy(&pn);
	return ret;
}

/* Fstat with parsedname already done */
ZERO_OR_ERROR FS_fstat_postparse(struct stat *stbuf, const struct parsedname *pn)
{
	memset(stbuf, 0, sizeof(struct stat));

	LEVEL_CALL("ATTRIBUTES path=%s", SAFESTRING(pn->path));
	if (KnownBus(pn) && pn->known_bus == NULL) {
		/* check for presence of first in-device at least since FS_ParsedName
		 * doesn't do it yet. */
		return -ENOENT;
	} else if (pn->selected_device == NO_DEVICE) {	/* root directory */
		int nr = 0;
		//printf("FS_fstat root\n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;	// plus number of sub-directories
		nr = -1;				// make it 1
		/*
		   If calculating NSUB is hard, the filesystem can set st_nlink of
		   directories to 1, and find will still work.  This is not documented
		   behavior of find, and it's not clear whether this is intended or just
		   by accident.  But for example the NTFS filesysem relies on this, so
		   it's unlikely that this "feature" will go away.
		 */
		stbuf->st_nlink += nr;
		stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = StateInfo.start_time;
	} else if (pn->selected_filetype == NO_FILETYPE) {
		int nr = 0;
		//printf("FS_fstat pn.selected_filetype == NULL  (1-wire device)\n");
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;	// plus number of sub-directories

		nr = -1;				// make it 1
		//printf("FS_fstat seem to be %d entries (%d dirs) in device\n", pn.selected_device->nft, nr);
		stbuf->st_nlink += nr;
		FSTATLOCK;
		stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = StateInfo.dir_time;
		FSTATUNLOCK;
	} else if (pn->selected_filetype->format == ft_directory || pn->selected_filetype->format == ft_subdir) {	/* other directory */
		int nr = 0;
		//printf("FS_fstat other dir inside device\n");
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;	// plus number of sub-directories
		nr = -1;				// make it 1
		//printf("FS_fstat seem to be %d entries (%d dirs) in device\n", NFT(pn.selected_filetype));
		stbuf->st_nlink += nr;

		FSTATLOCK;
		stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = StateInfo.dir_time;
		FSTATUNLOCK;
	} else {					/* known 1-wire filetype */
		stbuf->st_mode = S_IFREG;
        if (pn->selected_filetype->read != NO_READ_FUNCTION) {
			stbuf->st_mode |= 0444;
		}
		if (!Globals.readonly && (pn->selected_filetype->write != NO_WRITE_FUNCTION)) {
			stbuf->st_mode |= 0222;
        }
		stbuf->st_nlink = 1;

		switch (pn->selected_filetype->change) {
		case fc_volatile:
		case fc_second:
		case fc_statistic:
			stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = NOW_TIME;
			break;
		case fc_stable:
			FSTATLOCK;
			stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = StateInfo.dir_time;
			FSTATUNLOCK;
			break;
		default:
			stbuf->st_atime = stbuf->st_ctime = stbuf->st_mtime = StateInfo.start_time;
			break;
		}
		//printf("FS_fstat file\n");
	}
	stbuf->st_size = FullFileLength(pn);
	return 0;
}
