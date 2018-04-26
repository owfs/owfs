/*
   OWFS (owfs, owhttpd, owserver, owperl, owtcl, owphp, owpython, owcapi)
   one-wire file system and related programs

    By Paul H Alfille
    {c} 2008 GPL
    paul.alfille@gmail.com
*/

/* This is the libownet header
   a C programmikng interface to easily access owserver
   and thus the entire Dallas/Maxim 1-wire system.

   This header has all the public routines for a program linking in the library
*/

/* OWNETAPI - specific header */
/* OWNETAPI is the simple C library for OWFS */

#ifndef OWNETAPI_H
#define OWNETAPI_H

#ifdef __CYGWIN__
#define __BSD_VISIBLE 1 /* for u_int */
#endif /* __CYGWIN__ */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define MAX_READ_BUFFER_SIZE 10000

/* OWNET_HANDLE
   A (non-negative) integer corresponding to a particular owserver connection.
   It is used for each function call, and allows multiple owservers to be
   accessed
*/
	typedef int OWNET_HANDLE;

/* OWNET_HANDLE OWNET_init( const char * owserver )
   Starting routine -- takes a string corresponding to the tcp address of owserver
   e.g. "192.168.0.1:5000" or "5001" or even "" for the default localhost:4304

   returns a non-negative HANDLE, or <0 for error
*/
	OWNET_HANDLE OWNET_init(const char *owserver_tcp_address_and_port);

/* int OWNET_dirlist( OWNET_HANDLE h, const char * onewire_path, 
        char ** return_string )
   Get the 1-wire directory as a comma-separated list.
   return_string is allocated by this program, and must be free-ed by your program.
   
   return non-negative length of return_string on success
   return <0 error and NULL on error
*/
	int OWNET_dirlist(OWNET_HANDLE h, const char *onewire_path, char **return_string);

/* int OWNET_dirprocess( OWNET_HANDLE h, const char * onewire_path, 
        void (*dirfunc) (void * passed_on_value, const char* directory_element), 
        void * passed_on_value )
   Get the 1-wire directory corresponding to the given path
   Call function dirfunc on each element
   passed_on_value is an arbitrary pointer that gets included in the dirfunc call to
   add some state information

   returns number of elements processed,
   or <0 for error
*/
	int OWNET_dirprocess(OWNET_HANDLE h, const char *onewire_path, void (*dirfunc) (void *passed_on_value, const char *directory_element),
						 void *passed_on_value);


/* int OWNET_present( OWNET_HANDLE h, const char * onewire_path)
   Check if a one-wire device is present

   returns = 0 on success,
   returns <0 on error
*/
	
	int OWNET_present(OWNET_HANDLE h, const char *onewire_path);

/* int OWNET_read( OWNET_HANDLE h, const char * onewire_path, 
        unsigned char ** return_string )
   Read a value from a one-wire device property
   return_string has the result but must be free-ed by the calling program.

   returns length of result on success,
   returns <0 on error
*/
	int OWNET_read(OWNET_HANDLE h, const char *onewire_path, char **return_string);

/* int OWNET_lread( OWNET_HANDLE h, const char * onewire_path, 
        unsigned char * return_string, size_t size, off_t offset )
   Read a value from a one-wire device property
   Buffer should be pre-allocated, and size and offset specified.
   return_string has the result.

   returns length of result on success,
   returns <0 on error
*/
	int OWNET_lread(OWNET_HANDLE h, const char *onewire_path, char *return_string, size_t size, off_t offset);

/* int OWNET_put( OWNET_HANDLE h, const char * onewire_path, 
        const unsigned char * value_string, size_t size)
   Write a value to a one-wire device property,
   of specified size and offset
   
   return 0 on success
   return <0 on error
*/
	int OWNET_put(OWNET_HANDLE h, const char *onewire_path, const char *value_string, size_t size);

/* int OWNET_lwrite( OWNET_HANDLE h, const char * onewire_path, 
        const unsigned char * value_string, size_t size, off_t offset )
   Write a value to a one-wire device property,
   of specified size and offset
   
   return 0 on success
   return <0 on error
*/
	int OWNET_lwrite(OWNET_HANDLE h, const char *onewire_path, const char *value_string, size_t size, off_t offset);

/* void OWNET_close( OWNET_HANDLE h)
   close a particular owserver connection
*/
	void OWNET_close(OWNET_HANDLE h);

/* void OWNET_closeall( void )
   close all owserver connections
*/
	void OWNET_closeall(void);

/* void OWNET_finish( void )
   close all owserver connections and free all memory
*/
	void OWNET_finish(void);

/* get and set temperature scale
   Note that temperature scale applies to all HANDLES
   C - celsius
   F - farenheit
   R - rankine
   K - kelvin
   0 -> set default (C)
*/
	void OWNET_set_temperature_scale(char temperature_scale);
	char OWNET_get_temperature_scale(void);

/* get and set device format
   Note that device format applies to all HANDLES
   f.i default
   f.i.c
   fi.c
   fi
   f.ic
   fic
   NULL or "" -> set default
*/
	void OWNET_set_device_format(const char *device_format);
	const char *OWNET_get_device_format(void);

/* get and set trim state
 * This just means removing extra spaces from
 * numeric values
 * which is easier for some data parsing
   Note that trim state applies to all HANDLES
*/
	void OWNET_set_trim( int trim_state ) ;
	int OWNET_get_trim( void ) ;


#ifdef __cplusplus
}
#endif
#endif							/* OWNETAPI_H */
