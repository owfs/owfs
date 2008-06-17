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

static int FromServer(int file_descriptor, struct client_msg *cm, char *msg, size_t size);
static void *FromServerAlloc(int file_descriptor, struct client_msg *cm);
//static int ToServer( int file_descriptor, struct server_msg * sm, char * path, char * data, size_t datasize ) ;
static int ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp);
static void Server_setroutines(struct interface_routines *f);
static void Zero_setroutines(struct interface_routines *f);
static void Server_close(struct connection_in *in);
static uint32_t SetupSemi(int persistent, const struct parsedname *pn);
static int ConnectToServer(struct connection_in *in);
static int ToServerTwice(int file_descriptor, int persistent, struct server_msg *sm, struct serverpackage *sp, struct connection_in *in);

static int PersistentStart(int *persistent, struct connection_in *in);
static void PersistentEnd(int file_descriptor, int persistent, int granted, struct connection_in *in);
static void PersistentFree(int file_descriptor, struct connection_in *in);
static void PersistentClear(int file_descriptor, struct connection_in *in);
static int PersistentRequest(struct connection_in *in);
static int PersistentReRequest(int file_descriptor, struct connection_in *in);

int ServerDIRALL(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags);
int ServerDIR(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags);

static void Server_setroutines(struct interface_routines *f)
{
	f->detect = Server_detect;
//    f->reset         =;
//    f->next_both  ;
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
	in->file_descriptor = FD_PERSISTENT_NONE;	// No persistent connection yet
	in->Adapter = adapter_tcp;
	in->adapter_name = "tcp";
	in->busmode = bus_zero;
	in->reconnect_state = reconnect_ok;	// Special since slot reused
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
int ServerRead(struct one_wire_query *owq)
{
	struct server_msg sm;
	struct client_msg cm;
	struct parsedname *pn_file_entry = PN(owq);
	struct serverpackage sp = { pn_file_entry->path_busless, NULL, 0, pn_file_entry->tokenstring,
		pn_file_entry->tokens,
	};
	int persistent = 1;
	int connectfd;
	int ret = 0;

	//printf("ServerRead pn_file_entry->path=%s, size=%d, offset=%u\n",pn_file_entry->path,size,offset);
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = OWQ_size(owq);
	sm.offset = OWQ_offset(owq);

	//printf("ServerRead path=%s\n", pn_file_entry->path_busless);
	LEVEL_CALL("SERVER(%d)READ path=%s\n", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_busless));

	connectfd = PersistentStart(&persistent, pn_file_entry->selected_connection);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent, pn_file_entry);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_file_entry->selected_connection)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, OWQ_buffer(owq), OWQ_size(owq))
				   < 0) {
			ret = -EIO;
		} else {
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_file_entry->selected_connection);
	return ret;
}

// Send to an owserver using the PRESENT message
int ServerPresence(const struct parsedname *pn_file_entry)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_file_entry->path_busless, NULL, 0, pn_file_entry->tokenstring,
		pn_file_entry->tokens,
	};
	int persistent = 1;
	int connectfd;
	int ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_presence;

	//printf("ServerPresence path=%s\n", pn_file_entry->path_busless);
	LEVEL_CALL("SERVER(%d)PRESENCE path=%s\n", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_busless));

	connectfd = PersistentStart(&persistent, pn_file_entry->selected_connection);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent, pn_file_entry);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_file_entry->selected_connection)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			ret = cm.ret;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_file_entry->selected_connection);
	return ret;
}

