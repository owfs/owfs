/*
$Id$

    Written 2006 Paul H Alfille

    MIT license
*/

#ifndef OWSHELL_H				/* tedious wrapper */
#define OWSHELL_H

#include "config.h"
#include "owfs_config.h"

// Define this to avoid some VALGRIND warnings... (just for testing)
// Warning: This will partially remove the multithreaded support since ow_net.c
// will wait for a thread to complete before executing a new one.
//#define VALGRIND 1

#ifdef __CYGWIN__
#define __BSD_VISIBLE 1 /* for strep and u_int */
#include <sys/select.h> /* for anything select on newer cygwin */
#endif /* __CYGWIN__ */

#define _FILE_OFFSET_BITS   64
#ifdef HAVE_FEATURES_H
#include <features.h>
#endif
#ifdef HAVE_FEATURE_TESTS_H
#include <feature_tests.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>			/* for stat */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>			/* for stat */
#endif
#include <sys/times.h>			/* for times */
#include <ctype.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>				/* for bit twiddling */
#if OW_CYGWIN
#define _MSL_STDINT_H
#endif
#endif

#include <unistd.h>
#ifdef HAVE_GETOPT_H
#ifdef __GNU_LIBRARY__
#include <getopt.h>
#else							/* __GNU_LIBRARY__ */
#define __GNU_LIBRARY__
#include <getopt.h>
#undef __GNU_LIBRARY__
#endif							/* __GNU_LIBRARY__ */
#endif							/* HAVE_GETOPT_H */
#include <fcntl.h>
#ifndef __USE_XOPEN
#define __USE_XOPEN				/* for strptime fuction */
#include <time.h>
#undef __USE_XOPEN				/* for strptime fuction */
#else
#include <time.h>
#endif
#include <termios.h>
#include <errno.h>
#include <syslog.h>

#include <sys/uio.h>
#include <sys/time.h>			/* for gettimeofday */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <netdb.h>				/* addrinfo */

/* Can't include search.h when compiling owperl on Fedora Core 1. */
#ifndef SKIP_SEARCH_H
#ifndef __USE_GNU
#define __USE_GNU
#include <search.h>
#undef __USE_GNU
#else							/* __USE_GNU */
#include <search.h>
#endif							/* __USE_GNU */
#endif							/* SKIP_SEARCH_H */

#if OW_ZERO
/* Zeroconf / Bonjour */
#include "ow_dl.h"
#include "ow_dnssd.h"
#endif

/* Include some compatibility functions */
#include "compat.h"

/* Debugging and error messages separated out for readability */
#include "ow_debug.h"


/* Floating point */
/* I hate to do this, making everything a double */
/* The compiler complains mercilessly, however */
/* 1-wire really is low precision -- float is more than enough */
typedef double _FLOAT;
typedef time_t _DATE;
typedef unsigned char BYTE;
typedef char ASCII;
typedef unsigned int UINT;
typedef int INT;

/*
    OW -- One Wire
    Globals variables -- each invokation will have it's own data
*/

/* command line options */
/* These are the owlib-specific options */
#define OWLIB_OPT "m:c:f:p:s:hqu::d:t:CFRKVP:"
extern const struct option owopts_long[];
void owopt(const int c, const char *arg);

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Filetypes -- directory entries for each 1-wire chip found */
/*
Filetype is the most elaborate of the internal structures, though
simple in concept.

Actually a little misnamed. Each filetype corresponds to a device
property, and to a file in the file system (though there are Filetype
entries for some directory elements too)

Filetypes belong to a particular device. (i.e. each device has it's list
of filetypes) and have a name and pointers to processing functions. Filetypes
also have a data format (integer, ascii,...) and a data length, and an indication
of whether the property is static, changes only on command, or is volatile.

Some properties occur are arrays (pages of memory, logs of temperature
values). The "aggregate" structure holds to allowable size, and the method
of access. -- Aggregate properties are either accessed all at once, then
split, or accessed individually. The choice depends on the device hardware.
There is even a further wrinkle: mixed. In cases where the handling can be either,
mixed causes separate handling of individual items are querried, and combined
if ALL are requested. This is useful for the DS2450 quad A/D where volt and PIO functions
step on each other, but the conversion time for individual is rather costly.
 */

/* predeclare connection_in/out */
struct connection_in;

/* Exposed connection info */
extern int count_inbound_connections;

