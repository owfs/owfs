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

#include <stdlib.h>

#include "owfs_config.h"
#include "ow_devices.h"

/* Length of file based on filetype alone */
size_t FileLength( const struct parsedname * const pn ) {
    if ( pn-> type == pn_structure )
        return 30 ;
    /* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */
    if ( pn->ft->format==ft_directory ||  pn->ft->format==ft_subdir ) {
        return 8 ; /* arbitrary, but non-zero for "find" and "tree" commands */
    } else if ( pn->ft->format==ft_bitfield &&  pn->extension==-2 ) {
        return 12 ; /* bitfield */
    } else if ( pn->in->busmode==bus_remote && pn->ft->suglen<0 ) {
        int ret ;
        if ( pn->badcopy ) { /* need correct path to send to server */
            int len = strlen(pn->path) ;
            char fullpath[len+OW_FULLNAME_MAX+2] ;
            strcpy(fullpath,pn->path) ;
            fullpath[len] = '/' ;
            if ( FS_FileName(&fullpath[len+1],OW_FULLNAME_MAX,pn) ) return 0 ;
            ret = ServerSize(fullpath,pn) ;
        } else {
            ret = ServerSize(pn->path,pn) ;
        }
        return ret<0 ? 0 : ret ;
    } else {
        switch(pn->ft->suglen) {
        case -fl_type:
            return strlen(pn->dev->name) ;
        case -fl_adap_name:
            return strlen(pn->in->adapter_name) ;
        case -fl_adap_port:
            return strlen(pn->in->name) ;
        case -fl_pidfile:
            if ( pid_file ) return strlen(pid_file) ;
            return 0 ;
        default:
            return pn->ft->suglen ;
        }
    }
}

/* Length of file based on filetype and extension */
size_t FullFileLength( const struct parsedname * const pn ) {
    if ( pn->type == pn_structure ) return 30 ;
    /* longest seem to be /1wire/structure/0F/memory.ALL (28 bytes) so far... */
    if ( pn->ft->ag ) { /* aggregate files */
        switch (pn->extension) {
        case -1: /* ALL */
		if((pn->ft->format==ft_binary) || (pn->ft->format == ft_ascii)) {
		  /* not comma-separated values are ft_binary and ft_ascii
		     ft_binary is used for all memory devices
		     ft_ascii is used in ow_lcd.c:line16.ALL */
		  return (pn->ft->ag->elements) * (pn->ft->suglen) ;
		} else {
		  /* comma separated, but does not end with a comma
		     used in ow_lcd.c:gpio.ALL which is ft_yesno for example */
		  return ((pn->ft->ag->elements) * (pn->ft->suglen + 1)) - 1 ;
		}
        case -2: /* BYTE */
            return 12 ;
        }
    }
    /* default simple file */
    return FileLength( pn ) ;
}
