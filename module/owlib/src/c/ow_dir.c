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

static int FS_branchoff( const struct parsedname * const pn) ;
static int FS_devdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_alarmdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_typedir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_realdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;
static int FS_cache2real( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) ;

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
int FS_dir( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn ) {
    int ret = 0 ;
    struct parsedname pn2 ;

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
        ret = ( busmode == bus_remote ) ? ServerDir( dirfunc, pn ) : FS_alarmdir( dirfunc, &pn2 ) ;
    } else if ( pn->type != pn_real ) {  /* stat, sys or set dir */
        ret = FS_typedir( dirfunc, &pn2 ) ;
    } else if ( busmode == bus_remote ) {
//printf("DIR: to bus\n");
        ret = ServerDir( dirfunc, pn ) ;
    } else {
        pn2.state = (pn_alarm | (pn->state & pn_text)) ;
        dirfunc( &pn2 ) ;
        if ( pn->state & pn_uncached ) {
            ret = FS_realdir( dirfunc, &pn2 ) ;
        } else {  /* root or branch directory -- non-alarm */
            /* Show uncached and stats... (if root directory) */
            if ( pn2.pathlength == 0 ) { /* true root */
                pn2.state = pn->state ;
                if ( IsCacheEnabled(pn) ) { /* cached */
                    pn2.state = (pn_uncached | (pn->state & pn_text)) ;
                    dirfunc( &pn2 ) ;
                    pn2.state = (pn_normal | (pn->state & pn_text)) ;
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

            /* Show all devices in this directory */
            if ( IsCacheEnabled(pn) && timeout.dir ) {
                FS_cache2real( dirfunc, &pn2 ) ;
            } else {
                FS_realdir( dirfunc, &pn2 ) ;
            }
        }
    }
    STATLOCK
        AVERAGE_OUT(&dir_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK
//printf("DIR out\n");
    return ret ;
}

/* Device directory -- all from memory */
static int FS_devdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) {
    struct filetype * lastft = & (pn2->dev->ft[pn2->dev->nft]) ; /* last filetype struct */
    struct filetype * firstft ; /* first filetype struct */
    char s[33] ;
    int len ;

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
    BUSLOCK
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
        dirfunc( pn2 ) ;
        pn2->dev = NULL ; /* clear for the rest of directory listing */
        (ret=BUS_select(pn2)) || (ret=BUS_next_alarm(sn,pn2)) ;
//printf("ALARM sn: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X ret=%d\n",sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7],ret); 
    }
    BUSUNLOCK
    return ret ;
}

static int FS_branchoff( const struct parsedname * const pn ) {
    int ret ;
    unsigned char cmd[] = { 0xCC, 0x66, } ;

    /* Turn off all DS2409s */
    if ( (ret=BUS_reset(pn)) || (ret=BUS_send_data(cmd,2)) ) return ret ;
    return ret ;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static int FS_realdir( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) {
    unsigned char sn[8] ;
    int dindex = 0 ;
    int simul = 0 ;
    int ret ;

    /* STATISCTICS */
    STATLOCK
        ++dir_main.calls ;
    STATUNLOCK

    BUSLOCK

    /* Operate at dev level, not filetype */
    pn2->ft = NULL ;

    /* Turn off all DS2409s */
    FS_branchoff(pn2) ;

    (ret=BUS_select(pn2)) || (ret=BUS_first(sn,pn2)) ;
    while (ret==0) {
        char ID[] = "XX";
        STATLOCK
            ++dir_main.entries ;
        STATUNLOCK
//        FS_LoadPath( pn2 ) ;
        Cache_Add_Dir(sn,dindex,pn2) ;
        ++dindex ;
        num2string( ID, sn[0] ) ;
        memcpy( pn2->sn, sn, 8 ) ;
        /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
        FS_devicefind( ID, pn2 ) ;
//printf("DIR pathlength=%d, sn=%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X ft=%p \n",pn2->pathlength,pn2->sn[0],pn2->sn[1],pn2->sn[2],pn2->sn[3],pn2->sn[4],pn2->sn[5],pn2->sn[6],pn2->sn[7],pn2->ft);
        simul |= pn2->dev->flags & (DEV_temp|DEV_volt) ;
        dirfunc( pn2 ) ;
        pn2->dev = NULL ; /* clear for the rest of directory listing */
        (ret=BUS_select(pn2)) || (ret=BUS_next(sn,pn2)) ;
    }
    BUSUNLOCK
    Cache_Del_Dir(dindex,pn2) ;
    if ( simul ) {
        pn2->dev = DeviceSimultaneous ;
        if ( pn2->dev ) {
            dirfunc( pn2 ) ;
            pn2->dev = NULL ;
        }
    }
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
static int FS_cache2real( void (* dirfunc)(const struct parsedname * const), struct parsedname * const pn2 ) {
    unsigned char sn[8] ;
    int simul = 0 ;
    int dindex = 0 ;

    /* Test to see whether we should get the directory "directly" */
    if ( (pn2->state & pn_uncached) || Cache_Get_Dir(sn,0,pn2 ) )
        return FS_realdir(dirfunc,pn2) ;

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
        dirfunc( pn2 ) ;
        simul |= pn2->dev->flags & (DEV_temp|DEV_volt) ;
        pn2->dev = NULL ; /* clear for the rest of directory listing */
//        ++dindex ;
    } while ( Cache_Get_Dir( sn, ++dindex, pn2 )==0 ) ;
    STATLOCK
        dir_main.entries += dindex ;
    STATUNLOCK
    /* Add the simultaneous directory? */
    if ( simul ) {
        pn2->dev = DeviceSimultaneous ;
        if ( pn2->dev ) {
            dirfunc( pn2 ) ;
            pn2->dev = NULL ;
        }
    }
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
