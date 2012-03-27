/*
$Id$
   OWFS and OWHTTPD
   one-wire file system and
   one-wire web server

    By Paul H Alfille
    {c} 2003 GPL
    palfille@earthlink.net
*/

/* OWSERVER - specific header */

#ifndef OWSERVER_H
#define OWSERVER_H

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

pthread_mutex_t persistence_mutex ;
#define PERSISTENCELOCK    _MUTEX_LOCK(   persistence_mutex ) ;
#define PERSISTENCEUNLOCK  _MUTEX_UNLOCK( persistence_mutex ) ;

#define TOCLIENTLOCK(hd) _MUTEX_LOCK( (hd)->to_client )
#define TOCLIENTUNLOCK(hd) _MUTEX_UNLOCK( (hd)->to_client )

enum toclient_state {
	toclient_postping , // also initial state
	toclient_postmessage, // only for interme4diate messages like DIR entries
	toclient_complete, // final payload has been sent
} ;

// this structure holds the data needed for the handler function called in a separate thread by the ping wrapper
struct handlerdata {
	int file_descriptor;
	int persistent;
	pthread_mutex_t to_client;
	int ping_pipe[2] ;
	enum toclient_state toclient ;
	struct timeval tv;
	struct server_msg sm;
	struct serverpackage sp;
};

/* read from client, free return pointer if not Null */
int FromClient(struct handlerdata *hd);

/* Send fully configured message back to client */
int ToClient(int file_descriptor, struct client_msg *cm, char *data);

/* Read from 1-wire bus and return file contents */
void *ReadHandler(struct handlerdata *hd, struct client_msg *cm, struct one_wire_query *owq);

/* write a new value ot a 1-wire device */
void WriteHandler(struct handlerdata *hd, struct client_msg *cm, struct one_wire_query *owq);

/* Clasic directory -- one value at a time */
void DirHandler(struct handlerdata *hd, struct client_msg *cm, const struct parsedname *pn);

/* Newer directory-at-once */
void *DirallHandler(struct handlerdata *hd, struct client_msg *cm, const struct parsedname *pn);

/* Newer directory-at-once with directory '/' */
void *DirallslashHandler(struct handlerdata *hd, struct client_msg *cm, const struct parsedname *pn);

/* Handle the actual request -- pings handled higher up */
void *DataHandler(void *v);

/* Handle a client request, including timeout pings */
void Handler(FILE_DESCRIPTOR_OR_ERROR file_descriptor);

/* Send a response to client of an error */
void ErrorToClient(struct handlerdata *hd, struct client_msg * cm ) ;

#if OW_MT

/* Send a timeout ping */
void PingClient(struct handlerdata *hd);

/* Loop waiting for finish sending pings */
void PingLoop(struct handlerdata *hd) ;

#endif /* OW_MT */

#endif							/* OWSERVER_H */
