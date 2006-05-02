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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

#include "owfs_config.h"
#include "ow_xxxx.h"
#include "ow_counters.h"
#include "ow_connection.h"

#include <sys/time.h>

/* ------- Prototypes ------------ */
static int CheckPresence_low( struct connection_in * in, const struct parsedname * pn ) ;

/* ------- Functions ------------ */

/*
 * Funcation to calculated maximum delay between writing a
 * command to the bus, and reading the answer.
 * This will probably not work with simultanious reading...
 */
void update_max_delay(const struct parsedname * const pn) {
    long sec, usec;
    struct timeval *r, *w;
    struct timeval last_delay;
    if(!pn || !pn->in) return;
    gettimeofday( &(pn->in->bus_read_time) , NULL );
    r = &pn->in->bus_read_time;
    w = &pn->in->bus_write_time;

    sec = r->tv_sec - w->tv_sec;
    if((sec >= 0) && (sec <= 5)) {
        usec = r->tv_usec - w->tv_usec;
        last_delay.tv_sec = sec;
        last_delay.tv_usec = usec;

        while( last_delay.tv_usec >= 1000000 ) {
            last_delay.tv_usec -= 1000000 ;
            last_delay.tv_sec++;
        }
        if (
            (last_delay.tv_sec > max_delay.tv_sec)
            || ((last_delay.tv_sec >= max_delay.tv_sec) && (last_delay.tv_usec > max_delay.tv_usec))
           ) {
            STATLOCK;
                max_delay.tv_sec =  last_delay.tv_sec;
                max_delay.tv_usec =  last_delay.tv_usec;
            STATUNLOCK;
        }
    }
    /* DS9097(1410)_send_and_get() call this function many times, therefore
     * I reset bus_write_time after every calls */
    gettimeofday( &(pn->in->bus_write_time) , NULL );
    return;
}

int FS_type(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t len = strlen(pn->dev->name) ;
//printf("TYPE len=%d, size=%d\n",len,size);
    if ( offset ) return -EFAULT ;
    if ( len > size ) return -EMSGSIZE ;
    strcpy( buf , pn->dev->name ) ;
    return len ;
}

int FS_code(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 2 ) return -EMSGSIZE ;
    num2string( buf , pn->sn[0] ) ;
    return 2 ;
}

int FS_ID(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 12 ) return -EMSGSIZE ;
    bytes2string( buf , &(pn->sn[1]) , 6 ) ;
    return 12 ;
}

int FS_crc8(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 2 ) return -EMSGSIZE ;
    num2string(buf,pn->sn[7]) ;
    return 2 ;
}

int FS_address(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 16 ) return -EMSGSIZE ;
    bytes2string( buf , pn->sn , 8 ) ;
    return 16 ;
}

/* Check if device exists -- >=0 yes, -1 no */
int CheckPresence( const struct parsedname * const pn ) {
    if ( pn->type == pn_real && pn->dev != DeviceSimultaneous && pn->dev != DeviceThermostat ) {
        LEVEL_DETAIL("Checking presence of %s\n",SAFESTRING(pn->path) ) ;
        return CheckPresence_low(pn->in,pn) ;
    }
    return 0 ;
}

/* Check if device exists -- 0 yes, 1 no */
int Check1Presence( const struct parsedname * const pn ) {
    int y ;
    //printf("Check1Presence type=%d\n", pn->type);
    if ( pn->type == pn_real ) {
        //printf("Check1Presence pn->dev=%p DS=%p\n", pn->dev, DeviceSimultaneous);
        if ( pn->dev != DeviceSimultaneous ) return FS_present(&y,pn) || y==0 ;
    }
    return 0 ;
}

/* Check if device exists -- -1 no, >=0 yes (bus number) */
/* lower level, cycle through the devices */
#ifdef OW_MT

struct checkpresence_struct {
    struct connection_in * in ;
    const struct parsedname * pn ;
    int ret ;
} ;

