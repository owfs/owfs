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

#include <sys/time.h>

/* ------- Prototypes ------------ */
static int CheckPresence_low( const struct parsedname * const pn ) ;

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
            STATLOCK
                max_delay.tv_sec =  last_delay.tv_sec;
                max_delay.tv_usec =  last_delay.tv_usec;
            STATUNLOCK
        }
    }
    /* BUS_send_and_get() call this function many times, therefor
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

/* Check if device exists -- 0 yes, 1 no */
int CheckPresence( const struct parsedname * const pn ) {
    if ( pn->type == pn_real ) {
        if ( pn->dev != DeviceSimultaneous ) return CheckPresence_low(pn) ;
    }
    return 0 ;
}

/* Check if device exists -- 0 yes, 1 no */
int Check1Presence( const struct parsedname * const pn ) {
    int y ;
    if ( pn->type == pn_real ) {
        if ( pn->dev != DeviceSimultaneous ) return FS_present(&y,pn) || y==0 ;
    }
    return 0 ;
}

/* Check if device exists -- 0 yes, 1 no */
/* lower level, cycle through the devices */
static int CheckPresence_low( const struct parsedname * const pn ) {
    int ret = 0 ;
#ifdef OW_MT
    pthread_t thread ;
    int threadbad;
    void * v ;
    /* Embedded function */
    void * Check2( void * vp ) {
        struct parsedname *pn1 = (struct parsedname *)vp ;
        struct parsedname pnnext ;
        struct stateinfo si ;
        int ret;
        memcpy( &pnnext, pn1 , sizeof(struct parsedname) ) ;
        pnnext.in = pn1->in->next ;
        pnnext.si = &si ;
        ret = CheckPresence_low(&pnnext) ;
        pthread_exit((void *)ret);
    }
    //printf("CheckPresence_low\n"); UT_delay(100);
    threadbad = pn->in==NULL || pn->in->next==NULL || pthread_create( &thread, NULL, Check2, (void *)pn ) ;
#endif /* OW_MT */
    BUSLOCK(pn)
        ret = BUS_normalverify(pn) ;
    BUSUNLOCK(pn)
#ifdef OW_MT
    if ( threadbad == 0 ) { /* was a thread created? */
        if ( pthread_join( thread, &v ) ) return ret ; /* wait for it (or return only this result) */
        return ret && ((int) v) ;
    }
#endif /* OW_MT */
    return ret ;
}

int FS_present(int * y , const struct parsedname * pn) {
    if ( pn->type != pn_real || pn->dev == DeviceSimultaneous ) {
        y[0]=1 ;
    } else {
        BUSLOCK(pn)
            y[0] = BUS_normalverify(pn) ? 0 : 1 ;
        BUSUNLOCK(pn)
    }
    return 0 ;
}
