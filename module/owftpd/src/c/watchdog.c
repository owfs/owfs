/*
 * $Id$
 */

#include "owftpd.h"

static int invariant(struct watchdog_s *w);
static void insert(struct watchdog_s *w, struct watched_s *watched);
static void delete(struct watchdog_s *w, struct watched_s *watched);
static void *watcher(void *void_w);

int watchdog_init(struct watchdog_s *w, int inactivity_timeout)
{
	pthread_t thread_id;
	int error_code;

	daemon_assert(w != NULL);
	daemon_assert(inactivity_timeout > 0);

	_MUTEX_INIT(w->mutex);
	w->inactivity_timeout = inactivity_timeout;
	w->oldest = NULL;
	w->newest = NULL;

	error_code = pthread_create(&thread_id, DEFAULT_THREAD_ATTR, watcher, w);
	if (error_code != 0) {
		errno = error_code;
		ERROR_CONNECT("Watchdog thread create problem\n");
		return 0;
	}
	pthread_detach(thread_id);

	daemon_assert(invariant(w));

	return 1;
}

void watchdog_add_watched(struct watchdog_s *w, struct watched_s *watched)
{
	daemon_assert(invariant(w));

	_MUTEX_LOCK(w->mutex);

	watched->watched_thread = pthread_self();
	watched->watchdog = w;
	insert(w, watched);

	_MUTEX_UNLOCK(w->mutex);

	daemon_assert(invariant(w));
}

void watchdog_defer_watched(struct watched_s *watched)
{
	struct watchdog_s *w;

	daemon_assert(invariant(watched->watchdog));

	w = watched->watchdog;
	_MUTEX_LOCK(w->mutex);

	delete(w, watched);
	insert(w, watched);

	_MUTEX_UNLOCK(w->mutex);
	daemon_assert(invariant(w));
}

void watchdog_remove_watched(struct watched_s *watched)
{
	struct watchdog_s *w;

	daemon_assert(invariant(watched->watchdog));

	w = watched->watchdog;
	_MUTEX_LOCK(w->mutex);

	delete(w, watched);

	_MUTEX_UNLOCK(w->mutex);
	daemon_assert(invariant(w));
}

static void insert(struct watchdog_s *w, struct watched_s *watched)
{
   /*********************************************************************
    Set alarm to current time + timeout duration.  Note that this is not 
    strictly legal, since time_t is an abstract data type.
    *********************************************************************/
	watched->alarm_time = NOW_TIME + w->inactivity_timeout;

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

static void delete(struct watchdog_s *w, struct watched_s *watched)
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
	struct watchdog_s *w;
	struct timeval tvwatch = { 1, 0 } ; // 1 second
	time_t now;
	struct watched_s *watched;

	w = (struct watchdog_s *) void_w;
	for (;;) {
		struct timeval tv ;
		
		timercpy( &tv, &tvwatch ) ;
		select(0, NULL, NULL, NULL, &tv);

		time(&now);

		_MUTEX_LOCK(w->mutex);
		while ((w->oldest != NULL) && (difftime(now, w->oldest->alarm_time) > 0)) {
			watched = w->oldest;

			/*******************************************************
			This might seem like a memory leak, but in oftpd the 
			struct watched_s structure is held in the thread itself, so
			canceling the thread effectively frees the memory.  I'm
			not sure whether this is elegant or a hack.  :)
			*******************************************************/
			delete(w, watched);

			pthread_cancel(watched->watched_thread);
		}
		_MUTEX_UNLOCK(w->mutex);
	}
	return VOID_RETURN ;
}

#ifndef NDEBUG
static int invariant(struct watchdog_s *w)
{
	int ret_val;

	struct watched_s *ptr;
	int old_to_new_count;
	int new_to_old_count;


	if (w == NULL) {
		return 0;
	}

	ret_val = 0;
	_MUTEX_LOCK(w->mutex);

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
	_MUTEX_UNLOCK(w->mutex);
	return ret_val;
}
#endif							/* NDEBUG */
