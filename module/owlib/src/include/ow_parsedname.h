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

#ifndef OW_PARSEDNAME_H					/* tedious wrapper */
#define OW_PARSEDNAME_H

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"


/* predeclare some structures */
struct connection_in;
struct device ;
struct filetype ;

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

struct devlock {
	BYTE sn[8];
	UINT users;
	pthread_mutex_t lock;
};

#define EXTENSION_BYTE	-2
#define EXTENSION_ALL	-1
#define EXTENSION_ALL_SEPARATE	-1
#define EXTENSION_ALL_MIXED		-1
#define EXTENSION_ALL_AGGREGATE	-1

enum pn_type { pn_real =
		0, pn_statistics, pn_system, pn_settings, pn_structure
};
enum pn_state { pn_normal = 0, pn_uncached = 1, pn_alarm = 2, pn_text =
		4, pn_bus = 8, pn_buspath = 16,
};
struct parsedname {
	char *path;					// text-more device name
	char *path_busless;			// pointer to path without first bus
	int bus_nr;
	enum pn_type type;			// global branch
	enum pn_state state;		// global branch
	BYTE sn[8];					// 64-bit serial number
	struct device *selected_device;			// 1-wire device
	struct filetype *selected_filetype;		// device property
	int extension;				// numerical extension (for array values) or -1
	struct filetype *subdir;	// in-device grouping
	UINT pathlength;			// DS2409 branching depth
	struct buspath *bp;			// DS2409 branching route
	struct connection_in *head_inbound_list;	// Global head_inbound_list at definition time
	struct connection_in *selected_connection;
	uint32_t sg;				// more state info, packed for network transmission
	struct devlock **lock;		// need to clear dev lock?
	int tokens;					/* for anti-loop work */
	BYTE *tokenstring;			/* List of tokens from owservers passed */
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
#define NotUncachedDir(pn)    ( (((pn)->state)&pn_uncached) == 0 )
#define  IsUncachedDir(pn)    ( ! NotUncachedDir(pn) )
#define    NotAlarmDir(pn)    ( (((pn)->state)&pn_alarm) == 0 )
#define     IsAlarmDir(pn)    ( ! NotAlarmDir(pn) )
#define     NotRealDir(pn)    ( ((pn)->type) != pn_real )
#define      IsRealDir(pn)    ( ((pn)->type) == pn_real )

#define KnownBus(pn)          ((((pn)->state) & pn_bus) != 0 )
#define SetKnownBus(bus_number,pn)  do { (pn)->state |= pn_bus; \
                                        (pn)->bus_nr=(bus_number); \
                                        (pn)->selected_connection=find_connection_in(bus_number); \
                                    } while(0)
#define UnsetKnownBus(pn)           do { (pn)->state &= ~pn_bus; \
                                        (pn)->bus_nr=-1; \
                                        (pn)->selected_connection=NULL; \
                                    } while(0)

#define ShouldReturnBusList(ppn)  ( ((ppn)->sg & BUSRET_MASK) )
#define SpecifiedBus(pn)          ((((pn)->state) & pn_buspath) != 0 )
#define SetSpecifiedBus(bus_number,pn) do { (pn)->state |= pn_buspath ; SetKnownBus(bus_number,pn); } while(0)

#define RootNotBranch(pn)         (((pn)->pathlength)==0)
#define BYTE_MASK(x)        ((x)&0xFF)
#define BYTE_INVERSE(x)     BYTE_MASK((x)^0xFF)
#define LOW_HIGH_ADDRESS(x)         BYTE_MASK(x),BYTE_MASK((x)>>8)

#endif							/* OW_PARSEDNAME_H */
