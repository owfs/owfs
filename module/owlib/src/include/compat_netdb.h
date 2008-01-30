/* Copyright (C) 1996,1997,1998,1999,2000,2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* All data returned by the network data base library are supplied in
   host order and returned in network order (suitable for use in
   system calls).  */

#ifndef	_COMPAT_NETDB_H
#define	_COMPAT_NETDB_H	1

#include <config.h>
#include "owfs_config.h"

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#ifndef __USE_GNU
#define __USE_GNU
#endif

/* Doesn't exist for Solaris, make a test for it later */
/* #undef HAVE_SA_LEN */

#ifndef __HAS_IPV6__
#define __HAS_IPV6__ 0
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#ifndef __THROW
#define __THROW
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef __USE_MISC
/* This is necessary to make this include file properly replace the
   Sun version.  */
# include <rpc/netdb.h>
#endif

#ifdef HAVE_BITS_NETDB_H
#include <bits/netdb.h>
#endif

/* Absolute file name for network data base files.  */
#define	_PATH_HEQUIV		"/etc/hosts.equiv"
#define	_PATH_HOSTS		"/etc/hosts"
#define	_PATH_NETWORKS		"/etc/networks"
#define	_PATH_NSSWITCH_CONF	"/etc/nsswitch.conf"
#define	_PATH_PROTOCOLS		"/etc/protocols"
#define	_PATH_SERVICES		"/etc/services"

#ifndef __set_errno
#define __set_errno(x) (errno = (x))
#endif

#ifndef __set_h_errno
#define __set_h_errno(x) (h_errno = (x))
#endif


#ifndef HAVE_INET_NTOP
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif

#ifndef HAVE_INET_PTON
int inet_pton(int af, const char *src, void *dst);
#endif

#ifndef HAVE_GETADDRINFO

/* Possible values left in `h_errno'.  */
#define	NETDB_INTERNAL	-1		/* See errno.  */
#define	NETDB_SUCCESS	0		/* No problem.  */
#define	HOST_NOT_FOUND	1		/* Authoritative Answer Host not found.  */
#define	TRY_AGAIN	2			/* Non-Authoritative Host not found,
								   or SERVERFAIL.  */
#define	NO_RECOVERY	3			/* Non recoverable errors, FORMERR, REFUSED,
								   NOTIMP.  */
#define	NO_DATA		4			/* Valid name, no data record of requested
								   type.  */
#define	NO_ADDRESS	NO_DATA		/* No address, look for MX record.  */

#ifdef __USE_XOPEN2K
/* Highest reserved Internet port number.  */
# define IPPORT_RESERVED	1024
#endif


#ifdef __USE_GNU
/* Scope delimiter for getaddrinfo(), getnameinfo().  */
# define SCOPE_DELIMITER	'%'
#endif

/* Extension from POSIX.1g.  */
#ifdef	__USE_POSIX
/* Structure to contain information about address of a service provider.  */
struct addrinfo {
	int ai_flags;				/* Input flags.  */
	int ai_family;				/* Protocol family for socket.  */
	int ai_socktype;			/* Socket type.  */
	int ai_protocol;			/* Protocol for socket.  */
	socklen_t ai_addrlen;		/* Length of socket address.  */
	struct sockaddr *ai_addr;	/* Socket address for socket.  */
	char *ai_canonname;			/* Canonical name for service location.  */
	struct addrinfo *ai_next;	/* Pointer to next in list.  */
};

# ifdef __USE_GNU
/* Lookup mode.  */
#  define GAI_WAIT	0
#  define GAI_NOWAIT	1
# endif

/* Possible values for `ai_flags' field in `addrinfo' structure.  */
# define AI_PASSIVE	0x0001		/* Socket address is intended for `bind'.  */
# define AI_CANONNAME	0x0002	/* Request for canonical name.  */
# define AI_NUMERICHOST	0x0004	/* Don't use name resolution.  */

/* Error values for `getaddrinfo' function.  */
# define EAI_BADFLAGS	  -1	/* Invalid value for `ai_flags' field.  */
# define EAI_NONAME	  -2		/* NAME or SERVICE is unknown.  */
# define EAI_AGAIN	  -3		/* Temporary failure in name resolution.  */
# define EAI_FAIL	  -4		/* Non-recoverable failure in name res.  */
# define EAI_NODATA	  -5		/* No address associated with NAME.  */
# define EAI_FAMILY	  -6		/* `ai_family' not supported.  */
# define EAI_SOCKTYPE	  -7	/* `ai_socktype' not supported.  */
# define EAI_SERVICE	  -8	/* SERVICE not supported for `ai_socktype'.  */
# define EAI_ADDRFAMILY	  -9	/* Address family for NAME not supported.  */
# define EAI_MEMORY	  -10		/* Memory allocation failure.  */
# define EAI_SYSTEM	  -11		/* System error returned in `errno'.  */
# ifdef __USE_GNU
#  define EAI_INPROGRESS  -100	/* Processing request in progress.  */
#  define EAI_CANCELED	  -101	/* Request canceled.  */
#  define EAI_NOTCANCELED -102	/* Request not canceled.  */
#  define EAI_ALLDONE	  -103	/* All requests done.  */
#  define EAI_INTR	  -104		/* Interrupted by a signal.  */
# endif

# define NI_MAXHOST      1025
# define NI_MAXSERV      32

# define NI_NUMERICHOST	1		/* Don't try to look up hostname.  */
# define NI_NUMERICSERV 2		/* Don't convert port number to name.  */
# define NI_NOFQDN	4			/* Only return nodename portion.  */
# define NI_NAMEREQD	8		/* Don't return numeric addresses.  */
# define NI_DGRAM	16			/* Look up UDP service rather than TCP.  */

/* Translate name of a service location and/or a service name to set of
   socket addresses.  */
extern int getaddrinfo(__const char *__restrict __name,
					   __const char *__restrict __service, __const struct addrinfo *__restrict __req, struct addrinfo **__restrict __pai) __THROW;

/* Free `addrinfo' structure AI including associated storage.  */
extern void freeaddrinfo(struct addrinfo *__ai) __THROW;

/* Convert error return from getaddrinfo() to a string.  */
extern const char *gai_strerror(int __ecode) __THROW;

#endif							/* HAVE_GETADDRINFO */

#endif							/* POSIX */

#endif							/* COMPAT_NETDB_H */
