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
#include <owfs_config.h>

/* error functions */
enum e_err_level { e_err_default, e_err_connect, e_err_call, e_err_data, e_err_detail, e_err_debug, e_err_beyond, };
enum e_err_type { e_err_type_level, e_err_type_error, };
enum e_err_print { e_err_print_mixed, e_err_print_syslog, e_err_print_console, };

void err_msg(enum e_err_type errnoflag, enum e_err_level level, const char *fmt, ...);
void _Debug_Bytes(const char *title, const unsigned char *buf, int length);

extern int log_available;

#if OW_DEBUG
#define LEVEL_DEFAULT(...)    if (Global.error_level>=e_err_default) \
    err_msg(e_err_type_level,e_err_default,__VA_ARGS__)
#define LEVEL_CONNECT(...)    if (Global.error_level>=e_err_connect) \
    err_msg(e_err_type_level,e_err_connect,__VA_ARGS__)
#define LEVEL_CALL(...)       if (Global.error_level>=e_err_call)  \
    err_msg(e_err_type_level,e_err_call,__VA_ARGS__)
#define LEVEL_DATA(...)       if (Global.error_level>=e_err_data) \
    err_msg(e_err_type_level,e_err_data,__VA_ARGS__)
#define LEVEL_DETAIL(...)     if (Global.error_level>=e_err_detail) \
    err_msg(e_err_type_level,e_err_detail,__VA_ARGS__)
#define LEVEL_DEBUG(...)      if (Global.error_level>=e_err_debug) \
    err_msg(e_err_type_level,e_err_debug,__VA_ARGS__)

#define ERROR_DEFAULT(...)    if (Global.error_level>=e_err_default) \
    err_msg(e_err_type_error,e_err_default,__VA_ARGS__)
#define ERROR_CONNECT(...)    if (Global.error_level>=e_err_connect) \
    err_msg(e_err_type_error,e_err_connect,__VA_ARGS__)
#define ERROR_CALL(...)       if (Global.error_level>=e_err_call)  \
    err_msg(e_err_type_error,e_err_call,__VA_ARGS__)
#define ERROR_DATA(...)       if (Global.error_level>=e_err_data) \
    err_msg(e_err_type_error,e_err_data,__VA_ARGS__)
#define ERROR_DETAIL(...)     if (Global.error_level>=e_err_detail) \
    err_msg(e_err_type_error,e_err_detail,__VA_ARGS__)
#define ERROR_DEBUG(...)      if (Global.error_level>=e_err_debug) \
    err_msg(e_err_type_error,e_err_debug,__VA_ARGS__)

#define Debug_OWQ(owq)        if (Global.error_level>=e_err_debug) \
    _print_owq(owq)
#define Debug_Bytes(title,buf,length)    if (Global.error_level>=e_err_beyond) \
    _Debug_Bytes(title,buf,length)
#else
#define LEVEL_DEFAULT(...)    { } while (0);
#define LEVEL_CONNECT(...)    { } while (0);
#define LEVEL_CALL(...)       { } while (0);
#define LEVEL_DATA(...)       { } while (0);
#define LEVEL_DETAIL(...)     { } while (0);
#define LEVEL_DEBUG(...)      { } while (0);

#define ERROR_DEFAULT(...)    { } while (0);
#define ERROR_CONNECT(...)    { } while (0);
#define ERROR_CALL(...)       { } while (0);
#define ERROR_DATA(...)       { } while (0);
#define ERROR_DETAIL(...)     { } while (0);
#define ERROR_DEBUG(...)      { } while (0);

#define Debug_Bytes(title,buf,length)    { } while (0);
#define Debug_OWQ(owq)        { } while (0);
#endif

/* Make sure strings are safe for printf */
#define SAFESTRING(x) ((x) ? (x):"")

/* Easy way to show 64bit serial numbers */
#define SNformat	"%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X"
#define SNvar(sn)	(sn)[0],(sn)[1],(sn)[2],(sn)[3],(sn)[4],(sn)[5],(sn)[6],(sn)[7]

#endif							/* OW_DEBUG_H */
