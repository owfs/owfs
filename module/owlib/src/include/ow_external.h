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

#ifndef OW_EXTERNAL_H			/* tedious wrapper */
#define OW_EXTERNAL_H

#include "ow_device.h"
#include "ow_filetype.h"

enum external_type {
	et_none,
	et_internal,
	et_script,
	et_tcp,
	et_udp,
} ;

struct sensor_node {
	char * name ;
	char * family ;
	char * description ;
	char * data ;
	char payload[] ;
} ;

struct property_node {
	char * property ;
	char * family ;
	char * read ;
	char * write ;
	char * data ;
	char * other ;
	enum external_type et ;
	struct filetype ft ;
	struct aggregate ag ;
	char payload[] ;
} ;

struct family_node {
	struct device dev ; // Must be first since this is actual entry in device tree.
	char * family ;
	char payload[] ;
} ;

extern void * property_tree ;
extern void * family_tree ;
extern void * sensor_tree ;

void AddSensor( char * input_string ) ;
void AddProperty( char * input_string, enum external_type et ) ;

struct sensor_node * Find_External_Sensor( char * sensor ) ;
struct family_node * Find_External_Family( char * family ) ;
struct property_node * Find_External_Property( char * family, char * property ) ;

int sensor_compare( const void * a , const void * b ) ;
int family_compare( const void * a , const void * b ) ;
int property_compare( const void * a , const void * b ) ;

#endif							/* OW_EXTERNAL_H */
