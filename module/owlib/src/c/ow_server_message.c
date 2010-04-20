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
#include "ow_standard.h" // for FS_Alias

enum persistent_state { persistent_yes, persistent_no, } ;

static SIZE_OR_ERROR FromServer(int file_descriptor, struct client_msg *cm, char *msg, size_t size);
static void *FromServerAlloc(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct client_msg *cm);
static SIZE_OR_ERROR ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp);
static uint32_t SetupControlFlags(enum persistent_state persistent, const struct parsedname *pn);
static FILE_DESCRIPTOR_OR_ERROR ConnectToServer(struct connection_in *in);
static FILE_DESCRIPTOR_OR_ERROR ToServerTwice(FILE_DESCRIPTOR_OR_ERROR file_descriptor, enum persistent_state persistent, struct server_msg *sm, struct serverpackage *sp, struct connection_in *in);

static FILE_DESCRIPTOR_OR_ERROR PersistentStart(enum persistent_state *persistent, struct connection_in *in);
static void PersistentEnd(FILE_DESCRIPTOR_OR_ERROR file_descriptor, enum persistent_state persistent, int granted, struct connection_in *in);
static void PersistentFree(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct connection_in *in);
static void PersistentClear(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct connection_in *in);
static FILE_DESCRIPTOR_OR_ERROR PersistentRequest(struct connection_in *in);
static FILE_DESCRIPTOR_OR_ERROR PersistentReRequest(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct connection_in *in);

static ZERO_OR_ERROR ServerDIRALL(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags);
static ZERO_OR_ERROR ServerDIR(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags);

// Send to an owserver using the READ message
SIZE_OR_ERROR ServerRead(struct one_wire_query *owq)
{
	struct server_msg sm;
	struct client_msg cm;
	struct parsedname *pn_file_entry = PN(owq);
	struct serverpackage sp = { pn_file_entry->path_busless, NULL, 0, pn_file_entry->tokenstring,
		pn_file_entry->tokens,
	};
	enum persistent_state persistent = persistent_yes;
	FILE_DESCRIPTOR_OR_ERROR connectfd;
	SIZE_OR_ERROR ret = 0;

	// Special handling of alias property. alias should only be asked on local machine.
	if ( pn_file_entry->selected_filetype->change == fc_alias ) {
		return FS_alias(owq) ;
	}

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = OWQ_size(owq);
	sm.offset = OWQ_offset(owq);

	LEVEL_CALL("SERVER(%d) path=%s", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_busless));

	connectfd = PersistentStart(&persistent, pn_file_entry->selected_connection);
	if ( FILE_DESCRIPTOR_VALID( connectfd ) ) {
		sm.control_flags = SetupControlFlags(persistent, pn_file_entry);
		connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_file_entry->selected_connection) ;
		if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
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
	PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_file_entry->selected_connection);
	return ret;
}

// Send to an owserver using the PRESENT message
INDEX_OR_ERROR ServerPresence( BYTE * sn, const struct parsedname *pn_file_entry)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_file_entry->path_busless, NULL, 0, pn_file_entry->tokenstring,
		pn_file_entry->tokens,
	};
	BYTE * serial_number ;
	enum persistent_state persistent = persistent_yes;
	FILE_DESCRIPTOR_OR_ERROR connectfd;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_presence;

	LEVEL_CALL("SERVER(%d) path=%s", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_busless));

	connectfd = PersistentStart(&persistent, pn_file_entry->selected_connection);
	if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
		PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_file_entry->selected_connection);
		return -EIO ;
	}

	sm.control_flags = SetupControlFlags(persistent, pn_file_entry);
	connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_file_entry->selected_connection) ;
	if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
		PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_file_entry->selected_connection);
		return -EIO ;
	}

	serial_number = (BYTE *) FromServerAlloc( connectfd, &cm) ;
	if (cm.ret < 0) {
		PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_file_entry->selected_connection);
		return -EIO ;
	}
	if ( serial_number ) {
		// this is a newer owserver that adds serial number
		if ( AliasPath(pn_file_entry) ) {
			// this is an alias that was analyzed
			char * device = strrchr(pn_file_entry->path_busless,'/') ;
			if ( device != NULL ) {
				// Add the Alias to the database
				Cache_Add_Alias( device+1, serial_number ) ;
			}
		}
		memcpy( sn, serial_number, SERIAL_NUMBER_SIZE ) ;
		owfree( serial_number) ;
	} else {
		memcpy( sn, pn_file_entry->sn, SERIAL_NUMBER_SIZE ) ;
	}

	PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_file_entry->selected_connection);
	return cm.ret;
}

