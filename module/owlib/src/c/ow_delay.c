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
#include "ow.h"

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void UT_delay(const UINT len)
{
#ifdef DELAY_BUSY_WHILE
    /* Just a test to create a busy-while-loop and trace wait time. */
    struct timeval tv, now;
    unsigned long diff;
    int i, j = 0;

    if (len == 0)
	return;

    gettimeofday(&tv, NULL);
    while (1) {
	gettimeofday(&now, NULL);
	diff =
	    1000 * (now.tv_sec - tv.tv_sec) + (now.tv_usec -
					       tv.tv_usec) / 1000;
	if (diff >= len)
	    break;
	for (i = 0; i < 30; i++)
	    j += i;
    }
#else				/* DELAY_BUSY_WHILE */

#ifdef HAVE_NANOSLEEP

    struct timespec s;
    struct timespec rem;

    if (len == 0)
	return;

    rem.tv_sec = len / 1000;
    rem.tv_nsec = 1000000 * (len % 1000);

    while (1) {
	s.tv_sec = rem.tv_sec;
	s.tv_nsec = rem.tv_nsec;
	if (nanosleep(&s, &rem) < 0) {
	    if (errno != EINTR)
		break;
	    /* was interupted... continue sleeping... */
	    //printf("UT_delay: EINTR s=%ld.%ld r=%ld.%ld: %s\n", s.tv_sec, s.tv_nsec, rem.tv_sec, rem.tv_nsec, strerror(errno));
#ifdef __UCLIBC__
	    errno = 0;		// clear errno every time in uclibc at least
#endif				/* __UCLIBC__ */
	} else {
	    /* completed sleeping */
	    break;
	}
    }
#ifdef __UCLIBC__
    errno = 0;			// clear errno in uclibc at least
#endif				/* __UCLIBC__ */

#else				/* HAVE_NANOSLEEP */

    usleep((unsigned long) len);

#endif				/* HAVE_NANOSLEEP */

#endif				/* DELAY_BUSY_WHILE */
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' us
//
void UT_delay_us(const unsigned long len)
{
#ifdef DELAY_BUSY_WHILE
    /* Just a test to create a busy-while-loop and trace wait time. */
    struct timeval tv, now;
    unsigned long diff;
    int i, j = 0;

    if (len == 0)
	return;

    gettimeofday(&tv, NULL);
    while (1) {
	gettimeofday(&now, NULL);
	diff =
	    1000000 * (now.tv_sec - tv.tv_sec) + (now.tv_usec -
						  tv.tv_usec);
	if (diff >= len)
	    break;
	for (i = 0; i < 10; i++)
	    j += i;
    }
#else				/* DELAY_BUSY_WHILE */


#ifdef HAVE_NANOSLEEP

    struct timespec s;
    struct timespec rem;

    if (len == 0)
	return;

    rem.tv_sec = len / 1000000;
    rem.tv_nsec = 1000 * (len % 1000000);

    while (1) {
	s.tv_sec = rem.tv_sec;
	s.tv_nsec = rem.tv_nsec;
	if (nanosleep(&s, &rem) < 0) {
	    if (errno != EINTR)
		break;
	    /* was interupted... continue sleeping... */
	    //printf("UT_delay: EINTR s=%ld.%ld r=%ld.%ld: %s\n", s.tv_sec, s.tv_nsec, rem.tv_sec, rem.tv_nsec, strerror(errno));
#ifdef __UCLIBC__
	    errno = 0;		// clear errno every time in uclibc at least
#endif				/* __UCLIBC__ */
	} else {
	    /* completed sleeping */
	    break;
	}
    }
#ifdef __UCLIBC__
    errno = 0;			// clear errno in uclibc at least
#endif				/* __UCLIBC__ */

#else				/* HAVE_NANOSLEEP */

    usleep((unsigned long) len * 1000);

#endif				/* HAVE_NANOSLEEP */

#endif				/* DELAY_BUSY_WHILE */
}
