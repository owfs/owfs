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
enum deviceformat devform = fdi ;


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

int FS_dir( void (* dirfunc)(void *,const struct parsedname * const), void * const data, const struct parsedname * const pn ) {
    int ret ;
    struct parsedname pn2 ;

    STATLOCK
        AVERAGE_IN(&dir_avg)
        AVERAGE_IN(&all_avg)
    STATUNLOCK
    FSTATLOCK
        dir_time = time(NULL) ; // protected by mutex
    FSTATUNLOCK
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */
//printf("DIR\n");
    if ( pn == NULL ) {
        ret = -ENOENT ; /* should ever happen */
    } else if ( pn->dev ){ /* device directory */
        struct filetype * lastft = &pn2.dev->ft[pn2.dev->nft] ; /* last filetype struct */
        struct filetype * firstft ; /* first filetype struct */
        char s[33] ;
        int len ;
        STATLOCK
            ++dir_dev.calls ;
        STATUNLOCK
        if ( pn2.subdir ) { /* indevice subdir, name prepends */
//printf("DIR device subdirectory\n");
            strcpy( s , pn2.subdir->name ) ;
            strcat( s , "/" ) ;
            len = strlen(s) ;
            firstft = pn2.subdir  + 1 ;
        } else {
//printf("DIR device directory\n");
            len = 0 ;
            firstft = pn2.dev->ft ;
        }
        for ( pn2.ft=firstft ; pn2.ft < lastft ; ++pn2.ft ) { /* loop through filetypes */
            if ( len ) { /* subdir */
                if ( strncmp( pn2.ft->name , s , len ) ) break ;
            } else { /* promary device directory */
                if ( strchr( pn2.ft->name, '/' ) ) continue ;
            }
            if ( pn2.ft->ag ) {
                for ( pn2.extension=-1 ; pn2.extension < pn2.ft->ag->elements ; ++pn2.extension ) {
                    dirfunc( data, &pn2 ) ;
                    STATLOCK
                        ++dir_dev.entries ;
                    STATUNLOCK
                }
            } else {
                pn2.extension = 0 ;
                dirfunc( data, &pn2 ) ;
                STATLOCK
                    ++dir_dev.entries ;
                STATUNLOCK
            }
        }
        ret = 0 ;
    } else if ( pn->type == pn_alarm ) {  /* root or branch directory -- alarm state */
    /* Note -- alarm directory is smaller, no adapters or stats or uncached */
        struct device ** dpp ;
        unsigned char sn[8] ;

        /* STATISCTICS */
        STATLOCK
            ++dir_main.calls ;
        STATUNLOCK

        pn2.ft = NULL ; /* just in case not properly set */
        BUSLOCK
        /* Turn off all DS2409s */
        FS_branchoff(&pn2) ;
        (ret=BUS_select(&pn2)) || (ret=BUS_first_alarm(sn,&pn2)) ;
        while (ret==0) {
            char ID[] = "XX";
            STATLOCK
                ++dir_main.entries ;
            STATUNLOCK
            num2string( ID, sn[0] ) ;
            memcpy( pn2.sn, sn, 8 ) ;
            /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
            if ( (dpp = bsearch(ID,Devices,nDevices,sizeof(struct device *),devicecmp)) ) {
                pn2.dev = *dpp ;
            } else {
                pn2.dev = &NoDevice ; /* unknown device */
            }
            dirfunc( data, &pn2 ) ;
            pn2.dev = NULL ; /* clear for the rest of directory listing */
            (ret=BUS_select(&pn2)) || (ret=BUS_next_alarm(sn,&pn2)) ;
        }
        BUSUNLOCK
    } else {  /* root or branch directory -- non-alarm */
        struct device ** dpp ;
        unsigned char sn[8] ;

        /* STATISCTICS */
        STATLOCK
            ++dir_main.calls ;
        STATUNLOCK

        pn2.ft = NULL ; /* just in case not properly set */
        if ( pn2.pathlength == 0 ) { /* true root */
//printf("DIR: True root, interface=%d\n",Adapter) ;
            switch (Adapter) {
            case adapter_DS9097:
                dpp = bsearch("DS9097",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case adapter_DS1410:
                dpp = bsearch("DS1410",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case adapter_DS9097U:
                dpp = bsearch("DS9097U",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case adapter_LINK_Multi:
                dpp = bsearch("LINK_Multiport",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case adapter_LINK:
                dpp = bsearch("LINK",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case adapter_DS9490:
                dpp = bsearch("DS9490",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            default : /* just in case an adapter isn't set */
                dpp = NULL ;
            }
            if ( dpp ) {
                pn2.dev = *dpp ;
                dirfunc( data, &pn2 ) ;
                pn2.dev = NULL ; /* clear for the rest of directory listing */
            }
        }
//printf("DIR2\n");
        /* STATISCTICS */
        BUSLOCK
        /* Turn off all DS2409s */

        FS_branchoff(&pn2) ;
        (ret=BUS_select(&pn2)) || (ret=BUS_first(sn,&pn2)) ;
        while (ret==0) {
            char ID[] = "XX";
            STATLOCK
                ++dir_main.entries ;
            STATUNLOCK
            num2string( ID, sn[0] ) ;
            memcpy( pn2.sn, sn, 8 ) ;
            /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
            if ( (dpp = bsearch(ID,Devices,nDevices,sizeof(struct device *),devicecmp)) ) {
                pn2.dev = *dpp ;
            } else {
                pn2.dev = &NoDevice ; /* unknown device */
            }
            dirfunc( data, &pn2 ) ;
            pn2.dev = NULL ; /* clear for the rest of directory listing */
            (ret=BUS_select(&pn2)) || (ret=BUS_next(sn,&pn2)) ;
        }
        BUSUNLOCK
        if ( pn->pathlength == 0 ) { /* true root */
            int i ;
            for ( i=0 ; i<nDevices ; ++i ) {
                if ( Devices[i]->type == dev_statistic ) {
                    pn2.dev = Devices[i] ;
                    dirfunc( data, &pn2 ) ;
                    // pn2.dev = NULL ; /* clear for the rest of directory listing */
                }
            }
        }
    }
    STATLOCK
        AVERAGE_OUT(&dir_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK
    return ret ;
}
static int FS_branchoff( const struct parsedname * const pn ) {
    int ret ;
    unsigned char cmd[] = { 0xCC, 0x66, } ;

    /* Turn off all DS2409s */
    if ( (ret=BUS_reset(pn)) || (ret=BUS_send_data(cmd,2)) ) return ret ;
    return ret ;
}


/* device display format */
void FS_devicename( char * const buffer, const size_t length, const unsigned char * const sn ) {
    switch (devform) {
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
}
