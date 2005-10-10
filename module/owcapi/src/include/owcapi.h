/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By Paul H Alfille
    {c} 2003 GPL
    palfille@earthlink.net
*/

/* OWCAPI - specific header */

/* OWCAPI is the simple C library for OWFS */

#ifndef OWCAPI_H
#define OWCAPI_H

/* initialization, required befopre any other calls. Should be paired with a finish
    OW_init -- simplest, just a device name
              /dev/ttyS0 for serial
              u or u# for USB
              #### for TCP/IP port (owserver)
   OW_init_string -- looks just like the command line to owfs or owhttpd
   OW_init_args -- char** array usually from the main() call

  return value 0 = ok
         non-zero = error
  
  No need to call OW_finish if an error */
 
int OW_init(        const char * device ) ;
int OW_init_string( const char * params) ;
int OW_init_args(   int argc, char ** args ) ;

/* data read or directory read
  name is OWFS style name,
    "" or "/" for root directory
    "01.23456708ABDE" for device directory
    "10.468ACE13579B/temperature for a specific device property
  buffer holds the result
    ascii, possibly comma delimitted
    
  Note: name_length will be used for length, there is no requirement that name be null-terminated
  Note: buffer will ALWAYS be null-terminated, an error will be returned if it must be truncated
  Note: buffer will never be overrun -- buffer_length will be honored
  Note: There is no protection against an inaccurate name_length
  
  return value 0 = ok
         non-zero = error
*/
int OW_get( const char * name, size_t name_length, char * buffer, size_t buffer_length ) ;

/* data write
  name is OWFS style name,
    "05.468ACE13579B/PIO.A for a specific device property
  buffer holds the value
    ascii, possibly comma delimitted
    
  Note: name_length will be used for length, there is no requirement that name be null-terminated
  Note: buffer_length will be used for length, there is no requirement that buffer be null-terminated
  Note: There is no protection against an inaccurate name_length
  
  return value >= 0 length of returned data string
                < 0 error
*/ 
int OW_put( const char * name, size_t name_length, const char * buffer, size_t buffer_length ) ;

/* cleanup
  Clears internal buffer, frees file descriptors
  Notmal process cleanup will work if program ends before OW_finish is called
  But not calling OW_init more than once without an intervening OW_finish will cause a memory leak
  
  return value 0 = ok
         no error return
*/
int OW_finish( void ) ;

#endif /* OWCAPI_H */
