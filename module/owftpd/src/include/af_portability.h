#ifndef AF_PORTABILITY_H
#define AF_PORTABILITY_H

#include <netinet/in.h>
#include <sys/socket.h>

/* _x_ must be a pointer to a sockaddr structure */

#define SAFAM(_x_)	(((struct sockaddr *)(_x_))->sa_family)

#ifdef HAVE_NEW_SS_FAMILY
#define SSFAM(_x_)	(((struct sockaddr_storage *)(_x_))->ss_family)
#else
#define SSFAM(_x_)	(((struct sockaddr_storage *)(_x_))->__ss_family)
#endif

#define SIN4ADDR(_x_)	(((struct sockaddr_in *)(_x_))->sin_addr)
#define SIN4PORT(_x_)	(((struct sockaddr_in *)(_x_))->sin_port)
#define SIN6ADDR(_x_)	(((struct sockaddr_in6 *)(_x_))->sin6_addr)
#define SIN6PORT(_x_)	(((struct sockaddr_in6 *)(_x_))->sin6_port)

#ifdef INET6
#define SINADDR(_x_)	((SAFAM(_x_)==AF_INET6) ? SIN6ADDR(_x_) : SIN4ADDR(_x_))
#define SINPORT(_x_)	((SAFAM(_x_)==AF_INET6) ? SIN6PORT(_x_) : SIN4PORT(_x_))
#else
#define SINADDR(_x_)    SIN4ADDR(_x_)
#define SINPORT(_x_)    SIN4PORT(_x_)
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46 
#endif

#ifdef INET6
#define IP6_ADDRSTRLEN INET6_ADDRSTRLEN
#define IP4_ADDRSTRLEN INET_ADDRSTRLEN
#define IP_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#define IP_ADDRSTRLEN INET_ADDRSTRLEN
#endif

#ifdef INET6
typedef struct sockaddr_storage sockaddr_storage_t;
#else
typedef struct sockaddr_in sockaddr_storage_t;
#endif 

#endif /* AF_PORTABILITY_H */