// Send to an owserver using the WRITE message
int ServerWrite(struct one_wire_query *owq)
{
	struct server_msg sm;
	struct client_msg cm;
	struct parsedname *pn_file_entry = PN(owq);
	struct serverpackage sp = { pn_file_entry->path_busless, (BYTE *) OWQ_buffer(owq),
		OWQ_size(owq), pn_file_entry->tokenstring, pn_file_entry->tokens,
	};
	int persistent = 1;
	int connectfd;
	int ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_write;
	sm.size = OWQ_size(owq);
	sm.offset = OWQ_offset(owq);

	//printf("ServerRead path=%s\n", pn_file_entry->path_busless);
	LEVEL_CALL("SERVER(%d)WRITE path=%s\n", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_busless));

	connectfd = PersistentStart(&persistent, pn_file_entry->selected_connection);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent, pn_file_entry);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_file_entry->selected_connection)) < 0) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			int32_t sg = cm.sg & ~(BUSRET_MASK | PERSISTENT_MASK);
			ret = cm.ret;
            SGLOCK ;
			if (SemiGlobal != sg) {
				//printf("ServerRead: cm.sg changed!  SemiGlobal=%X cm.sg=%X\n", SemiGlobal, cm.sg);
				SemiGlobal = sg;
			}
            SGUNLOCK ;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_file_entry->selected_connection);
	return ret;
}

// Send to an owserver using either the DIR or DIRALL message
int ServerDir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	int ret;
	// Do we know this server doesn't support DIRALL?
	if (pn_whole_directory->selected_connection->connin.tcp.no_dirall) {
		return ServerDIR(dirfunc, v, pn_whole_directory, flags);
	}
	// device directories have lower latency with DIR
	if (IsRealDir(pn_whole_directory) || IsAlarmDir(pn_whole_directory)) {
		return ServerDIR(dirfunc, v, pn_whole_directory, flags);
	}
	// try DIRALL and see if supported
	if ((ret = ServerDIRALL(dirfunc, v, pn_whole_directory, flags)) == -ENOMSG) {
		pn_whole_directory->selected_connection->connin.tcp.no_dirall = 1;
		return ServerDIR(dirfunc, v, pn_whole_directory, flags);
	}
	return ret;
}

// Send to an owserver using the DIR message
int ServerDIR(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_whole_directory->path_busless, NULL, 0,
		pn_whole_directory->tokenstring, pn_whole_directory->tokens,
	};
	int persistent = 1;
	int connectfd;
	int ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dir;

	LEVEL_CALL("SERVER(%d)DIR path=%s path_busless=%s\n",
			   pn_whole_directory->selected_connection->index, SAFESTRING(pn_whole_directory->path), SAFESTRING(pn_whole_directory->path_busless));

	connectfd = PersistentStart(&persistent, pn_whole_directory->selected_connection);
	if (connectfd > FD_CURRENT_BAD) {
		sm.sg = SetupSemi(persistent, pn_whole_directory);
		if ((connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_whole_directory->selected_connection)) < 0) {
			ret = -EIO;
		} else {
			char *return_path;
			size_t devices = 0;
			struct dirblob db;

			/* If cacheable, try to allocate a blob for storage */
			/* only for "read devices" and not alarm */
			DirblobInit(&db);
			if (IsRealDir(pn_whole_directory)
				&& NotAlarmDir(pn_whole_directory)
				&& !SpecifiedBus(pn_whole_directory)
				&& pn_whole_directory->selected_device == NULL) {
				if (RootNotBranch(pn_whole_directory)) {	/* root dir */
					BUSLOCK(pn_whole_directory);
					db.allocated = pn_whole_directory->selected_connection->last_root_devs;	// root dir estimated length
					BUSUNLOCK(pn_whole_directory);
				}
			} else {
				db.troubled = 1;	// no dirblob cache
			}

			while ((return_path = FromServerAlloc(connectfd, &cm))) {
				struct parsedname pn_directory_element;
				return_path[cm.payload - 1] = '\0';	/* Ensure trailing null */
				LEVEL_DEBUG("ServerDir: got=[%s]\n", return_path);

				ret = -EINVAL;

				if (SpecifiedRemoteBus(pn_whole_directory)) {
					// Specified remote bus, add the bus to the path (in front)
					char BigBuffer[cm.payload + 12];
					if (snprintf(BigBuffer, cm.payload + 11, "/bus.%d/%s",
								 pn_whole_directory->selected_connection->index, &return_path[(return_path[0] == '/') ? 1 : 0])
						> 0) {
						ret = FS_ParsedName_BackFromRemote(BigBuffer, &pn_directory_element);
					}
				} else {
					ret = FS_ParsedName_BackFromRemote(return_path, &pn_directory_element);
				}

				free(return_path);

				if (ret) {
					cm.ret = ret;
					break;
				}
				//printf("SERVERDIR path=%s\n",pn_directory_element.path);
				/* we got a device on bus_nr = pn_whole_directory->selected_connection->index. Cache it so we
				   find it quicker next time we want to do read values from the
				   the actual device
				 */
				if (IsRealDir(pn_whole_directory)) {
					/* If we get a device then cache the bus_nr */
					Cache_Add_Device(pn_whole_directory->selected_connection->index, &pn_directory_element);
				}
				/* Add to cache Blob -- snlist is also a flag for cachable */
				if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
					DirblobAdd(pn_directory_element.sn, &db);
				}
				++devices;

				DIRLOCK;
				dirfunc(v, &pn_directory_element);
				DIRUNLOCK;

				FS_ParsedName_destroy(&pn_directory_element);	// destroy the last parsed name
			}
			/* Add to the cache (full list as a single element */
			if (DirblobPure(&db)) {
				Cache_Add_Dir(&db, pn_whole_directory);	// end with a null entry
				if (RootNotBranch(pn_whole_directory)) {
					BUSLOCK(pn_whole_directory);
					pn_whole_directory->selected_connection->last_root_devs = db.devices;	// root dir estimated length
					BUSUNLOCK(pn_whole_directory);
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
	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_whole_directory->selected_connection);
	return ret;
}