// Send to an owserver using the WRITE message
ZERO_OR_ERROR ServerWrite(struct one_wire_query *owq)
{
	struct server_msg sm;
	struct client_msg cm;
	struct parsedname *pn_file_entry = PN(owq);
	struct serverpackage sp = { pn_file_entry->path_busless, (BYTE *) OWQ_buffer(owq),
		OWQ_size(owq), pn_file_entry->tokenstring, pn_file_entry->tokens,
	};
	enum persistent_state persistent = persistent_yes;
	FILE_DESCRIPTOR_OR_ERROR connectfd;
	ZERO_OR_ERROR ret = 0;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_write;
	sm.size = OWQ_size(owq);
	sm.offset = OWQ_offset(owq);

	LEVEL_CALL("SERVER(%d) path=%s", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_busless));

	connectfd = PersistentStart(&persistent, pn_file_entry->selected_connection);
	if ( FILE_DESCRIPTOR_VALID( connectfd ) ) {
		sm.control_flags = SetupControlFlags(persistent, pn_file_entry);
		connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_file_entry->selected_connection) ;
		if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
			ret = -EIO;
		} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
			ret = -EIO;
		} else {
			int32_t control_flags = cm.control_flags & ~(SHOULD_RETURN_BUS_LIST | PERSISTENT_MASK | SAFEMODE);
			// keep current safemode
			control_flags |=  LocalControlFlags & SAFEMODE ;
			ret = cm.ret;
			CONTROLFLAGSLOCK;
			if (LocalControlFlags != control_flags) {
				// replace control flags (except safemode persists)
				//printf("ServerRead: cm.control_flags changed!  controlflags=%X cm.control_flags=%X\n", SemiGlobal, cm.control_flags);
				LocalControlFlags = control_flags;
			}
			CONTROLFLAGSUNLOCK;
		}
	} else {
		ret = -EIO;
	}
	PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_file_entry->selected_connection);
	return ret;
}

// Send to an owserver using either the DIR or DIRALL message
ZERO_OR_ERROR ServerDir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	ZERO_OR_ERROR ret;

	// Do we know this server doesn't support DIRALL?
	if (pn_whole_directory->selected_connection->connin.tcp.no_dirall) {
		return ServerDIR(dirfunc, v, pn_whole_directory, flags);
	}
	// Did we ask for no DIRALL explicitly?
	if (Globals.no_dirall) {
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
static ZERO_OR_ERROR ServerDIR(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_whole_directory->path_busless, NULL, 0,
		pn_whole_directory->tokenstring, pn_whole_directory->tokens,
	};
	enum persistent_state persistent = persistent_yes;
	FILE_DESCRIPTOR_OR_ERROR connectfd;
	ZERO_OR_ERROR ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dir;

	LEVEL_CALL("SERVER(%d) path=%s path_busless=%s",
			   pn_whole_directory->selected_connection->index, SAFESTRING(pn_whole_directory->path), SAFESTRING(pn_whole_directory->path_busless));

	connectfd = PersistentStart(&persistent, pn_whole_directory->selected_connection);
	if ( FILE_DESCRIPTOR_VALID( connectfd ) ) {
		sm.control_flags = SetupControlFlags(persistent, pn_whole_directory);
		connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_whole_directory->selected_connection) ;
		if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
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
				struct parsedname s_pn_directory_element;
				struct parsedname * pn_directory_element = & s_pn_directory_element ;
				return_path[cm.payload - 1] = '\0';	/* Ensure trailing null */
				LEVEL_DEBUG("got=[%s]", return_path);

				if (SpecifiedRemoteBus(pn_whole_directory)) {
					// Specified remote bus, add the bus to the path (in front)
					char BigBuffer[cm.payload + 12];
					int sn_ret ;
					char * no_leading_slash = &return_path[(return_path[0] == '/') ? 1 : 0] ;
					UCLIBCLOCK ;
					sn_ret = snprintf(BigBuffer, cm.payload + 11, "/bus.%d/%s",pn_whole_directory->selected_connection->index, no_leading_slash ) ;
					UCLIBCUNLOCK ;
					if (sn_ret > 0) {
						ret = FS_ParsedName_BackFromRemote(BigBuffer, pn_directory_element);
					} else {
						ret = -EINVAL ;
					}
				} else {
					ret = FS_ParsedName_BackFromRemote(return_path, pn_directory_element);
				}

				owfree(return_path);

				if (ret) {
					DirblobPoison(&db); // don't cache a mistake
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
					Cache_Add_Device(pn_whole_directory->selected_connection->index, pn_directory_element->sn);
				}
				/* Add to cache Blob -- snlist is also a flag for cachable */
				DirblobAdd(pn_directory_element->sn, &db);
				++devices;
				FS_alias_subst(dirfunc,v,pn_directory_element);
				FS_ParsedName_destroy(pn_directory_element);	// destroy the last parsed name
			}
			/* Add to the cache (full list as a single element */
			if (DirblobPure(&db)) {
				Cache_Add_Dir(&db, pn_whole_directory);	// end with a null entry
				if (RootNotBranch(pn_whole_directory)) {
					BUSLOCK(pn_whole_directory);
					pn_whole_directory->selected_connection->last_root_devs = DirblobElements(&db);	// root dir estimated length
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
	PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_whole_directory->selected_connection);
	return ret;
}

