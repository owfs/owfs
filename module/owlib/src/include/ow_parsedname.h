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
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

#ifndef OW_PARSEDNAME_H			/* tedious wrapper */
#define OW_PARSEDNAME_H

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"


/* predeclare some structures */
struct connection_in;
struct device;
struct filetype;

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

/* Semi-global information (for remote control) */
	/* bit0: cacheenabled  bit1: return bus-list */
	/* presencecheck */
	/* tempscale */
	/* device format */
extern int32_t SemiGlobal;

struct buspath {
	BYTE sn[8];
	BYTE branch;
};

// dynamically created access control for a 1-wire device
// used to negotiate between different threads (queries)
struct devlock {
	pthread_mutex_t lock;
	BYTE sn[8];
	UINT users;
};

#define EXTENSION_BYTE	-2
#define EXTENSION_ALL	-1
#define EXTENSION_ALL_SEPARATE	-1
#define EXTENSION_ALL_MIXED		-1
#define EXTENSION_ALL_AGGREGATE	-1

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
};

struct parsedname {
	char *path;				// text-more device name
	char *path_busless;			// pointer to path without first bus
	struct connection_in *known_bus;	// where this device is located
	enum ePN_type type;			// real? settings? ...
	enum ePS_state state;			// alarm?
	BYTE sn[8];				// 64-bit serial number
	struct device *selected_device;		// 1-wire device
	struct filetype *selected_filetype;	// device property
	int extension;				// numerical extension (for array values) or -1
	struct filetype *subdir;		// in-device grouping
	int dirlength ;				// Length of just directory part of path
	UINT pathlength;			// DS2409 branching depth
	struct buspath *bp;			// DS2409 branching route
	struct connection_in *selected_connection;	// which bus is assigned to this item
	int terminal_bus_number;		// last bus is list -- used for return trip
	uint32_t sg;				// more state info, packed for network transmission
	struct devlock *lock;			// pointer to a device-specific lock
	int tokens;				// for anti-loop work
	BYTE *tokenstring;			// List of tokens from owservers passed
};

/* ---- end Parsedname ----------------- */
#define CACHE_MASK     ( (UINT) 0x00000001 )
#define CACHE_BIT      0
#define BUSRET_MASK    ( (UINT) 0x00000002 )
#define BUSRET_BIT     1
#define PERSISTENT_MASK    ( (UINT) 0x00000004 )
#define PERSISTENT_BIT     2
#define TEMPSCALE_MASK ( (UINT) 0x00FF0000 )
#define TEMPSCALE_BIT  16
#define DEVFORMAT_MASK ( (UINT) 0xFF000000 )
#define DEVFORMAT_BIT  24
#define IsLocalCacheEnabled(ppn)  ( ((ppn)->sg &  CACHE_MASK) )
#define IsPersistent(ppn)         ( ((ppn)->sg & PERSISTENT_MASK) )
#define SetPersistent(ppn,b)      UT_Setbit(((ppn)->sg),PERSISTENT_BIT,(b))
#define TemperatureScale(ppn)     ( (enum temp_type) (((ppn)->sg & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define SGTemperatureScale(sg)    ( (enum temp_type)(((sg) & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define DeviceFormat(ppn)         ( (enum deviceformat) (((ppn)->sg & DEVFORMAT_MASK) >> DEVFORMAT_BIT) )
#define set_semiglobal(s, mask, bit, val) do { *(s) = (*(s) & ~(mask)) | ((val)<<bit); } while(0)

#define IsDir( pn )    ( ((pn)->selected_device)==NULL \
                      || ((pn)->selected_filetype)==NULL  \
                      || ((pn)->selected_filetype)->format==ft_subdir \
                      || ((pn)->selected_filetype)->format==ft_directory )
#define NotUncachedDir(pn)    ( (((pn)->state)&ePS_uncached) == 0 )
#define  IsUncachedDir(pn)    ( ! NotUncachedDir(pn) )
#define IsStructureDir(pn)    ( ((pn)->type) == ePN_structure )
#define    NotAlarmDir(pn)    ( (((pn)->state)&ePS_alarm) == 0 )
#define     IsAlarmDir(pn)    ( ! NotAlarmDir(pn) )
#define     NotRealDir(pn)    ( ((pn)->type) != ePN_real )
#define      IsRealDir(pn)    ( ((pn)->type) == ePN_real )

#define KnownBus(pn)          ((((pn)->state) & ePS_bus) != 0 )
#define SetKnownBus(bus_number,pn)  do { (pn)->state |= ePS_bus; \
                                        (pn)->selected_connection=find_connection_in(bus_number); \
                                        (pn)->known_bus=(pn)->selected_connection; \
                                    } while(0)
#define UnsetKnownBus(pn)           do { (pn)->state &= ~ePS_bus; \
                                        (pn)->known_bus=NULL; \
                                        (pn)->selected_connection=NULL; \
                                    } while(0)

#define ShouldReturnBusList(ppn)  ( ((ppn)->sg & BUSRET_MASK) )

#define SpecifiedVeryRemoteBus(pn)     ((((pn)->state) & ePS_busveryremote) != 0 )
#define SpecifiedRemoteBus(pn)         ((((pn)->state) & ePS_busremote) != 0 )
#define SpecifiedLocalBus(pn)          ((((pn)->state) & ePS_buslocal) != 0 )
#define SpecifiedBus(pn)          ( SpecifiedLocalBus(pn) || SpecifiedRemoteBus(pn) )

#define SetSpecifiedBus(bus_number,pn) do { SetKnownBus(bus_number,pn); (pn)->state |= BusIsServer((pn)->selected_connection) ? ePS_busremote : ePS_buslocal ; } while(0)

#define RootNotBranch(pn)         (((pn)->pathlength)==0)

#endif							/* OW_PARSEDNAME_H */