// Send to an owserver using the DIRALL message
int ServerDIRALL(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	ASCII *comma_separated_list;
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_whole_directory->path_busless, NULL, 0,
		pn_whole_directory->tokenstring, pn_whole_directory->tokens,
	};
	int persistent = 1;
	int connectfd;
	int ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dirall;

	LEVEL_CALL("SERVER(%d)DIRALL path=%s path_busless=%s\n",
			   pn_whole_directory->selected_connection->index, SAFESTRING(pn_whole_directory->path), SAFESTRING(pn_whole_directory->path_busless));

	// Get a file descriptor, possibly a persistent one
	connectfd = PersistentStart(&persistent, pn_whole_directory->selected_connection);
	if (connectfd < 0) {
		PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_whole_directory->selected_connection);
		return -EIO;
	}
	// Now try to get header. If fails, may need a new non-persistent file_descriptor
	sm.sg = SetupSemi(persistent, pn_whole_directory);
	connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_whole_directory->selected_connection);
	if (connectfd < 0) {
		PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_whole_directory->selected_connection);
		return -EIO;
	}
	// Success, get data
	comma_separated_list = FromServerAlloc(connectfd, &cm);
	LEVEL_DEBUG("DIRALL got %s\n", SAFESTRING(comma_separated_list));
	if (cm.ret == 0) {
		ASCII *current_file;
		ASCII *rest_of_comma_list = comma_separated_list;
		size_t devices = 0;
		struct dirblob db;

		LEVEL_DEBUG("DIRALL start parsing\n");

		/* If cacheable, try to allocate a blob for storage */
		/* only for "read devices" and not alarm */
		DirblobInit(&db);
		if (IsRealDir(pn_whole_directory)
			&& NotAlarmDir(pn_whole_directory)
			&& !SpecifiedBus(pn_whole_directory)
			&& pn_whole_directory->selected_device == NULL) {
			if (RootNotBranch(pn_whole_directory)) {	/* root dir */
				BUSLOCK(pn_whole_directory);
				db.allocated = pn_whole_directory->selected_connection->last_root_devs;	// root dir estimated length
				BUSUNLOCK(pn_whole_directory);
			}
		} else {
			db.troubled = 1;	// no dirblob cache
		}

		while ((current_file = strsep(&rest_of_comma_list, ",")) != NULL) {
			struct parsedname pn_directory_element;
			int path_length = strlen(current_file);
			LEVEL_DEBUG("ServerDirall: got=[%s]\n", current_file);

			ret = -EINVAL;

			if (SpecifiedRemoteBus(pn_whole_directory)) {
				// Specified remote bus, add the bus to the path (in front)
				char BigBuffer[path_length + 12];
				if (snprintf(BigBuffer, path_length + 11, "/bus.%d/%s",
							 pn_whole_directory->selected_connection->index, &current_file[(current_file[0] == '/') ? 1 : 0])
					> 0) {
					ret = FS_ParsedName_BackFromRemote(BigBuffer, &pn_directory_element);
				}
			} else {
				ret = FS_ParsedName_BackFromRemote(current_file, &pn_directory_element);
			}

			if (ret) {
				cm.ret = ret;
				break;
			}
			//printf("SERVERDIR path=%s\n",pn_directory_element.path);
			/* we got a device on bus_nr = pn_whole_directory->selected_connection->index. Cache it so we
			   find it quicker next time we want to do read values from the
			   the actual device
			 */
			if (IsRealDir(pn_whole_directory)) {
				/* If we get a device then cache the bus_nr */
				Cache_Add_Device(pn_whole_directory->selected_connection->index, &pn_directory_element);
			}
			/* Add to cache Blob -- snlist is also a flag for cachable */
			if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
				DirblobAdd(pn_directory_element.sn, &db);
			}
			++devices;
			DIRLOCK;
			dirfunc(v, &pn_directory_element);
			DIRUNLOCK;

			FS_ParsedName_destroy(&pn_directory_element);	// destroy the last parsed name
		}
		/* Add to the cache (full list as a single element */
		if (DirblobPure(&db)) {
			Cache_Add_Dir(&db, pn_whole_directory);	// end with a null entry
			if (RootNotBranch(pn_whole_directory)) {
				BUSLOCK(pn_whole_directory);
				pn_whole_directory->selected_connection->last_root_devs = db.devices;	// root dir estimated length
				BUSUNLOCK(pn_whole_directory);
			}
		}
		DirblobClear(&db);

		DIRLOCK;
		/* flags are sent back in "offset" of final blank entry */
		flags[0] |= cm.offset;
		DIRUNLOCK;

	}
	// free the allocated memory
	if (comma_separated_list != NULL) {
		free(comma_separated_list);
	}
	ret = cm.ret;

	PersistentEnd(connectfd, persistent, cm.sg & PERSISTENT_MASK, pn_whole_directory->selected_connection);
	return ret;
}

