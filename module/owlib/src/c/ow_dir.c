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

static int FS_branchoff( void) ;
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

int FS_dir( void (* dirfunc)(void *,const struct parsedname * const), void * const data, struct parsedname * const pn ) {
    int ret ;

//printf("DIR\n");
    if ( pn==NULL ) {
        return -ENOENT ;
    } else if ( pn->dev ) { /* device directory */
        int i ;
        struct filetype * ft = pn->ft ; /* temp storage */
        int extension = pn->extension ; /* temp storage */
        for ( i=0 ; i < pn->dev->nft ; ++i ) { /* loop through filetypes */
            pn->ft = &(pn->dev->ft[i]) ;
            if ( pn->ft->ag ) {
                for ( pn->extension=-1 ; pn->extension < pn->ft->ag->elements ; ++pn->extension ) {
                    dirfunc( data, pn ) ;
                }
                pn->extension = 0 ;
            } else {
                dirfunc( data, pn ) ;
            }
        }
        pn->ft = ft ; /* restore */
        pn->extension = extension ; /* restore */
        return 0 ;
    } else { /* root or branch directory */
        struct device ** dpp ;
        unsigned char sn[8] ;
        char ID[] = "XX";
        pn->ft = NULL ; /* just in case not properly set */
        if ( pn->pathlength == 0 ) { /* true root */
//printf("DIR: True root, interface=%d\n",Version2480) ;
            switch (Version2480) {
            case 0:
                dpp = bsearch("DS9097",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case 1:
                dpp = bsearch("DS1410",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case 3:
                dpp = bsearch("DS9097U",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case 6:
                dpp = bsearch("LINK_Multiport",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case 7:
                dpp = bsearch("LINK",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            case 8:
                dpp = bsearch("DS9490",Devices,nDevices,sizeof(struct device *),devicecmp) ;
                break ;
            default : /* just in case an adapter isn't set */
                dpp = NULL ;
            }
            if ( dpp ) {
                pn->dev = *dpp ;
                dirfunc( data, pn ) ;
                pn->dev = NULL ; /* clear for the rest of directory listing */
            }
        }
        BUS_lock() ;
        /* Turn off all DS2409s */
        FS_branchoff() ;
//printf("DIR2\n");
        /* Triplicate bus read */
        /* STATISCTICS */
        ++dir_tries[0] ;
        (ret=BUS_select(pn)) || (ret=BUS_first(sn)) ;
        if (ret) {
            /* STATISCTICS */
            ++dir_tries[1] ;
            (ret=BUS_select(pn)) || (ret=BUS_first(sn)) ;
        }
        if (ret) {
            /* STATISCTICS */
            ++dir_tries[2] ;
            (ret=BUS_select(pn)) || (ret=BUS_first(sn)) ;
        } else {
            /* STATISCTICS */
            ++dir_success ;
        }
        while (ret==0) {
            num2string( ID, sn[0] ) ;
            memcpy( pn->sn, sn, 8 ) ;
            /* Search for known 1-wire device -- keyed to device name (family code in HEX) */
            if ( (dpp = bsearch(ID,Devices,nDevices,sizeof(struct device *),devicecmp)) ) {
                pn->dev = *dpp ;
            } else {
                 pn->dev = &NoDevice ; /* unknown device */
            }
            dirfunc( data, pn ) ;
            pn->dev = NULL ; /* clear for the rest of directory listing */
            (ret=BUS_select(pn)) || (ret=BUS_next(sn)) ;
        }
        BUS_unlock() ;
        if ( pn->pathlength == 0 ) { /* true root */
            int i ;
            for ( i=0 ; i<nDevices ; ++i ) {
                if ( Devices[i]->type == dev_statistic ) {
                    pn->dev = Devices[i] ;
                    dirfunc( data, pn ) ;
                    pn->dev = NULL ; /* clear for the rest of directory listing */
                }
            }
        }
        return ret ;
    }
}
static int FS_branchoff( void ) {
    int ret ;
    unsigned char cmd[] = { 0xCC, 0x66, } ;

    /* Turn off all DS2409s */
    if ( (ret=BUS_reset()) || (ret=BUS_send_data(cmd,2)) ) return ret ;
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
