/*
$Id$
    OW -- One-Wire filesystem

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
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/

#ifndef OW_GENERIC_READ_H				/* tedious wrapper */
#define OW_GENERIC_READ_H

#define UNPAGED_MEMORY	0
struct generic_read {
	int pagesize ;
	enum read_crc_type { rct_no_crc, rct_crc8, rct_crc16, }  crc_type ;
	enum read_address_type { rat_not_supported, rat_2byte, rat_1byte, rat_page, rat_complement, } addressing_type ;
	BYTE command ;
} ;

#endif							/* OW_GENERIC_READ_H */
