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

static int FS_ParsedNameSub( const char * const path , struct parsedname * pn ) ;
static int BranchAdd( struct parsedname * const pn ) ;
static int DevicePart( const char * filename, const char ** next, struct parsedname * pn ) ;
static int NamePart( const char * filename, const char ** next, struct parsedname * pn ) ;
static int FilePart( const char * const filename, const char ** next, struct parsedname * const pn ) ;

static int filecmp(const void * name , const void * ex ) ;

#define BRANCH_INCR (9)

/* ---------------------------------------------- */
/* Filename (path) parsing functions              */
/* ---------------------------------------------- */
void FS_ParsedName_destroy( struct parsedname * const pn ) {
    if ( pn && pn->bp ) {
        free( pn->bp ) ;
        pn->bp = NULL ;
    }
//printf("PN_destroy post\n");
}

/* Parse off starting "mode" directory (uncached, alarm...) */
int FS_ParsedName( const char * const path , struct parsedname * const pn ) {
    static char * uncached = NULL ;
      static size_t luncached ;
    static char * structure ;
      static size_t lstructure ;
    static char * text ;
      static size_t ltext ;
    static char * system_ ;
      static size_t lsystem ;
    static char * settings ;
      static size_t lsettings ;
    static char * statistics ;
      static size_t lstatistics ;
    const char * pathnow = path ;

    if ( uncached == NULL ) { // first time through
      ltext      = strlen ( text      = (char *)FS_dirname_state(pn_text      )) ;
      luncached  = strlen ( uncached  = (char *)FS_dirname_state(pn_uncached  )) ;
      lstructure = strlen ( structure = (char *)FS_dirname_type( pn_structure )) ;
      lsystem    = strlen ( system_   = (char *)FS_dirname_type( pn_system    )) ;
      lsettings  = strlen ( settings  = (char *)FS_dirname_type( pn_settings  )) ;
      lstatistics= strlen ( statistics= (char *)FS_dirname_type( pn_statistics)) ;
    }
//printf("PN_pn\n");
    if ( pn == NULL ) return -EINVAL ;

    pn->pathlength = 0 ; /* No branches yet */
    pn->bp = NULL ; /* No list of branches */
    pn->dev = NULL ; /* No Device */
    pn->ft = NULL ; /* No filetypes */
    pn->subdir = NULL ; /* Not subdirectory */
    memset(pn->sn,0,8) ; /* Blank number if not a device */

    if ( path[0] != '/' ) return -ENOENT ;
    ++pathnow ;

    /* Default attributes */
    pn->state = pn_normal ;
    pn->type = pn_real ;

    /* text is a special case, it can preceed anything */
    if ( strncasecmp(pathnow,text,ltext)==0 ) {
        pn->state |= pn_text ;
        pathnow += ltext ;
        if ( pathnow[0] == '\0' ) return 0 ;
        if ( pathnow[0] != '/' ) return -ENOENT ;
        ++pathnow ;
    }

    /* uncached is a special case, it can preceed anything */
    if ( strncasecmp(pathnow,uncached,luncached)==0 ) {
        pn->state |= pn_uncached ;
        pathnow += luncached ;
        if ( pathnow[0] == '\0' ) return 0 ;
        if ( pathnow[0] != '/' ) return -ENOENT ;
        ++pathnow ;
    }

    /* look for special root directory -- it is really a flag */
    if ( strncasecmp(pathnow,statistics,lstatistics)==0 ) {
        pn->type = pn_statistics ;
        pathnow += lstatistics ;
    } else if ( strncasecmp(pathnow,structure,lstructure)==0 ) {
        pn->type = pn_structure ;
        pathnow += lstructure ;
    } else if ( strncasecmp(pathnow,system_,lsystem)==0 ) {
        pn->type = pn_system ;
        pathnow += lsystem ;
    } else if ( strncasecmp(pathnow,settings,lsettings)==0 ) {
        pn->type = pn_settings ;
        pathnow += lsettings ;
    } else {
        --pathnow ; // just to reset for the check that follows
    }
    if ( pathnow[0] == '\0' ) return 0 ;
    if ( pathnow[0] != '/' ) return -ENOENT ;
    ++pathnow ;
    return FS_ParsedNameSub( pathnow, pn ) ;
}

