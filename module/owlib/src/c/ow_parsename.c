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

#include <stdlib.h>
#include <string.h>

static int FS_ParsedNameSub( char * pathnow , char * pathnext, struct parsedname * pn ) ;
static int BranchAdd( struct parsedname * pn ) ;
static int DevicePart( char * filename, struct parsedname * pn ) ;
static int NamePart( char * filename, struct parsedname * pn ) ;
static int FilePart( char * filename, struct parsedname * pn ) ;

static int filecmp(const void * name , const void * ex ) ;

#define BRANCH_INCR (9)

/* ---------------------------------------------- */
/* Filename (path) parsing functions              */
/* ---------------------------------------------- */
void FS_ParsedName_destroy( struct parsedname * const pn ) {
    if(!pn) return ;
    if ( pn->bp ) {
        free( pn->bp ) ;
        pn->bp = NULL ;
        /* Reset persistent states from "local" stateinfo */
    }
    if ( pn->path ) {
        free(pn->path);
        pn->path = NULL;
    }
    if ( pn->path_busless ) {
        free(pn->path_busless);
        pn->path_busless = NULL;
    }
    if ( pn->in ) {
        if ( get_busmode(pn->in) != bus_remote ) {
	  if ((SemiGlobal & ~BUSRET_MASK) != (pn->si->sg & ~BUSRET_MASK)) {
	      CACHELOCK
	      SemiGlobal = (pn->si->sg & ~BUSRET_MASK) ;
	      CACHEUNLOCK
	  }
	}
    }
//printf("PN_destroy post\n");
}