// Default owserver port (assigned by the IANA)
#define OWSERVER_DEFAULT_PORT	"4304"

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)
#define OW_DEFAULT_LENGTH (128)

/* Unique token for owserver loop checks */
struct antiloop {
	BYTE uuid[16];
};

/* Globals information (for local control) */
struct global {
	int announce_off;			// use zeroconf?
	struct antiloop Token;
	int readonly;
	int autoserver;
	int quiet ;
	int trim ;
	/* Special parameter to trigger William Robison <ibutton@n952.dyndns.ws> timings */
	int timeout_network ;

};
extern struct global Globals;


/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic };
/* Gobal temperature scale */
enum temp_type { temp_celsius, temp_fahrenheit, temp_kelvin, temp_rankine,
};
/* Global pressure scale up tp 8 */
enum pressure_type { pressure_mbar, pressure_atm, pressure_mmhg, pressure_inhg, pressure_psi, pressure_Pa, pressure_end_mark
};

/* Server (Socket-based) interface */
enum msg_classification {
	msg_error,
	msg_nop,
	msg_read,
	msg_write,
	msg_dir,
	msg_size,					// No longer used, leave here to compatibility
	msg_presence,
	msg_dirall,
	msg_get,
	msg_dirallslash,
	msg_getslash,
};
/* message to owserver */
struct server_msg {
	int32_t version;
	int32_t payload;
	int32_t type;
	int32_t sg;
	int32_t size;
	int32_t offset;
};

/* message to client */
struct client_msg {
	int32_t version;
	int32_t payload;
	int32_t ret;
	int32_t sg;
	int32_t size;
	int32_t offset;
};

struct serverpackage {
	ASCII *path;
	BYTE *data;
	size_t datasize;
	BYTE *tokenstring;
	size_t tokens;
};

#define Servermessage       (((int32_t)1)<<16)
#define isServermessage( version )    (((version)&Servermessage)!=0)
#define Servertokens(version)    ((version)&0xFFFF)

/* large enough for arrays of 2048 elements of ~49 bytes each */
#define MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE  100050

/* -------------------------------------------- */

extern int hexflag ; // read/write in hex mode?
extern int slashflag ; // directory with '/'?
extern int size_of_data ;
extern int offset_into_data ;
extern int uncached ;
extern int unaliased ;
extern int trim ;
extern enum temp_type temperature_scale ;
extern enum pressure_type pressure_scale ;
extern enum deviceformat device_format ;

ssize_t tcp_read(int file_descriptor, void *vptr, size_t n, const struct timeval *ptv);
int ClientAddr(char *sname);
int ClientConnect(void);

void DefaultOwserver(void);
void ARG_Net(const char *arg);
void Setup(void);
void ow_help(void);
void OW_Browse(void);

void Server_detect(void);
int ServerRead(ASCII * path);
int ServerWrite(ASCII * path, ASCII * data, int size);
int ServerDir(ASCII * path);
int ServerDirall(ASCII * path);
int ServerPresence(ASCII * path);

#define SHOULD_RETURN_BUS_LIST      ( (UINT) 0x00000002 )
#define ALIAS_REQUEST               ( (UINT) 0x00000008 )
#define TEMPSCALE_MASK              ( (UINT) 0x00FF0000 )
#define TEMPSCALE_BIT  16
#define PRESSURESCALE_BIT  18
#define DEVFORMAT_MASK              ( (UINT) 0xFF000000 )
#define DEVFORMAT_BIT  24
#define UNCACHED                    ( (UINT) 0x00000020 )
#define TRIM                        ( (UINT) 0x00000040 )
#define OWNET                       ( (UINT) 0x00000100 )

#define PRINT_ERROR(...)		while ( ! Globals.quiet ) { fprintf( stderr, __VA_ARGS__ ) ; break ; }
#define PERROR(...)				while ( ! Globals.quiet ) { perror( __VA_ARGS__ ) ; break ; }

struct connection_in;

struct device_search {
	int LastDiscrepancy;		// for search
	int LastDevice;				// for search
	BYTE sn[8];
	BYTE search;
};

struct connection_in {
	char *name;
	int file_descriptor;
	char *host;
	char *service;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
};

extern struct connection_in s_owserver_connection;
extern struct connection_in *owserver_connection;

__attribute__((noreturn)) void Exit(int exit_code);

#endif							/* OWSHELL_H */
