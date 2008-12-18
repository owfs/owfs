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

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_server.h"

static int FromServer(int file_descriptor, struct client_msg *cm, char *msg, size_t size);
static void *FromServerAlloc(int file_descriptor, struct client_msg *cm);
static int ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp);
static void Server_setroutines(struct interface_routines *f);
static void Zero_setroutines(struct interface_routines *f);
static void Server_close(struct connection_in *in);
static uint32_t SetupSemi(int persistent);
static int ConnectToServer(struct connection_in *in);
static int ToServerTwice(int file_descriptor, int persistent, struct server_msg *sm, struct serverpackage *sp, struct connection_in *in);

static int PersistentStart(int *persistent, struct connection_in *in);
static void PersistentEnd(int file_descriptor, int persistent, int granted, struct connection_in *in);
static void PersistentFree(int file_descriptor, struct connection_in *in);
static void PersistentClear(int file_descriptor, struct connection_in *in);
static int PersistentRequest(struct connection_in *in);
static int PersistentReRequest(int file_descriptor, struct connection_in *in);

static int ServerDIR(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp);
static int ServerDIRALL(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp);

static void Server_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
	f->close = Server_close;
}

static void Zero_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
	f->close = Server_close;
}

// bus_zero is a server found by zeroconf/Bonjour
// It differs in that the server must respond
int Zero_detect(struct connection_in *in)
{
//    if ( Server_detect(in) || ServerNOP(in) )
//        if ( ServerNOP(in) ) return -1 ;
	if (in->name == NULL)
		return -1;
	if (ClientAddr(in->name, in))
		return -1;
	in->file_descriptor = FD_PERSISTENT_NONE;	// No persistent connection yet
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	in->busmode = bus_zero;
	Zero_setroutines(&(in->iroutines));
	return 0;
}

// Set up inbound connection to an owserver
// Actual tcp connection created as needed
int Server_detect(struct connection_in *in)
{
	if (in->name == NULL)
		return -1;
	if (ClientAddr(in->name, in))
		return -1;
	in->file_descriptor = FD_PERSISTENT_NONE;	// No persistent connection yet
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	in->busmode = bus_server;
	Server_setroutines(&(in->iroutines));
	return 0;
}

// Free up the owserver inbound connection
// actual connections opened and closed independently
static void Server_close(struct connection_in *in)
{
	if (in->file_descriptor > FD_PERSISTENT_NONE) {	// persistent connection
		close(in->file_descriptor);
		in->file_descriptor = FD_PERSISTENT_NONE;
	}
	FreeClientAddr(in);
}

// Send to an owserver using the READ message
int ServerRead(struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	int connectfd;
	int ret = 0;

	//printf("ServerRead pn_file_entry->path=%s, size=%d, offset=%u\n",pn_file_entry->path,size,offset);
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = rp->data_length;
	sm.offset = rp->data_offset;

	//printf("ServerRead path=%s\n", pn_file_entry->path_busless);
	LEVEL_CALL("SERVER READ path=%s\n", SAFESTRING(rp->path));

	connectfd = PersistentStart(&persistent, rp->owserver);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, rp->owserver)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, (ASCII *) rp->read_value, rp->data_length) < 0) {
			ret = -EIO;
		} else {
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
	return ret;
}

// Send to an owserver using the PRESENT message
int ServerPresence(struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	int connectfd;
	int ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_presence;

	//printf("ServerPresence path=%s\n", pn_file_entry->path_busless);
	LEVEL_CALL("SERVER PRESENCE path=%s\n", SAFESTRING(rp->path));

	connectfd = PersistentStart(&persistent, rp->owserver);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, rp->owserver)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
	return ret;
}

// Send to an owserver using the WRITE message
int ServerWrite(struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, (BYTE *) rp->write_value, rp->data_length, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	int connectfd;
	int ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_write;
	sm.size = rp->data_length;
	sm.offset = rp->data_offset;

	//printf("ServerRead path=%s\n", pn_file_entry->path_busless);
	LEVEL_CALL("SERVER WRITE path=%s\n", SAFESTRING(rp->path));

	connectfd = PersistentStart(&persistent, rp->owserver);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, rp->owserver)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			uint32_t sg = cm.sg & ~(BUSRET_MASK | PERSISTENT_MASK);
			ret = cm.ret;
			if (ow_Global.sg != sg) {
				//printf("ServerRead: cm.sg changed!  SemiGlobal=%X cm.sg=%X\n", SemiGlobal, cm.sg);
				ow_Global.sg = sg;
			}
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
	return ret;
}

