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

#ifndef OW_XXXX_H
#define OW_XXXX_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
#include "ow.h"

/* ------- Prototypes ----------- */
void update_max_delay(struct parsedname *pn);
int FS_type(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;
int FS_code(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;
int FS_crc8(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;
int FS_ID(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;
int FS_address(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;
int FS_present(int *, const struct parsedname * pn) ;

/* ------- Structures ----------- */

#define F_address  \
    {"address"   ,  16,  NULL, ft_ascii , ft_static  , {a:FS_address}      , {v:NULL}, NULL, }
#define F_crc8     \
    {"crc8"      ,   2,  NULL, ft_ascii , ft_static  , {a:FS_crc8}         , {v:NULL}, NULL, }
#define F_id       \
    {"id"        ,  12,  NULL, ft_ascii , ft_static  , {a:FS_ID}           , {v:NULL}, NULL, }
#define F_code     \
    {"family"    ,   2,  NULL, ft_ascii , ft_static  , {a:FS_code}         , {v:NULL}, NULL, }
#define F_present  \
    {"present"   ,   1,  NULL, ft_yesno , ft_volatile, {y:FS_present}      , {v:NULL}, NULL, }
#define F_type     \
    {"type"      ,-fl_type,  NULL, ft_ascii , ft_static  , {a:FS_type}  , {v:NULL}, NULL, }

#define F_STANDARD          F_address,F_code,F_crc8,F_id,F_present,F_type

#endif

