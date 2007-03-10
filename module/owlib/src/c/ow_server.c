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
#include "ow_connection.h"

static int FromServer(int fd, struct client_msg *cm, char *msg,
					  size_t size);
static void *FromServerAlloc(int fd, struct client_msg *cm);
//static int ToServer( int fd, struct server_msg * sm, char * path, char * data, size_t datasize ) ;
static int ToServer(int fd, struct server_msg *sm,
					struct serverpackage *sp);
static void Server_setroutines(struct interface_routines *f);
static void Zero_setroutines(struct interface_routines *f);
static void Server_close(struct connection_in *in);
static uint32_t SetupSemi(int persistent, const struct parsedname *pn);
static int ConnectToServer(struct connection_in *in);
static int ToServerTwice(int fd, int persistent, struct server_msg *sm,
						 struct serverpackage *sp,
						 struct connection_in *in);

static int PersistentStart(int *persistent, struct connection_in *in);
static void PersistentEnd(int fd, int persistent, int granted,
						  struct connection_in *in);
static void PersistentFree(int fd, struct connection_in *in);
static void PersistentClear(int fd, struct connection_in *in);
static int PersistentRequest(struct connection_in *in);
static int PersistentReRequest(int fd, struct connection_in *in);


static void Server_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
//    f->reset         =;
//    f->next_both  ;
//    f->overdrive = ;
//    f->testoverdrive = ;
//    f->PowerByte     = ;
//    f->ProgramPulse = ;
//    f->sendback_data = ;
//    f->sendback_bits = ;
//    f->select        = ;
	f->reconnect = NULL;
	f->close = Server_close;
}

static void Zero_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
//    f->reset         =;
//    f->next_both  ;
//    f->overdrive = ;
//    f->testoverdrive = ;
//    f->PowerByte     = ;
//    f->ProgramPulse = ;
//    f->sendback_data = ;
//    f->sendback_bits = ;
//    f->select        = ;
	f->reconnect = NULL;
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
	in->fd = -1;				// No persistent connection yet
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	in->busmode = bus_zero;
	in->reconnect_state = reconnect_ok;	// Special since slot reused
	Zero_setroutines(&(in->iroutines));
	return 0;
}

int Server_detect(struct connection_in *in)
{
	if (in->name == NULL)
		return -1;
	if (ClientAddr(in->name, in))
		return -1;
	in->fd = -1;				// No persistent connection yet
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	in->busmode = bus_server;
	Server_setroutines(&(in->iroutines));
	return 0;
}

static void Server_close(struct connection_in *in)
{
	if (in->fd > -1) {			// persistent connection
		close(in->fd);
		in->fd = -1;
	}
	FreeClientAddr(in);
}

