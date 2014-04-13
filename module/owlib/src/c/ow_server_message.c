/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
#include "ow_standard.h" // for FS_?_alias

struct server_connection_state {
	FILE_DESCRIPTOR_OR_ERROR file_descriptor ;
	enum persistent_state { persistent_yes, persistent_no, } persistence ;
	struct connection_in * in ;
} ;

struct directory_element_structure {
	const struct parsedname * pn_whole_directory ;
	struct dirblob db ;
	void (*dirfunc) (void *, const struct parsedname * const) ;
	void * v ;
} ;

static uint32_t SetupControlFlags(const struct parsedname *pn);

static ZERO_OR_ERROR ServerDIRALL(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags);
static ZERO_OR_ERROR ServerDIR(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags);

static void Directory_Element_Init( struct directory_element_structure * des );
static void Directory_Element_Finish( struct directory_element_structure * des );
static ZERO_OR_ERROR Directory_Element( char * current_file, struct directory_element_structure * des );

static void Close_Persistent( struct server_connection_state * scs) ;
static void Release_Persistent( struct server_connection_state * scs, int granted ) ;

static GOOD_OR_BAD To_Server( struct server_connection_state * scs, struct server_msg * sm, struct serverpackage *sp) ;
static SIZE_OR_ERROR WriteToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp);

static SIZE_OR_ERROR From_Server( struct server_connection_state * scs, struct client_msg *cm, char *msg, size_t size) ;
static void *From_ServerAlloc(struct server_connection_state * scs, struct client_msg *cm) ;


// Send to an owserver using the READ message
SIZE_OR_ERROR ServerRead(struct one_wire_query *owq)
{
	struct server_msg sm;
	struct client_msg cm;
	struct parsedname *pn_file_entry = PN(owq);
	struct serverpackage sp = { pn_file_entry->path_to_server, NULL, 0, pn_file_entry->tokenstring,
		pn_file_entry->tokens,
	};
	struct server_connection_state scs ;

	// initialization
	scs.in = pn_file_entry->selected_connection ;
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = OWQ_size(owq);
	sm.offset = OWQ_offset(owq);

	// Alias should show local understanding except if bus.x specified
	if ( pn_file_entry->selected_filetype->format == ft_alias && ! SpecifiedRemoteBus(pn_file_entry) ) {
		ignore_result = FS_r_alias( owq ) ;
		return OWQ_length(owq) ;
	}

	LEVEL_CALL("SERVER(%d) path=%s", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_to_server));

	// Send to owserver
	sm.control_flags = SetupControlFlags(pn_file_entry);
	if ( BAD( To_Server( &scs, &sm, &sp) ) ) {
		Release_Persistent( &scs, 0);
		return -EIO ;
	}
	
	// Receive from owserver
	if ( From_Server( &scs, &cm, OWQ_buffer(owq), OWQ_size(owq)) < 0 ) {
		Release_Persistent( &scs, 0);
		return -EIO ;
	}
	Release_Persistent( &scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}

// Send to an owserver using the PRESENT message
INDEX_OR_ERROR ServerPresence( struct parsedname *pn_file_entry)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_file_entry->path_to_server, NULL, 0, pn_file_entry->tokenstring,
		pn_file_entry->tokens,
	};
	BYTE * serial_number ;
	struct server_connection_state scs ;

	// initialization
	scs.in = pn_file_entry->selected_connection ;
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_presence;

	LEVEL_CALL("SERVER(%d) path=%s", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_to_server));

	// Send to owserver
	sm.control_flags = SetupControlFlags( pn_file_entry);
	if ( BAD( To_Server( &scs, &sm, &sp) ) ) {
		Release_Persistent( &scs, 0 ) ;
		return INDEX_BAD ;
	}

	// Receive from owserver
	serial_number = (BYTE *) From_ServerAlloc( &scs, &cm) ;
	if (cm.ret < 0) {
		Release_Persistent(&scs, 0 );
		return INDEX_BAD ;
	}

	// Newer owservers return the serial number -- relevant for alias propagation
	if ( serial_number ) {
		memcpy( pn_file_entry->sn, serial_number, SERIAL_NUMBER_SIZE ) ;
		owfree( serial_number) ;
	}

	Release_Persistent(&scs, cm.control_flags & PERSISTENT_MASK);
	return INDEX_VALID(cm.ret) ? pn_file_entry->selected_connection->index : INDEX_BAD;
}

