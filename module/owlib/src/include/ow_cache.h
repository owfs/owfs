/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifdef OW_CACHE

#ifndef OW_CACHE_H
#define OW_CACHE_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif

#include <sys/types.h>
#include <limits.h>
#include <db.h>

void Cache_Open( void ) ;
void Cache_Close( void ) ;

int Cache_Add( const char * path, const size_t size, const void * data, const enum ft_change change ) ;
int Cache_Get( const char * path, size_t *size, void * data, const enum ft_change change ) ;
int Cache_Del( const char * path ) ;

int Storage_Add( const char * path, const size_t size, const void * data ) ;
int Storage_Get( const char * path, size_t *size, void * data ) ;
int Storage_Del( const char * path ) ;

extern DB * dbcache ;
extern DB * dbstore ;

#endif /* OW_CACHE_H */

#endif /* OW_CACHE */
