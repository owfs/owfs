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
void err_msg(int errnoflag, int level, const char *fmt, ...) ;

extern int error_print ;
extern int error_level ;
extern int now_background ;
extern int log_available ;

#if OW_DEBUG
#define LEVEL_DEFAULT(...)    if (error_level>0) err_msg(0,0,__VA_ARGS__) ;
#define LEVEL_CONNECT(...)    if (error_level>1) err_msg(0,1,__VA_ARGS__) ;
#define LEVEL_CALL(...)       if (error_level>2) err_msg(0,2,__VA_ARGS__) ;
#define LEVEL_DATA(...)       if (error_level>3) err_msg(0,3,__VA_ARGS__) ;
#define LEVEL_DETAIL(...)     if (error_level>4) err_msg(0,4,__VA_ARGS__) ;
#define LEVEL_DEBUG(...)      if (error_level>5) err_msg(0,5,__VA_ARGS__) ;

#define ERROR_DEFAULT(...)    if (error_level>5) err_msg(1,0,__VA_ARGS__) ;
#define ERROR_CONNECT(...)    if (error_level>5) err_msg(1,1,__VA_ARGS__) ;
#define ERROR_CALL(...)       if (error_level>5) err_msg(1,2,__VA_ARGS__) ;
#define ERROR_DATA(...)       if (error_level>5) err_msg(1,3,__VA_ARGS__) ;
#define ERROR_DETAIL(...)     if (error_level>5) err_msg(1,4,__VA_ARGS__) ;
#define ERROR_DEBUG(...)      if (error_level>5) err_msg(1,5,__VA_ARGS__) ;
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
#endif

/* Make sure strings are safe for printf */
#define SAFESTRING(x) ((x) ? (x):"")

/* Easy way to show 64bit serial numbers */
#define SNformat	"%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X"
#define SNvar(sn)	(sn)[0],(sn)[1],(sn)[2],(sn)[3],(sn)[4],(sn)[5],(sn)[6],(sn)[7]

#endif /* OW_DEBUG_H */
