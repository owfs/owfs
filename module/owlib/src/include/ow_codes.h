/* $Id$
   Debug and error messages
   Meant to be included in ow.h
   Separated just for readability
*/

/* OWFS source code
   1-wire filesystem for linux
   {c} 2006 Paul H Alfille
   License GPL2.0
*/

#ifndef OW_CODES_H
#define OW_CODES_H

/* 1-wire ROM commands */
#define _1W_READ_ROM               0x33
#define _1W_MATCH_ROM              0x55
#define _1W_SKIP_ROM               0xCC
#define _1W_SEARCH_ROM             0xF0
#define _1W_CONDITIONAL_SEARCH_ROM 0xEC
#define _1W_RESUME                 0xA5
#define _1W_OVERDRIVE_SKIP_ROM     0x3C
#define _1W_OVERDRIVE_MATCH_ROM    0x69
#define _1W_SWAP                   0xAA	// used by the DS27xx

#endif							/* OW_CODES_H */
