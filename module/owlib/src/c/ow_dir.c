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

static int FS_dir_seek( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn, uint32_t * flags ) ;
static int FS_branchoff( const struct parsedname * const pn) ;
static int FS_devdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_alarmdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_typedir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_realdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2, uint32_t * flags  ) ;
static int FS_cache2real( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2, uint32_t * flags  ) ;

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
int FS_dir( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn ) {
    int ret = 0 ;
    struct parsedname pn2 ;
    uint32_t flags = 0 ;

    if ( pn->in==NULL ) return -ENODEV ;
    STATLOCK
        AVERAGE_IN(&dir_avg)
        AVERAGE_IN(&all_avg)
    STATUNLOCK
    FSTATLOCK
        dir_time = time(NULL) ; // protected by mutex
    FSTATUNLOCK
    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */
    pn2.badcopy = 1 ;
//printf("DIR1 path=%s\n",path);
    if ( pn == NULL ) {
        ret = -ENOENT ; /* should never happen */
    } else if ( pn->dev ){ /* device directory */
        ret = FS_devdir( dirfunc, &pn2 ) ;
    } else if ( pn->state & pn_alarm ) {  /* root or branch directory -- alarm state */
        ret = FS_dir_seek( dirfunc, &pn2, &flags ) ;
    } else if ( pn->type != pn_real ) {  /* stat, sys or set dir */
        ret = FS_typedir( dirfunc, &pn2 ) ;
    } else if ( pn->state & pn_uncached ) {  /* root or branch directory -- uncached */
        ret = FS_dir_seek( dirfunc, &pn2, &flags ) ;
    } else {
        /* Show uncached and stats... (if root directory) */
        if ( pn2.pathlength == 0 ) { /* true root */
            /* restore state */
            pn2.state = pn->state ;
            if ( IsLocalCacheEnabled(pn) ) { /* cached */
                pn2.state = (pn_uncached | (pn->state & pn_text)) ;
                dirfunc( &pn2 ) ;
                /* restore state */
                pn2.state = pn->state ;
            }
            pn2.type = pn_settings ;
            dirfunc( &pn2 ) ;
            pn2.type = pn_system ;
            dirfunc( &pn2 ) ;
            pn2.type = pn_statistics ;
            dirfunc( &pn2 ) ;
            pn2.type = pn_structure ;
            dirfunc( &pn2 ) ;
            pn2.type = pn_real ;
        }
        ret = FS_dir_seek( dirfunc, &pn2, &flags ) ;
    }
    /* alarm directory */
    if ( flags & DEV_alarm ) {
        pn2.state = (pn_alarm | (pn->state & pn_text)) ;
        dirfunc( &pn2 ) ;
        pn2.state = pn->state ;
    }
    /* simultaneous directory */
    if ( flags & (DEV_temp|DEV_volt) ) {
        pn2.dev = DeviceSimultaneous ;
        dirfunc( &pn2 ) ;
    }
    STATLOCK
        AVERAGE_OUT(&dir_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK
//printf("DIR out\n");
    return ret ;
}

/* path is the path which "pn" parses */
/* FS_dir_remote is the entry into FS_sir_seek from ServerDir */
/* More checking is done, and the flags are returned */
int FS_dir_remote( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn, uint32_t * flags ) {
    int ret = 0 ;

    /* initialize flags */
    flags[0] = 0 ;
    if ( pn->in==NULL ) return -ENODEV ;
    STATLOCK
        AVERAGE_IN(&dir_avg)
        AVERAGE_IN(&all_avg)
    STATUNLOCK
    FSTATLOCK
        dir_time = time(NULL) ; // protected by mutex
    FSTATUNLOCK
//printf("DIR1 path=%s\n",path);
    if ( pn == NULL ) {
    } else if ( pn->dev ){ /* device directory */
    } else if ( pn->state & pn_alarm ) {  /* root or branch directory -- alarm state */
        ret = FS_dir_seek( dirfunc, pn, flags ) ;
    } else if ( pn->type != pn_real ) {  /* stat, sys or set dir */
    } else {
        ret = FS_dir_seek( dirfunc, pn, flags ) ;
    }
    STATLOCK
        AVERAGE_OUT(&dir_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK
//printf("DIR out\n");
    return ret ;
}

/* path is the path which "pn" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
static int FS_dir_seek( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn, uint32_t * flags ) {
    int ret = 0 ;
#ifdef OW_MT
    pthread_t thread ;
    /* Embedded function */
    void * Dir2( void * vp ) {
        struct parsedname pnnext ;
        /* we need a different state (search state) for a different bus -- subtle error */
        struct stateinfo si ;
        (void) vp ;
        memcpy( &pnnext, pn , sizeof(struct parsedname) ) ;
        pnnext.si = &si ;
        pnnext.in = pn->in->next ;
        return (void *) FS_dir_seek(dirfunc,&pnnext,flags) ;
    }
    int threadbad = pn->in->next==NULL || pthread_create( &thread, NULL, Dir2, NULL ) ;
#endif /* OW_MT */

//printf("DIRseek adapter=%d path=%s\n",pn->in->index,pn->path);

    if ( pn->in->busmode == bus_remote ) {
        ret = ServerDir(dirfunc,pn,flags) ;
    } else {
        struct parsedname pncopy ;
        /* Make a copy (shallow) of pn to modify for directory entries */
        memcpy( &pncopy, pn , sizeof( struct parsedname ) ) ; /*shallow copy */
        pncopy.badcopy = 1 ;
        if ( pn->state & pn_alarm ) {  /* root or branch directory -- alarm state */
            ret = FS_alarmdir(dirfunc,&pncopy) ;
        } else {
            if ( (pn->state&pn_uncached) || !IsLocalCacheEnabled(pn) || timeout.dir==0 ) {
                ret = FS_realdir( dirfunc, &pncopy, flags ) ;
            } else {
                ret = FS_cache2real( dirfunc, &pncopy, flags ) ;
            }
        }
    }
#ifdef OW_MT
    if ( threadbad == 0 ) { /* was a thread created? */
        void * v ;
        int rt ;
        if ( pthread_join( thread, &v ) ) return ret ; /* wait for it (or return only this result) */
        rt = (int) v ;
        if ( rt >= 0 ) return rt ; /* is it an error return? Then return this one */
    }
#endif /* OW_MT */
    return ret ;
}

/* Device directory -- all from memory */
static int FS_devdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) {
    struct filetype * lastft = & (pn2->dev->ft[pn2->dev->nft]) ; /* last filetype struct */
    struct filetype * firstft ; /* first filetype struct */
    char s[33] ;
    size_t len ;

    STATLOCK
        ++dir_dev.calls ;
    STATUNLOCK
    if ( pn2->subdir ) { /* indevice subdir, name prepends */
//printf("DIR device subdirectory\n");
        strcpy( s , pn2->subdir->name ) ;
        strcat( s , "/" ) ;
        len = strlen(s) ;
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
                STATLOCK
                    ++dir_dev.entries ;
                STATUNLOCK
            }
        } else {
            pn2->extension = 0 ;
            dirfunc( pn2 ) ;
            STATLOCK
                ++dir_dev.entries ;
            STATUNLOCK
        }
    }
    return 0 ;
}

