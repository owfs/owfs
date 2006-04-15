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

/* error functions */
void err_msg(const char *fmt, ...) ;
void err_bad(const char *fmt, ...) ;
void err_debug(const char *fmt, ...) ;
extern int error_print ;
extern int error_level ;
extern int now_background ;
extern int log_available ;
#define LEVEL_DEFAULT(...)    if (error_level>0) err_msg(__VA_ARGS__) ;
#define LEVEL_CONNECT(...)    if (error_level>1) err_bad(__VA_ARGS__) ;
#define LEVEL_CALL(...)       if (error_level>2) err_msg(__VA_ARGS__) ;
#define LEVEL_DATA(...)       if (error_level>3) err_msg(__VA_ARGS__) ;
#define LEVEL_DEBUG(...)      if (error_level>4) err_debug(__VA_ARGS__) ;

/* Make sure strings are safe for printf */
#define SAFESTRING(x) ((x) ? (x):"")

/* Easy way to show 64bit serial numbers */
#define SNformat	"%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X"
#define SNvar(sn)	(sn)[0],(sn)[1],(sn)[2],(sn)[3],(sn)[4],(sn)[5],(sn)[6],(sn)[7]

#endif /* OW_DEBUG_H */
