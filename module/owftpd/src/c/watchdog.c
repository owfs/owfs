/*
 * $Id$
 */

#include "owfs_config.h"
#include <unistd.h>
#include "daemon_assert.h"
#include "watchdog.h"

#include    <pthread.h>

static int invariant(watchdog_t *w);
static void insert(watchdog_t *w, watched_t *watched);
static void delete(watchdog_t *w, watched_t *watched);
static void *watcher(void *void_w);

int watchdog_init(watchdog_t *w, int inactivity_timeout, error_code_t *err)
{
    pthread_t thread_id;
    int error_code;

    daemon_assert(w != NULL);
    daemon_assert(inactivity_timeout > 0);
    daemon_assert(err != NULL);

#if defined(__uClinux__) || defined(EMBEDDED)
    {
        pthread_mutexattr_t attr ;
        pthread_mutexattr_init(&attr) ;
        pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ADAPTIVE_NP) ;
        pthread_mutex_init(&w->mutex,&attr) ;
        pthread_mutexattr_destroy(&attr) ;
    }
#else /* non-embedded */
    pthread_mutex_init(&w->mutex, NULL);
#endif /* EMBEDDED */
    w->inactivity_timeout = inactivity_timeout;
    w->oldest = NULL;
    w->newest = NULL;

    error_code = pthread_create(&thread_id, NULL, watcher, w);
    if (error_code != 0) {
	error_init(err, error_code, "error %d from pthread_create()", 
	  error_code);
        return 0;
    }
    pthread_detach(thread_id);

    daemon_assert(invariant(w));

    return 1;
}

void watchdog_add_watched(watchdog_t *w, watched_t *watched)
{
    daemon_assert(invariant(w));

    pthread_mutex_lock(&w->mutex);

    watched->watched_thread = pthread_self();
    watched->watchdog = w;
    insert(w, watched);

    pthread_mutex_unlock(&w->mutex);

    daemon_assert(invariant(w));
}

void watchdog_defer_watched(watched_t *watched)
{
    watchdog_t *w;

    daemon_assert(invariant(watched->watchdog));

    w = watched->watchdog;
    pthread_mutex_lock(&w->mutex);

    delete(w, watched);
    insert(w, watched);

    pthread_mutex_unlock(&w->mutex);
    daemon_assert(invariant(w));
}

void watchdog_remove_watched(watched_t *watched)
{
    watchdog_t *w;

    daemon_assert(invariant(watched->watchdog));

    w = watched->watchdog;
    pthread_mutex_lock(&w->mutex);

    delete(w, watched);

    pthread_mutex_unlock(&w->mutex);
    daemon_assert(invariant(w));
}

static void insert(watchdog_t *w, watched_t *watched)
{
   /*********************************************************************
    Set alarm to current time + timeout duration.  Note that this is not 
    strictly legal, since time_t is an abstract data type.
    *********************************************************************/
    watched->alarm_time = time(NULL) + w->inactivity_timeout;

   /*********************************************************************
    If the system clock got set backwards, we really should search for 
    the correct location, instead of just inserting at the end.  However, 
    this happens very rarely (ntp and other synchronization protocols 
    speed up or slow down the clock to adjust the time), so we'll just 
    set our alarm to the time of the newest alarm - giving any watched
    processes added some extra time.
    *********************************************************************/
    if (w->newest != NULL) {
        if (w->newest->alarm_time > watched->alarm_time) {
	    watched->alarm_time = w->newest->alarm_time;
	}
    }

    /* set our pointers */
    watched->older = w->newest;
    watched->newer = NULL;

    /* add to list */
    if (w->oldest == NULL) {
        w->oldest = watched;
    } else {
        w->newest->newer = watched;
    }
    w->newest = watched;
    watched->in_list = 1;
}

static void delete(watchdog_t *w, watched_t *watched)
{
    if (!watched->in_list) {
        return;
    }

    if (watched->newer == NULL) {
        daemon_assert(w->newest == watched);
	w->newest = w->newest->older;
        if (w->newest != NULL) {
	    w->newest->newer = NULL;
	}
    } else {
        daemon_assert(w->newest != watched);
	watched->newer->older = watched->older;
    }

    if (watched->older == NULL) {
        daemon_assert(w->oldest == watched);
	w->oldest = w->oldest->newer;
        if (w->oldest != NULL) {
	    w->oldest->older = NULL;
	}
    } else {
        daemon_assert(w->oldest != watched);
	watched->older->newer = watched->newer;
    }

    watched->older = NULL;
    watched->newer = NULL;
    watched->in_list = 0;
}

static void *watcher(void *void_w)
{
    watchdog_t *w;
    struct timeval tv;
    time_t now;
    watched_t *watched;

    w = (watchdog_t *)void_w;
    for (;;) {
        tv.tv_sec = 1;
	tv.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tv);

	time(&now);

        pthread_mutex_lock(&w->mutex);
	while ((w->oldest != NULL) && 
	    (difftime(now, w->oldest->alarm_time) > 0))
	{
	    watched = w->oldest;

            /*******************************************************
	     This might seem like a memory leak, but in oftpd the 
	     watched_t structure is held in the thread itself, so 
	     canceling the thread effectively frees the memory.  I'm
	     not sure whether this is elegant or a hack.  :)
             *******************************************************/
	    delete(w, watched);

	    pthread_cancel(watched->watched_thread);
	}
        pthread_mutex_unlock(&w->mutex);
    }
}

#ifndef NDEBUG
static int invariant(watchdog_t *w)
{
    int ret_val;

    watched_t *ptr;
    int old_to_new_count;
    int new_to_old_count;


    if (w == NULL) {
        return 0;
    }

    ret_val = 0;
    pthread_mutex_lock(&w->mutex);

    if (w->inactivity_timeout <= 0) {
        goto exit_invariant;
    }

    /* either oldest and newest are both NULL, or neither is */
    if (w->oldest != NULL) {
        if (w->newest == NULL) {
            goto exit_invariant;
	}

        /* check list from oldest to newest */
	old_to_new_count = 0;
	ptr = w->oldest;
        while (ptr != NULL) {
	    old_to_new_count++;
	    if (ptr->older != NULL) {
	        if (ptr->alarm_time < ptr->older->alarm_time) {
                    goto exit_invariant;
		}
	    }
	    if (ptr->newer != NULL) {
	        if (ptr->alarm_time > ptr->newer->alarm_time) {
                    goto exit_invariant;
		}
	    }
	    ptr = ptr->newer;
        }

        /* check list from newest to oldest */
	new_to_old_count = 0;
	ptr = w->newest;
	while (ptr != NULL) {
	    new_to_old_count++;
	    if (ptr->older != NULL) {
	        if (ptr->alarm_time < ptr->older->alarm_time) {
                    goto exit_invariant;
		}
	    }
	    if (ptr->newer != NULL) {
	        if (ptr->alarm_time > ptr->newer->alarm_time) {
                    goto exit_invariant;
		}
	    }
	    ptr = ptr->older;
	}

	/* verify forward and backward lists at least have the same count */
	if (old_to_new_count != new_to_old_count) {
            goto exit_invariant;
	}

    } else {
        if (w->newest != NULL) {
            goto exit_invariant;
	}
    }

    /* at this point, we're probably okay */
    ret_val = 1;

exit_invariant:
    pthread_mutex_unlock(&w->mutex);
    return ret_val;
}
#endif /* NDEBUG */

