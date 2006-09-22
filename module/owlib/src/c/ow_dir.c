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
#include "ow_connection.h"
#include "ow_dirblob.h"

static int FS_dir_both( void (* dirfunc)(const struct parsedname *), const struct parsedname * pn, uint32_t * flags ) ;
static int FS_dir_seek( void (* dirfunc)(const struct parsedname * const), struct connection_in * in, const struct parsedname * pn, uint32_t * flags ) ;
static int FS_devdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * pn2 ) ;
static int FS_alarmdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * pn2 ) ;
static int FS_typedir( void (* dirfunc)(const struct parsedname * const), struct parsedname * pn2 ) ;
static int FS_realdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * pn2, uint32_t * flags ) ;
static int FS_cache2real( void (* dirfunc)(const struct parsedname * const), struct parsedname * pn2, uint32_t * flags  ) ;
static int FS_busdir( void (* dirfunc)(const struct parsedname *), const struct parsedname * pn ) ;

/* Calls dirfunc() for each element in directory */
/* void * data is arbitrary user data passed along -- e.g. output file descriptor */
/* pn -- input:
    pn->dev == NULL -- root directory, give list of all devices
    pn->dev non-null, -- device directory, give all properties
    branch aware
    cache aware

   pn -- output (with each call to dirfunc)
    ROOT
    pn->dev set
    pn->sn set appropriately
    pn->ft not set

    DEVICE
    pn->dev and pn->sn still set
    pn->ft loops through
*/
/* path is the path which "pn" parses */
/* FS_dir produces the "invariant" portion of the directory, passing on to
   FS_dir_seek the variable part */
int FS_dir( void (* dirfunc)(const struct parsedname *), const struct parsedname * pn ) {
    uint32_t flags ;

    return FS_dir_both( dirfunc, pn, &flags ) ;
}

/* path is the path which "pn" parses */
/* FS_dir_remote is the entry into FS_dir_seek from ServerDir */
/* More checking is done, and the flags are returned */
int FS_dir_remote( void (* dirfunc)(const struct parsedname *), const struct parsedname * pn, uint32_t * flags ) {
    return FS_dir_both( dirfunc, pn, flags ) ;
}

