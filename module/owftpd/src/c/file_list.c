/*
 * $Id$
 */

//#include <stdio.h>
//#include <limits.h>
//#include <string.h>
//#include <sys/stat.h>
//#include <unistd.h>
//#include <glob.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <ctype.h>

//#include "owfs_config.h"

#include <owftpd.h>

/* if no localtime_r() is available, provide one */
#ifndef HAVE_LOCALTIME_R
//#include <pthread.h>

/* uClibc on Coldfire need to initiate mutex with pthread_mutex_init() */
pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

struct tm *localtime_r(const time_t *timep, struct tm *timeptr)
{
    pthread_mutex_lock(&time_lock);
    *timeptr = *(localtime(timep));
    pthread_mutex_unlock(&time_lock);
    return timeptr;
}
#endif /* HAVE_LOCALTIME_R */