// Send to an owserver using the WRITE message
ZERO_OR_ERROR ServerWrite(struct one_wire_query *owq)
{
	struct server_msg sm;
	struct client_msg cm;
	struct parsedname *pn_file_entry = PN(owq);
	struct serverpackage sp = { pn_file_entry->path_to_server, (BYTE *) OWQ_buffer(owq),
		OWQ_size(owq), pn_file_entry->tokenstring, pn_file_entry->tokens,
	};
	struct server_connection_state scs ;

	// initialization
	scs.in = pn_file_entry->selected_connection ;
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_write;
	sm.size = OWQ_size(owq);
	sm.offset = OWQ_offset(owq);

	LEVEL_CALL("SERVER(%d) path=%s", pn_file_entry->selected_connection->index, SAFESTRING(pn_file_entry->path_to_server));

	// Send to owserver
	sm.control_flags = SetupControlFlags( pn_file_entry);
	if ( BAD( To_Server( &scs, &sm, &sp) ) ) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}

	// Receive from owserver
	if ( From_Server( &scs, &cm, NULL, 0) < 0) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}
	{
		int32_t control_flags = cm.control_flags & ~(SHOULD_RETURN_BUS_LIST | PERSISTENT_MASK | SAFEMODE);
		// keep current safemode
		control_flags |=  LocalControlFlags & SAFEMODE ;
		CONTROLFLAGSLOCK;
		if (LocalControlFlags != control_flags) {
			// replace control flags (except safemode persists)
			//printf("ServerRead: cm.control_flags changed!  controlflags=%X cm.control_flags=%X\n", SemiGlobal, cm.control_flags);
			LocalControlFlags = control_flags;
		}
		CONTROLFLAGSUNLOCK;
	}

	Release_Persistent(&scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}

// Send to an owserver using either the DIR or DIRALL message
ZERO_OR_ERROR ServerDir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	ZERO_OR_ERROR ret;
	struct connection_in * in = pn_whole_directory->selected_connection ;

	// Do we know this server doesn't support DIRALL?
	if (in->master.server.no_dirall) {
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
		in->master.server.no_dirall = 1;
		return ServerDIR(dirfunc, v, pn_whole_directory, flags);
	}
	return ret;
}

// Send to an owserver using the DIR message
static ZERO_OR_ERROR ServerDIR(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_whole_directory->path_to_server, NULL, 0,
		pn_whole_directory->tokenstring, pn_whole_directory->tokens,
	};
	struct server_connection_state scs ;
	struct directory_element_structure des ;

	char *return_path;

	// initialization
	scs.in = pn_whole_directory->selected_connection ;
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dir;

	LEVEL_CALL("SERVER(%d) path=%s path_to_server=%s",
			   scs.in->index, SAFESTRING(pn_whole_directory->path), SAFESTRING(pn_whole_directory->path_to_server));

	// Send to owserver
	sm.control_flags = SetupControlFlags( pn_whole_directory);
	if ( BAD( To_Server( &scs, &sm, &sp) ) ) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}

	des.dirfunc = dirfunc ;
	des.v = v ;
	des.pn_whole_directory = pn_whole_directory ;
	Directory_Element_Init( &des ) ;

	// Receive from owserver -- in a loop for each directory entry
	while ( (return_path = From_ServerAlloc(&scs, &cm)) != NO_PATH ) {
		ZERO_OR_ERROR ret;

		return_path[cm.payload - 1] = '\0';	/* Ensure trailing null */
		
		ret = Directory_Element( return_path, &des ) ;

		owfree(return_path);

		if (ret) {
			cm.ret = ret;
			break;
		}
	}
	
	Directory_Element_Finish( &des ) ;

	DIRLOCK;
	/* flags are sent back in "offset" of final blank entry */
	flags[0] |= cm.offset;
	DIRUNLOCK;

	Release_Persistent(&scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}