static int FS_dir_both( void (* dirfunc)(const struct parsedname *), const struct parsedname * pn, uint32_t * flags ) {
    int ret = 0 ;
    struct parsedname pn2 ;

    /* initialize flags */
    flags[0] = 0 ;
    if ( pn == NULL || pn->in==NULL ) return -ENODEV ;
    LEVEL_CALL("DIRECTORY path=%s\n",SAFESTRING(pn->path)) ;
    
    STATLOCK;
        AVERAGE_IN(&dir_avg)
        AVERAGE_IN(&all_avg)
    STATUNLOCK;
    FSTATLOCK;
        dir_time = time(NULL) ; // protected by mutex
    FSTATUNLOCK;

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */

    if ( pn->ft ) {
        ret = -ENOTDIR ;
    } else if ( pn->dev ) { /* device directory */
        if ( pn->type == pn_structure ) {
        /* Device structure is always known for ordinary devices, so don't
        * bother calling ServerDir() */
            ret = FS_devdir( dirfunc, &pn2 ) ;
        } else if ( SpecifiedBus(pn) && (is_servermode(pn->in)) ) {
            ret = ServerDir(dirfunc, &pn2, flags) ;
        } else {
            ret = FS_devdir( dirfunc, &pn2 ) ;
        }
    } else if ( IsAlarmDir(pn) ) {  /* root or branch directory -- alarm state */
        //printf("ALARM\n");
        ret = SpecifiedBus(pn) ? FS_alarmdir(dirfunc, &pn2) : FS_dir_seek( dirfunc, pn2.in, &pn2, flags ) ;
    } else if ( NotRealDir(pn) ) {  /* stat, sys or set dir */
        /* there are some files with variable sizes, and /system/adapter have variable
        * number of entries and we have to call ServerDir() */
        ret = (SpecifiedBus(pn) && (is_servermode(pn->in)))
                ? ServerDir(dirfunc, &pn2, flags)
                : FS_typedir( dirfunc, &pn2 ) ;
    } else { /* Directory of some kind */
        if ( pn->pathlength == 0 ) { /* root directory */
            if ( (Global.opt!=opt_server) && !SpecifiedBus(pn) && NotUncachedDir(pn) ) { /* structure only in true root */
                pn2.type = pn_structure ;
                dirfunc( &pn2 ) ;
                pn2.type = pn_real ;
            }
            if( !SpecifiedBus(pn) && ShouldReturnBusList(pn) ) {
                /* restore state */
                pn2.type = pn_real ;
                FS_busdir(dirfunc, pn ) ;
                if ( NotUncachedDir(pn) ) {
                    if ( IsLocalCacheEnabled(pn) ) { /* cached */
                        pn2.state = pn->state | pn_uncached ;
                        dirfunc( &pn2 ) ;
                        pn2.state = pn->state ;
                    }
                    pn2.type = pn_settings ;
                    dirfunc( &pn2 ) ;
                    pn2.type = pn_system ;
                    dirfunc( &pn2 ) ;
                    pn2.type = pn_statistics ;
                    dirfunc( &pn2 ) ;
                }
                pn2.type = pn->type;
            }
        }
        /* Now get the actual devices */
        ret = SpecifiedBus(pn) ? FS_cache2real(dirfunc, &pn2, flags) : FS_dir_seek( dirfunc, pn2.in, &pn2, flags ) ;
    }
    if ( Global.opt!=opt_server ) {
        if( NotAlarmDir(pn) ) {
            /* don't show alarm directory in alarm directory */
            /* alarm directory */
            if ( flags[0] & DEV_alarm ) {
                pn2.state = (pn_alarm | (pn->state & pn_text)) ;
                dirfunc( &pn2 ) ;
                pn2.state = pn->state ;
            }
        }
        /* simultaneous directory */
        if ( flags[0] & (DEV_temp|DEV_volt) ) {
            pn2.dev = DeviceSimultaneous ;
            dirfunc( &pn2 ) ;
        }
    }
    STATLOCK;
        AVERAGE_OUT(&dir_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK;
    //printf("FS_dir out ret=%d\n", ret);
    return ret ;
}

/* path is the path which "pn" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
#if OW_MT
struct dir_seek_struct {
    struct connection_in * in ;
    const struct parsedname * pn ;
    void (* dirfunc)(const struct parsedname *) ;
    uint32_t * flags ;
    int ret ;
} ;

/* Embedded function */
static void * FS_dir_seek_callback( void * vp ) {
    struct dir_seek_struct * dss = (struct dir_seek_struct *)vp ;
    dss->ret = FS_dir_seek(dss->dirfunc,dss->in,dss->pn,dss->flags) ;
    pthread_exit(NULL);
    return NULL;
}

static int FS_dir_seek( void (* dirfunc)(const struct parsedname *), struct connection_in * in, const struct parsedname * pn, uint32_t * flags ) {
    int ret = 0 ;
    struct dir_seek_struct dss = {in->next,pn,dirfunc,flags,0} ;
    struct parsedname pn2 ;
    pthread_t thread ;
    int threadbad = 1;

    if(!(pn->state & pn_bus)) {
        threadbad = in->next==NULL || pthread_create( &thread, NULL, FS_dir_seek_callback, (void *)(&dss) ) ;
    }
    
    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    pn2.in = in ;
    
    if ( TestConnection(&pn2) ) { // reconnect ok?
    ret = -ECONNABORTED ;
    } else if ( (pn->state & pn_bus) && (is_servermode(in)) ) { /* is this a remote bus? */
        //printf("FS_dir_seek: Call ServerDir %s\n", pn->path);
        ret = ServerDir(dirfunc,&pn2,flags) ;
    } else { /* local bus */
        if ( IsAlarmDir(pn) ) {  /* root or branch directory -- alarm state */
            //printf("FS_dir_seek: Call FS_alarmdir %s\n", pn->path);
            ret = FS_alarmdir(dirfunc,&pn2) ;
        } else {
            //printf("FS_dir_seek: call FS_cache2real bus %d\n", pn->in->index);
            ret = FS_cache2real( dirfunc, &pn2, flags ) ;
        }
    }
    //printf("FS_dir_seek4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn->in->index,ret);
    /* See if next bus was also queried */
    if ( threadbad == 0 ) { /* was a thread created? */
        void * v ;
        if ( pthread_join( thread, &v ) ) return ret ; /* wait for it (or return only this result) */
        if ( dss.ret >= 0 ) return dss.ret ; /* is it an error return? Then return this one */
    }
    return ret ;
}

#else /* OW_MT */

/* path is the path which "pn" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
static int FS_dir_seek( void (* dirfunc)(const struct parsedname *), struct connection_in * in, const struct parsedname * pn, uint32_t * flags ) {
    int ret = 0 ;
    struct parsedname pn2 ;

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; //shallow copy
    pn2.in = in ;
    if ( TestConnection(&pn2) ) { // reconnect ok?
    ret = -ECONNABORTED ;
    } else if ( (pn->state & pn_bus) && (is_servermode(in)) ) { /* is this a remote bus? */
        //printf("FS_dir_seek: Call ServerDir %s\n", pn->path);
        ret = ServerDir(dirfunc,&pn2,flags) ;
    } else { /* local bus */
        if ( IsAlarmDir(&pn2) ) {  /* root or branch directory -- alarm state */
            //printf("FS_dir_seek: Call FS_alarmdir %s\n", pn->path);
            ret = FS_alarmdir(dirfunc,&pn2) ;
        } else {
            //printf("FS_dir_seek: call FS_cache2real bus %d\n", pn->in->index);
            ret = FS_cache2real( dirfunc, &pn2, flags ) ;
        }
    }
    //printf("FS_dir_seek4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn->in->index,ret);
    if ( in->next && ret<=0 ) return FS_dir_seek( dirfunc, in->next, pn, flags ) ;
    return ret ;
}
#endif /* OW_MT */

