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

#include <config.h>
#include "owfs_config.h"
#include "ow_devices.h"
#include "ow_counters.h"

#include <stdlib.h>
#include <string.h>

static int BranchAdd( struct parsedname * pn ) ;

static int filecmp(const void * name , const void * ex ) ;

enum  parse_enum {parse_first, parse_done, parse_error, parse_real, parse_nonreal, parse_prop, parse_subprop } ;

static enum parse_enum Parse_Unspecified( char * pathnow, int remote, struct parsedname * pn ) ;
static enum parse_enum Parse_Real( char * pathnow, int remote, struct parsedname * pn ) ;
static enum parse_enum Parse_NonReal( char * pathnow, struct parsedname * pn ) ;
static enum parse_enum Parse_RealDevice( char * filename, int remote, struct parsedname * pn ) ;
static enum parse_enum Parse_NonRealDevice( char * filename, struct parsedname * pn ) ;
static enum parse_enum Parse_Property( char * filename, struct parsedname * pn ) ;
static enum parse_enum Parse_Bus( const enum parse_enum pe_default, char * pathnow, struct parsedname * pn ) ;
static int FS_ParsedName_anywhere( const char * path , int remote, struct parsedname * pn ) ;

#define BRANCH_INCR (9)

/* ---------------------------------------------- */
/* Filename (path) parsing functions              */
/* ---------------------------------------------- */
void FS_ParsedName_destroy( struct parsedname * pn ) {
    if(!pn) return ;
    if ( pn->bp ) {
        free( pn->bp ) ;
        pn->bp = NULL ;
    }
    if ( pn->path ) {
        free(pn->path);
        pn->path = NULL;
    }
    if ( pn->lock ) {
        free(pn->lock) ;
        pn->lock = NULL ;
    }
}

/* Parse a path to check it's validity and attach to the propery data structures */
int FS_ParsedName( const char * path , struct parsedname * pn ) {
    return FS_ParsedName_anywhere( path, 0, pn ) ;
}

/* Parse a path from a remote source back -- so don't check presence */
int FS_ParsedName_Remote( const char * path , struct parsedname * pn ) {
    return FS_ParsedName_anywhere( path, 1, pn ) ;
}

/* Parse off starting "mode" directory (uncached, alarm...) */
static int FS_ParsedName_anywhere( const char * path , int remote, struct parsedname * pn ) {
    char * pathcpy ;
    char * pathnow ;
    char * pathnext ;
    char * pathlast ;
    int ret = 0 ;
    enum parse_enum pe = parse_first ;

    // To make the debug output useful it's cleared here.
    // Even on normal glibc, errno isn't cleared on good system calls
    errno = 0;

    LEVEL_CALL("PARSENAME path=[%s]\n", SAFESTRING(path));

    if ( pn == NULL ) return -EINVAL ;

    pn->path = pn->path_busless = NULL ;
    pn->in = indevice ;
    pn->pathlength = 0 ; /* No branches yet */
    pn->bp = NULL ; /* No list of branches */
    pn->dev = NULL ; /* No Device */
    pn->ft = NULL ; /* No filetypes */
    pn->subdir = NULL ; /* Not subdirectory */
    pn->bus_nr = -1 ; /* all busses */
    pn->lock = NULL ; /* device lock array */
    memset(pn->sn,0,8) ; /* Blank number if not a device */

    /* Set the persistent state info (temp scale, ...) -- will be overwritten by client settings in the server */
    pn->sg = SemiGlobal | (1<<BUSRET_BIT) ; // initial flag as the bus-returning level, will change if a bus is specified
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

    /* make a copy for destructive parsing */
    pathcpy = strdup( path ) ;
    pn->path = (char *) malloc(2 * strlen(path)+ 2) ;
    pn->lock = calloc( indevices, sizeof(struct devlock *) ) ;

    if ( pathcpy==NULL || pn->path==NULL || pn->lock==NULL ) {
        if (pathcpy) free(pathcpy) ;
        if (pn->path) {
	  free(pn->path) ;
	  pn->path = NULL;
	}
        if (pn->lock) {
	  free(pn->lock) ;
	  pn->lock = NULL;
	}
        return -ENOMEM ;
    }
    
    /* pointer to rest of path after current token peeled off */
    pathnext = pathcpy ;

    /* Have to save pn->path at once */
    strcpy( pn->path, path ) ;
    pn->path_busless = pn->path + strlen(path) + 1 ;
    strcpy( pn->path_busless, path ) ;

    /* remove initial "/" */
    if ( pathnext[0]=='/' ) ++pathnext ;

    //printf("1pathnow=[%s] pathnext=[%s] pn->type=%d\n", pathnow, pathnext, pn->type);

    while(1) {
        switch( pe ) {
            case parse_done:
                //printf("PARSENAME parse_done\n") ;
                goto end ;
            case parse_error:
                //printf("PARSENAME parse_error\n") ;
                ret = -ENOENT ;
                goto end ;
            default: break ;
        }
        pathnow = strsep(&pathnext,"/") ;
        //LEVEL_DEBUG("PARSENAME pathnow=[%s] rest=[%s]\n",pathnow,pathnext) ;
        if ( pathnow==NULL || pathnow[0]== '\0' ) goto end ;
        switch( pe ) {
            case parse_first:
                //printf("PARSENAME parse_first\n") ;
                pe = Parse_Unspecified( pathnow, remote, pn ) ;
                break ;
            case parse_real:
                //printf("PARSENAME parse_real\n") ;
                pe = Parse_Real( pathnow, remote, pn ) ;
                break ;
            case parse_nonreal:
                //printf("PARSENAME parse_nonreal\n") ;
                pe = Parse_NonReal( pathnow, pn ) ;
                break ;
            case parse_prop:
                //printf("PARSENAME parse_prop\n") ;
                pathlast = pathnow ; /* Save for concatination if subdirectory later wanted */
                pe = Parse_Property( pathnow, pn ) ;
                break ;
            case parse_subprop:
                //printf("PARSENAME parse_subprop\n") ;
                pathnow[-1] = '/' ;
                pe = Parse_Property( pathlast, pn ) ;
                break ;
            default: break ;
        }
        //printf("PARSENAME pe=%d\n",pe) ;
    } 
end:
    //printf("PARSENAME end ret=%d\n",ret) ;
    free(pathcpy) ;
    if (ret) FS_ParsedName_destroy(pn) ;
    return ret ;
}

