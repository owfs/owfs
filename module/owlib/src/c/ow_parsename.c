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
    int preleng ; /* preamble length */
//printf("PN_pn\n");
    if ( pn == NULL ) return -EINVAL ;

    pn->pathlength = 0 ; /* No branches yet */
    pn->bp = NULL ; /* No list of branches */
    pn->dev = NULL ; /* No Device */
    pn->ft = NULL ; /* No filetypes */

    if ( path[0] != '/' ) return -ENOENT ;

#ifdef OW_CACHE
    if ( strncasecmp(path,"/uncached",(preleng=9))==0 ) {
        pn->type = pn_uncached ;
    } else
#endif /* OW_CACHE */
    if ( strncasecmp(path,"/alarm",(preleng=6))==0 ) {
        pn->type = pn_alarm ;
    } else {
        preleng=0 ;
        pn->type = pn_normal ; /* Not yet cache, etc... */
    }
    if ( path[preleng] == '\0' ) return 0 ;
    if ( path[preleng] != '/' ) return -ENOENT ;
    return FS_ParsedNameSub( &path[preleng+1], pn ) ;
}

/* Parse the path to the correct device, filetype, directory, etc... */
/* returns 0=GOOD, 1=BAD
      pn->dev == NULL,     pn->ft == NULL    ROOT DIRECTORY
      pn->dev == NoDevice, pn->ft == NULL    Unknown device directory
      pn->dev == NoDevice, pn->ft != NULL    Unknown device, known file type
      pn->dev != NoDevice, pn->ft == NULL    Known device directory
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
    if ( (ret=NamePart( path, &pFile, pn )) ) return ret ;
//printf("PN:POST %s\n",path);

    /* Now examine filetype */
    if ( pFile ==NULL || pFile[0]=='\0' ) {
//printf("PN no file\n");
        return (presencecheck && CheckPresence(pn)) ? -ENOENT : 0 ; /* directory */
    }

    if ( (ret=FilePart(pFile, &next, pn )) ) return ret ;

    if ( pn->ft->format==ft_directory ) {
        if ( (ret=BranchAdd(pn)) ) return ret ;
        /* STATISCTICS */
        if ( pn->pathlength > dir_depth ) dir_depth = pn->pathlength ;
//if ( next ) printf("Resub = %s->%s\n",pFile,next) ;
        return next ? FS_ParsedNameSub( next , pn ) : 0 ;
    }

    return next ? -ENOENT : 0 ; /* Bad file type for this device */
}

/* Parse Name (device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next statement, of NULL if not filetype
 */
int NamePart( const char * filename, const char ** next, struct parsedname * pn ) {
	struct device ** dpp ;
	const char * f = filename ;
	if ( isxdigit(filename[0]) && isxdigit(filename[1]) ) { /* starts with 2 hex digits */
		unsigned char ID[14] = { filename[0], filename[1], 0x00, } ;
		int i ;
//printf("NP hex = %s\n",filename ) ;
		/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
		if ( (dpp = bsearch(ID,Devices,nDevices,sizeof(struct device *),devicecmp)) ) {
			pn->dev = *dpp ;
		} else {
			pn->dev = &NoDevice ; /* unknown device */
		}
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
			pn->sn[7] = CRC8compute(pn->sn,7) ;
		}
		if ( *f == '.' ) ++f ;
		if ( isxdigit(f[0]) && isxdigit(f[1]) ) {
			char crc[2] ;
			num2string(crc,pn->sn[7]) ;
			if ( strncasecmp( crc, f, 2 ) ) return -ENOENT ;
			f += 2 ;
		}
	} else {
		char * sep ;
		char fn[33] ;
//printf("NP nonhex = %s\n",filename ) ;
		strncpy(fn,filename,32);
		fn[32] = '\0' ;
		if ( (sep=strchr(fn,'/')) ) *sep = '\0' ;
		if ( (dpp = bsearch(fn,Devices,nDevices,sizeof(struct device *),devicecmp)) ) {
			pn->dev = *dpp ;
			f += strlen(fn) ;
		} else {
			return -ENOENT ; /* unknown device */
		}
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

int FilePart( const char * const filename, const char ** next, struct parsedname * const pn ) {
    char pFile[33] ;
    char * pExt ;
    char * p ;

//printf("FilePart\n");
    strncpy( pFile , filename , 32 ) ; /* Max length of filename */
    pFile[32] = '\0' ;
    if ( (p=strchr(pFile,'/')) ) { /* found continued path */
        *next = &filename[p-pFile] +1 ; /* start of next path element */
        *p = '\0' ;
    } else if ( strlen(filename) > 32 ) { /* filename too long */
        return -ENOENT ; /* filename too long */
    } else { /* no continued path */
        *next = NULL ;
    }

    pExt = strchr(pFile,'.') ; /* look for extension */
    if ( pExt ) {
    	*pExt = '\0' ;
    	++pExt ;
//printf("FP file with extension=%s\n",pExt);
    }

    /* Mathc to known filetypes for this device */
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
///printf("FP Good\n") ;
		return 0 ; /* Good file */
    }
    return -ENOENT ; /* filetype not found */
}