// Send to an owserver using the DIRALL message
static ZERO_OR_ERROR ServerDIRALL(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	ASCII *comma_separated_list;
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_whole_directory->path_busless, NULL, 0,
		pn_whole_directory->tokenstring, pn_whole_directory->tokens,
	};
	enum persistent_state persistent = persistent_yes;
	FILE_DESCRIPTOR_OR_ERROR connectfd;
	ZERO_OR_ERROR ret;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dirall;

	LEVEL_CALL("SERVER(%d) path=%s path_busless=%s",
			   pn_whole_directory->selected_connection->index, SAFESTRING(pn_whole_directory->path), SAFESTRING(pn_whole_directory->path_busless));

	// Get a file descriptor, possibly a persistent one
	connectfd = PersistentStart(&persistent, pn_whole_directory->selected_connection);
	if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
		PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_whole_directory->selected_connection);
		return -EIO;
	}
	// Now try to get header. If fails, may need a new non-persistent file_descriptor
	sm.control_flags = SetupControlFlags(persistent, pn_whole_directory);
	connectfd = ToServerTwice(connectfd, persistent, &sm, &sp, pn_whole_directory->selected_connection);
	if ( FILE_DESCRIPTOR_NOT_VALID( connectfd ) ) {
		PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_whole_directory->selected_connection);
		return -EIO;
	}
	// Success, get data
	comma_separated_list = FromServerAlloc(connectfd, &cm);
	LEVEL_DEBUG("got %s", SAFESTRING(comma_separated_list));
	if (cm.ret == 0) {
		ASCII *current_file;
		ASCII *rest_of_comma_list = comma_separated_list;
		size_t devices = 0;
		struct dirblob db;

		LEVEL_DEBUG("start parsing");

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
			struct parsedname s_pn_directory_element;
			struct parsedname * pn_directory_element = & s_pn_directory_element ;
			int path_length = strlen(current_file);
			LEVEL_DEBUG("got=[%s]", current_file);

			ret = -EINVAL;

			if (SpecifiedRemoteBus(pn_whole_directory)) {
				// Specified remote bus, add the bus to the path (in front)
				char BigBuffer[path_length + 12];
				int sn_ret ;
				char * no_leading_slash = &current_file[(current_file[0] == '/') ? 1 : 0] ;
				UCLIBCLOCK ;
				sn_ret = snprintf(BigBuffer, cm.payload + 11, "/bus.%d/%s",pn_whole_directory->selected_connection->index, no_leading_slash ) ;
				UCLIBCUNLOCK ;
				if (sn_ret > 0) {
					ret = FS_ParsedName_BackFromRemote(BigBuffer, pn_directory_element);
				}
			} else {
				ret = FS_ParsedName_BackFromRemote(current_file, pn_directory_element);
			}

			if (ret) {
				cm.ret = ret;
				break;
			}
			/* we got a device on bus_nr = pn_whole_directory->selected_connection->index. Cache it so we
			   find it quicker next time we want to do read values from the
			   the actual device
			 */
			if (IsRealDir(pn_whole_directory)) {
				/* If we get a device then cache the bus_nr */
				Cache_Add_Device(pn_whole_directory->selected_connection->index, pn_directory_element->sn);
			}
			/* Add to cache Blob -- snlist is also a flag for cachable */
			if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
				DirblobAdd(pn_directory_element->sn, &db);
			}
			++devices;
			FS_alias_subst(dirfunc,v,pn_directory_element) ;
			FS_ParsedName_destroy(pn_directory_element);	// destroy the last parsed name
		}
		/* Add to the cache (full list as a single element */
		if (DirblobPure(&db)) {
			Cache_Add_Dir(&db, pn_whole_directory);	// end with a null entry
			if (RootNotBranch(pn_whole_directory)) {
				BUSLOCK(pn_whole_directory);
				pn_whole_directory->selected_connection->last_root_devs = DirblobElements(&db);	// root dir estimated length
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
		owfree(comma_separated_list);
	}
	ret = cm.ret;

	PersistentEnd(connectfd, persistent, cm.control_flags & PERSISTENT_MASK, pn_whole_directory->selected_connection);
	return ret;
}