static enum parse_enum Parse_Unspecified( char * pathnow, int remote, struct parsedname * pn ) {
    if ( strcasecmp( pathnow, "alarm" )==0 ) {
        pn->state |= pn_alarm ;
        return parse_real ;
    } else if ( strncasecmp( pathnow, "bus.", 4 )==0 ) {
        return Parse_Bus( parse_first, pathnow, pn ) ;
    } else if ( strcasecmp( pathnow, "settings" )==0 ) {
        pn->type = pn_settings ;
        return parse_nonreal ;
    } else if ( strcasecmp( pathnow, "simultaneous" )==0 ) {
        pn->dev = DeviceSimultaneous ;
        return parse_prop ;
    } else if ( strcasecmp( pathnow, "statistics" )==0 ) {
        pn->type = pn_statistics ;
        return parse_nonreal ;
    } else if ( strcasecmp( pathnow, "structure" )==0 ) {
        pn->type = pn_structure ;
        return parse_nonreal ;
    } else if ( strcasecmp( pathnow, "system" )==0 ) {
        if ( SpecifiedBus(pn) ) { /* already specified a "bus." */
            /* structure only for root (remote tested remotely) */
            if ( pn->in->busmode != bus_remote ) return parse_error ;
        }
        pn->type = pn_system ;
        return parse_nonreal ;
    } else if ( strcasecmp( pathnow, "text" )==0 ) {
        pn->state |= pn_text ;
        return parse_first ;
    } else if ( strcasecmp( pathnow, "thermostat" )==0 ) {
        pn->dev = DeviceThermostat ;
        return parse_prop ;
    } else if ( strcasecmp( pathnow, "uncached" )==0 ) {
        pn->state |= pn_uncached ;
        return parse_first ;
    } else {
        return Parse_RealDevice( pathnow, remote, pn ) ;
    }
}

static enum parse_enum Parse_Real( char * pathnow, int remote, struct parsedname * pn ) {
    if ( strcasecmp( pathnow, "alarm" )==0 ) {
        pn->state |= pn_alarm ;
        return parse_real ;
    } else if ( strncasecmp( pathnow, "bus.", 4 )==0 ) {
        return Parse_Bus( parse_nonreal, pathnow, pn ) ;
    } else if ( strcasecmp( pathnow, "simultaneous" )==0 ) {
        pn->dev = DeviceSimultaneous ;
        return parse_prop ;
    } else if ( strcasecmp( pathnow, "text" )==0 ) {
        pn->state |= pn_text ;
        return parse_real ;
    } else if ( strcasecmp( pathnow, "thermostat" )==0 ) {
        pn->dev = DeviceThermostat ;
        return parse_prop ;
    } else if ( strcasecmp( pathnow, "uncached" )==0 ) {
        pn->state |= pn_uncached ;
        return parse_real ;
    } else {
        return Parse_RealDevice( pathnow, remote, pn ) ;
    }
    return parse_error ;
}

