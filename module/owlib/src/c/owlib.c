/*
$Id$
	OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include "owfs_config.h"
#include "ow.h"
#include "ow_devices.h"

speed_t speed = B9600;
int portnum = -1 ; /* TCP port (for owhttpd) */
char * devport = NULL ; /* Device name (COM or parallel port) */
int devfd = -1; /*file descriptor for serial/parallel port*/
int useusb = 0 ; /* which DS9490 to connect */

#ifdef OW_USB
    usb_dev_handle * devusb = NULL ;
#else /* OW_USB */
    void * devusb = NULL ;
#endif /* OW_USB */

/* Device List Structure */
/* Defined here, after all files defined */
struct device * Devices[] = {
    & d_DS18S20 ,
    & d_DS18B20 ,
    & d_DS1822 ,
    & d_DS1921 ,
    & d_DS1992 ,
    & d_DS1993 ,
    & d_DS1995 ,
    & d_DS1996 ,
    & d_DS2401 ,
    & d_DS2404 ,
    & d_DS2404S ,
    & d_DS2405 ,
    & d_DS2406 ,
    & d_DS2408 ,
    & d_DS2409 ,
    & d_DS2415 ,
    & d_DS2417 ,
    & d_DS2423 ,
    & d_DS2436 ,
    & d_DS2438 ,
    & d_DS2450 ,
    & d_DS2502 ,
    & d_DS1982U ,
    & d_DS2505 ,
    & d_DS1985U ,
    & d_DS2506 ,
    & d_DS1986U ,
    & d_DS2890 ,
    & d_LCD ,
    & d_DS9490 ,
    & d_DS9097 ,
    & d_DS1410 ,
    & d_DS9097U ,
    & d_iButtonLink_Multiport ,
    & d_iButtonLink ,
    & d_stats_cache ,
    & d_stats_read ,
    & d_stats_write ,
    & d_stats_directory ,
    & d_stats_bus ,
}  ;

size_t nDevices = (size_t) (sizeof(Devices)/sizeof(struct device *)) ;

int presencecheck = 1 ; /* check if present whenever opening a directory or static file */
time_t start_time ;

int cacheavailable = 0 ; /* assume no cache */
int background = 1 ; /* operate in background mode */
int timeout = DEFAULT_TIMEOUT ;
int timeout_slow = 10*DEFAULT_TIMEOUT ;

struct interface_routines iroutines ;

enum temp_type tempscale = temp_celsius ; /* Default Celsius */

/* Static buffer for serial conmmunications */
/* Since only used during actual transfer to/from the adapter,
   should be protected from contention even when multithreading allowed */

unsigned char combuffer[UART_FIFO_SIZE] ;

/* ----------------- */
/* -- Statistics --- */
/* ----------------- */
unsigned int cache_tries = 0 ;
unsigned int cache_hits = 0 ;
unsigned int cache_misses = 0 ;
unsigned int cache_flips = 0 ;
unsigned int cache_dels = 0 ;
unsigned int cache_adds = 0 ;
unsigned int cache_expired = 0 ;

unsigned int read_calls = 0 ;
unsigned int read_cache = 0 ;
unsigned int read_bytes = 0 ;
unsigned int read_cachebytes = 0 ;
unsigned int read_array = 0 ;
unsigned int read_aggregate = 0 ;
unsigned int read_arraysuccess = 0 ;
unsigned int read_aggregatesuccess = 0 ;
unsigned int read_tries[3] = {0,0,0,} ;
unsigned int read_success = 0 ;

unsigned int write_calls = 0 ;
unsigned int write_bytes = 0 ;
unsigned int write_array = 0 ;
unsigned int write_aggregate = 0 ;
unsigned int write_arraysuccess = 0 ;
unsigned int write_aggregatesuccess = 0 ;
unsigned int write_tries[3] = {0,0,0,} ;
unsigned int write_success = 0 ;

unsigned int dir_tries[3] = {0, 0, 0, } ;
unsigned int dir_success = 0 ;
unsigned int dir_depth = 0 ;

struct timeval bus_time = {0, 0, } ;
struct timeval bus_pause = {0, 0, } ;
unsigned int bus_locks = 0 ;
unsigned int crc8_tries = 0 ;
unsigned int crc8_errors = 0 ;
unsigned int crc16_tries = 0 ;
unsigned int crc16_errors = 0 ;
unsigned int read_timeout = 0 ;

