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
void FS_devicename( char * const buffer, const size_t length, const struct parsedname * pn ) {
    const unsigned char * p = pn->sn ;
    UCLIBCLOCK
//printf("dev format sg=%X DeviceFormat = %d\n",pn->si->sg,DeviceFormat(pn)) ;
    switch (DeviceFormat(pn)) {
    case fdi:
        snprintf( buffer , length, "%02X.%02X%02X%02X%02X%02X%02X",p[0],p[1],p[2],p[3],p[4],p[5],p[6]) ;
        break ;
    case fi:
        snprintf( buffer , length, "%02X%02X%02X%02X%02X%02X%02X",p[0],p[1],p[2],p[3],p[4],p[5],p[6]) ;
        break ;
    case fdidc:
        snprintf( buffer , length, "%02X.%02X%02X%02X%02X%02X%02X.%02X",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]) ;
        break ;
    case fdic:
        snprintf( buffer , length, "%02X.%02X%02X%02X%02X%02X%02X%02X",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]) ;
        break ;
    case fidc:
        snprintf( buffer , length, "%02X%02X%02X%02X%02X%02X%02X.%02X",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]) ;
        break ;
    case fic:
        snprintf( buffer , length, "%02X%02X%02X%02X%02X%02X%02X%02X",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]) ;
        break ;
    }
    UCLIBCUNLOCK
}

const char dirname_state_uncached[] = "uncached";
const char dirname_state_alarm[]    = "alarm";
const char dirname_state_text[]     = "text";
const char dirname_state_unknown[]  = "";

char * FS_dirname_state( const struct parsedname * pn ) {
    char tmp[64];
    //printf("dirname state on %.2X\n", pn->state);
    if ( pn->state & pn_alarm   ) return strdup(dirname_state_alarm) ;
    //should never return text in a directory list... it's a hidden feature.
    //if ( pn->state & pn_text    ) return strdup(dirname_state_text) ;
    if ( pn->state & pn_uncached) return strdup(dirname_state_uncached) ;
    if ( pn->state & pn_bus ) {
      sprintf(tmp, "bus.%d", pn->bus_nr) ;
      return strdup(tmp) ;
    }
    printf("FS_dirname_state: unknown state %.2X on %s\n", pn->state, pn->path);
    return strdup(dirname_state_unknown);
}

const char dirname_type_statistics[] = "statistics";
const char dirname_type_system[]     = "system";
const char dirname_type_settings[]   = "settings";
const char dirname_type_structure[]  = "structure";
const char dirname_type_unknown[]    = "";

const char * FS_dirname_type( const enum pn_type type ) {
    switch (type) {
    case pn_statistics:
        return dirname_type_statistics;
    case pn_system:
        return dirname_type_system;
    case pn_settings:
        return dirname_type_settings;
    case pn_structure:
        return dirname_type_structure;
    default:
        return dirname_type_unknown;
    }
}

/* name of file from filetype structure -- includes extension */
int FS_FileName( char * name, const size_t size, const struct parsedname * pn ) {
    int s ;
    if ( pn->ft == NULL ) return -ENOENT ;
    UCLIBCLOCK
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
    UCLIBCUNLOCK
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

        UCLIBCLOCK
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
        UCLIBCUNLOCK
    } else if ( pn->subdir ) { /* in-device subdirectory */
        strncpy( buffer, pn->subdir->name, size) ;
    } else if (pn->dev == NULL ) { /* root-type directory */
        if ( pn->state ) {
	      char *dname ;
	      if( (dname = FS_dirname_state(pn)) ) {
		strncpy( buffer, dname, size );
		free(dname) ;
	      }
        } else {
            strncpy( buffer, FS_dirname_type( pn->type ), size ) ;
        }
    } else if ( pn->dev == DeviceSimultaneous ) {
        strncpy( buffer, DeviceSimultaneous->code, size ) ;
    } else if ( pn->type == pn_real ) { /* real device */
        FS_devicename( buffer, size, pn ) ;
    } else { /* pseudo device */
        strncpy( buffer, pn->dev->code, size ) ;
    }
}