/* read from server, free return pointer if not Null */
/* Adds an extra null byte at end */
static void *FromServerAlloc(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct client_msg *cm)
{
	char *msg;
	struct timeval tv = { Globals.timeout_network + 1, 0, };
	size_t actual_size ;

	do {						/* loop until non delay message (payload>=0) */
		tcp_read(file_descriptor, cm, sizeof(struct client_msg), &tv, &actual_size);
		if (actual_size != sizeof(struct client_msg)) {
			memset(cm, 0, sizeof(struct client_msg));
			cm->ret = -EIO;
			return NULL;
		}
		cm->payload = ntohl(cm->payload);
		cm->size = ntohl(cm->size);
		cm->ret = ntohl(cm->ret);
		cm->control_flags = ntohl(cm->control_flags);
		cm->offset = ntohl(cm->offset);
	} while (cm->payload < 0);

	if (cm->payload == 0) {
		return NULL;
	}
	if (cm->ret < 0) {
		return NULL;
	}
	if (cm->payload > MAX_OWSERVER_PROTOCOL_PACKET_SIZE) {
		return NULL;
	}

	if ((msg = (char *) owmalloc((size_t) cm->payload + 1))) {
		tcp_read(file_descriptor, msg, (size_t) (cm->payload), &tv, &actual_size);
		if ((ssize_t)actual_size != cm->payload) {
			cm->payload = 0;
			cm->offset = 0;
			cm->ret = -EIO;
			owfree(msg);
			msg = NULL;
		}
	}
	msg[cm->payload] = '\0';	// safety NULL
	return msg;
}

/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static SIZE_OR_ERROR FromServer(int file_descriptor, struct client_msg *cm, char *msg, size_t size)
{
	size_t rtry;
	size_t actual_read ;
	struct timeval tv = { Globals.timeout_network + 1, 0, };

	do {						// read regular header, or delay (delay when payload<0)
		tcp_read(file_descriptor, cm, sizeof(struct client_msg), &tv, &actual_read);
		if (actual_read != sizeof(struct client_msg)) {
			cm->size = 0;
			cm->ret = -EIO;
			return -EIO;
		}

		cm->payload = ntohl(cm->payload);
		cm->size = ntohl(cm->size);
		cm->ret = ntohl(cm->ret);
		cm->control_flags = ntohl(cm->control_flags);
		cm->offset = ntohl(cm->offset);
	} while (cm->payload < 0);	// flag to show a delay message

	if (cm->payload == 0) {
		return 0;				// No payload, done.
	}
	rtry = cm->payload < (ssize_t) size ? (size_t) cm->payload : size;
	tcp_read(file_descriptor, msg, rtry, &tv, &actual_read);	// read expected payload now.
	if (actual_read != rtry) {
		cm->ret = -EIO;
		return -EIO;
	}

	if (cm->payload > (ssize_t) size) {	// Uh oh. payload bigger than expected. read it in and discard
		size_t extra_length = cm->payload - size ;
		char extra[extra_length] ;
		tcp_read(file_descriptor, extra, extra_length, &tv, &actual_read);
		return size;
	}
	return cm->payload;
}