// Send to an owserver using either the DIR or DIRALL message
int ServerDir(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp)
{
	int ret;
	// Do we know this server doesn't support DIRALL?
	if (rp->owserver->connin.tcp.no_dirall) {
		return ServerDIR(dirfunc, v, rp);
	}
	// try DIRALL and see if supported
	if ((ret = ServerDIRALL(dirfunc, v, rp)) == -ENOMSG) {
		rp->owserver->connin.tcp.no_dirall = 1;
		return ServerDIR(dirfunc, v, rp);
	}
	return ret;
}

// Send to an owserver using the DIR message
static int ServerDIR(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	int connectfd;
	int ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dir;

	LEVEL_CALL("SERVER DIR path=%s\n", SAFESTRING(rp->path));

	connectfd = PersistentStart(&persistent, rp->owserver);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, rp->owserver)) < 0) {
			ret = -EIO;
		} else {
			char *return_path;

			while ((return_path = FromServerAlloc(connectfd, &cm))) {
				return_path[cm.payload - 1] = '\0';	/* Ensure trailing null */
				LEVEL_DEBUG("ServerDir: got=[%s]\n", return_path);
				dirfunc(v, return_path);
				free(return_path);
			}

			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
	return ret;
}

// Send to an owserver using the DIRALL message
static int ServerDIRALL(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp)
{
	ASCII *comma_separated_list;
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	int connectfd;
	int ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dirall;

	LEVEL_CALL("SERVER DIRALL path=%s\n", SAFESTRING(rp->path));

	// Get a file descriptor, possibly a persistent one
	connectfd = PersistentStart(&persistent, rp->owserver);
	if (connectfd < 0) {
		PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
		return -EIO;
	}
	// Now try to get header. If fails, may need a new non-persistent file_descriptor
	sm.sg = SetupSemi(persistent);
	connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, rp->owserver);
	if (connectfd < 0) {
		PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
		return -EIO;
	}
	// Success, get data
	comma_separated_list = FromServerAlloc(connectfd, &cm);
	LEVEL_DEBUG("DIRALL got %s\n", SAFESTRING(comma_separated_list));
	if (cm.ret == 0) {
		ASCII *current_file;
		ASCII *rest_of_comma_list = comma_separated_list;

		LEVEL_DEBUG("DIRALL start parsing\n");

		while ((current_file = strsep(&rest_of_comma_list, ",")) != NULL) {
			LEVEL_DEBUG("ServerDirall: got=[%s]\n", current_file);
			dirfunc(v, current_file);
		}
	}
	// free the allocated memory
	if (comma_separated_list != NULL) {
		free(comma_separated_list);
	}
	ret = cm.ret;

	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, rp->owserver);
	return ret;
}

/* read from server, free return pointer if not Null */
/* Adds an extra null byte at end */
static void *FromServerAlloc(int file_descriptor, struct client_msg *cm)
{
	char *msg;
	int ret;
	struct timeval tv = { ow_Global.timeout_network + 1, 0, };

	do {						/* loop until non delay message (payload>=0) */
		//printf("OW_SERVER loop1\n");
		ret = tcp_read(file_descriptor, cm, sizeof(struct client_msg), &tv);
		if (ret != sizeof(struct client_msg)) {
			memset(cm, 0, sizeof(struct client_msg));
			cm->ret = -EIO;
			return NULL;
		}
		cm->payload = ntohl(cm->payload);
		cm->size = ntohl(cm->size);
		cm->ret = ntohl(cm->ret);
		cm->sg = ntohl(cm->sg);
		cm->offset = ntohl(cm->offset);
	} while (cm->payload < 0);
	//printf("OW_SERVER loop1 done\n");

	//printf("FromServerAlloc payload=%d size=%d ret=%d sg=%X offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
	//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
	if (cm->payload == 0)
		return NULL;
	if (cm->ret < 0)
		return NULL;
	if (cm->payload > MAX_OWSERVER_PROTOCOL_PACKET_SIZE) {
		//printf("FromServerAlloc payload too large\n");
		return NULL;
	}

	if ((msg = (char *) malloc((size_t) cm->payload + 1))) {
		ret = tcp_read(file_descriptor, msg, (size_t) (cm->payload), &tv);
		if (ret != cm->payload) {
			//printf("FromServer couldn't read payload\n");
			cm->payload = 0;
			cm->offset = 0;
			cm->ret = -EIO;
			free(msg);
			msg = NULL;
		}
		//printf("FromServer payload read ok\n");
	}
	msg[cm->payload] = '\0';	// safety NULL
	return msg;
}