/* Device directory -- all from memory */
static int FS_devdir( void (* dirfunc)(const struct parsedname *), struct parsedname * pn2 ) {
    struct filetype * lastft = & (pn2->dev->ft[pn2->dev->nft]) ; /* last filetype struct */
    struct filetype * firstft ; /* first filetype struct */
    char s[33] ;
    size_t len ;

    STAT_ADD1(dir_dev.calls);
    if ( pn2->subdir ) { /* indevice subdir, name prepends */
//printf("DIR device subdirectory\n");
        len = snprintf(s, 32, "%s/", pn2->subdir->name );
        firstft = pn2->subdir  + 1 ;
    } else {
//printf("DIR device directory\n");
        len = 0 ;
        firstft = pn2->dev->ft ;
    }
    for ( pn2->ft=firstft ; pn2->ft < lastft ; ++pn2->ft ) { /* loop through filetypes */
        if ( len ) { /* subdir */
            /* test start of name for directory name */
            if ( strncmp( pn2->ft->name , s , len ) ) break ;
        } else { /* primary device directory */
            if ( strchr( pn2->ft->name, '/' ) ) continue ;
        }
        if ( pn2->ft->ag ) {
            for ( pn2->extension=(pn2->ft->format==ft_bitfield)?-2:-1 ; pn2->extension < pn2->ft->ag->elements ; ++pn2->extension ) {
                dirfunc( pn2 ) ;
                STAT_ADD1(dir_dev.entries);
            }
        } else {
            pn2->extension = 0 ;
            dirfunc( pn2 ) ;
            STAT_ADD1(dir_dev.entries);
        }
    }
    return 0 ;
}