/* Send a message to server, or try a new connection and send again */
/* return file descriptor */
static FILE_DESCRIPTOR_OR_ERROR ToServerTwice(FILE_DESCRIPTOR_OR_ERROR file_descriptor, enum persistent_state persistent, struct server_msg *sm, struct serverpackage *sp, struct connection_in *in)
{
	FILE_DESCRIPTOR_OR_ERROR newfd;
	if (ToServer(file_descriptor, sm, sp) >= 0) {
		return file_descriptor;
	}
	if (persistent == persistent_no) {
		close(file_descriptor);
		return FILE_DESCRIPTOR_BAD;
	}
	
	/* Special case if the connection was persistent
	 * it may have timed out, so try a new conection */
	newfd = PersistentReRequest(file_descriptor, in);
	if ( ! FILE_DESCRIPTOR_VALID( newfd ) ) {
		return FILE_DESCRIPTOR_BAD;
	}
	if (ToServer(newfd, sm, sp) >= 0) {
		return newfd;
	}
	close(newfd);
	return FILE_DESCRIPTOR_BAD;
}

// should be const char * data but iovec has problems with const arguments
static SIZE_OR_ERROR ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp)
{
	int payload = 0;
	int tokens = 0;
	int nio = 0;
	struct iovec io[5] = { {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, };
	struct server_msg local_sm ;

	// First block to send, the header
	io[nio].iov_base = &local_sm;
	io[nio].iov_len = sizeof(struct server_msg);
	nio++;

	// Next block, the path
	if ((io[nio].iov_base = sp->path)) {	// send path (if not null)
		io[nio].iov_len = payload = strlen(sp->path) + 1;
		nio++;
		LEVEL_DEBUG("path=%s", sp->path);
	}
	// next block, data (only for writes)
	if ((sp->datasize>0) && (io[nio].iov_base = sp->data)) {	// send data only for writes (if datasize not zero)
		payload += (io[nio].iov_len = sp->datasize);
		nio++;
		Debug_Bytes("Data sent to server", sp->data,sp->datasize);
	}

	sm->version = MakeServerprotocol(OWSERVER_PROTOCOL_VERSION);
	if (Globals.opt == opt_server) {
		tokens = sp->tokens;
		sm->version |= MakeServermessage;

		// next block prior tokens (if an owserver)
		if (tokens > 0) {		// owserver: send prior tags
			io[nio].iov_base = sp->tokenstring;
			io[nio].iov_len = tokens * sizeof(union antiloop);
			nio++;
		}
		// final block, new token (if an owserver)
		++tokens;
		sm->version |= MakeServertokens(tokens);
		io[nio].iov_base = &(Globals.Token);	// owserver: add our tag
		io[nio].iov_len = sizeof(union antiloop);
		nio++;
		LEVEL_DEBUG("tokens=%d", tokens);
	}
	LEVEL_DEBUG("version=%u payload=%d size=%d type=%d SG=%X offset=%d",sm->version,payload,sm->size,sm->type,sm->control_flags,sm->offset);

	// encode in network order (just the header)
	local_sm.version = htonl(sm->version);
	local_sm.payload = htonl(payload);
	local_sm.size = htonl(sm->size);
	local_sm.type = htonl(sm->type);
	local_sm.control_flags = htonl(sm->control_flags);
	local_sm.offset = htonl(sm->offset);

	Debug_Writev(io, nio);
	return writev(file_descriptor, io, nio) != (ssize_t) (payload + sizeof(struct server_msg) + tokens * sizeof(union antiloop));
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupControlFlags(enum persistent_state persistent, const struct parsedname *pn)
{
	uint32_t control_flags = pn->control_flags;

	control_flags &= ~PERSISTENT_MASK;
	if (persistent==persistent_yes) {
		control_flags |= PERSISTENT_MASK;
	}

	/* from owlib to owserver never wants alias */
	control_flags &= ~ALIAS_REQUEST ;

	control_flags &= ~SHOULD_RETURN_BUS_LIST;
	if (SpecifiedBus(pn)) {
		control_flags |= SHOULD_RETURN_BUS_LIST;
	}

	return control_flags;
}

/* Wrapper for ClientConnect */
static FILE_DESCRIPTOR_OR_ERROR ConnectToServer(struct connection_in *in)
{
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;

	file_descriptor = ClientConnect(in);
	if ( FILE_DESCRIPTOR_VALID( file_descriptor ) ) {
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
static FILE_DESCRIPTOR_OR_ERROR PersistentRequest(struct connection_in *in)
{
	FILE_DESCRIPTOR_OR_ERROR file_descriptor ;
	BUSLOCKIN(in);
	switch ( in->file_descriptor ) {
		case FILE_DESCRIPTOR_PERSISTENT_IN_USE:
			// Currently in use, so return bad to force a new (transient) conection
			// We don't just allocate a new one because the higher level needs to know
			// that this isn't a persistent connection'
			file_descriptor = FILE_DESCRIPTOR_BAD ;
			break ;
		case FILE_DESCRIPTOR_BAD:
			// no conection currently, so make a new one
			file_descriptor = ConnectToServer(in) ;
			if ( FILE_DESCRIPTOR_VALID( file_descriptor ) ) {
				in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE ;
			}	
			break ;
		default:
			// persistent connection idle and waiting for use
			file_descriptor = in->file_descriptor;
			in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE;
		break ;
	}
	BUSUNLOCKIN(in);
	return file_descriptor;
}

/* A persistent connection didn't work (probably expired on the other end
   recreate it, or clear and return -1
 */
static FILE_DESCRIPTOR_OR_ERROR PersistentReRequest(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct connection_in *in)
{
	close(file_descriptor);
	BUSLOCKIN(in);
	file_descriptor = ConnectToServer(in);
	if ( FILE_DESCRIPTOR_VALID( file_descriptor ) ) {
		// leave it marked as in use (though it's a different fd)
		in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE;
	} else {
		// can't reconnect
		in->file_descriptor = FILE_DESCRIPTOR_BAD;
	}
	BUSUNLOCKIN(in);
	return file_descriptor;
}

/* Clear a persistent connection */
static void PersistentClear(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct connection_in *in)
{
	Test_and_Close( &file_descriptor) ;
	BUSLOCKIN(in);
	in->file_descriptor = FILE_DESCRIPTOR_BAD;
	BUSUNLOCKIN(in);
}

/* Free a persistent connection */
static void PersistentFree(FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct connection_in *in)
{
	if ( FILE_DESCRIPTOR_VALID( file_descriptor ) ) {
		// mark as available
		BUSLOCKIN(in);
		in->file_descriptor = file_descriptor;
		BUSUNLOCKIN(in);
	} else {
		PersistentClear(file_descriptor, in);
	}
}

/* All the startup code
   file_descriptor will get the file descriptor
   persistent starts yes or no for whether persistence is wanted
   persistent returns yes or no for whether persistence is granted
*/
static FILE_DESCRIPTOR_OR_ERROR PersistentStart(enum persistent_state *persistent, struct connection_in *in)
{
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	if (*persistent == persistent_no) {		
		// no persistence wanted
		*persistent = persistent_no;		// still not persistent
		return ConnectToServer(in);
	}
	
	file_descriptor = PersistentRequest(in) ;
	if ( FILE_DESCRIPTOR_NOT_VALID( file_descriptor ) ) {	// tried but failed
		*persistent = persistent_no;		// not persistent
		return ConnectToServer(in);	// non-persistent backup request
	}
		
	*persistent = persistent_yes;		// flag as persistent
	return file_descriptor;
}

/* Clean up at end of routine,
   either leave connection open and persistent flag available,
   or close and leave available
*/
static void PersistentEnd(FILE_DESCRIPTOR_OR_ERROR file_descriptor, enum persistent_state persistent, int granted, struct connection_in *in)
{
	if (persistent == persistent_no) {		// non-persistence from the start
		Test_and_Close(&file_descriptor) ;
	} else if (granted == 0) {	// not granted
		PersistentClear(file_descriptor, in);
	} else {					// Let the persistent connection be used
		PersistentFree(file_descriptor, in);
	}
}