/* Note -- alarm directory is smaller, no adapters or stats or uncached */
static int FS_alarmdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) {
    int ret ;
    unsigned char sn[8] ;

    /* STATISCTICS */
    STATLOCK
        ++dir_main.calls ;
    STATUNLOCK
//printf("DIR alarm directory\n");

    pn2->ft = NULL ; /* just in case not properly set */
    BUSLOCK(pn2)
    /* Turn off all DS2409s */
    FS_branchoff(pn2) ;
    (ret=BUS_select(pn2)) || (ret=BUS_first_alarm(sn,pn2)) ;
    while (ret==0) {
        char ID[] = "XX";
        STATLOCK
            ++dir_main.entries ;
        STATUNLOCK
        num2string( ID, sn[0] ) ;
        memcpy( pn2->sn, sn, 8 ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefind( ID, pn2 ) ;
        DIRLOCK
            dirfunc( pn2 ) ;
        DIRUNLOCK
        pn2->dev = NULL ; /* clear for the rest of directory listing */
        (ret=BUS_select(pn2)) || (ret=BUS_next_alarm(sn,pn2)) ;
//printf("ALARM sn: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X ret=%d\n",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7],ret);
    }
    BUSUNLOCK(pn2)
    return ret ;
}

static int FS_branchoff( const struct parsedname * const pn ) {
    int ret ;
    unsigned char cmd[] = { 0xCC, 0x66, } ;

    /* Turn off all DS2409s */
    if ( (ret=BUS_reset(pn)) || (ret=BUS_send_data(cmd,2,pn)) ) return ret ;
    return ret ;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static int FS_realdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2, uint32_t * flags ) {
    unsigned char sn[8] ;
    int dindex = 0 ;
    int ret ;

    /* STATISCTICS */
    STATLOCK
        ++dir_main.calls ;
    STATUNLOCK

    flags[0] = 0 ; /* start out with no flags set */

    BUSLOCK(pn2)
        /* Operate at dev level, not filetype */
        pn2->ft = NULL ;
        /* Turn off all DS2409s */
        FS_branchoff(pn2) ;
        /* it appears that plugging in a new device sends a "presence pulse" that screws up BUS_first */
        /* Actually it's probably stale information in the stateinfo structure */
        (ret=BUS_select(pn2)) || (ret=BUS_first(sn,pn2)) ;
        while (ret==0) {
            char ID[] = "XX";
    //        FS_LoadPath( pn2 ) ;
            Cache_Add_Dir(sn,dindex,pn2) ;
            ++dindex ;
            num2string( ID, sn[0] ) ;
            memcpy( pn2->sn, sn, 8 ) ;
            /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
            FS_devicefind( ID, pn2 ) ;
//printf("DIR adapter=%d, element=%d, sn=%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",pn2->in->index,dindex,pn2->sn[0],pn2->sn[1],pn2->sn[2],pn2->sn[3],pn2->sn[4],pn2->sn[5],pn2->sn[6],pn2->sn[7]);
            DIRLOCK
                dirfunc( pn2 ) ;
                flags[0] |= pn2->dev->flags ;
            DIRUNLOCK
            pn2->dev = NULL ; /* clear for the rest of directory listing */
            (ret=BUS_select(pn2)) || (ret=BUS_next(sn,pn2)) ;
        }
    BUSUNLOCK(pn2)
    STATLOCK
        dir_main.entries += dindex ;
    STATUNLOCK
    Cache_Del_Dir(dindex,pn2) ;
    return 0 ;
}

void FS_LoadPath( unsigned char * sn, const struct parsedname * const pn ) {
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
static int FS_cache2real( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2, uint32_t * flags ) {
    unsigned char sn[8] ;
    int dindex = 0 ;

    /* Test to see whether we should get the directory "directly" */
    if ( (pn2->state & pn_uncached) || Cache_Get_Dir(sn,0,pn2 ) )
        return FS_realdir(dirfunc,pn2,flags) ;

    /* STATISCTICS */
    STATLOCK
        ++dir_main.calls ;
    STATUNLOCK

    /* Get directory from the cache */
    do {
        char ID[] = "XX";
        num2string( ID, sn[0] ) ;
        memcpy( pn2->sn, sn, 8 ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefind( ID, pn2 ) ;
        DIRLOCK
            dirfunc( pn2 ) ;
            flags[0] |= pn2->dev->flags ;
        DIRUNLOCK
        pn2->dev = NULL ; /* clear for the rest of directory listing */
//        ++dindex ;
    } while ( Cache_Get_Dir( sn, ++dindex, pn2 )==0 ) ;
    STATLOCK
        dir_main.entries += dindex ;
    STATUNLOCK
    return 0 ;
}

/* Show the pn->type (statistics, system, ...) entries */
/* Only the top levels, the rest will be shown by FS_devdir */
static int FS_typedir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) {
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
