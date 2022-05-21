/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

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

*/

/* Cannot stand alone -- separated out of ow.h for clarity */

#ifndef OW_PRESSURE_H			/* tedious wrapper */
#define OW_PRESSURE_H

/* Global pressure scale up tp 8 */
enum pressure_type { pressure_mbar, pressure_atm, pressure_mmhg, pressure_inhg, pressure_psi, pressure_Pa, pressure_end_mark
};

_FLOAT Pressure(_FLOAT P, const struct parsedname *pn);
_FLOAT fromPressure(_FLOAT P, const struct parsedname *pn);
const char *PressureScaleName(enum pressure_type p);

#endif							/* OW_PRESSURE_H */

