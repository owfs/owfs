/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions

    LICENSE (As of version 2.5p4 2-Oct-2006)
    owlib: GPL v2
    owfs, owhttpd, owftpd, owserver: GPL v2
    owshell(owdir owread owwrite owpresent): GPL v2
    owcapi (libowcapi): GPL v2
    owperl: GPL v2
    owtcl: LGPL v2
    owphp: GPL v2
    owpython: GPL v2
    owsim.tcl: GPL v2
    where GPL v2 is the "Gnu General License version 2"
    and "LGPL v2" is the "Lesser Gnu General License version 2"


    Written 2003 Paul H Alfille
*/

#ifndef OW_PARSEDNAME_H			/* tedious wrapper */
#define OW_PARSEDNAME_H

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"


/* predeclare some structures */
struct connection_in;
struct device;
struct filetype;
struct devloc ;

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)
#define OW_DEFAULT_LENGTH (128)

/* Parsedname -- path converted into components */
/*
Parsed name is the primary structure interpreting a
owfs systrem call. It is the interpretation of the owfs
file name, or the owhttpd URL. It contains everything
but the operation requested. The operation (read, write
or directory is in the extended URL or the actual callback
function requested).
*/
/*
Parsed name has several components:
sn is the serial number of the device
dev and ft are pointers to device and filetype
  members corresponding to the element
buspath and pathlength interpret the route through
  DS2409 branch controllers
filetype and extension correspond to property
  (filetype) details
subdir points to in-device groupings
*/

#define NO_PARSEDNAME NULL

/* LocalControlFlags information (for remote control) */
	/* bit0: cacheenabled  bit1: return bus-list */
	/* presencecheck */
	/* tempscale */
	/* device format */
extern int32_t LocalControlFlags;

struct buspath {
	BYTE sn[SERIAL_NUMBER_SIZE];
	BYTE branch;
};

#define EXTENSION_BYTE	-2
#define EXTENSION_ALL	-1
#define EXTENSION_ALL_SEPARATE	-1
#define EXTENSION_ALL_MIXED		-1
#define EXTENSION_ALL_AGGREGATE	-1

#define NO_FILETYPE NULL
#define NO_SUBDIR NULL
#define NO_DEVICE NULL
#define NO_CONNECTION NULL

enum ePN_type {
	ePN_root,
	ePN_real,
	ePN_statistics,
	ePN_system,
	ePN_settings,
	ePN_structure,
	ePN_interface,
	ePN_max_type,
};

extern char *ePN_name[];		// must match ePN_type

enum ePS_state {
	ePS_normal = 0x0000,
	ePS_uncached = 0x0001,
	ePS_alarm = 0x0002,
	ePS_text = 0x0004,
	ePS_bus = 0x0008,
	ePS_buslocal = 0x0010,
	ePS_busremote = 0x0020,
	ePS_busveryremote = 0x0040,
	ePS_reconnection = 0x0080,
};

struct parsedname {
	char *path;				// text-more device name
	char *path_to_server;			// pointer to path without first bus
	struct connection_in *known_bus;	// where this device is located
	enum ePN_type type;			// real? settings? ...
	enum ePS_state state;			// alarm?
	BYTE sn[SERIAL_NUMBER_SIZE];				// 64-bit serial number
	struct device *selected_device;		// 1-wire device
	struct filetype *selected_filetype;	// device property
	int extension;				// numerical extension (for array values) or -1
	struct filetype *subdir;		// in-device grouping
	int dirlength ;				// Length of just directory part of path
	UINT pathlength;			// DS2409 branching depth
	struct buspath *bp;			// DS2409 branching route
	struct connection_in *selected_connection;	// which bus is assigned to this item
	uint32_t control_flags;				// more state info, packed for network transmission
	struct devlock *lock;			// pointer to a device-specific lock
	int tokens;				// for anti-loop work
	BYTE *tokenstring;			// List of tokens from owservers passed
};