/* Parse off starting "mode" directory (uncached, alarm...) */
int FS_ParsedName( const char * const path , struct parsedname * const pn ) {
    char * pathcpy ;
    char * pathnow ;
    char * pathnext ;
    int ret ;

    //printf("PARSENAME on %s\n",path);
    if ( pn == NULL ) return -EINVAL ;

    pn->path = NULL ;
    pn->desc = desc_error ; /* assume the worst */
    pn->path_busless = NULL ;
    pn->in = indevice ;
    pn->pathlength = 0 ; /* No branches yet */
    pn->bp = NULL ; /* No list of branches */
    pn->dev = NULL ; /* No Device */
    pn->ft = NULL ; /* No filetypes */
    pn->subdir = NULL ; /* Not subdirectory */
    pn->bus_nr = 0 ;
    memset(pn->sn,0,8) ; /* Blank number if not a device */

    if ( pn->si == NULL ) return -EINVAL ; /* Haven't set the stateinfo buffer */
    /* Set the persistent state info (temp scale, ...) -- will be overwritten by client settings in the server */
    pn->si->sg = SemiGlobal ;
//        pn->si->sg.u[0]&0x01 = cacheenabled ;
//        pn->si->sg.u[0]&0x02 = return bus-list from owserver
//        pn->si->sg.u[1]      = presencecheck ;
//        pn->si->sg.u[2]      = tempscale ;
//        pn->si->sg.u[3]      = devform ;

    /* minimal structure for setup use */
    if ( path==NULL ) return 0 ;

    /* Default attributes */
    pn->state = pn_normal ;
    pn->type = pn_real ;

    if ( (pathcpy=strdup( path )) == NULL ) return -ENOMEM ;
    pathnext = pathcpy ;

    pathnow = strsep(&pathnext,"/") ;

    /* remove initial "/" */
    if ( pathnow[0]=='\0' ) pathnow = strsep(&pathnext,"/") ;

    /* text is a special case, it can preceed anything */
    if ( pathnow && strcasecmp(pathnow,"text")==0 ) {
        pathnow = strsep(&pathnext,"/") ;
        pn->state |= pn_text ;
    }

    /* uncached is a special case, it can preceed anything except text */
    if ( pathnow && strcasecmp(pathnow,"uncached")==0 ) {
        pathnow = strsep(&pathnext,"/") ;
        pn->state |= pn_uncached ;
    }

    /* Processing for bus.X directories -- eventually will make this more generic */
    while ( pathnow && strncasecmp(pathnow,"bus.",4)==0 ) {
        if(!isdigit(pathnow[4])) return -ENOENT ;
	/* Should make a presence check on remote busses here, but
	 * it's not a major problem if people use bad paths since
	 * they will just end up with empty direcotry listings. */
        if(!(pn->state & pn_bus)) {
            size_t len = 1+5+9; // max size of text+uncached
            /* this will only be reached once */
            pn->state |= pn_bus ;
            pn->bus_nr = atoi(&pathnow[4]);
            /* Since we are going to use a specific in-device now, set
             * pn->in to point at that device at once. */
	    pn->in = find_connection_in(pn->bus_nr) ;
	    //printf("parse bus.=%d\n", pn->bus_nr);
	    /* We have to allow any bus-number here right now. We receive
	     * paths like /bus.4 from a remote owserver, and we have to trust
	     * this result. */
#if 0
	    if(!pn->in) {
	      printf("bus_nr %d doesn't exist\n", pn->bus_nr);
	      return -ENOENT ;
	    }
#endif
            if(pathnext) len += strlen(&path[pathnext-pathcpy]);
            if(!(pn->path_busless = malloc(len))) return -ENOMEM;
            pn->path_busless[0] = '\000';
            if(pn->state & pn_text) {
                strcat(pn->path_busless, "/text");
            }
            if(pn->state & pn_uncached) {
                strcat(pn->path_busless, "/uncached");
            }
            strcat(pn->path_busless, "/");
            if(pathnext) strcat(pn->path_busless, &path[pathnext-pathcpy]);

            pn->path = strdup(path);
	    //printf("PN set pn->path=%s\n", pn->path);
	    //printf("PN set pn->path_busless=%s\n", pn->path_busless);
        }
        pathnow = strsep(&pathnext,"/") ;
    }
    if ( !pn->path ) {
        pn->path = strdup(path);
        pn->path_busless = strdup(path);
	//printf("PN set2 pn->path=%s\n", pn->path);
	//printf("PN set2 pn->path_busless=%s\n", pn->path_busless);
    }
    //printf("pn->path=%s\n", pn->path);
    //printf("pn->path_busless=%s\n", pn->path_busless);


    //printf("1pathnow=[%s] pathnext=[%s] pn->type=%d\n", pathnow, pathnext, pn->type);

    /* look for special root directory -- it is really a flag */
    if ( pathnow && strcasecmp(pathnow,"statistics")==0 ) {
        pathnow = strsep(&pathnext,"/") ;
        pn->type |= pn_statistics ;
    } else if ( pathnow && strcasecmp(pathnow,"system")==0 ) {
        pathnow = strsep(&pathnext,"/") ;
        pn->type |= pn_system ;
    } else if ( pathnow && strcasecmp(pathnow,"settings")==0 ) {
        pathnow = strsep(&pathnext,"/") ;
        pn->type |= pn_settings ;
    } else if ( pathnow && strcasecmp(pathnow,"structure")==0 ) {
        pathnow = strsep(&pathnext,"/") ;
        pn->type |= pn_structure ;
    }

    //printf("2pathnow=[%s] pathnext=[%s] pn->type=%d\n", pathnow, pathnext, pn->type);
    ret = FS_ParsedNameSub( pathnow, pathnext, pn ) ;
    free(pathcpy) ;
    return ret ;
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
      pn->extension = -2 for BYTE, special bitfield representation of the data
*/
static int FS_ParsedNameSub( char * pathnow, char * pathnext, struct parsedname * pn ) {
    int ret ;
//printf("PN_sub: %s %s : %s\n",pn->path,pathnow,pathnext);

    /* must be of form /sdfa.sf/asdf.sdf */
    /* extensions optional */
    /* subdirectory optional (but only directories in "root") */
//printf("PN_sub\n");

    if( pathnow && strncasecmp( pathnow, "bus.", 4) == 0) {
        if(!isdigit(pathnow[4])) return -ENOENT ;
        pathnow = strsep(&pathnext,"/") ;
	//printf("deeper bus. request pathnow=%s\n", pathnow);
        return FS_ParsedNameSub( pathnow, pathnext, pn ) ;
    }
    if ( pathnow==NULL || pathnow[0]=='\0' ) {
        if ( pn->state & pn_alarm ) {
            pn->desc = desc_alarmdir ;
        } else if ( pn->dev == DeviceSimultaneous ) {
            pn->desc = desc_simuldir ;
        } else if ( pn->pathlength ) {
            pn->desc = desc_branchdir ;
        } else {
            pn->desc = desc_rootdir ;
        }
        return 0 ; /* root entry */
    }

    switch( pn->type ) {
    case pn_real:
        if ( strcasecmp( pathnow, "uncached" )==0 ) {
            pn->state |= pn_uncached ;
            pathnow = strsep(&pathnext,"/") ;
//printf("PARSENAME uncached %s\n",pathnext) ;
            return FS_ParsedNameSub( pathnow, pathnext, pn ) ;
        }
        if ( strcasecmp( pathnow, "alarm" )==0 ) {
            pn->state |= pn_alarm ;
            pathnow = strsep(&pathnext,"/") ;
//printf("PARSENAME alarm %s\n",pathnext) ;
            return FS_ParsedNameSub( pathnow, pathnext, pn ) ;
        }
        if ( strcasecmp( pathnow, "simultaneous" ) == 0 ) {
            pn->dev = DeviceSimultaneous ;
        } else if ( (ret=DevicePart( pathnow, pn )) ) { // search for valid 1-wire sn
//printf("PN DevicePart failed for %s %s\n", pathnow, pn->path);
            return ret ;
        }
        pn->desc = desc_device ;
        break ;
    default:
        ret = NamePart( pathnow, pn );
        if (ret ) {
//printf("PN NamePart failed for %s\n", pn->path);
            pn->desc = desc_error ;
            return ret ; // device name match?
        }
        pn->desc = desc_nondevice ;
        break ;
    }

    if ( pathnext==NULL || pathnext[0]=='\0' ) {
        if((pn->type ==pn_real) && !(pn->state & pn_bus)) {
	  int bus_nr = -1;
	  if(Cache_Get_Device(&bus_nr, pn)) {
	    //printf("PN Cache_Get_Device didn't find bus_nr\n");
	    bus_nr = CheckPresence(pn);
	    if(bus_nr >= 0) {
	      //printf("PN CheckPresence(%s) found bus_nr %d (add to cache)\n", pn->path, bus_nr);
	      Cache_Add_Device(bus_nr, pn);
	    }
	  } else {
	    //printf("PN Cache_Get_Device found bus! %d\n", bus_nr);
	  }
	  if(bus_nr < 0) {
	    //printf("PN CheckPresence failed\n");
	    return -ENOENT ;
	  }
	  /* fake that we read from only one indevice now! */
	  pn->in = find_connection_in(bus_nr);
	  pn->state |= pn_bus ;
	  pn->bus_nr = bus_nr ;
	}
        return 0 ;
    }
    pathnow = strsep(&pathnext,"/") ;
    pn->desc = desc_error ; /* Assume the worst, again */
//printf("PN2: %s %s : %s\n",pn->path,pathnow,pathnext);

    /* Now examine filetype */
    if ( (ret=FilePart( pathnow, pn )) ) {
      //printf("PN didn't find filetype %s %s\n", pathnow, pn->path);
      return ret ;
    }
    if ( pn->ft->format==ft_directory && pn->type == pn_real ) {
        if ( (ret=BranchAdd(pn)) ) {
//printf("PN BranchAdd failed for %s\n", pn->path);
            return ret ;
        }
        /* STATISCTICS */
        if ( pn->pathlength > dir_depth ) dir_depth = pn->pathlength ;
        pathnow = strsep(&pathnext,"/") ;
        return FS_ParsedNameSub( pathnow, pathnext, pn ) ;
    } else if ( pn->ft->format==ft_subdir ) { /* in-device subdirectory */
        if ( pathnext==NULL || pathnext[0]=='\0' ) {
            pn->subdir = pn->ft ;
            pn->ft = NULL ;
            pn->desc = desc_subdir ;
            return 0 ;
        } else {
            char * p = strsep( &pathnext, "/" ) ;
            p[-1] = '/' ; /* replace former "/" to make subdir */
            if ( (ret=FilePart( pathnow, pn )) ) {
//printf("PN FilePart failed for %s %s\n", pathnow, pn->path);
                return ret ;
            }
        }
    }
    pn->desc = desc_file ;

    if ( pathnext==NULL || pathnext[0]=='\0' ) {
        if((pn->type ==pn_real) && !(pn->state & pn_bus)) {
	  int bus_nr = -1;
	  if(Cache_Get_Device(&bus_nr, pn)) {
	    //printf("PN2 Cache_Get_Device didn't find bus_nr\n");
	    bus_nr = CheckPresence(pn);
	    if(bus_nr >= 0) {
	      //printf("PN2 CheckPresence(%s) found bus_nr %d (add to cache)\n", pn->path, bus_nr);
	      Cache_Add_Device(bus_nr, pn);
	    }
	  } else {
	    //printf("PN2 Cache_Get_Device found bus! %d\n", bus_nr);
	  }
	  if(bus_nr < 0) {
	    //printf("PN2 CheckPresence failed\n");
	    return -ENOENT ;
	  }
	  /* fake that we read from only one indevice now! */
	  pn->in = find_connection_in(bus_nr);
	  pn->state |= pn_bus ;
	  pn->bus_nr = bus_nr ;
	}
	return 0 ;
    }
    return -ENOENT ; /* Bad file type for this device */
}

/* Parse Name (non-device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
*/
static int NamePart( char * filename, struct parsedname * pn ) {
    FS_devicefind( filename, pn ) ;
    return ( pn->dev == &NoDevice ) ? -ENOENT : 0 ;
}

/* Parse Name (only device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next statement, of NULL if not filetype
 */
static int DevicePart( char * filename, struct parsedname * pn ) {
    char *dot = filename;

    //printf("DevicePart: %s %s\n", filename, pn->path);
    
    if(filename == NULL) return -ENOENT;
    
    while(dot && strncasecmp(dot, "/bus.", 5) == 0) {
        dot = strchr(&dot[1], '/');
        if(!dot) {
//printf("DevicePart: no more slash??\n");
            return -ENOENT ;
        }
        //printf("move dot from [%s] to [%s]\n", filename, dot);
        filename = dot;
    }

    if ( !isxdigit(filename[0]) || !isxdigit(filename[1]) ) {
//printf("devicepart2: not xdigit\n");
        return -ENOENT ; /* starts with 2 hex digits ? */
    } else {
        unsigned char ID[14] = { filename[0], filename[1], 0x00, } ;
        int i ;
//printf("NP hex = %s\n",filename ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefind( ID, pn ) ;
//printf("NP cmp'ed %s\n",ID ) ;
        for ( i=2, filename+=2 ; i<14 ; ++i,++filename ) { /* get ID number */
            if ( *filename == '.' ) ++filename ;
            if ( isxdigit(*filename) ) {
                ID[i] = *filename ;
            } else {
                return -ENOENT ;
            }
        }
//printf("NP0\n");
        pn->sn[0] = string2num( &ID[0] ) ;
        pn->sn[1] = string2num( &ID[2] ) ;
        pn->sn[2] = string2num( &ID[4] ) ;
        pn->sn[3] = string2num( &ID[6] ) ;
        pn->sn[4] = string2num( &ID[8] ) ;
        pn->sn[5] = string2num( &ID[10] ) ;
        pn->sn[6] = string2num( &ID[12] ) ;
        pn->sn[7] = CRC8compute(pn->sn,7,0) ;
        if ( *filename == '.' ) ++filename ;
//printf("NP1\n");
        if ( isxdigit(filename[0]) && isxdigit(filename[1]) ) {
            char crc[2] ;
            num2string(crc,pn->sn[7]) ;
            if ( strncasecmp( crc, filename, 2 ) ) return -ENOENT ;
        }
    }
//printf("NP2\n");
    return 0 ;
}

static int FilePart( char * filename, struct parsedname * pn ) {
    char * dot = filename ;

    //printf("FilePart: %s %s\n", filename, pn->path);

    if(filename == NULL) return -ENOENT;

    while(dot && strncasecmp(dot, "/bus.", 5) == 0) {
        dot = strchr(&dot[1], '/');
        if(!dot) {
//printf("FilePart: no more slash??\n");
            dot = filename;
            break;
        }
//printf("move dot from [%s] to [%s]\n", filename, dot);
        filename = dot;
    }
    
    filename = strsep(&dot,".") ;
//printf("FP name=%s, dot=%s\n", filename, dot);
    /* Match to known filetypes for this device */
    if ( (pn->ft = bsearch( filename , pn->dev->ft , (size_t) pn->dev->nft , sizeof(struct filetype) , filecmp )) ) {
//printf("FP known filetype %s\n",pn->ft->name) ;
        /* Filetype found, now process extension */
        if ( dot==NULL  || dot[0]=='\0' ) { /* no extension */
            if ( pn->ft->ag ) return -ENOENT ; /* aggregate filetypes need an extension */
            pn->extension = 0 ; /* default when no aggregate */
        } else if ( pn->ft->ag == NULL ) {
            return -ENOENT ; /* An extension not allowed when non-aggregate */
        } else if ( strcasecmp(dot,"ALL")==0 ) {
//printf("FP ALL\n");
            pn->extension = -1 ; /* ALL */
        } else if ( pn->ft->format==ft_bitfield && strcasecmp(dot,"BYTE")==0 ) {
            pn->extension = -2 ; /* BYTE */
//printf("FP BYTE\n") ;
        } else { 
            if ( pn->ft->ag->letters == ag_letters ) {
//printf("FP letters\n") ;
                if ( (strlen(dot)!=1) || !isupper(dot[0]) ) return -ENOENT ;
                pn->extension = dot[0] - 'A' ; /* Letter extension */
            } else { /* Numbers */
                char * p ;
//printf("FP numbers\n") ;
                pn->extension = strtol(dot,&p,0) ; /* Number extension */
                if ( (p==dot) || ((pn->extension == 0) && (errno==-EINVAL)) ) return -ENOENT ; /* Bad number */
            }
//printf("FP ext=%d nr_elements=%d\n", pn->extension, pn->ft->ag->elements) ;
            if (pn->type!=pn_real) {
	      /*
	       * We have to agree any extension from remote bus
	       * otherwise /system/adapter/address.4 and
	       * /statistics/bus/bus_locks.4 wouldn't be accepted
	       * Should not be needed on known devices...
	       */
            } else
                if ( (pn->extension < 0) || (pn->extension >= pn->ft->ag->elements) ) {
//printf("FP Extension out of range %d %d %s\n", pn->extension, pn->ft->ag->elements, pn->path);
                    return -ENOENT ; /* Extension out of range */
                }
//printf("FP in range\n") ;
        }
//printf("FP Good\n") ;
        return 0 ; /* Good file */
    }
//printf("FP not found\n") ;
    return -ENOENT ; /* filetype not found */
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

static int filecmp(const void * name , const void * ex ) {
    return strcmp( (const char *) name , ((const struct filetype *) ex)->name ) ;
}

static void my_delay(const unsigned int len) {
    struct timespec s;
    struct timespec rem;
    rem.tv_sec = len / 1000 ;
    rem.tv_nsec = 1000000*(len%1000) ;

    while(1) {
        s.tv_sec = rem.tv_sec;
        s.tv_nsec = rem.tv_nsec;
        if(nanosleep(&s, &rem) < 0) {
            if(errno != EINTR) break ;
            /* was interupted... continue sleeping... */
//printf("UT_delay: EINTR s=%ld.%ld r=%ld.%ld: %s\n", s.tv_sec, s.tv_nsec, rem.tv_sec, rem.tv_nsec, strerror(errno));
        } else {
            /* completed sleeping */
            break;
        }
    }
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void UT_delay(const unsigned int len) {
    if(len == 0) return;
    return my_delay(len);
}