/* All ow library setup */
void LibSetup( void ) {
    /* special resort in case static data (devices and filetypes) not properly sorted */
//printf("LibSetup\n");
    DeviceSort() ;

	/* DB cache code */
#ifdef OW_CACHE
//printf("CacheOpen\n");
    Cache_Open() ;
//printf("CacheOpened\n");
#endif /* OW_CACHE */
    start_time = time(NULL) ;
}

/* Start the owlib process -- actually only tests for backgrounding */
int LibStart( void ) {
    if ( background && daemon(1,0) ) {
        fprintf(stderr,"Cannot enter background mode, quitting.\n") ;
        return 1 ;
    }
    if (pid_file) {
        FILE * pid = fopen(  pid_file, "w+" ) ;
        if ( pid == NULL ) {
            syslog(LOG_WARNING,"Cannot open PID file: %s Error=%s\n",pid_file,strerror(errno) ) ;
            free( pid_file ) ;
            pid_file = NULL ;
	    return 1 ;
        }
//printf("opopts pid = %ld\n",(long unsigned int)getpid());
        fprintf(pid,"%lu",(long unsigned int)getpid() ) ;
        fclose(pid) ;
    }
    return 0 ;
}

/* Actaully COM and Parallel */
int ComSetup( const char * busdev ) {
    struct stat s ;
    int ret ;

    if ( devport ) {
        fprintf(stderr,"1-wire port already set to %s, ignoring %s.\n",devport,busdev) ;
        return 1 ;
    }
    if ( (devport=strdup(busdev)) == NULL ) return -ENOMEM ;
    if ( (ret=stat( devport, &s )) ) {
        syslog( LOG_ERR, "Cannot stat port: %s error=%s\n",devport,strerror(ret)) ;
        return -ret ;
    }
    if ( ! S_ISCHR(s.st_mode) ) syslog( LOG_INFO , "Not a character device: %s\n",devport) ;
    if ((devfd = open(devport, O_RDWR | O_NONBLOCK)) < 0) {
        ret = errno ;
        syslog( LOG_ERR,"Cannot open port: %s error=%s\n",devport,strerror(ret)) ;
        return -ret ;
    }
    if ( major(s.st_rdev) == 99 ) { /* parport device */
        if ( DS1410_detect() ) {
            syslog(LOG_WARNING, "Cannot detect the DS1410E parallel adapter\n");
            return 1 ;
        }
    } else { /* serial device */
        if ( COM_open() ) return -ENODEV ;
        /* Set up DS2480/LINK interface */
        if ( DS2480_detect() ) {
            syslog(LOG_WARNING,"Cannot detect DS2480 or LINK interface on %s.\n",devport) ;
            if ( DS9097_detect() ) {
                syslog(LOG_WARNING,"Cannot detect DS9097 (passive) interface on %s.\n",devport) ;
                return 1 ;
            }
        }
    }
    syslog(LOG_INFO,"Interface type = %d on %s\n",Version2480,devport) ;
    return 0 ;
}

int USBSetup( void ) {
#ifdef OW_USB
    if ( devport ) {
        fprintf(stderr,"1-wire port already set to %s, ignoring USB.\n",devport) ;
        return 1 ;
    }
    return DS9490_detect() ;
#else /* OW_USB */
    fprintf(stderr,"Attempt to use a USB 1-wire interface without compiling support for libusb\n");
    return 1 ;
#endif /* OW_USB */
}

/* All ow library closeup */
void LibClose( void ) {
    if ( pid_file ) {
       if ( unlink( pid_file ) ) syslog(LOG_WARNING,"Cannot remove PID file: %s error=%s\n",pid_file,strerror(errno)) ;
       free( pid_file ) ;
       pid_file = NULL ;
    }
    COM_close() ;
    if ( devport ) {
        free(devport) ;
        devport = NULL ;
    }
#ifdef OW_USB
    DS9490_close() ;
#endif /* OW_USB */
#ifdef OW_CACHE
    Cache_Close() ;
#endif /* OW_CACHE */
}

void Timeout( const char * c ) {
	timeout = strtol( c,NULL,10 ) ;
	if ( errno || timeout<1 ) timeout = DEFAULT_TIMEOUT ;
	timeout_slow = 10*timeout ;
}

/* Library presence function -- NOP */
void owlib_signature_function( void ) {
}