/* Parse the path to the correct device, filetype, directory, etc... */
/* returns 0=GOOD, 1=BAD
      pn->dev == NULL,     pn->ft == NULL    ROOT DIRECTORY
      pn->dev == NoDevice, pn->ft == NULL    Unknown device directory
      pn->dev == NoDevice, pn->ft != NULL    Unknown device, known file type
      pn->dev != NoDevice, pn->ft == NULL    Known device directory
      pn->dev != NoDevice, pn->ft == NULL  pn->subdir != NULL   Known device subdirectory
      pn->dev != NoDevice, pn->ft != NULL    Known device, known file type

      pn->extension = -1 for ALL, 0 if non-aggregate, else 0-max extensionss-1
*/
static int FS_ParsedNameSub( const char * const path , struct parsedname * pn ) {
    int ret ;
    const char * pFile ;
    const char * next ;
//printf("PN: %s\n",path);

    /* must be of form /sdfa.sf/asdf.sdf */
    /* extensions optional */
    /* subdirectory optional (but only directories in "root") */
//printf("PN_sub\n");
    if ( path[0] == '\0' ) return 0 ; /* root entry */

    /* Setup and initialization */
    /* Note -- Path is zero-ended */
    /* First element, device name (family.ID) */
//printf("PN:Pre %s\n",path);
    switch( pn->type ) {
    case pn_real:
        if ( strcmp( path, "alarm" )==0 ) {
            pn->state |= pn_alarm ;
            return 0 ; /* directory */
        }
        if ( strncmp( path, "alarm/", 6 )==0 ) {
            pn->state |= pn_alarm ;
            return FS_ParsedNameSub( &path[6], pn ) ;
        }
        if ( (ret=DevicePart( path, &pFile, pn )) ) return ret ; // search for valid 1-wire sn
        if ( pFile == NULL || pFile[0]=='\0' ) return (presencecheck && CheckPresence(pn)) ? -ENOENT : 0 ; /* directory */
        break ;
    default:
        if ( (ret=NamePart( path, &pFile, pn )) ) return ret ; // device name match?
        if ( pFile == NULL || pFile[0]=='\0' ) return 0 ; /* directory */
        break ;
    }

    /* Now examine filetype */
    if ( (ret=FilePart(pFile, &next, pn )) ) return ret ;

    if ( pn->ft->format==ft_directory && pn->type == pn_real ) {
        if ( (ret=BranchAdd(pn)) ) return ret ;
        /* STATISCTICS */
        if ( pn->pathlength > dir_depth ) dir_depth = pn->pathlength ;
//if ( next ) printf("Resub = %s->%s\n",pFile,next) ;
        return next ? FS_ParsedNameSub( next , pn ) : 0 ;
    } else if ( pn->ft->format==ft_subdir ) { /* in-device subdirectory */
        pn->subdir = pn->ft ;
        pn->ft = NULL ; /* subdir, not a normal file yet */
        if ( next ) { /* Now re-examine filetype */
            if ( (ret=FilePart(next, &next, pn )) ) return ret ;
        }
    }

    return next ? -ENOENT : 0 ; /* Bad file type for this device */
}

/* Parse Name (non-device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next statement, of NULL if not filetype
 */
static int NamePart( const char * filename, const char ** next, struct parsedname * pn ) {
    const char * f = filename ;
    char * sep ;
    char fn[33] ;

    strncpy(fn,filename,32);
    fn[32] = '\0' ;
    if ( (sep=strchr(fn,'/')) ) *sep = '\0' ;

    FS_devicefind( fn, pn ) ;
    if ( pn->dev != &NoDevice ) {
        f += strlen(fn) ;
    } else {
        return -ENOENT ; /* unknown device */
    }

    if ( f[0]=='/' ) {
        *next = &f[1] ;
    } else if ( f[0] == '\0' ) {
        *next = NULL ;
    } else {
        return -ENOENT ;
    }
    return 0 ;
}