int ServerRead(struct one_wire_query * owq)
{
	struct server_msg sm;
	struct client_msg cm;
    struct parsedname * pn = &OWQ_pn(owq) ;
	struct serverpackage sp =
		{ pn->path_busless, NULL, 0, pn->tokenstring, pn->tokens, };
	int persistent = 1;
	int connectfd;
	int ret = 0;

	//printf("ServerRead pn->path=%s, size=%d, offset=%u\n",pn->path,size,offset);
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
    sm.size = OWQ_size(owq);
    sm.offset = OWQ_offset(owq);

	//printf("ServerRead path=%s\n", pn->path_busless);
	LEVEL_CALL("SERVER(%d)READ path=%s\n", pn->in->index,
			   SAFESTRING(pn->path_busless));

	connectfd = PersistentStart(&persistent, pn->in);
	if (connectfd > -1) {
		sm.sg = SetupSemi(persistent, pn);
		if ((connectfd =
			 ToServerTwice(connectfd, persistent, &sm, &sp, pn->in)) < 0) {
			ret = -EIO;
             } else if (FromServer(connectfd, &cm, OWQ_buffer(owq), OWQ_size(owq)) < 0) {
			ret = -EIO;
		} else {
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn->in);
	return ret;
}

int ServerPresence(const struct parsedname *pn)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp =
		{ pn->path_busless, NULL, 0, pn->tokenstring, pn->tokens, };
	int persistent = 1;
	int connectfd;
	int ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_presence;

	//printf("ServerPresence path=%s\n", pn->path_busless);
	LEVEL_CALL("SERVER(%d)PRESENCE path=%s\n", pn->in->index,
			   SAFESTRING(pn->path_busless));

	connectfd = PersistentStart(&persistent, pn->in);
	if (connectfd > -1) {
		sm.sg = SetupSemi(persistent, pn);
		if ((connectfd =
			 ToServerTwice(connectfd, persistent, &sm, &sp, pn->in)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn->in);
	return ret;
}

int ServerWrite(struct one_wire_query * owq )
{
	struct server_msg sm;
	struct client_msg cm;
    struct parsedname * pn = &OWQ_pn(owq) ;
	struct serverpackage sp =
    { pn->path_busless, (BYTE *)OWQ_buffer(owq), OWQ_size(owq), pn->tokenstring, pn->tokens, };
	int persistent = 1;
	int connectfd;
	int ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_write;
    sm.size = OWQ_size(owq);
    sm.offset = OWQ_offset(owq);

	//printf("ServerRead path=%s\n", pn->path_busless);
	LEVEL_CALL("SERVER(%d)WRITE path=%s\n", pn->in->index,
			   SAFESTRING(pn->path_busless));

	connectfd = PersistentStart(&persistent, pn->in);
	if (connectfd > -1) {
		sm.sg = SetupSemi(persistent, pn);
		if ((connectfd =
			 ToServerTwice(connectfd, persistent, &sm, &sp, pn->in)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			int32_t sg = cm.sg & ~(BUSRET_MASK | PERSISTENT_MASK);
			ret = cm.ret;
			if (SemiGlobal != sg) {
				//printf("ServerRead: cm.sg changed!  SemiGlobal=%X cm.sg=%X\n", SemiGlobal, cm.sg);
				CACHELOCK;
				SemiGlobal = sg;
				CACHEUNLOCK;
			}
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn->in);
	return ret;
}

int ServerDir(void (*dirfunc) (void *, const struct parsedname * const),
			  void *v, const struct parsedname *pn, uint32_t * flags)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp =
		{ pn->path_busless, NULL, 0, pn->tokenstring, pn->tokens, };
	int persistent = 1;
	int connectfd;
	int ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dir;

	LEVEL_CALL("SERVER(%d)DIR path=%s\n", pn->in->index,
			   SAFESTRING(pn->path_busless));

	connectfd = PersistentStart(&persistent, pn->in);
	if (connectfd > -1) {
		sm.sg = SetupSemi(persistent, pn);
		if ((connectfd =
			 ToServerTwice(connectfd, persistent, &sm, &sp, pn->in)) < 0) {
			ret = -EIO;
		} else {
			char *path2;
			size_t devices = 0;
			struct parsedname pn2;
			struct dirblob db;

			/* If cacheable, try to allocate a blob for storage */
			/* only for "read devices" and not alarm */
			DirblobInit(&db);
			if (IsRealDir(pn) && NotAlarmDir(pn) && !SpecifiedBus(pn)
				&& pn->dev == NULL) {
				if (RootNotBranch(&pn2)) {	/* root dir */
					BUSLOCK(pn);
					db.allocated = pn->in->last_root_devs;	// root dir estimated length
					BUSUNLOCK(pn);
				}
			} else {
				db.troubled = 1;	// no dirblob cache
			}

			while ((path2 = FromServerAlloc(connectfd, &cm))) {
				path2[cm.payload - 1] = '\0';	/* Ensure trailing null */
				LEVEL_DEBUG("ServerDir: got=[%s]\n", path2);

				if (FS_ParsedName_BackFromRemote(path2, &pn2)) {
					cm.ret = -EINVAL;
					free(path2);
					break;
				}
                //printf("SERVERDIR path=%s\n",pn2.path);
				/* we got a device on bus_nr = pn->in->index. Cache it so we
				   find it quicker next time we want to do read values from the
				   the actual device
				 */
				if (pn2.dev && IsRealDir(&pn2)) {
					/* If we get a device then cache the bus_nr */
					Cache_Add_Device(pn->in->index, &pn2);
				}
				/* Add to cache Blob -- snlist is also a flag for cachable */
				if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
					DirblobAdd(pn2.sn, &db);
				}
				++devices;

				DIRLOCK;
				dirfunc(v, &pn2);
				DIRUNLOCK;

				FS_ParsedName_destroy(&pn2);	// destroy the last parsed name
				free(path2);
			}
			/* Add to the cache (full list as a single element */
			if (DirblobPure(&db)) {
				Cache_Add_Dir(&db, pn);	// end with a null entry
				if (RootNotBranch(&pn2)) {
					BUSLOCK(pn);
					pn->in->last_root_devs = db.devices;	// root dir estimated length
					BUSUNLOCK(pn);
				}
			}
			DirblobClear(&db);

			DIRLOCK;
			/* flags are sent back in "offset" of final blank entry */
			flags[0] |= cm.offset;
			DIRUNLOCK;
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn->in);
	return ret;
}

/* read from server, free return pointer if not Null */
static void *FromServerAlloc(int fd, struct client_msg *cm)
{
	char *msg;
	int ret;
	struct timeval tv = { Global.timeout_network + 1, 0, };

	do {						/* loop until non delay message (payload>=0) */
		//printf("OW_SERVER loop1\n");
		ret = tcp_read(fd, cm, sizeof(struct client_msg), &tv);
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
	if (cm->payload > 65000) {
//printf("FromServerAlloc payload too large\n");
		return NULL;
	}

	if ((msg = (char *) malloc((size_t) cm->payload))) {
		ret = tcp_read(fd, msg, (size_t) (cm->payload), &tv);
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
	return msg;
}

/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static int FromServer(int fd, struct client_msg *cm, char *msg,
					  size_t size)
{
	size_t rtry;
	size_t ret;
	struct timeval tv = { Global.timeout_network + 1, 0, };

	do {						// read regular header, or delay (delay when payload<0)
		//printf("OW_SERVER loop2\n");
		ret = tcp_read(fd, cm, sizeof(struct client_msg), &tv);
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
	ret = tcp_read(fd, msg, rtry, &tv);	// read expected payload now.
	if (ret != rtry) {
		cm->ret = -EIO;
		return -EIO;
	}

	if (cm->payload > (ssize_t) size) {	// Uh oh. payload bigger than expected. read it in and discard
		size_t d = cm->payload - size;
		char extra[d];
		ret = tcp_read(fd, extra, d, &tv);
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
static int ToServerTwice(int fd, int persistent, struct server_msg *sm,
						 struct serverpackage *sp,
						 struct connection_in *in)
{
	int newfd;
	if (ToServer(fd, sm, sp) == 0)
		return fd;
	if (persistent == 0) {
		close(fd);
		return -1;
	}
	newfd = PersistentReRequest(fd, in);
	if (newfd < 0)
		return -1;
	if (ToServer(newfd, sm, sp) == 0)
		return newfd;
	close(newfd);
	return -1;
}

// should be const char * data but iovec has problems with const arguments
//static int ToServer( int fd, struct server_msg * sm, const char * path, const char * data, int datasize ) {
static int ToServer(int fd, struct server_msg *sm,
					struct serverpackage *sp)
{
	int payload = 0;
	struct iovec io[5] = {
		{sm, sizeof(struct server_msg),},	// send "server message" header structure
	};
	if ((io[1].iov_base = sp->path)) {	// send path (if not null)
		io[1].iov_len = payload = strlen(sp->path) + 1;
	} else {
		io[1].iov_len = payload = 0;
	}
	if ((io[2].iov_base = sp->data)) {	// send data (if datasize not zero)
		payload += (io[2].iov_len = sp->datasize);
	} else {
		io[2].iov_len = 0;
	}
	if (Global.opt != opt_server) {	// if not being called from owserver, that's it
		io[3].iov_base = io[4].iov_base = NULL;
		io[3].iov_len = io[4].iov_len = 0;
		sp->tokens = 0;
		sm->version = 0;
	} else {
		if (sp->tokens > 0) {	// owserver: send prior tags
			io[3].iov_base = sp->tokenstring;
			io[3].iov_len = sp->tokens * sizeof(union antiloop);
		} else {
			io[3].iov_base = NULL;
			io[3].iov_len = 0;
		}
		++sp->tokens;
		sm->version = Servermessage + (sp->tokens);
		io[4].iov_base = &(Global.Token);	// owserver: add our tag
		io[4].iov_len = sizeof(union antiloop);
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

	return writev(fd, io, 5) !=
		(ssize_t) (payload + sizeof(struct server_msg) +
				   sp->tokens * sizeof(union antiloop));
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupSemi(int persistent, const struct parsedname *pn)
{
	uint32_t sg = pn->sg;

	sg &= ~PERSISTENT_MASK;
	if (persistent) {
		sg |= PERSISTENT_MASK;
	}

	sg &= ~BUSRET_MASK;
	if (SpecifiedBus(pn)) {
		sg |= BUSRET_MASK;
	}

	return sg;
}

/* Wrapper for ClientConnect */
static int ConnectToServer(struct connection_in *in)
{
	int fd;

	fd = ClientConnect(in);
	if (fd < 0) {
		STAT_ADD1(in->reconnect_state);
	}
	return fd;
}

/* Request a persistent connection
   Three possibilities:
     1. no persistent connection currently (in->fd = -1)
        create one, flag in->fd as -2, and return fd
        or return -1 if a new one can't be created
     2. persistent available (in->fd > -1 )
        use it, flag in->fd as -2, and return in->fd
     3. in use, (in->fd = -2)
        return -1
*/
static int PersistentRequest(struct connection_in *in)
{
	int fd;
	BUSLOCKIN(in);
	if (in->fd == -2) {			// in use
		fd = -1;
	} else if (in->fd > -1) {	// available
		fd = in->fd;
		in->fd = -2;
	} else if ((fd = ConnectToServer(in)) == -1) {	// can't get a connection
		fd = -1;
	} else {					// new connection -- make it persistent
		in->fd = -2;
	}
	BUSUNLOCKIN(in);
	return fd;
}

/* A persistent connection didn't work (probably expired on the other end
   recreate it, or clear and return -1
 */
static int PersistentReRequest(int fd, struct connection_in *in)
{
	close(fd);
	BUSLOCKIN(in);
	fd = ConnectToServer(in);
	if (fd == -1) {				// bad connection
		in->fd = -1;
	}
	// else leave as in->fd = -2
	BUSUNLOCKIN(in);
	return fd;
}

/* Clear a persistent connection */
static void PersistentClear(int fd, struct connection_in *in)
{
	if (fd > -1) {
		close(fd);
	}
	BUSLOCKIN(in);
	in->fd = -1;
	BUSUNLOCKIN(in);
}

/* Free a persistent connection */
static void PersistentFree(int fd, struct connection_in *in)
{
	if (fd == -1) {
		PersistentClear(fd, in);
	} else {
		BUSLOCKIN(in);
		in->fd = fd;
		BUSUNLOCKIN(in);
	}
}

/* All the startup code
   fd will get the file descriptor
   persistent starts 0 or 1 for whether persistence is wanted
   persistent returns 0 or 1 for whether persistence is granted
*/
static int PersistentStart(int *persistent, struct connection_in *in)
{
	int fd;
	if (*persistent == 0) {
		fd = ConnectToServer(in);
		*persistent = 0;
	} else if ((fd = PersistentRequest(in)) == -1) {	// tried but failed
		fd = ConnectToServer(in);
		*persistent = 0;
	} else {
		*persistent = 1;
	}
	return fd;
}

/* Clean up at end of routine,
   either leave connection open and persistent flag available,
   or close and leave available
*/
static void PersistentEnd(int fd, int persistent, int granted,
						  struct connection_in *in)
{
	if (persistent == 0) {
		close(fd);
	} else if (granted == 0) {
		PersistentClear(fd, in);
	} else {
		PersistentFree(fd, in);
	}
}