static void * CheckPresence_callback( void * vp ) {
    struct checkpresence_struct * cps = (struct checkpresence_struct *) vp ;
    cps->ret = CheckPresence_low( cps->in, cps->pn ) ;
    pthread_exit(NULL) ;
    return NULL ;
}

static int CheckPresence_low( struct connection_in * in, const struct parsedname * const pn ) {
    int ret = 0 ;
    pthread_t thread ;
    int threadbad = 1 ;
    struct parsedname pn2 ;
    struct checkpresence_struct cps = { in->next, pn, 0 } ;

    if(!(pn->state & pn_bus)) {
        threadbad = in->next==NULL || pthread_create( &thread, NULL, CheckPresence_callback, (void *)(&cps) ) ;
    }

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    pn2.in = in ;
    
    //printf("CheckPresence_low:\n");
    if ( TestConnection(&pn2) ) { // reconnect successful?
        ret = -ECONNABORTED ;
    } else if(get_busmode(in) == bus_remote) {
        //printf("CheckPresence_low: call ServerPresence\n");
        ret = ServerPresence(&pn2) ;
        if(ret >= 0) {
            /* Device was found on this in-device, return it's index */
            ret = in->index;
        } else {
            ret = -1;
        }
      //printf("CheckPresence_low: ServerPresence(%s) pn->in->index=%d ret=%d\n", pn->path, pn->in->index, ret);
    } else {
        //printf("CheckPresence_low: call BUS_normalverify\n");
        /* this can only be done on local busses */
        BUSLOCK(&pn2);
            ret = BUS_normalverify(&pn2) ;
        BUSUNLOCK(&pn2);
        if(ret == 0) {
            /* Device was found on this in-device, return it's index */
            ret = in->index;
        } else {
            ret = -1;
        }
    }
    if ( threadbad == 0 ) { /* was a thread created? */
        void * v ;
        if ( pthread_join( thread, &v ) ) return ret ; /* wait for it (or return only this result) */
        if ( cps.ret >= 0 ) return cps.in->index ;
    }
    //printf("Presence return = %d\n",ret) ;
    return ret ;
}

#else /* OW_MT */

static int CheckPresence_low( struct connection_in * in, const struct parsedname * pn ) {
    int ret = 0 ;
    struct parsedname pn2 ;

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    //printf("CheckPresence_low:\n");
    pn2.in = in ;
    if ( TestConnection(&pn2) ) { // reconnect successful?
    ret = -ECONNABORTED ;
    } else if(get_busmode(in) == bus_remote) {
        //printf("CheckPresence_low: call ServerPresence\n");
        ret = ServerPresence(&pn2) ;
        if(ret >= 0) {
            /* Device was found on this in-device, return it's index */
            ret = in->index;
        } else {
            ret = -1;
        }
      //printf("CheckPresence_low: ServerPresence(%s) pn->in->index=%d ret=%d\n", pn->path, pn->in->index, ret);
    } else {
        //printf("CheckPresence_low: call BUS_normalverify\n");
        /* this can only be done on local busses */
        BUSLOCK(&pn2);
            ret = BUS_normalverify(&pn2) ;
        BUSUNLOCK(&pn2);
        if(ret == 0) {
            /* Device was found on this in-device, return it's index */
            ret = in->index;
        } else {
            ret = -1;
        }
    }

    if ( ret < 0 && in->next ) return CheckPresence_low( in->next, pn ) ;
    return ret ;
}
#endif /* OW_MT */

int FS_present(int * y , const struct parsedname * pn) {
    if ( pn->type != pn_real || pn->dev == DeviceSimultaneous || pn->dev == DeviceThermostat ) {
        y[0]=1 ;
    } else {
        BUSLOCK(pn);
            y[0] = BUS_normalverify(pn) ? 0 : 1 ;
        BUSUNLOCK(pn);
    }
    return 0 ;
}
