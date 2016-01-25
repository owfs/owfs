/**
 * ow_ftdi lowlevel communication routines
 */

#if OW_FTDI

#ifndef OW_FTDI_H
#define OW_FTDI_H

// for some local types
#include "ow_localreturns.h"

struct connection_in ;

GOOD_OR_BAD owftdi_open( struct connection_in * in ) ;
void owftdi_close( struct connection_in * in ) ;
GOOD_OR_BAD owftdi_change(struct connection_in *in) ;
void owftdi_free( struct connection_in *in ) ;
void owftdi_flush( const struct connection_in *in ) ;
void owftdi_break( struct connection_in *in ) ;
SIZE_OR_ERROR owftdi_read(BYTE * data, size_t length, struct connection_in *in) ;
GOOD_OR_BAD owftdi_write( const BYTE * data, size_t length, struct connection_in *connection) ;

#endif 	/* OW_FTDI_H */

#endif /* OW_FTDI */