/* Note -- alarm directory is smaller, no adapters or stats or uncached */
static int FS_alarmdir( void (* dirfunc)(const struct parsedname *), struct parsedname * pn2 ) {
    int ret ;
    UINT flags ;
    struct device_search ds ; // holds search state

    /* cache from Server if this is a remote bus */
    if ( is_servermode(pn2->in) ) return ServerDir( dirfunc, pn2, &flags ) ;

    /* STATISCTICS */
    STAT_ADD1(dir_main.calls);
//printf("DIR alarm directory\n");

    BUSLOCK(pn2);
    pn2->ft = NULL ; /* just in case not properly set */
    ret = BUS_first_alarm(&ds,pn2) ;
    if(ret) {
        BUSUNLOCK(pn2);
        if(ret == -ENODEV) return 0; /* no more alarms is ok */
        return ret ;
    }
    while (ret==0) {
        STAT_ADD1(dir_main.entries);
        memcpy( pn2->sn, ds.sn, 8 ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefindhex( ds.sn[0], pn2 ) ;  // lookup ID and set pn2.dev
        DIRLOCK;
            dirfunc( pn2 ) ;
        DIRUNLOCK;
        pn2->dev = NULL ; /* clear for the rest of directory listing */
        ret = BUS_next(&ds,pn2) ;
//printf("ALARM sn: "SNformat" ret=%d\n",SNvar(sn),ret);
    }
    BUSUNLOCK(pn2);
    if(ret == -ENODEV) return 0; /* no more alarms is ok */
    return ret ;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static int FS_realdir( void (* dirfunc)(const struct parsedname *), struct parsedname * pn2, uint32_t * flags ) {
    struct device_search ds ;
    size_t devices = 0 ;
    struct dirblob db ;
    int ret ;

    /* cache from Server if this is a remote bus */
    if ( is_servermode(pn2->in) ) return ServerDir( dirfunc, pn2, flags ) ;

    /* STATISTICS */
    STAT_ADD1(dir_main.calls);

    DirblobInit( &db ) ; // set up a fresh dirblob
    
    /* Operate at dev level, not filetype */
    pn2->ft = NULL ;
    flags[0] = 0 ; /* start out with no flags set */

    BUSLOCK(pn2);
    
    /* it appears that plugging in a new device sends a "presence pulse" that screws up BUS_first */
    if( (ret=BUS_first(&ds,pn2)) ) {
        BUSUNLOCK(pn2);
        if(ret == -ENODEV) {
            if ( pn2->pathlength == 0 ) pn2->in->last_root_devs = 0 ; // root dir estimated length
            return 0; /* no more devices is ok */
        }
        return -EIO ;
    }
    /* BUS still locked */
    if ( pn2->pathlength == 0 ) db.allocated = pn2->in->last_root_devs ; // root dir estimated length
    do {
        BUSUNLOCK(pn2);
        if ( DirblobPure( &db ) ) { /* only add if there is a blob allocated successfully */
            DirblobAdd( ds.sn, &db ) ;
        }
        ++devices ;
        
        memcpy( pn2->sn, ds.sn, 8 ) ;
        /* Add to Device location cache */
        Cache_Add_Device(pn2->in->index, pn2 ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefindhex( ds.sn[0], pn2 ) ;  // lookup ID and set pn2.dev
        
        DIRLOCK;
            dirfunc( pn2 ) ;
            flags[0] |= pn2->dev->flags ;
        DIRUNLOCK;
        pn2->dev = NULL ; /* clear for the rest of directory listing */

        BUSLOCK(pn2);
    } while ( (ret=BUS_next(&ds,pn2))==0 ) ;
    /* BUS still locked */
    if ( pn2->pathlength==0 && ret==-ENODEV ) pn2->in->last_root_devs = devices ; // root dir estimated length
    BUSUNLOCK(pn2);

    /* Add to the cache (full list as a single element */
    if ( DirblobPure(&db) && ret==-ENODEV ) {
        Cache_Add_Dir(&db,pn2) ;  // end with a null entry
    }
    DirblobClear(&db) ;

    STATLOCK;
        dir_main.entries += devices ;
    STATUNLOCK;
    if(ret == -ENODEV) return 0 ; // no more devices is ok */
    return ret ;
}

void FS_LoadPath( BYTE * sn, const struct parsedname * pn ) {
    if ( pn->pathlength==0 ) {
        memset(sn,0,8) ;
    } else {
        memcpy( sn,pn->bp[pn->pathlength-1].sn,7) ;
        sn[7] = pn->bp[pn->pathlength-1].branch ;
    }
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Cache2Real try the cache first, else can directory from bus (and add to cache) */
static int FS_cache2real( void (* dirfunc)(const struct parsedname *), struct parsedname * pn2, uint32_t * flags ) {
    size_t dindex ;
    struct dirblob db ;

    /* Test to see whether we should get the directory "directly" */
    //printf("Pre test cache for dir\n") ;
    if ( SpecifiedBus(pn2) || IsUncachedDir(pn2) || Cache_Get_Dir(&db,pn2 ) ) {
        //printf("FS_cache2real: didn't find anything at bus %d\n", pn2->in->index);
        return FS_realdir(dirfunc,pn2,flags) ;
    }
    //printf("Post test cache for dir, snlist=%p, devices=%lu\n",snlist,devices) ;
    /* We have a cached list in snlist. Note that we have to free this memory */
    /* STATISCTICS */
    STAT_ADD1(dir_main.calls);

    /* Get directory from the cache */
    for ( dindex = 0 ; DirblobGet(dindex,pn2->sn,&db)==0 ; ++dindex )  {
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefindhex( pn2->sn[0], pn2 ) ;  // lookup ID and set pn2.dev
        DIRLOCK;
            dirfunc( pn2 ) ;
            flags[0] |= pn2->dev->flags ;
        DIRUNLOCK;
    }
    DirblobClear(&db) ; /* allocated in Cache_Get_Dir */
    pn2->dev = NULL ; /* clear for the rest of directory listing */
    STATLOCK;
        dir_main.entries += dindex ;
    STATUNLOCK;
    return 0 ;
}

/* Show the pn->type (statistics, system, ...) entries */
/* Only the top levels, the rest will be shown by FS_devdir */
static int FS_typedir( void (* dirfunc)(const struct parsedname *), struct parsedname * pn2 ) {
    void action( const void * t, const VISIT which, const int depth ) {
        (void) depth ;
//printf("Action\n") ;
        switch(which) {
        case leaf:
        case postorder:
            pn2->dev = ((const struct device_opaque *)t)->key ;
            dirfunc( pn2 ) ;
        default:
            break ;
        }
    } ;
    twalk( Tree[pn2->type],action) ;
    pn2->dev = NULL ;
    return 0 ;
}

/* Show the bus entries */
/* No reason to lock or use a copy */
static int FS_busdir( void (* dirfunc)(const struct parsedname *), const struct parsedname * pn ) {
    struct parsedname pn2 ;

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    pn2.state = pn_bus ;
    
    for( pn2.in = indevice ; pn2.in ; pn2.in = pn2.in->next ) {
        pn2.bus_nr = pn2.in->index ;
        dirfunc( &pn2 ) ;
    }
    
    return 0 ;
}
