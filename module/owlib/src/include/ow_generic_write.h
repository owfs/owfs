/*
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

#ifndef OW_GENERIC_WRITE_H				/* tedious wrapper */
#define OW_GENERIC_WRITE_H

struct generic_write {
	int pagesize ;
	enum write_crc_type { wct_no_crc, wct_crc8, wct_crc16, }  crc_type ;
	enum write_address_type { wat_not_supported, wat_2byte, wat_1byte, wat_page, wat_complement, } addressing_type ;
	BYTE write_cmd ;
	BYTE read_cmd ;
	BYTE copy_cmd ;
} ;

#define NO_GENERIC_WRITE NULL

#endif							/* OW_GENERIC_WRITE_H */
