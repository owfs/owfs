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

#include "owfs_config.h"
#include "ow_devices.h"

/* device display format */
void FS_devicename( char * const buffer, const size_t length, const unsigned char * sn, const struct parsedname * pn ) {
    UCLIBCLOCK;
//printf("dev format sg=%X DeviceFormat = %d\n",pn->si->sg,DeviceFormat(pn)) ;
        switch (DeviceFormat(pn)) {
        case fdi:
            snprintf( buffer , length, "%02X.%02X%02X%02X%02X%02X%02X",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6]) ;
            break ;
        case fi:
            snprintf( buffer , length, "%02X%02X%02X%02X%02X%02X%02X",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6]) ;
            break ;
        case fdidc:
            snprintf( buffer , length, "%02X.%02X%02X%02X%02X%02X%02X.%02X",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7]) ;
            break ;
        case fdic:
            snprintf( buffer , length, "%02X.%02X%02X%02X%02X%02X%02X%02X",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7]) ;
            break ;
        case fidc:
            snprintf( buffer , length, "%02X%02X%02X%02X%02X%02X%02X.%02X",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7]) ;
            break ;
        case fic:
            snprintf( buffer , length, "%02X%02X%02X%02X%02X%02X%02X%02X",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7]) ;
            break ;
        }
    UCLIBCUNLOCK;
}

static const char dirname_state_uncached[] = "uncached";
static const char dirname_state_alarm[]    = "alarm";
static const char dirname_state_text[]     = "text";

/* copy state into buffer (for constructing path) return number of chars added */
int FS_dirname_state( char * const buffer, const size_t length, const struct parsedname * pn ) {
    const char * p ;
    int len ;
//printf("dirname state on %.2X\n", pn->state);
    if ( pn->state & pn_alarm   ) {
        p = dirname_state_alarm ;
#if 0
    } else if ( pn->state & pn_text ) {
        /* should never return text in a directory listing, since it's a
	 * hidden feature. Uncached should perhaps be the same... */
        strncpy(buffer, dirname_state_text, length ) ;
#endif
    } else if ( pn->state & pn_uncached) {
        p = dirname_state_uncached ;
    } else if ( pn->state & pn_bus ) {
        UCLIBCLOCK;
	len = snprintf(buffer, length, "bus.%d", pn->bus_nr) ;
        UCLIBCUNLOCK;
        return len;
    } else {
        return 0 ;
    }
//printf("FS_dirname_state: unknown state %.2X on %s\n", pn->state, pn->path);
    strncpy( buffer, p, length ) ;
    len = strlen(p) ;
    if ( len<length ) return len ;
    return length ;
}

static const char dirname_type_statistics[] = "statistics";
static const char dirname_type_system[]     = "system";
static const char dirname_type_settings[]   = "settings";
static const char dirname_type_structure[]  = "structure";

/* copy type into buffer (for constructing path) return number of chars added */
int FS_dirname_type( char * const buffer, const size_t length, const struct parsedname * pn ) {
    const char * p ;
    int len ;
    switch (pn->type) {
    case pn_statistics:
        p = dirname_type_statistics ;
        break ;
    case pn_system:
        p = dirname_type_system ;
        break ;
    case pn_settings:
        p = dirname_type_settings ;
        break ;
    case pn_structure:
        p = dirname_type_structure ;
        break ;
    default:
        return 0 ;
    }
    strncpy( buffer, p, length ) ;
    len = strlen(p) ;
    if ( len<length ) return len ;
    return length ;
}

/* name of file from filetype structure -- includes extension */
int FS_FileName( char * name, const size_t size, const struct parsedname * pn ) {
    int s ;
    if ( pn->ft == NULL ) return -ENOENT ;
    UCLIBCLOCK;
        if ( pn->ft->ag == NULL ) {
            s = snprintf( name , size, "%s",pn->ft->name) ;
        } else if ( pn->extension == -1 ) {
            s = snprintf( name , size, "%s.ALL",pn->ft->name) ;
        } else if ( pn->extension == -2 ) {
            s = snprintf( name , size, "%s.BYTE",pn->ft->name) ;
        } else if ( pn->ft->ag->letters == ag_letters ) {
            s = snprintf( name , size, "%s.%c",pn->ft->name,pn->extension+'A') ;
        } else {
            s = snprintf( name , size, "%s.%-d",pn->ft->name,pn->extension) ;
        }
    UCLIBCUNLOCK;
    return (s<0) ? -ENOBUFS : 0 ;
}

/* Return the last part of the file name specified by pn */
/* This can be a device, directory, subdiirectory, if property file */
/* Prints this directory element (not the whole path) */
/* Suggest that size = OW_FULLNAME_MAX */
void FS_DirName( char * buffer, const size_t size, const struct parsedname * const pn ) {
    if ( pn->ft ) { /* A real file! */
        char * pname = strchr(pn->ft->name,'/') ; // for subdirectories
        if ( pname ) {
            ++ pname ;
        } else {
            pname = pn->ft->name ;
        }

        UCLIBCLOCK;
            if ( pn->ft->ag == NULL ) {
                snprintf( buffer , size, "%s",pname) ;
            } else if ( pn->extension == -1 ) {
                snprintf( buffer , size, "%s.ALL",pname) ;
            } else if ( pn->extension == -2 ) {
                snprintf( buffer , size, "%s.BYTE",pname) ;
            } else if ( pn->ft->ag->letters == ag_letters ) {
                snprintf( buffer , size, "%s.%c",pname,pn->extension+'A') ;
            } else {
                snprintf( buffer , size, "%s.%-d",pname,pn->extension) ;
            }
        UCLIBCUNLOCK;
    } else if ( pn->subdir ) { /* in-device subdirectory */
        strncpy( buffer, pn->subdir->name, size) ;
    } else if (pn->dev == NULL ) { /* root-type directory */
#if 1
        if ( pn->type != pn_real ) {
            FS_dirname_type( buffer, size, pn ) ;
        } else {
            FS_dirname_state(buffer, size, pn) ;
        }
#else
	/* All bits except pn_text */
        if ( pn->state & ~pn_text) {
            FS_dirname_state(buffer, size, pn) ;
        } else {
            FS_dirname_type( buffer, size, pn ) ;
        }
#endif
    } else if ( pn->dev == DeviceSimultaneous ) {
        strncpy( buffer, DeviceSimultaneous->code, size ) ;
    } else if ( pn->type == pn_real ) { /* real device */
        FS_devicename( buffer, size, pn->sn, pn ) ;
    } else { /* pseudo device */
        strncpy( buffer, pn->dev->code, size ) ;
    }
}
