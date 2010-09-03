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
	UT_delay_us( len * 1000 ) ;
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' us
//
void UT_delay_us(const unsigned long len)
{
#ifdef DELAY_BUSY_WHILE
	/* Just a test to create a busy-while-loop and trace wait time. */
	struct timeval rem = { 
		len / 1000000,
		len % 1000000
		} ;
	struct timeval tv, tvnow;
	int i, j = 0;

	if (len == 0) {
		return;
	}

	timernow( &tv );
	timeradd( &tv, &rem, &tv );
	do {
		timernow( &tvnow );
		for (i = 0; i < 10; i++) {
			j += i;
		}
	} while ( timercmp( &tvnow, &tv, < ) ) ;
#else							/* DELAY_BUSY_WHILE */


#ifdef HAVE_NANOSLEEP

	struct timespec s;
	struct timespec rem = { 
		len / 1000000,
		1000 * (len % 1000000)
		} ;

	if (len == 0) {
		return;
	}

	while (1) {
		s.tv_sec = rem.tv_sec;
		s.tv_nsec = rem.tv_nsec;
		if (nanosleep(&s, &rem) < 0) {
			if (errno != EINTR) {
				break;
			}
			/* was interupted... continue sleeping... */
			//printf("UT_delay: EINTR s=%ld.%ld r=%ld.%ld: %s\n", s.tv_sec, s.tv_nsec, rem.tv_sec, rem.tv_nsec, strerror(errno));
#ifdef __UCLIBC__
			errno = 0;			// clear errno every time in uclibc at least
#endif							/* __UCLIBC__ */
		} else {
			/* completed sleeping */
			break;
		}
	}
#ifdef __UCLIBC__
	errno = 0;					// clear errno in uclibc at least
#endif							/* __UCLIBC__ */

#else							/* HAVE_NANOSLEEP */

	usleep((unsigned long) len);

#endif							/* HAVE_NANOSLEEP */

#endif							/* DELAY_BUSY_WHILE */
}