/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static int FromServer(int file_descriptor, struct client_msg *cm, char *msg, size_t size)
{
	size_t rtry;
	size_t ret;
	struct timeval tv = { ow_Global.timeout_network + 1, 0, };

	do {						// read regular header, or delay (delay when payload<0)
		//printf("OW_SERVER loop2\n");
		ret = tcp_read(file_descriptor, cm, sizeof(struct client_msg), &tv);
		if (ret != sizeof(struct client_msg)) {
			//printf("OW_SERVER loop2 bad\n");
			cm->size = 0;
			cm->ret = -EIO;
			return -EIO;
		}

		cm->payload = ntohl(cm->payload);
		cm->size = ntohl(cm->size);
		cm->ret = ntohl(cm->ret);
		cm->sg = ntohl(cm->sg);
		cm->offset = ntohl(cm->offset);
	} while (cm->payload < 0);	// flag to show a delay message
	//printf("OW_SERVER loop2 done\n");

	//printf("FromServer payload=%d size=%d ret=%d sg=%d offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
	//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
	if (cm->payload == 0)
		return 0;				// No payload, done.

	rtry = cm->payload < (ssize_t) size ? (size_t) cm->payload : size;
	ret = tcp_read(file_descriptor, msg, rtry, &tv);	// read expected payload now.
	if (ret != rtry) {
		cm->ret = -EIO;
		return -EIO;
	}

	if (cm->payload > (ssize_t) size) {	// Uh oh. payload bigger than expected. read it in and discard
		size_t d = cm->payload - size;
		char extra[d];
		ret = tcp_read(file_descriptor, extra, d, &tv);
		if (ret != d) {
			cm->ret = -EIO;
			return -EIO;
		}
		return size;
	}
	return cm->payload;
}

/* Send a message to server, or try a new connection and send again */
/* return file descriptor */
static int ToServerTwice(int file_descriptor, int persistent, struct server_msg *sm, struct serverpackage *sp, struct connection_in *in)
{
	int newfd;
	if (ToServer(file_descriptor, sm, sp) == 0)
		return file_descriptor;
	if (persistent == 0) {
		close(file_descriptor);
		return FD_CURRENT_BAD;
	}
	newfd = PersistentReRequest(file_descriptor, in);
	if (newfd < 0)
		return FD_CURRENT_BAD;
	if (ToServer(newfd, sm, sp) == 0)
		return newfd;
	close(newfd);
	return FD_CURRENT_BAD;
}