/* Parse Name (only device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next statement, of NULL if not filetype
 */
static int DevicePart( const char * filename, const char ** next, struct parsedname * pn ) {
    const char * f = filename ;
    if ( isxdigit(filename[0]) && isxdigit(filename[1]) ) { /* starts with 2 hex digits */
        unsigned char ID[14] = { filename[0], filename[1], 0x00, } ;
        int i ;
//printf("NP hex = %s\n",filename ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefind( ID, pn ) ;
//printf("NP cmp'ed %s\n",ID ) ;
        for ( i=2, f+=2 ; i<14 ; ++i,++f ) { /* get ID number */
            if ( *f == '.' ) ++f ;
            if ( isxdigit(*f) ) {
                ID[i] = *f ;
            } else {
                return -ENOENT ;
            }
            pn->sn[0] = string2num( &ID[0] ) ;
            pn->sn[1] = string2num( &ID[2] ) ;
            pn->sn[2] = string2num( &ID[4] ) ;
            pn->sn[3] = string2num( &ID[6] ) ;
            pn->sn[4] = string2num( &ID[8] ) ;
            pn->sn[5] = string2num( &ID[10] ) ;
            pn->sn[6] = string2num( &ID[12] ) ;
            pn->sn[7] = CRC8compute(pn->sn,7,0) ;
        }
        if ( *f == '.' ) ++f ;
        if ( isxdigit(f[0]) && isxdigit(f[1]) ) {
            char crc[2] ;
            num2string(crc,pn->sn[7]) ;
            if ( strncasecmp( crc, f, 2 ) ) return -ENOENT ;
            f += 2 ;
        }
    } else if ( strncasecmp( filename, "simultaneous", 12 ) == 0 ) {
       FS_devicefind( "simultaneous", pn ) ;
       f += 12 ;
    } else {
        return -ENOENT ; /* unknown device */
    }
    if ( f[0]=='/' ) {
        *next = &f[1] ;
    } else if ( f[0] == '\0' ) {
        *next = NULL ;
    } else {
        return -ENOENT ;
    }
    return 0 ;
}

static int FilePart( const char * const filename, const char ** next, struct parsedname * const pn ) {
    char pFile[65] ;
    char * pF2 = pFile ;
    char * pExt ;
    char * p ;

//printf("FilePart on %s\n",filename);
    if ( pn->subdir ) { // subdirectory in progress
        strcpy( pFile , pn->subdir->name ) ;
        pF2 += strlen(pFile) ;
        strcpy( pF2 , "/" ) ;
        ++pF2 ;
        pn->subdir = NULL ; // clear subdir field -- normal file */
//printf("FilePart 2nd pass subdir=%s\n",pFile);
    }

    strncpy( pF2 , filename , 32 ) ; /* Max length of filename */
    pF2[32] = '\0' ;
    if ( (p=strchr(pF2,'/')) ) { /* found continued path */
        if (next) *next = &filename[p-pF2] +1 ; /* start of next path element */
        *p = '\0' ;
    } else if ( strlen(filename) > 32 ) { /* filename too long */
        return -ENOENT ; /* filename too long */
    } else { /* no continued path */
        if (next) *next = NULL ;
    }
//printf("FilePart 2nd pass name=%s\n",pFile);

    pExt = strchr(pF2,'.') ; /* look for extension */
    if ( pExt ) {
        *pExt = '\0' ;
        ++pExt ;
//printf("FP file with extension=%s\n",pExt);
    }

    /* Match to known filetypes for this device */
    if ( (pn->ft = bsearch( pFile , pn->dev->ft , (size_t) pn->dev->nft , sizeof(struct filetype) , filecmp )) ) {
//printf("FP known filetype %s\n",pn->ft->name) ;
        /* Filetype found, now process extension */
        if ( pExt==NULL ) { /* no extension */
            if ( pn->ft->ag ) return -ENOENT ; /* aggregate filetypes need an extension */
            pn->extension = 0 ; /* default when no aggregate */
        } else if ( pn->ft->ag == NULL ) {
            return -ENOENT ; /* An extension not allowed when non-aggregate */
        } else if ( strcasecmp(pExt,"ALL")==0 ) {
            pn->extension = -1 ; /* ALL */
//printf("FP ALL\n") ;
        } else {
            if ( pn->ft->ag->letters == ag_letters ) {
//printf("FP letters\n") ;
                if ( (strlen(pExt)!=1) || !isupper(*pExt) ) return -ENOENT ;
                pn->extension = *pExt - 'A' ; /* Letter extension */
            } else { /* Numbers */
//printf("FP numbers\n") ;
                pn->extension = strtol(pExt,&p,0) ; /* Number extension */
                if ( (p==pExt) || ((pn->extension == 0) && (errno==-EINVAL)) ) return -ENOENT ; /* Bad number */
            }
            if ( (pn->extension < 0) || (pn->extension >= pn->ft->ag->elements) ) return -ENOENT ; /* Extension out of range */
//printf("FP in range\n") ;
        }
//printf("FP Good\n") ;
        return 0 ; /* Good file */
    }
    return -ENOENT ; /* filetype not found */
}