// Send to an owserver using the DIRALL message
static ZERO_OR_ERROR ServerDIRALL(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	ASCII *comma_separated_list;
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { pn_whole_directory->path_to_server, NULL, 0,
		pn_whole_directory->tokenstring, pn_whole_directory->tokens,
	};
	struct connection_in * in = pn_whole_directory->selected_connection ;
	struct server_connection_state scs ;

	// initialization
	scs.in = in ;
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dirall;

	LEVEL_CALL("SERVER(%d) path=%s path_to_server=%s",
			   in->index, SAFESTRING(pn_whole_directory->path), SAFESTRING(pn_whole_directory->path_to_server));

	// Send to owserver
	sm.control_flags = SetupControlFlags( pn_whole_directory);
	if ( BAD( To_Server( &scs, &sm, &sp) ) ) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}

	// Receive from owserver
	comma_separated_list = From_ServerAlloc(&scs, &cm);
	LEVEL_DEBUG("got %s", SAFESTRING(comma_separated_list));
	if (cm.ret == 0) {
		ASCII *current_file;
		ASCII *rest_of_comma_list = comma_separated_list;
		struct directory_element_structure des ;

		des.dirfunc = dirfunc ;
		des.v = v ;
		des.pn_whole_directory = pn_whole_directory ;
		Directory_Element_Init( &des ) ;

		while ((current_file = strsep(&rest_of_comma_list, ",")) != NULL) {
			ZERO_OR_ERROR ret = Directory_Element( current_file, &des );
			if (ret) {
				cm.ret = ret;
				break;
			}
		}
		
		Directory_Element_Finish( &des ) ;

		DIRLOCK;
		/* flags are sent back in "offset" */
		flags[0] |= cm.offset;
		DIRUNLOCK;

	}
	// free the allocated memory
	SAFEFREE(comma_separated_list) ;

	Release_Persistent( &scs, cm.control_flags & PERSISTENT_MASK );
	return cm.ret;
}

static void Directory_Element_Init( struct directory_element_structure * des )
{
	/* If cacheable, try to allocate a blob for storage */
	/* only for "read devices" and not alarm */
	DirblobInit(&(des->db) );
	if (IsRealDir(des->pn_whole_directory)
		&& NotAlarmDir(des->pn_whole_directory)
		&& !SpecifiedBus(des->pn_whole_directory)
		&& des->pn_whole_directory->selected_device == NO_DEVICE) {
		if (RootNotBranch(des->pn_whole_directory)) {	/* root dir */
			BUSLOCK(des->pn_whole_directory);
			des->db.allocated = des->pn_whole_directory->selected_connection->last_root_devs;	// root dir estimated length
			BUSUNLOCK(des->pn_whole_directory);
		}
	} else {
		DirblobPoison( &(des->db) ); // don't cache other directories
	}
}

static void Directory_Element_Finish( struct directory_element_structure * des )
{
	/* Add to the cache (full list as a single element */
	if ( DirblobPure( &(des->db) ) ) {
		Cache_Add_Dir( &(des->db), des->pn_whole_directory);	// end with a null entry
		if (RootNotBranch(des->pn_whole_directory)) {
			BUSLOCK(des->pn_whole_directory);
			des->pn_whole_directory->selected_connection->last_root_devs = DirblobElements( &(des->db) );	// root dir estimated length
			BUSUNLOCK(des->pn_whole_directory);
		}
	}
	DirblobClear( & (des->db) );
}

