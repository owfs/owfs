/*
$Id$
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

#ifndef OW_FD_H			/* tedious wrapper */
#define OW_FD_H

/* Simple type for file descriptors */
typedef int FILE_DESCRIPTOR_OR_ERROR ;
#define FILE_DESCRIPTOR_BAD -1

#define FILE_DESCRIPTOR_VALID(fd) ((fd)>FILE_DESCRIPTOR_BAD)
#define FILE_DESCRIPTOR_NOT_VALID(fd) (! FILE_DESCRIPTOR_VALID(fd))

/* Add a little complexity for Persistence
 * There is another state where a channel is alreaddy opened
 * and we with to test that one first
 */
typedef int FILE_DESCRIPTOR_OR_PERSISTENT ;
#define  FILE_DESCRIPTOR_PERSISTENT_IN_USE    -2

/* Pipe channels */
enum fd_pipe_channels {
	fd_pipe_read = 0 ,
	fd_pipe_write = 1 ,
} ;

#endif							/* OW_LOCALRETURNS_H */