// should be const char * data but iovec has problems with const arguments
static int ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp)
{
	int payload = 0;
	int nio = 0;
	struct iovec io[5] = { {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, };

	// First block to send, the header
	io[nio].iov_base = sm;
	io[nio].iov_len = sizeof(struct server_msg);
	nio++;

	// Next block, the path
	if ((io[nio].iov_base = sp->path)) {	// send path (if not null)
		io[nio].iov_len = payload = strlen(sp->path) + 1;
		nio++;
		LEVEL_DEBUG("ToServer path=%s\n", sp->path);
	}

	// next block, data (only for writes)
	if ((sp->datasize>0) && (io[nio].iov_base = sp->data)) {	// send data only for writes (if datasize not zero)
		payload += (io[nio].iov_len = sp->datasize);
		nio++;
        LEVEL_DEBUG("ToServer data=%s\n", sp->data);
	}

	if (1) {					// if not being called from owserver, that's it
		sp->tokens = 0;
		sm->version = 0; // shouldn't this be MakeServerprotocol(OWSERVER_PROTOCOL_VERSION);
	} else {
		/* I guess this can't happen since it's ownet */
		if (sp->tokens > 0) {	// owserver: send prior tags
			io[nio].iov_base = sp->tokenstring;
			io[nio].iov_len = sp->tokens * sizeof(union antiloop);
			nio++;
		}
		++sp->tokens;
		sm->version = Servermessage + (sp->tokens);
		io[nio].iov_base = &(Globals.Token);	// owserver: add our tag
		io[nio].iov_len = sizeof(union antiloop);
		nio++;
	}

	//printf("ToServer payload=%d size=%d type=%d tempscale=%X offset=%d\n",payload,sm->size,sm->type,sm->sg,sm->offset);
	//printf("<%.4d|%.4d\n",sm->type,payload);
	//printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(sm->sg)));

	// encode in network order (just the header)
	sm->version = htonl(sm->version);
	sm->payload = htonl(payload);
	sm->size = htonl(sm->size);
	sm->type = htonl(sm->type);
	sm->sg = htonl(sm->sg);
	sm->offset = htonl(sm->offset);

	Debug_Writev(io, nio);
	return writev(file_descriptor, io, nio) != (ssize_t) (payload + sizeof(struct server_msg) + sp->tokens * sizeof(union antiloop));
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupSemi(int persistent)
{
	uint32_t sg = ow_Global.sg;

	sg &= ~PERSISTENT_MASK;
	if (persistent) {
		sg |= PERSISTENT_MASK;
	}

	sg |= BUSRET_MASK;

	return sg;
}

/* Wrapper for ClientConnect */
static int ConnectToServer(struct connection_in *in)
{
	int file_descriptor;

	file_descriptor = ClientConnect(in);
	return file_descriptor;
}

/* Request a persistent connection
   Three possibilities:
     1. no persistent connection currently (in->file_descriptor = -1)
        create one, flag in->file_descriptor as -2, and return file_descriptor
        or return -1 if a new one can't be created
     2. persistent available (in->file_descriptor > -1 )
        use it, flag in->file_descriptor as -2, and return in->file_descriptor
     3. in use, (in->file_descriptor = -2)
        return -1
*/
static int PersistentRequest(struct connection_in *in)
{
	int file_descriptor;
	BUSLOCKIN(in);
	if (in->file_descriptor == FD_PERSISTENT_IN_USE) {	// in use
		file_descriptor = FD_CURRENT_BAD;
	} else if (in->file_descriptor > FD_PERSISTENT_NONE) {	// available
		file_descriptor = in->file_descriptor;
		in->file_descriptor = FD_PERSISTENT_IN_USE;
	} else if ((file_descriptor = ConnectToServer(in)) == FD_CURRENT_BAD) {	// can't get a connection
		file_descriptor = FD_CURRENT_BAD;
	} else {					// new connection -- make it persistent
		in->file_descriptor = FD_PERSISTENT_IN_USE;
	}
	BUSUNLOCKIN(in);
	return file_descriptor;
}

/* A persistent connection didn't work (probably expired on the other end
   recreate it, or clear and return -1
 */
static int PersistentReRequest(int file_descriptor, struct connection_in *in)
{
	close(file_descriptor);
	BUSLOCKIN(in);
	file_descriptor = ConnectToServer(in);
	if (file_descriptor == FD_CURRENT_BAD) {	// bad connection
		in->file_descriptor = FD_PERSISTENT_NONE;
	}
	// else leave as in->file_descriptor = -2
	BUSUNLOCKIN(in);
	return file_descriptor;
}

/* Clear a persistent connection */
static void PersistentClear(int file_descriptor, struct connection_in *in)
{
	if (file_descriptor > FD_CURRENT_BAD) {
		close(file_descriptor);
	}
	BUSLOCKIN(in);
	in->file_descriptor = FD_PERSISTENT_NONE;
	BUSUNLOCKIN(in);
}

/* Free a persistent connection */
static void PersistentFree(int file_descriptor, struct connection_in *in)
{
	if (file_descriptor == FD_CURRENT_BAD) {
		PersistentClear(file_descriptor, in);
	} else {
		BUSLOCKIN(in);
		in->file_descriptor = file_descriptor;
		BUSUNLOCKIN(in);
	}
}

/* All the startup code
   file_descriptor will get the file descriptor
   persistent starts 0 or 1 for whether persistence is wanted
   persistent returns 0 or 1 for whether persistence is granted
*/
static int PersistentStart(int *persistent, struct connection_in *in)
{
	int file_descriptor;
	if (*persistent == 0) {		// no persistence wanted
		file_descriptor = ConnectToServer(in);
		*persistent = 0;		// still not persistent
	} else if ((file_descriptor = PersistentRequest(in)) == FD_CURRENT_BAD) {	// tried but failed
		file_descriptor = ConnectToServer(in);	// non-persistent backup request
		*persistent = 0;		// not persistent
	} else {					// successfully
		*persistent = 1;		// flag as persistent
	}
	return file_descriptor;
}

/* Clean up at end of routine,
   either leave connection open and persistent flag available,
   or close and leave available
*/
static void PersistentEnd(int file_descriptor, int persistent, int granted, struct connection_in *in)
{
	if (persistent == 0) {		// non-persistence from the start
		close(file_descriptor);
	} else if (granted == 0) {	// not granted
		PersistentClear(file_descriptor, in);
	} else {					// Let the persistent connection be used
		PersistentFree(file_descriptor, in);
	}
}