static enum parse_enum Parse_NonReal( char * pathnow, struct parsedname * pn ) {
    if ( strncasecmp( pathnow, "bus.", 4 )==0 ) {
        return Parse_Bus( parse_nonreal, pathnow, pn ) ;
    } else if ( strcasecmp( pathnow, "text" )==0 ) {
        pn->state |= pn_text ;
        return parse_nonreal ;
    } else if ( strcasecmp( pathnow, "uncached" )==0 ) {
        pn->state |= pn_uncached ;
        return parse_nonreal ;
    } else {
        return Parse_NonRealDevice( pathnow, pn ) ;
    }
    return parse_error ;
}

static enum parse_enum Parse_Bus( const enum parse_enum pe_default, char * pathnow, struct parsedname * pn ) {
    /* Processing for bus.X directories -- eventually will make this more generic */
    if(!isdigit(pathnow[4])) return 1 ;
        
    /* Should make a presence check on remote busses here, but
     * it's not a major problem if people use bad paths since
     * they will just end up with empty directory listings. */
    if ( SpecifiedBus(pn) ) { /* already specified a "bus." */
        /* too many levels of bus for a non-remote adapter */
        if ( pn->in->busmode != bus_remote ) return parse_error ;
    } else {
        char * found ;
        int length = 0 ;
        /* this will only be reached once */
        pn->state |= pn_bus ;
        pn->state |= pn_buspath ; /* specified a bus */
        pn->sg &= (~BUSRET_MASK) ;
        pn->bus_nr = atoi(&pathnow[4]);
        if ( pn->bus_nr < 0 || pn->bus_nr >= indevices ) return parse_error ;
        /* Since we are going to use a specific in-device now, set
         * pn->in to point at that device at once. */
        pn->in = find_connection_in(pn->bus_nr) ;
        /* We have to allow any bus-number here right now. We receive
         * paths like /bus.4 from a remote owserver, and we have to trust
         * this result. */
        
        if ( !(found = strstr(pn->path, "/bus.")) ) {
            length = pn->path_busless - pn->path - 1;
            strncpy( pn->path_busless, pn->path, length ) ;
        } else {
            length = found - pn->path ;
            strncpy( pn->path_busless, pn->path, length ) ;
            if ( (found = strchr( found+1, '/' )) ) {
                strcpy( &(pn->path_busless[length]), found ) ;
            } else {
                pn->path_busless[length] = '\0' ;
            }
        }
        //LEVEL_DEBUG("PARSENAME test path=%s, path_busless=%s\n",pn->path, pn->path_busless ) ;
    }
    return pe_default ;
}

/* Parse Name (only device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next segment, or NULL if not filetype
 */
static enum parse_enum Parse_RealDevice( char * filename, int remote, struct parsedname * pn ) {
    //printf("DevicePart: %s %s\n", filename, pn->path);

    if ( !isxdigit(filename[0]) || !isxdigit(filename[1]) ) {
        //printf("devicepart2: not xdigit\n");
        return parse_error ; /* starts with 2 hex digits ? */
    } else {
        BYTE ID[14] = { filename[0], filename[1], 0x00, } ;
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
                return parse_error ;
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
            if ( strncasecmp( crc, filename, 2 ) ) return parse_error ;
        }
    }
    //printf("NP2\n");
    if( !remote && !(pn->state & pn_bus) ) { /* Check the presence, and cache the proper bus number for better performance */
        int bus_nr = -1;
        if(Cache_Get_Device(&bus_nr, pn)) {
            //printf("PN Cache_Get_Device didn't find bus_nr\n");
            bus_nr = CheckPresence(pn);
            if(bus_nr >= 0) {
                //printf("PN CheckPresence(%s) found bus_nr %d (add to cache)\n", pn->path, bus_nr);
                Cache_Add_Device(bus_nr, pn);
            } else {
                return parse_error ; /* CheckPresence failed */
            }
        } else {
            //printf("PN Cache_Get_Device found bus! %d\n", bus_nr);
        }
        /* fake that we read from only one indevice now! */
        pn->in = find_connection_in(bus_nr);
        pn->state |= pn_bus ;
        pn->bus_nr = bus_nr ;
    }
    return parse_prop ;
}

/* Parse Name (non-device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
*/
static enum parse_enum Parse_NonRealDevice( char * filename, struct parsedname * pn ) {   
    //printf("Parse_NonRealDevice: [%s] [%s]\n", filename, pn->path);
    FS_devicefind( filename, pn ) ;
    return ( pn->dev == &NoDevice ) ? parse_error : parse_prop ;
}

