/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

\    LICENSE (As of version 2.5p4 2-Oct-2006)
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
//     ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

/* Cannot stand alone -- part of ow.h but separated for clarity */

#ifndef OW_SIBLING_H			/* tedious wrapper */
#define OW_SIBLING_H

ZERO_OR_ERROR FS_r_sibling_F(_FLOAT *F, const char * sibling, struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_r_sibling_U(  UINT *U, const char * sibling, struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_r_sibling_Y(   INT *Y, const char * sibling, struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_r_sibling_binary(BYTE * data, size_t * size, const char * sibling, struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_w_sibling_binary(BYTE * data, size_t size, off_t offset, const char * sibling, struct one_wire_query *owq) ;

ZERO_OR_ERROR FS_w_sibling_bitwork(UINT set, UINT mask, const char * sibling, struct one_wire_query *owq) ;

ZERO_OR_ERROR FS_w_sibling_F(_FLOAT F, const char * sibling, struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_w_sibling_U(  UINT U, const char * sibling, struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_w_sibling_Y(   INT Y, const char * sibling, struct one_wire_query *owq) ;

void FS_del_sibling(const char * sibling, struct one_wire_query *owq) ;

#endif							/* OW_SIBLING_H */
