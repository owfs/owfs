/*
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

#ifndef OW_BITFIELD_H			/* tedious wrapper */
#define OW_BITFIELD_H

#include "ow_localreturns.h"

// struct bitfield { "alias_link", number_of_bits, shift_left, }

struct bitfield {
	ASCII * link ; // name of property
	unsigned int size ; // number of bits
	unsigned int shift ; // bits to shift over
} ;

ZERO_OR_ERROR FS_r_bitfield(struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_w_bitfield(struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_r_bit_array(struct one_wire_query *owq) ;
ZERO_OR_ERROR FS_w_bit_array(struct one_wire_query *owq) ;

#endif							/* OW_BITFIELD_H */