/* read from server, free return pointer if not Null */
/* Adds an extra null byte at end */
static void *FromServerAlloc(int file_descriptor, struct client_msg *cm)
{
	char *msg;
	int ret;
	struct timeval tv = { Globals.timeout_network + 1, 0, };

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
	struct timeval tv = { Globals.timeout_network + 1, 0, };

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
    int tokens = 0 ;
    struct iovec io[5] = { {NULL,0}, {NULL,0}, {NULL,0}, {NULL,0}, {NULL,0}, } ;

    // First block to send, the header
    io[0].iov_base = sm ;
    io[0].iov_len = sizeof(struct server_msg) ;

    // Next block, the path
	if ((io[1].iov_base = sp->path)) {	// send path (if not null)
		io[1].iov_len = payload = strlen(sp->path) + 1;
	}

    // next block, data (only for writes)
	if ((io[2].iov_base = sp->data)) {	// send data (if datasize not zero)
		payload += (io[2].iov_len = sp->datasize);
	}

    
    sm->version = MakeServerprotocol(OWSERVER_PROTOCOL_VERSION) ;
	if (Globals.opt == opt_server) {
        tokens = sp->tokens ;
        sm->version |= MakeServermessage ;

        // next block prior tokens (if an owserver)
        if (tokens > 0) {	// owserver: send prior tags
			io[3].iov_base = sp->tokenstring;
			io[3].iov_len = tokens * sizeof(union antiloop);
		}

        // final block, new token (if an owserver)
		++tokens;
		sm->version |= MakeServertokens(tokens);
		io[4].iov_base = &(Globals.Token);	// owserver: add our tag
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

	return writev(file_descriptor, io, 5) != (ssize_t) (payload + sizeof(struct server_msg) + tokens * sizeof(union antiloop));
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
	int file_descriptor;

	file_descriptor = ClientConnect(in);
	if (file_descriptor == FD_CURRENT_BAD) {
		STAT_ADD1(in->reconnect_state);
	}
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