static ZERO_OR_ERROR Directory_Element( char * current_file, struct directory_element_structure * des )
{
	struct parsedname s_pn_directory_element;
	struct parsedname * pn_directory_element = & s_pn_directory_element ;
	struct connection_in * in = des->pn_whole_directory->selected_connection ;
	ZERO_OR_ERROR ret ;

	LEVEL_DEBUG("got=[%s]", current_file);

	if (SpecifiedRemoteBus(des->pn_whole_directory)) {
		// Specified remote bus, add the bus to the path (in front)
		int path_length = strlen(current_file);
		char BigBuffer[path_length + 12];
		int sn_ret ;
		char * no_leading_slash = &current_file[(current_file[0] == '/') ? 1 : 0] ;
		UCLIBCLOCK ;
		sn_ret = snprintf(BigBuffer, path_length + 11, "/bus.%d/%s",in->index, no_leading_slash ) ;
		UCLIBCUNLOCK ;
		if (sn_ret < 0) {
			return -EINVAL ;
		}
		ret = FS_ParsedName_BackFromRemote(BigBuffer, pn_directory_element);
	} else {
		ret = FS_ParsedName_BackFromRemote(current_file, pn_directory_element);
	}

	if ( ret < 0 ) {
		DirblobPoison( &(des->db) ); // don't cache a mistake
		return ret;
	}

	/* we got a device on bus_nr = pn_whole_directory->selected_connection->index. Cache it so we
	   find it quicker next time we want to do read values from the
	   the actual device
	 */
	if (IsRealDir(des->pn_whole_directory)) {
		/* If we get a device then cache the bus_nr */
		Cache_Add_Device(in->index, pn_directory_element->sn);
	}
	/* Add to cache Blob -- snlist is also a flag for cachable */
	if (DirblobPure( &(des->db) ) ) {	/* only add if there is a blob allocated successfully */
		DirblobAdd(pn_directory_element->sn, &(des->db) );
	}

	// Now actually do the directory function on this element
	FS_dir_entry_aliased(des->dirfunc,des->v,pn_directory_element) ;
	
	// Cleanup
	FS_ParsedName_destroy(pn_directory_element);	// destroy the last parsed name

	return 0 ;
}

/* read from server, free return pointer if not Null */
/* Adds an extra null byte at end */
static void *From_ServerAlloc(struct server_connection_state * scs, struct client_msg *cm)
{
	BYTE *msg;
	struct timeval tv = { Globals.timeout_network + 1, 0, };
	size_t actual_size ;

	do {						/* loop until non delay message (payload>=0) */
		tcp_read(scs->file_descriptor, (BYTE *) cm, sizeof(struct client_msg), &tv, &actual_size);
		if (actual_size != sizeof(struct client_msg)) {
			memset(cm, 0, sizeof(struct client_msg));
			cm->ret = -EIO;
			return NO_PATH;
		}
		cm->payload = ntohl(cm->payload);
		cm->size = ntohl(cm->size);
		cm->ret = ntohl(cm->ret);
		cm->control_flags = ntohl(cm->control_flags);
		cm->offset = ntohl(cm->offset);
	} while (cm->payload < 0);

	if (cm->payload == 0) {
		return NO_PATH;
	}
	if (cm->ret < 0) {
		return NO_PATH;
	}
	if (cm->payload > MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE) {
		return NO_PATH;
	}

	msg = owmalloc((size_t) cm->payload + 1) ;
	if ( msg != NO_PATH ) {
		tcp_read(scs->file_descriptor, msg, (size_t) (cm->payload), &tv, &actual_size);
		if ((ssize_t)actual_size != cm->payload) {
			cm->payload = 0;
			cm->offset = 0;
			cm->ret = -EIO;
			owfree(msg);
			msg = NO_PATH;
		}
	}
	if(msg != NO_PATH) {
		msg[cm->payload] = '\0';	// safety NULL
	}
	return msg;
}