/* ---- end Parsedname ----------------- */
#define SHOULD_RETURN_BUS_LIST    ( (UINT) 0x00000002 )
#define PERSISTENT_MASK    ( (UINT) 0x00000004 )
#define PERSISTENT_BIT     2
#define ALIAS_REQUEST      ( (UINT) 0x00000008 )
#define SAFEMODE      ( (UINT) 0x00000010 )
#define TEMPSCALE_MASK ( (UINT) 0x00030000 )
#define TEMPSCALE_BIT  16
#define PRESSURESCALE_MASK ( (UINT) 0x001C0000 )
#define PRESSURESCALE_BIT  18
#define DEVFORMAT_MASK ( (UINT) 0xFF000000 )
#define DEVFORMAT_BIT  24
#define IsPersistent(ppn)         ( ((ppn)->control_flags & PERSISTENT_MASK) )
#define SetPersistent(ppn,b)      UT_Setbit(((ppn)->control_flags),PERSISTENT_BIT,(b))
#define TemperatureScale(ppn)     ( (enum temp_type) (((ppn)->control_flags & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define PressureScale(ppn)     ( (enum pressure_type) (((ppn)->control_flags & PRESSURESCALE_MASK) >> PRESSURESCALE_BIT) )
#define SGTemperatureScale(sg)    ( (enum temp_type)(((sg) & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define SGPressureScale(sg)    ( (enum pressure_type)(((sg) & PRESSURESCALE_MASK) >> PRESSURESCALE_BIT) )
#define DeviceFormat(ppn)         ( (enum deviceformat) (((ppn)->control_flags & DEVFORMAT_MASK) >> DEVFORMAT_BIT) )
#define set_controlflags(s, mask, bit, val) do { *(s) = (*(s) & ~(mask)) | ((val)<<bit); } while(0)

#define IsDir( pn )    ( ((pn)->selected_device)==NO_DEVICE \
                      || ((pn)->selected_filetype)==NO_FILETYPE  \
                      || ((pn)->selected_filetype)->format==ft_subdir \
                      || ((pn)->selected_filetype)->format==ft_directory )
#define NotUncachedDir(pn)    ( (((pn)->state)&ePS_uncached) == 0 )
#define  IsUncachedDir(pn)    ( ! NotUncachedDir(pn) )
#define IsStructureDir(pn)    ( ((pn)->type) == ePN_structure )
#define    NotAlarmDir(pn)    ( (((pn)->state)&ePS_alarm) == 0 )
#define     IsAlarmDir(pn)    ( ! NotAlarmDir(pn) )
#define     NotRealDir(pn)    ( ((pn)->type) != ePN_real )
#define      IsRealDir(pn)    ( ((pn)->type) == ePN_real )

#define   NotReconnect(pn)    ( (((pn)->state)&ePS_reconnection) == 0 )
#define ClearReconnect(pn)    do { ((pn)->state)&=~ePS_reconnection; } while(0)
#define   SetReconnect(pn)    do { ((pn)->state)|=ePS_reconnection; } while(0)

#define     InSafeMode(pn)    ( (((pn)->control_flags) & SAFEMODE ) != 0 )

#define KnownBus(pn)          ((((pn)->state) & ePS_bus) != 0 )
#define UnsetKnownBus(pn)           do { (pn)->state &= ~ePS_bus; \
                                        (pn)->known_bus=NULL; \
                                        (pn)->selected_connection=NO_CONNECTION; \
                                    } while(0)

#define ShouldReturnBusList(ppn)  ( ((ppn)->control_flags & SHOULD_RETURN_BUS_LIST) )

#define SpecifiedVeryRemoteBus(pn)     ((((pn)->state) & ePS_busveryremote) != 0 )
#define SpecifiedRemoteBus(pn)         ((((pn)->state) & ePS_busremote) != 0 )
#define SpecifiedLocalBus(pn)          ((((pn)->state) & ePS_buslocal) != 0 )
#define SpecifiedBus(pn)          ( SpecifiedLocalBus(pn) || SpecifiedRemoteBus(pn) )

#define RootNotBranch(pn)         (((pn)->pathlength)==0)

enum parse_enum {
	parse_first,
	parse_done,
	parse_error,
	parse_real,
	parse_branch,
	parse_nonreal,
	parse_prop,
	parse_subprop
};

enum parse_enum Parse_Property(char *filename, struct parsedname *pn);


#endif							/* OW_PARSEDNAME_H */
