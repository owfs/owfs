/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

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
//     ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

/* Cannot stand alone -- part of ow.h but separated for clarity */

#ifndef OW_ARG_H			/* tedious wrapper */
#define OW_ARG_H

GOOD_OR_BAD ARG_Net(const char *arg);
GOOD_OR_BAD ARG_Server(const char *arg);
GOOD_OR_BAD ARG_USB(const char *arg);
GOOD_OR_BAD ARG_Device(const char *arg);
GOOD_OR_BAD ARG_Generic(const char *arg);
GOOD_OR_BAD ARG_Serial(const char *arg);
GOOD_OR_BAD ARG_Parallel(const char *arg);
GOOD_OR_BAD ARG_I2C(const char *arg);
GOOD_OR_BAD ARG_HA5( const char *arg);
GOOD_OR_BAD ARG_HA7(const char *arg);
GOOD_OR_BAD ARG_HA7E(const char *arg);
GOOD_OR_BAD ARG_DS1WM(const char *arg);
GOOD_OR_BAD ARG_K1WM(const char *arg);
GOOD_OR_BAD ARG_ENET(const char *arg);
GOOD_OR_BAD ARG_PBM(const char *arg);
GOOD_OR_BAD ARG_EtherWeather(const char *arg);
GOOD_OR_BAD ARG_External(const char *arg);
GOOD_OR_BAD ARG_Xport(const char *arg);
GOOD_OR_BAD ARG_Fake(const char *arg);
GOOD_OR_BAD ARG_Tester(const char *arg);
GOOD_OR_BAD ARG_Mock(const char *arg);
GOOD_OR_BAD ARG_Link(const char *arg);
GOOD_OR_BAD ARG_W1_monitor(void);
GOOD_OR_BAD ARG_MasterHub(const char *arg);
GOOD_OR_BAD ARG_Browse(void);
GOOD_OR_BAD ARG_Passive(char *adapter_type_name, const char *arg);

#endif							/* OW_ARG_H */
