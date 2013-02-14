/* $Id: */

/* 
 * From Clemmens Egger <clemens.egger@eurac.edu> :
 * 
 * Working with the DS1921 Thermochron iButton I noticed that, when writing a time value to 
 * clock/udate and reading it immediately afterwards, the returned value differs notably to 
 * the written value. This is dependent on the local time zone and whether it's daylight saving 
 * time or not. My environment variable TZ usually reads "Europe/Berlin". 
 * 
 * In the following example there's a difference of 3600 seconds i.e. 1 hour between the two values.
 * $ ./owwrite -s 3000 21.D5542E000000/clock/udate 1352387000; ./owread -s 3000 21.D5542E000000/clock/udate
  1352383400
  * 
  * Looking at the source code, I discovered that the function mktime is used to convert a struct 
  * tm to time_t. However mktime uses the local time zone which explains the time shift.
  * 
  * The following workaround makes the time shift disappear: just set the TZ environment variable 
  * to UTC when invoking the server.
  * TZ=UTC ./owserver -u -p 3000
  * 
  * There's a function timegm which is basically the opposite part to gmtime. The latter is 
  * already used in the code when writing a udate value to the iButton. In my opinion timegm 
  * would be more suitable than mktime.
  * 
  * Code obtained 2012-10-17 from timegm(3)
  * */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#if (!defined _BSD_SOURCE && !defined _SVID_SOURCE)
#include <stdlib.h>

time_t timegm(struct tm *tm)
{
	time_t ret;
	char *tz;

	TIMEGMLOCK ;
	tz = getenv("TZ");
	setenv("TZ", "", 1);
	tzset();
	ret = mktime(tm);
	if (tz)
		setenv("TZ", tz, 1);
	else
		unsetenv("TZ");
	tzset();
	TIMEGMUNLOCK ;
	
	return ret;
}

#endif
