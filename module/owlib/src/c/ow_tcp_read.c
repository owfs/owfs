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

/* ow_net holds the network utility routines. Many stolen unashamedly from Steven's Book */
/* Much modification by Christian Magnusson especially for Valgrind and embedded */
/* non-threaded fixes by Jerry Scharf */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"

/* Wait for something to be readable or timeout */
int tcp_wait(int fd, const struct timeval * ptv)
{
    int rc;
    fd_set readset;
    struct timeval tv = { ptv->tv_sec, ptv->tv_usec, };

    /* Initialize readset */
    FD_ZERO(&readset);
    FD_SET(fd, &readset);

    /* Read if it doesn't timeout first */
    rc = select(fd + 1, &readset, NULL, NULL, &tv);
    if (rc > 0) {
        /* Is there something to read? */
        if (FD_ISSET(fd, &readset) == 0) {
            return -EIO;	/* error */
        }
        return 0 ;
    } else {				/* timed out */
        return -EAGAIN;
    }
}

/* Read "n" bytes from a descriptor. */
/* Stolen from Unix Network Programming by Stevens, Fenner, Rudoff p89 */
ssize_t tcp_read(int fd, void *vptr, size_t n, const struct timeval * ptv)
{
    size_t nleft;
    ssize_t nread;
    char *ptr;
    //printf("NetRead attempt %d bytes Time:(%ld,%ld)\n",n,ptv->tv_sec,ptv->tv_usec ) ;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        int rc;
        fd_set readset;
        struct timeval tv = { ptv->tv_sec, ptv->tv_usec, };

        /* Initialize readset */
        FD_ZERO(&readset);
        FD_SET(fd, &readset);

        /* Read if it doesn't timeout first */
        rc = select(fd + 1, &readset, NULL, NULL, &tv);
        if (rc > 0) {

            /* Is there something to read? */
            if (FD_ISSET(fd, &readset) == 0) {
//                  STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
                return -EIO;    /* error */
            }
//                    update_max_delay(pn);
            if ((nread = read(fd, ptr, nleft)) < 0) {
                if (errno == EINTR) {
                    errno = 0;  // clear errno. We never use it anyway.
                    nread = 0;  /* and call read() again */
                } else {
                    ERROR_DATA("Network data read error\n");
                    STAT_ADD1(NET_read_errors);
                    return (-1);
                }
            } else if (nread == 0) {
                break;          /* EOF */
            }
            //{ int i ; for ( i=0 ; i<nread ; ++i ) printf("%.2X ",ptr[i]) ; printf("\n") ; }
            nleft -= nread;
            ptr += nread;
        } else if (rc < 0) {    /* select error */
            if (errno == EINTR) {
                /* select() was interrupted, try again */
//                STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                continue;
            }
            ERROR_DATA("Selection error (network)\n");
//            STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
            return -EINTR;
        } else {                /* timed out */
            LEVEL_CONNECT("TIMEOUT after %d bytes\n", n - nleft);
//            STAT_ADD1_BUS(BUS_read_timeout_errors,pn->in);
            return -EAGAIN;
        }
    }
    return (n - nleft);         /* return >= 0 */
}
