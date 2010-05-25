/* From Geo Carncross geocar@internetconnection.net -- GPL */
/* File for incomplete semaphore implementations */
/* $Id$ */
/* Note: gcc wants inline before int */

#ifndef __semaphore_h
#define __semaphore_h

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#else							/* HAVE_SEMAPHORE_H */

#include <pthread.h>
#include <errno.h>
#include <stdio.h>

typedef struct {
	pthread_mutex_t m;
	pthread_cond_t c;

	volatile unsigned int v, w;
} sem_t;

//static int inline sem_destroy(sem_t *s) {
static inline int sem_destroy(sem_t * s)
{
	pthread_mutex_lock(&s->m);
	if (s->w) {
		pthread_mutex_unlock(&s->m);
		errno = EBUSY;
		return -1;
	}
	pthread_cond_destroy(&s->c);
	pthread_mutex_unlock(&s->m);
	pthread_mutex_destroy(&s->m);
	return 0;
}

//static int inline sem_init(sem_t *s, int ign, int val) {
static inline int sem_init(sem_t * s, int ign, int val)
{
	if (ign != 0) {
		errno = ENOSYS;
		return -1;
	}
	if (pthread_mutex_init(&s->m, NULL) != 0)
		return -1;
	if (pthread_cond_init(&s->c, NULL) != 0)
		return -1;
	s->v = val;
	s->w = 0;

	return 0;
}

//static int inline sem_post(sem_t *s) {
static inline int sem_post(sem_t * s)
{
	int ok;
	if (pthread_mutex_lock(&s->m) == -1)
		return -1;
	s->v++;
	if (s->w == 1)
		ok = pthread_cond_signal(&s->c);
	else if (s->w > 1)
		ok = pthread_cond_broadcast(&s->c);
	pthread_mutex_unlock(&s->m);
	return ok;
}

//static int inline sem_wait(sem_t *s) {
static inline int sem_wait(sem_t * s)
{
	int ok = 0;
	if (pthread_mutex_lock(&s->m) == -1)
		return -1;
	while (s->v == 0) {
		s->w++;
		if (pthread_cond_wait(&s->c, &s->m) == -1) {
			ok = -1;
			break;
		}
		s->w--;
	}
	s->v--;
	pthread_mutex_unlock(&s->m);
	return ok;
}

#endif							/* HAVE_SEMAPHORE_H */

#endif							/* __semaphore_h */
