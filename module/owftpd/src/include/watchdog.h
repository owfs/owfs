/* 
 * $Id$
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <pthread.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "error.h"

/* each watched thread gets one of these structures */
typedef struct watched {
    /* thread to monitor */
    pthread_t watched_thread;

    /* flag whether in a watchdog list */
    int in_list;

    /* time when to cancel thread if no activity */
    time_t alarm_time;

    /* for location in doubly-linked list */
    struct watched *older;
    struct watched *newer;

    /* watchdog that this watched_t is in */
    void *watchdog;
} watched_t;

/* the watchdog keeps track of all information */
typedef struct {
    pthread_mutex_t mutex;
    int inactivity_timeout;

    /* the head and tail of our list */
    watched_t *oldest;
    watched_t *newest;
} watchdog_t;

int watchdog_init(watchdog_t *w, int inactivity_timeout, error_t *err);
void watchdog_add_watched(watchdog_t *w, watched_t *watched);
void watchdog_defer_watched(watched_t *watched);
void watchdog_remove_watched(watched_t *watched);

#endif /* WATCHDOG_H */

