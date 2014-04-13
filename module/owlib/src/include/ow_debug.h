/* $Id$
   Debug and error messages
   Meant to be included in ow.h
   Separated just for readability
*/

/* OWFS source code
   1-wire filesystem for linux
   {c} 2006 Paul H Alfille
   License GPL2.0
*/

#ifndef OW_DEBUG_H
#define OW_DEBUG_H

#include <config.h>
#include "owfs_config.h"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include <stdarg.h>

/* module/ownet/c/src/include/ow_debug.h & module/owlib/src/include/ow_debug.h
   are identical except this define */
//#define OWNETC_OW_DEBUG 1

/* error functions */
enum e_err_level { e_err_default, e_err_connect, e_err_call, e_err_data,
	e_err_detail, e_err_debug, e_err_beyond,
};
enum e_err_type { e_err_type_level, e_err_type_error, };
enum e_err_print { e_err_print_mixed, e_err_print_syslog,
	e_err_print_console,
};

void err_msg(enum e_err_type errnoflag, enum e_err_level level, const char * file, int line, const char * func, const char *fmt, ...);
void _Debug_Bytes(const char *title, const unsigned char *buf, int length);
void fatal_error(const char * file, int line, const char * func, const char *fmt, ...);
static inline int return_ok(void) { return 0; }

extern int log_available;

#if OW_DEBUG
#define LEVEL_DEFAULT(...)    if (Globals.error_level>=e_err_default) {\
    err_msg(e_err_type_level,e_err_default,__FILE__,__LINE__,__func__,__VA_ARGS__); }
#define LEVEL_CONNECT(...)    if (Globals.error_level>=e_err_connect) {\
    err_msg(e_err_type_level,e_err_connect,__FILE__,__LINE__,__func__,__VA_ARGS__); }
#define LEVEL_CALL(...)       if (Globals.error_level>=e_err_call)  {\
    err_msg(e_err_type_level,e_err_call,   __FILE__,__LINE__,__func__,__VA_ARGS__); }
#define LEVEL_DATA(...)       if (Globals.error_level>=e_err_data) {\
    err_msg(e_err_type_level,e_err_data,   __FILE__,__LINE__,__func__,__VA_ARGS__); }
#define LEVEL_DETAIL(...)     if (Globals.error_level>=e_err_detail) {\
    err_msg(e_err_type_level,e_err_detail, __FILE__,__LINE__,__func__,__VA_ARGS__); }
#define LEVEL_DEBUG(...)      if (Globals.error_level>=e_err_debug) {\
    err_msg(e_err_type_level,e_err_debug,  __FILE__,__LINE__,__func__,__VA_ARGS__); }
    
#define ERROR_DEFAULT(...)    if (Globals.error_level>=e_err_default) {\
    err_msg(e_err_type_error,e_err_default,__FILE__,__LINE__,__func__,__VA_ARGS__); }
#define ERROR_CONNECT(...)    if (Globals.error_level>=e_err_connect) {\
    err_msg(e_err_type_error,e_err_connect,__FILE__,__LINE__,__func__,__VA_ARGS__); }
#define ERROR_CALL(...)       if (Globals.error_level>=e_err_call)  {\
    err_msg(e_err_type_error,e_err_call,   __FILE__,__LINE__,__func__,__VA_ARGS__); }
#define ERROR_DATA(...)       if (Globals.error_level>=e_err_data) {\
    err_msg(e_err_type_error,e_err_data,   __FILE__,__LINE__,__func__,__VA_ARGS__); }
#define ERROR_DETAIL(...)     if (Globals.error_level>=e_err_detail) {\
    err_msg(e_err_type_error,e_err_detail, __FILE__,__LINE__,__func__,__VA_ARGS__); }
#define ERROR_DEBUG(...)      if (Globals.error_level>=e_err_debug) {\
    err_msg(e_err_type_error,e_err_debug,  __FILE__,__LINE__,__func__,__VA_ARGS__); }

#define FATAL_ERROR(...) fatal_error(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define Debug_OWQ(owq)        if (Globals.error_level>=e_err_debug) { _print_owq(owq); }
#define Debug_Bytes(title,buf,length)    if (Globals.error_level>=e_err_beyond) { _Debug_Bytes(title,buf,length) ; }

#else /* not OW_DEBUG */

#define LEVEL_DEFAULT(...)    do { } while (0)
#define LEVEL_CONNECT(...)    do { } while (0)
#define LEVEL_CALL(...)       do { } while (0)
#define LEVEL_DATA(...)       do { } while (0)
#define LEVEL_DETAIL(...)     do { } while (0)
#define LEVEL_DEBUG(...)      do { } while (0)

#define ERROR_DEFAULT(...)    do { } while (0)
#define ERROR_CONNECT(...)    do { } while (0)
#define ERROR_CALL(...)       do { } while (0)
#define ERROR_DATA(...)       do { } while (0)
#define ERROR_DETAIL(...)     do { } while (0)
#define ERROR_DEBUG(...)      do { } while (0)