/* devicecmp is used in search of device table. called in ow_dir.c, too */
int devicecmp(const void * code , const void * dev ) {
//printf ("CMP code=%s compare to %s  dev=%p\n",(const char *) code,(*((struct device * const *) dev))->code,dev) ;
    return strcmp ( (const char *) code , (*((struct device * const *) dev))->code ) ;
}

static int filecmp(const void * name , const void * ex ) {
    return strcmp( (const char *) name , ((const struct filetype *) ex)->name ) ;
}


//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void UT_delay(const unsigned int len)
{
    struct timespec s = {0,0,};              // Set aside memory space on the stack

    bus_pause.tv_usec += len*1000 ;
    if ( bus_pause.tv_usec > 100000000 ) {
        bus_pause.tv_usec -= 100000000 ;
        bus_pause.tv_sec  += 100 ;
    }

    s.tv_sec += len / 1000 ;
    s.tv_nsec = 1000000*(len%1000) ;

    nanosleep(&s, NULL);
}

/* Length of file based on filetype alone */
size_t FileLength( const struct parsedname * const pn ) {
    if ( pn-> type == pn_structure ) {
        return 26 ;
    } else if ( pn->ft->format==ft_directory ||  pn->ft->format==ft_subdir ) {
        return 8 ; /* arbitrary, but non-zero for "find" and "tree" commands */
    } else {
        switch(pn->ft->suglen) {
        case -fl_type:
            return strlen(pn->dev->name) ;
        case -fl_adap_name:
            return strlen(adapter_name) ;
        case -fl_adap_port:
            return strlen(devport) ;
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
    if ( pn->type != pn_structure && pn->ft->ag && pn->extension==-1 ) {
        if ( pn->ft->format==ft_binary ) return pn->ft->suglen * pn->ft->ag->elements ;
        return (1+pn->ft->suglen)*(pn->ft->ag->elements)-1 ;
    }
    return FileLength( pn ) ;
}

static int BranchAdd( struct parsedname * const pn ) {
//printf("BRANCHADD\n");
    if ( (pn->pathlength%BRANCH_INCR)==0 ) {
        if ( (pn->bp=realloc( pn->bp, (BRANCH_INCR+pn->pathlength) * sizeof( struct buspath ) ))==NULL ) return -ENOMEM ;
    }
    memcpy( pn->bp[pn->pathlength].sn, pn->sn , 8 ) ; /* copy over DS2409 name */
    pn->bp[pn->pathlength].branch = (int)(pn->ft->data) ;
    ++pn->pathlength ;
    pn->ft = NULL ;
    pn->dev = NULL ;
    return 0 ;
}