/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static SIZE_OR_ERROR From_Server( struct server_connection_state * scs, struct client_msg *cm, char *msg, size_t size)
{
	size_t rtry;
	size_t actual_read ;
	struct timeval tv1 = { Globals.timeout_network + 1, 0, };
	struct timeval tv2 = { Globals.timeout_network + 1, 0, };

	do {						// read regular header, or delay (delay when payload<0)
		tcp_read(scs->file_descriptor, (BYTE *) cm, sizeof(struct client_msg), &tv1, &actual_read);
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
	tcp_read(scs->file_descriptor, (BYTE *) msg, rtry, &tv2, &actual_read);	// read expected payload now.
	if (actual_read != rtry) {
		LEVEL_DEBUG("Read only %d of %d\n",(int)actual_read,(int)rtry) ;
		cm->ret = -EIO;
		return -EIO;
	}
	if (cm->payload > (ssize_t) size) {	// Uh oh. payload bigger than expected. close the connection
		Close_Persistent( scs ) ;
		return size;
	}
	return cm->payload;
}

static GOOD_OR_BAD To_Server( struct server_connection_state * scs, struct server_msg * sm, struct serverpackage *sp)
{
	struct connection_in * in = scs->in ; // for convenience
	struct port_in * pin = in->pown ;
	BYTE test_read[1] ;
	int old_flags ;
	ssize_t rcv_value ;
	int saved_errno ;
	
	// initialize the variables
	scs->file_descriptor = FILE_DESCRIPTOR_BAD ;
	scs->persistence = Globals.no_persistence ? persistent_no : persistent_yes ;

	// First set up the file descriptor based on persistent state
	if (scs->persistence == persistent_no) {		
		// no persistence wanted
		scs->file_descriptor = ClientConnect(in);
	} else {
		// Persistence desired
		BUSLOCKIN(in);
		switch ( pin->file_descriptor ) {
			case FILE_DESCRIPTOR_PERSISTENT_IN_USE:
				// Currently in use, so make new non-persistent connection
				scs->file_descriptor = ClientConnect(in);
				scs->persistence = persistent_no ;
				break ;
			case FILE_DESCRIPTOR_BAD:
				// no conection currently, so make a new one
				scs->file_descriptor = ClientConnect(in);
				if ( FILE_DESCRIPTOR_VALID( scs->file_descriptor ) ) {
					pin->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE ;
				}	
				break ;
			default:
				// persistent connection idle and waiting for use
				// connection_in is locked so this is safe
				scs->file_descriptor = pin->file_descriptor;
				pin->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE;
			break ;
		}
		BUSUNLOCKIN(in);
	}
	
	// Check if the server closed the connection
	// This is contributed by Jacob Joseph to fix a timeout problem.
	// http://permalink.gmane.org/gmane.comp.file-systems.owfs.devel/7306
	//rcv_value = recv(scs->file_descriptor, test_read, 1, MSG_DONTWAIT | MSG_PEEK) ;
	old_flags = fcntl( scs->file_descriptor, F_GETFL, 0 ) ; // save socket flags
	if ( old_flags == -1 ) {
		rcv_value = -2 ;
	} else if ( fcntl( scs->file_descriptor, F_SETFL, old_flags | O_NONBLOCK ) == -1 ) { // set non-blocking
		rcv_value = -2 ;
	} else {
		rcv_value = recv(scs->file_descriptor, test_read, 1, MSG_PEEK) ; // test read the socket to see if closed
		saved_errno = errno ;
		if ( fcntl( scs->file_descriptor, F_SETFL, old_flags ) == -1 ) { // restore  socket flags
			rcv_value = -2 ;
		}
	}

	switch ( rcv_value ) {
		case -1:
			if ( saved_errno==EAGAIN || saved_errno==EWOULDBLOCK ) {
				// No data to be read -- so connection healthy
				break ;
			}
			// real error, fall through to close connection case
		case -2:
			// fnctl error, fall through again
		case 0:
			LEVEL_DEBUG("Server connection was closed.  Reconnecting.");
			Close_Persistent( scs);
			scs->file_descriptor = ClientConnect(in);
			if ( FILE_DESCRIPTOR_VALID( scs->file_descriptor ) ) {
				in->pown->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE ;
			}
			break ;
		default:
			// data to be read, so a good connection
			break ;
	}

	// Now test
	if ( FILE_DESCRIPTOR_NOT_VALID( scs->file_descriptor ) ) {
		STAT_ADD1(in->reconnect_state);
		Close_Persistent( scs ) ;
		return gbBAD ;
	}

	// Do the real work
	if (WriteToServer(scs->file_descriptor, sm, sp) >= 0) {
		// successful message
		return gbGOOD;
	}

	// This is where it gets a bit tricky. For non-persistent conections we're done'
	if ( scs->persistence == persistent_no ) {
		// not persistent, so no reconnection needed
		Close_Persistent( scs ) ;
		return gbBAD ;
	}
	
	// perhaps the persistent connection is stale?
	// Make a new one
	scs->file_descriptor = ClientConnect(in) ;

	// Now retest
	if ( FILE_DESCRIPTOR_NOT_VALID( scs->file_descriptor ) ) {
		// couldn't make that new connection -- free everything
		STAT_ADD1(in->reconnect_state);
		Close_Persistent( scs ) ;
		return gbBAD ;
	}
	
	// Leave in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE

	// Second attempt at the write, now with new connection
	if (WriteToServer(scs->file_descriptor, sm, sp) >= 0) {
		// successful message
		return gbGOOD;
	}

	// bad write the second time -- clear everything
	Close_Persistent( scs ) ;
	return gbBAD ;
}

static void Close_Persistent( struct server_connection_state * scs)
{
	// First set up the file descriptor based on persistent state
	if (scs->persistence == persistent_yes) {
		// no persistence wanted
		BUSLOCKIN(scs->in);
			scs->in->pown->file_descriptor = FILE_DESCRIPTOR_BAD ;
		BUSUNLOCKIN(scs->in);
	}
	
	scs->persistence = persistent_no ;
	Test_and_Close( &(scs->file_descriptor) ) ;
}

// should be const char * data but iovec has problems with const arguments
static SIZE_OR_ERROR WriteToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp)
{
	int payload = 0;
	int tokens = 0;
	int server_type = Globals.program_type==program_type_server || Globals.program_type==program_type_external ;

	// We use vector write -- several ranges
	int nio = 0;
	struct iovec io[5] = { {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, };

	struct server_msg net_sm ;

	// Set the version
	sm->version = MakeServerprotocol(OWSERVER_PROTOCOL_VERSION);

	// First block to send, the header
	// We'll do this last since the header values (e.g. payload) change
	nio++;

	// Next block, the path
	if (sp->path != 0) {	// send path (if not null)
		// writev should take const data pointers, but I can't fix the library
#if ( __GNUC__ > 4 ) || (__GNUC__ == 4 && __GNUC_MINOR__ > 4 )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
		io[nio].iov_base = (char *) sp->path ; // gives a spurious compiler error -- constant is OK HERE!
#pragma GCC diagnostic pop
#else
		io[nio].iov_base = (char *) sp->path ; // gives a spurious compiler error -- constant is OK HERE!
#endif
		io[nio].iov_len = strlen(sp->path) + 1; // we're adding the null (though it isn't required post 2.9p3)
		payload =  io[nio].iov_len; 
		nio++;
	}

	// Next block, data (only for writes)
	if ((sp->datasize>0) && (sp->data!=NULL)) {	// send data only for writes (if datasize not zero)
		io[nio].iov_base = sp->data ;
		io[nio].iov_len = sp->datasize ;
		payload += sp->datasize;
		nio++;
	}

	if ( server_type ) {
		printf("---------------- Server type = YES\n");
		tokens = sp->tokens;

		// Next block prior tokens (if an owserver)
		if (tokens > 0) {		// owserver: send prior tags
			io[nio].iov_base = sp->tokenstring;
			io[nio].iov_len = tokens * sizeof(struct antiloop);
			nio++;
		}
		// Final block, new token (if an owserver)
		
		++tokens;
		
		if ( tokens == MakeServermessage ) {
			LEVEL_DEBUG( "Too long a list of tokens -- %d owservers in the chain",tokens);
			return -ELOOP ;
		}
		
		io[nio].iov_base = &(Globals.Token);	// owserver: add our tag
		io[nio].iov_len = sizeof(struct antiloop);

		// put token information in header (lower 17 bits of version)
		sm->version |= MakeServermessage; // bit 17
		sm->version |= MakeServertokens(tokens); // lower 16 bits
		nio++;
	} else {
		printf("---------------- Server type = NO\n");
	}
	
	// First block to send, the header
	// revisit now that the header values are set

	// encode in network order (just the header)
	net_sm.version       = htonl( sm->version       );
	net_sm.payload       = htonl( payload           );
	net_sm.size          = htonl( sm->size          );
	net_sm.type          = htonl( sm->type          );
	net_sm.control_flags = htonl( sm->control_flags );
	net_sm.offset        = htonl( sm->offset        );

	// set the header into the first (index=0) block
	io[0].iov_base = &net_sm;
	io[0].iov_len = sizeof(struct server_msg);

	// debug data on packet
	LEVEL_DEBUG("version=%u payload=%d size=%d type=%d SG=%X offset=%d",sm->version,payload,sm->size,sm->type,sm->control_flags,sm->offset);

	// Here is some traffic display code
	{
		int traffic_counter ;
		traffic_counter = 0 ;
		TrafficOutFD("write header" ,io[traffic_counter].iov_base,io[traffic_counter].iov_len,file_descriptor);
		++traffic_counter;
		TrafficOutFD("write path"  ,io[traffic_counter].iov_base,io[traffic_counter].iov_len,file_descriptor);
		if ((sp->datasize>0) && (sp->data!=NULL)) {	// send data only for writes (if datasize not zero)
			++traffic_counter;
			TrafficOutFD("write data" ,io[traffic_counter].iov_base,io[traffic_counter].iov_len,file_descriptor);
		}
		if ( server_type ) {
			if (sp->tokens > 0) {		// owserver: send prior tags
				++traffic_counter;
				TrafficOutFD("write old tokens" ,io[traffic_counter].iov_base,io[traffic_counter].iov_len,file_descriptor);
			}
			++traffic_counter;
			TrafficOutFD("write new tokens" ,io[traffic_counter].iov_base,io[traffic_counter].iov_len,file_descriptor);
		}
	}
	// End traffic display code

	// Actual write of data to owserver
	return writev(file_descriptor, io, nio) != (ssize_t) (payload + sizeof(struct server_msg) + tokens * sizeof(struct antiloop));
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupControlFlags(const struct parsedname *pn)
{
	uint32_t control_flags = pn->control_flags;

	control_flags &= ~PERSISTENT_MASK;
	if (Globals.no_persistence == 0) {
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

/* Clean up at end of routine,
   either leave connection open and persistent flag available,
   or close
*/
static void Release_Persistent( struct server_connection_state * scs, int granted )
{
	if ( granted == 0 ) {
		Close_Persistent( scs ) ;
		return ;
	}
	
	if ( FILE_DESCRIPTOR_NOT_VALID( scs->file_descriptor) ) {
		Close_Persistent( scs ) ;
		return ;
	}
	
	if (scs->persistence == persistent_no) {		
		// non-persistence from the start
		Close_Persistent( scs ) ;
		return ;
	}

	// mark as available
	BUSLOCKIN(scs->in);
	scs->in->pown->file_descriptor = scs->file_descriptor;
	BUSUNLOCKIN(scs->in);
	scs->persistence = persistent_no ; // we no longer own this connection
	scs->file_descriptor = FILE_DESCRIPTOR_BAD ;
}