static enum parse_enum Parse_Property( char * filename, struct parsedname * pn ) {
    char * dot = filename ;

    //printf("FilePart: %s %s\n", filename, pn->path);

    filename = strsep(&dot,".") ;
    //printf("FP name=%s, dot=%s\n", filename, dot);
    /* Match to known filetypes for this device */
    if ( (pn->ft = bsearch( filename , pn->dev->ft , (size_t) pn->dev->nft , sizeof(struct filetype) , filecmp )) ) {
        //printf("FP known filetype %s\n",pn->ft->name) ;
        /* Filetype found, now process extension */
        if ( dot==NULL  || dot[0]=='\0' ) { /* no extension */
            if ( pn->ft->ag ) return parse_error ; /* aggregate filetypes need an extension */
            pn->extension = 0 ; /* default when no aggregate */
        } else if ( pn->ft->ag == NULL ) {
            return parse_error ; /* An extension not allowed when non-aggregate */
        } else if ( strcasecmp(dot,"ALL")==0 ) {
            //printf("FP ALL\n");
            pn->extension = -1 ; /* ALL */
        } else if ( pn->ft->format==ft_bitfield && strcasecmp(dot,"BYTE")==0 ) {
            pn->extension = -2 ; /* BYTE */
            //printf("FP BYTE\n") ;
        } else {
            if ( pn->ft->ag->letters == ag_letters ) {
                //printf("FP letters\n") ;
                if ( (strlen(dot)!=1) || !isupper(dot[0]) ) return parse_error ;
                pn->extension = dot[0] - 'A' ; /* Letter extension */
            } else { /* Numbers */
                char * p ;
                //printf("FP numbers\n") ;
                pn->extension = strtol(dot,&p,0) ; /* Number extension */
                if ( (p==dot) || ((pn->extension == 0) && (errno==-EINVAL)) ) return parse_error ; /* Bad number */
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
                    return parse_error ; /* Extension out of range */
                }
                //printf("FP in range\n") ;
        }
        //printf("FP Good\n") ;
        switch( pn->ft->format ) {
            case ft_directory:
                if ( BranchAdd(pn) ) {
                    //printf("PN BranchAdd failed for %s\n", pn->path);
                    return parse_error ;
                }
                /* STATISTICS */
                STATLOCK ;
                if ( pn->pathlength > dir_depth ) dir_depth = pn->pathlength ;
                STATUNLOCK ;
                return parse_real ;
            case ft_subdir:
                pn->subdir = pn->ft ;
                pn->ft = NULL ;
                return parse_subprop ;
            default:
                return parse_done ;
        }
    }
    //printf("FP not found\n") ;
    return parse_error ; /* filetype not found */
}

static int BranchAdd( struct parsedname * pn ) {
    //printf("BRANCHADD\n");
    if ( (pn->pathlength%BRANCH_INCR)==0 ) {
        void * temp = pn->bp ;
        if ( (pn->bp=realloc( temp, (BRANCH_INCR+pn->pathlength) * sizeof( struct buspath ) ))==NULL ) {
            if (temp) free(temp) ;
            return -ENOMEM ;
        }
    }
    memcpy( pn->bp[pn->pathlength].sn, pn->sn , 8 ) ; /* copy over DS2409 name */
    pn->bp[pn->pathlength].branch = pn->ft->data.i ;
    ++pn->pathlength ;
    pn->ft = NULL ;
    pn->dev = NULL ;
    return 0 ;
}

static int filecmp(const void * name , const void * ex ) {
    return strcmp( (const char *) name , ((const struct filetype *) ex)->name ) ;
}

/* Parse a path/file combination */
int FS_ParsedNamePlus( const char * path, const char * file, struct parsedname * pn ) {
    if ( path==NULL || path[0]=='\0' ) {
        return FS_ParsedName( file, pn ) ;
    } else if ( file==NULL || file[0]=='\0' ) {
        return FS_ParsedName( path, pn ) ;
    } else {
        int ret = 0 ; 
        char * fullpath ;
        fullpath = malloc( strlen(file) + strlen(path) + 2 ) ;
        if ( fullpath == NULL ) return -ENOMEM ;
        strcpy( fullpath, path ) ;
        if ( fullpath[strlen(fullpath)-1] != '/' ) strcat( fullpath, "/" ) ;
        strcat( fullpath, file ) ;
        //printf("PARSENAMEPLUS path=%s pre\n",fullpath) ;
        ret = FS_ParsedName( fullpath, pn ) ;
        //printf("PARSENAMEPLUS path=%s post\n",fullpath) ;
        free( fullpath ) ;
        //printf("PARSENAMEPLUS free\n") ;
        return ret ;
    }
}