int devicecmp(const void * code , const void * dev ) {
//printf ("CMP code=%s compare to %s  dev=%p\n",(const char *) code,(*((struct device * const *) dev))->code,dev) ;
    return strcmp ( (const char *) code , (*((struct device * const *) dev))->code ) ;
}

int filecmp(const void * name , const void * ex ) {
    return strcmp( (const char *) name , ((const struct filetype *) ex)->name ) ;
}


//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void UT_delay(const int len)
{
    struct timespec s;              // Set aside memory space on the stack

    s.tv_sec = len / 1000;
    s.tv_nsec = (len - (s.tv_sec * 1000)) * 1000000;
    bus_pause.tv_usec += len*1000 ;
    if ( bus_pause.tv_usec > 100000000 ) {
        bus_pause.tv_usec -= 100000000 ;
        bus_pause.tv_sec  += 100 ;
    }

    nanosleep(&s, NULL);
}

/* --------------------- */
/* Sorting functions for */
/* the name parsing data */
/* structures.           */
/* --------------------- */
int devicesort( const void * a , const void * b ) {
    return strcmp( (*((struct device * const *)a))->code , (*((struct device * const *)b))->code ) ;
}
int filesort( const void * a , const void * b ) {
    return strcmp( ((const struct filetype *)a)->name , ((const struct filetype *)b)->name ) ;
}

/* ------------------------------- */
/* special resort in case static data not properly sorted */
void DeviceSort( void ) {
    int i ;

    /* Sort the devices */
    qsort(Devices,nDevices,sizeof(struct device *),devicesort) ;

    /* Sort the filetypes withing the devices */
    /* Also set 3rd element */
    for ( i=0 ; (size_t)i<nDevices ; ++i ) {
//printf("DeviceSort code=%s name=%s nft=%d\n",Devices[i]->code,Devices[i]->name,Devices[i]->nft) ;
        qsort( Devices[i]->ft,(size_t) Devices[i]->nft,sizeof(struct filetype),filesort) ;
    }

    /* Sort the filetypes for the unrecognized device */
    qsort( NoDevice.ft,(size_t) NoDevice.nft,sizeof(struct filetype),filesort) ;
}

void FS_parse_dir( char * const dest , const char * const buf ) { /* shift address into path filename format */
    dest[ 0]= buf[ 0] ;
    dest[ 1] = buf[ 1] ;
    dest[ 2] = '.' ;
    dest[ 3] = buf[ 2] ;
    dest[ 4] = buf[ 3] ;
    dest[ 5] = buf[ 4] ;
    dest[ 6] = buf[ 5] ;
    dest[ 7] = buf[ 6] ;
    dest[ 8] = buf[ 7] ;
    dest[ 9] = buf[ 8] ;
    dest[10] = buf[ 9] ;
    dest[11] = buf[10] ;
    dest[12] = buf[11] ;
    dest[13] = buf[12] ;
    dest[14] = buf[13] ;
    dest[15] = '\0' ;
}

/* Length of file based on filetype and extension */
size_t FileLength( const struct parsedname * const pn ) {
	if ( pn->ft->suglen == ft_len_type ) {
		return strlen(pn->dev->name) ;
	} else if ( pn->ft->ag && pn->extension==-1 ) {
			if ( pn->ft->format==ft_binary ) return pn->ft->suglen * pn->ft->ag->elements ;
			return (1+pn->ft->suglen)*(pn->ft->ag->elements)-1 ;
	} else {
		return pn->ft->suglen ;
	}
}

/* For array properties -- length of full aggregate file */
size_t FullFileLength( const struct parsedname * const pn ) {
	if ( pn->ft->suglen == ft_len_type ) {
		return strlen(pn->dev->name) ;
	} else if ( pn->ft->ag ) {
			if ( pn->ft->format==ft_binary ) return pn->ft->suglen * pn->ft->ag->elements ;
			return (1+pn->ft->suglen)*(pn->ft->ag->elements)-1 ;
	} else {
		return pn->ft->suglen ;
	}
}

static int BranchAdd( struct parsedname * const pn ) {
//printf("BRANCHADD\n");
    if ( pn->pathlength == 0 ) {
        if ( (pn->bp=calloc( BRANCH_INCR, sizeof( struct buspath ) ))==NULL ) return -ENOMEM ;
    } else if ( (pn->pathlength%BRANCH_INCR)==0 ) {
        if ( (pn->bp=realloc( pn->bp, (BRANCH_INCR+pn->pathlength) * sizeof( struct buspath ) ))==NULL ) return -ENOMEM ;
    }
    memcpy( pn->bp[pn->pathlength].sn, pn->sn , 8 ) ; /* copy over DS2409 name */
    pn->bp[pn->pathlength].branch = (int)(pn->ft->data) ;
    ++pn->pathlength ;
    pn->ft = NULL ;
    pn->dev = NULL ;
    return 0 ;
}