#define FATAL_ERROR(...)      do { exit(EXIT_FAILURE); } while (0)

#define Debug_Bytes(title,buf,length)    do { } while (0)
#define Debug_OWQ(owq)        do { } while (0)

#endif /* OW_DEBUG */

/* Make sure strings are safe for printf */
#define SAFESTRING(x) ((x) ? (x):"")

/* Easy way to show 64bit serial numbers */
#define SNformat	"%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X"
#define SNvar(sn)	(sn)[0],(sn)[1],(sn)[2],(sn)[3],(sn)[4],(sn)[5],(sn)[6],(sn)[7]

extern const char  sem_init_failed[];
// FreeBSD note: if we continue with failed semaphores, we WILL segfault later on.
#define my_sem_init(sem, shared, value)              do {\
	int mrc = sem_init(sem, shared, value);	            \
	if(mrc != 0) { fprintf(stderr, sem_init_failed,        errno, strerror(errno)); \
		if(errno == 28) { \
			LEVEL_DEFAULT("Too many semaphores running, please run fewer apps utilizing semaphores (owfs apps for example), or tweak sysctl kern.ipc.sem*. Exiting.\n"); \
		} \
		FATAL_ERROR(sem_init_failed, mrc, strerror(mrc));			\
	}} while(0)
 
 
/* Need to define those functions to get FILE and LINE information */

extern const char  mutex_init_failed[];
#define my_pthread_mutex_init(mutex, attr)              do {\
	int mrc = pthread_mutex_init(mutex, attr);	            \
	if(mrc != 0) { FATAL_ERROR( mutex_init_failed,        mrc, strerror(mrc)); }} while(0)

extern const char  mutex_destroy_failed[];
#define my_pthread_mutex_destroy(mutex)                 do {\
	int mrc = pthread_mutex_destroy(mutex);                 \
	if(mrc != 0) { FATAL_ERROR( mutex_destroy_failed,     mrc, strerror(mrc)); }} while(0)

extern const char  mutex_lock_failed[];
#define my_pthread_mutex_lock(mutex)                    do {\
	int mrc = pthread_mutex_lock(mutex);		            \
	if(mrc != 0) { FATAL_ERROR( mutex_lock_failed,        mrc, strerror(mrc)); }} while (0)

extern const char  mutex_unlock_failed[];
#define my_pthread_mutex_unlock(mutex)                  do {\
	int mrc = pthread_mutex_unlock(mutex);                  \
	if(mrc != 0) { FATAL_ERROR( mutex_unlock_failed,      mrc, strerror(mrc)); }} while (0)

extern const char  mutexattr_init_failed[];
#define my_pthread_mutexattr_init(attr)                 do {\
	int mrc = pthread_mutexattr_init(attr);	                \
	if(mrc != 0) { FATAL_ERROR( mutexattr_init_failed,    mrc, strerror(mrc)); }} while (0)

extern const char  mutexattr_destroy_failed[];
#define my_pthread_mutexattr_destroy(attr)              do {\
	int mrc = pthread_mutexattr_destroy(attr);	            \
	if(mrc != 0) { FATAL_ERROR( mutexattr_destroy_failed, mrc, strerror(mrc)); }} while (0)

extern const char  mutexattr_settype_failed[];
#define my_pthread_mutexattr_settype(attr, typ)         do {\
	int mrc = pthread_mutexattr_settype(attr, typ);	        \
	if(mrc != 0) { FATAL_ERROR( mutexattr_settype_failed, mrc, strerror(mrc)); }} while (0)

extern const char  cond_timedwait_failed[];
#define my_pthread_cond_timedwait(cond, mutex, abstime)	do {\
	int mrc = pthread_cond_timedwait(cond, mutex, abstime);	\
	if(mrc != 0) { FATAL_ERROR( cond_timedwait_failed,    mrc, strerror(mrc)); }} while (0)

extern const char  cond_wait_failed[];
#define my_pthread_cond_wait(cond, mutex)	            do {\
	int mrc = pthread_cond_wait(cond, mutex);	            \
	if(mrc != 0) { FATAL_ERROR( cond_wait_failed,         mrc, strerror(mrc)); }} while (0)

extern const char  cond_signal_failed[];
#define my_pthread_cond_signal(cond)	                do {\
	int mrc = pthread_cond_signal(cond);	                \
	if(mrc != 0) { FATAL_ERROR( cond_signal_failed,       mrc, strerror(mrc)); }} while (0)

extern const char  cond_init_failed[];
#define my_pthread_cond_init(cond, attr)                do {\
	int mrc = pthread_cond_init(cond, attr);			    \
	if(mrc != 0) { FATAL_ERROR( cond_init_failed,         mrc, strerror(mrc)); }} while (0)

extern const char  cond_destroy_failed[];
#define my_pthread_cond_destroy(cond)	                do {\
	int mrc = pthread_cond_destroy(cond);	                \
	if(mrc != 0) { FATAL_ERROR( cond_destroy_failed,      mrc, strerror(mrc)); }} while (0)

#endif							/* OW_DEBUG_H */
