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
#include "ow_server.h"

struct server_connection_state {
	FILE_DESCRIPTOR_OR_ERROR file_descriptor ;
	enum persistent_state { persistent_yes, persistent_no, } persistence ;
	struct connection_in * in ;
} ;

static uint32_t SetupSemi(int persistent);

static void Release_Persistent( struct server_connection_state * scs, int granted ) ;
static void Close_Persistent( struct server_connection_state * scs) ;
static int To_Server( struct server_connection_state * scs, struct server_msg * sm, struct serverpackage *sp) ;
static int WriteToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp);
static int From_Server( struct server_connection_state * scs, struct client_msg *cm, char *msg, size_t size);
static void *From_ServerAlloc(struct server_connection_state * scs, struct client_msg *cm) ;

static int ServerDIR(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp);
static int ServerDIRALL(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp);

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
	in->file_descriptor = FILE_DESCRIPTOR_BAD;	// No persistent connection yet
	in->busmode = bus_zero;
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
	in->file_descriptor = FILE_DESCRIPTOR_BAD;	// No persistent connection yet
	in->busmode = bus_server;
	return 0;
}

// Send to an owserver using the READ message
int ServerRead(struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	struct server_connection_state scs ;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = rp->data_length;
	sm.offset = rp->data_offset;
	scs.persistence = persistent_yes ;
	scs.persistence = persistent_yes ;
	scs.in =rp->owserver ;

	LEVEL_CALL("SERVER READ path=%s\n", SAFESTRING(rp->path));

	// Send to owserver
	sm.control_flags = SetupSemi(persistent);
	if ( To_Server( &scs, &sm, &sp) == 1 ) {
		Release_Persistent( &scs, 0);
		return -EIO ;
	}
	
	// Receive from owserver
	if ( From_Server( &scs, &cm, (ASCII *) rp->read_value, rp->data_length) < 0 ) {
		Release_Persistent( &scs, 0);
		return -EIO ;
	}
	Release_Persistent( &scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}

// Send to an owserver using the PRESENT message
int ServerPresence(struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	struct server_connection_state scs ;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_presence;
	scs.persistence = persistent_yes ;
	scs.in =rp->owserver ;

	LEVEL_CALL("SERVER PRESENCE path=%s\n", SAFESTRING(rp->path));

	// Send to owserver
	sm.control_flags = SetupSemi(persistent);
	if ( To_Server( &scs, &sm, &sp) == 1 ) {
		Release_Persistent( &scs, 0 ) ;
		return 1 ;
	}

	// Receive from owserver
	if (From_Server(&scs, &cm, NULL, 0) < 0) {
		Release_Persistent(&scs, 0 );
		return 1 ;
	}

	Release_Persistent(&scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}


// Send to an owserver using the WRITE message
int ServerWrite(struct request_packet *rp)
{
	struct server_msg sm;
	struct client_msg cm;
#if ( __GNUC__ > 4 ) || (__GNUC__ == 4 && __GNUC_MINOR__ > 4 )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
	struct serverpackage sp = { rp->path, (BYTE *) rp->write_value, rp->data_length, rp->tokenstring, rp->tokens, };
#pragma GCC diagnostic pop	
#else
	struct serverpackage sp = { rp->path, (BYTE *) rp->write_value, rp->data_length, rp->tokenstring, rp->tokens, };
#endif
	int persistent = 1;
	struct server_connection_state scs ;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_write;
	sm.size = rp->data_length;
	sm.offset = rp->data_offset;
	scs.persistence = persistent_yes ;
	scs.in =rp->owserver ;

	LEVEL_CALL("SERVER WRITE path=%s\n", SAFESTRING(rp->path));

	// Send to owserver
	sm.control_flags = SetupSemi(persistent);
	if ( To_Server( &scs, &sm, &sp) == 1 ) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}

	// Receive from owserver
	if ( From_Server( &scs, &cm, NULL, 0) < 0) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}
	{
		uint32_t control_flags = cm.control_flags & ~(SHOULD_RETURN_BUS_LIST | PERSISTENT_MASK );
		if (ow_Global.control_flags != control_flags) {
			// replace control flags (except safemode persists)
			ow_Global.control_flags = control_flags;
		}
	}

	Release_Persistent(&scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}

// Send to an owserver using either the DIR or DIRALL message
int ServerDir(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp)
{
	int ret;
	// Do we know this server doesn't support DIRALL?
	if (rp->owserver->tcp.no_dirall) {
		return ServerDIR(dirfunc, v, rp);
	}
	// try DIRALL and see if supported
	if ((ret = ServerDIRALL(dirfunc, v, rp)) == -ENOMSG) {
		rp->owserver->tcp.no_dirall = 1;
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
	struct server_connection_state scs ;
	char *return_path;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dir;
	scs.persistence = persistent_yes ;
	scs.in =rp->owserver ;

	LEVEL_CALL("SERVER DIR path=%s\n", SAFESTRING(rp->path));

	// Send to owserver
	sm.control_flags = SetupSemi(persistent);
	if ( To_Server( &scs, &sm, &sp) == 1 ) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}

	// Receive from owserver -- in a loop for each directory entry
	while ( (return_path = From_ServerAlloc(&scs, &cm))  != NULL ) {
		return_path[cm.payload - 1] = '\0';	/* Ensure trailing null */
		LEVEL_DEBUG("ServerDir: got=[%s]\n", return_path);
		dirfunc(v, return_path);
		free(return_path);
	}

	Release_Persistent(&scs, cm.control_flags & PERSISTENT_MASK);
	return cm.ret;
}

// Send to an owserver using the DIRALL message
static int ServerDIRALL(void (*dirfunc) (void *, const char *), void *v, struct request_packet *rp)
{
	ASCII *comma_separated_list;
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { rp->path, NULL, 0, rp->tokenstring, rp->tokens, };
	int persistent = 1;
	struct server_connection_state scs ;

	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_dirall;
	scs.persistence = persistent_yes ;
	scs.in =rp->owserver ;

	LEVEL_CALL("SERVER DIRALL path=%s\n", SAFESTRING(rp->path));

	// Send to owserver
	sm.control_flags = SetupSemi(persistent);
	if ( To_Server( &scs, &sm, &sp) == 1 ) {
		Release_Persistent( &scs, 0 ) ;
		return -EIO ;
	}

	// Receive from owserver
	comma_separated_list = From_ServerAlloc(&scs, &cm);
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

	Release_Persistent( &scs, cm.control_flags & PERSISTENT_MASK );
	return cm.ret;
}
/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupSemi(int persistent)
{
	uint32_t sg = ow_Global.control_flags;

	sg &= ~PERSISTENT_MASK;
	if (persistent) {
		sg |= PERSISTENT_MASK;
	}

	/* End user -- add full bus list and apply aliases */
	sg |= SHOULD_RETURN_BUS_LIST | ALIAS_REQUEST ;

	return sg;
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
	
	if ( scs->file_descriptor <= FILE_DESCRIPTOR_BAD  ) {
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
	scs->in->file_descriptor = scs->file_descriptor;
	BUSUNLOCKIN(scs->in);
	scs->persistence = persistent_no ; // we no longer own this connection
	scs->file_descriptor = FILE_DESCRIPTOR_BAD ;
}

static void Close_Persistent( struct server_connection_state * scs)
{
	// First set up the file descriptor based on persistent state
	if (scs->persistence == persistent_yes) {
		// no persistence wanted
		BUSLOCKIN(scs->in);
			scs->in->file_descriptor = FILE_DESCRIPTOR_BAD ;
		BUSUNLOCKIN(scs->in);
	}
	
	scs->persistence = persistent_no ;
	if ( scs->file_descriptor > FILE_DESCRIPTOR_BAD ) {
		close(scs->file_descriptor) ;
		scs->file_descriptor = FILE_DESCRIPTOR_BAD ;
	}
}

/* return 0 good, 1 bad */
static int To_Server( struct server_connection_state * scs, struct server_msg * sm, struct serverpackage *sp)
{
	struct connection_in * in = scs->in ; // for convenience
	BYTE test_read[1] ;
	int old_flags ;
	ssize_t rcv_value ;
	int saved_errno ;
	
	// initialize the variables
	scs->file_descriptor = FILE_DESCRIPTOR_BAD ;

	// First set up the file descriptor based on persistent state
	if (scs->persistence == persistent_no) {		
		// no persistence wanted
		scs->file_descriptor = ClientConnect(in);
	} else {
		// Persistence desired
		BUSLOCKIN(in);
		switch ( in->file_descriptor ) {
			case FILE_DESCRIPTOR_PERSISTENT_IN_USE:
				// Currently in use, so make new non-persistent connection
				scs->file_descriptor = ClientConnect(in);
				scs->persistence = persistent_no ;
				break ;
			case FILE_DESCRIPTOR_BAD:
				// no conection currently, so make a new one
				scs->file_descriptor = ClientConnect(in);
				if ( scs->file_descriptor > FILE_DESCRIPTOR_BAD ) {
					in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE ;
				}	
				break ;
			default:
				// persistent connection idle and waiting for use
				// connection_in is locked so this is safe
				scs->file_descriptor = in->file_descriptor;
				in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE;
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
			if ( scs->file_descriptor > FILE_DESCRIPTOR_BAD ) {
				in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE ;
			}
			break ;
		default:
			// data to be read, so a good connection
			break ;
	}

	// Now test
	if ( scs->file_descriptor <= FILE_DESCRIPTOR_BAD ) {
		Close_Persistent( scs ) ;
		return 1 ;
	}

	// Do the real work
	if (WriteToServer(scs->file_descriptor, sm, sp) >= 0) {
		// successful message
		return 0;
	}

	// This is where it gets a bit tricky. For non-persistent conections we're done'
	if ( scs->persistence == persistent_no ) {
		// not persistent, so no reconnection needed
		Close_Persistent( scs ) ;
		return 1 ;
	}
	
	// perhaps the persistent connection is stale?
	// Make a new one
	scs->file_descriptor = ClientConnect(in) ;

	// Now retest
	if ( scs->file_descriptor <= FILE_DESCRIPTOR_BAD ) {
		// couldn't make that new connection -- free everything
		Close_Persistent( scs ) ;
		return 1 ;
	}
	
	// Leave in->file_descriptor = FILE_DESCRIPTOR_PERSISTENT_IN_USE

	// Second attempt at the write, now with new connection
	if (WriteToServer(scs->file_descriptor, sm, sp) >= 0) {
		// successful message
		return 0;
	}

	// bad write the second time -- clear everything
	Close_Persistent( scs ) ;
	return 1 ;
}

// should be const char * data but iovec has problems with const arguments
static int WriteToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp)
{
	int payload = 0;
	int tokens = 0;
	int nio = 0;
	struct iovec io[5] = { {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, };
	struct server_msg net_sm ;


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
		io[nio].iov_len = strlen(sp->path) + 1;
		payload =  io[nio].iov_len +1;
		nio++;
		LEVEL_DEBUG("ToServer path=%s\n", sp->path);
	}

	// Next block, data (only for writes)
	if ((sp->datasize>0) && (sp->data!=NULL)) {	// send data only for writes (if datasize not zero)
		io[nio].iov_base = sp->data ;
		io[nio].iov_len = sp->datasize ;
		payload += sp->datasize;
		nio++;
        LEVEL_DEBUG("ToServer data size=%d bytes\n", sp->datasize);
	}

	// Next block, old tokens (there aren't any since this isn't an owserver)
	// Next block, new token (there aren't any since this isn't an owserver)
	sp->tokens = 0; // ownet isn't a server
	
	// First block to send, the header
	// revisit now that the header values are set
	sm->version = MakeServerprotocol(OWSERVER_PROTOCOL_VERSION);

	// encode in network order (just the header)
	net_sm.version       = htonl( sm->version       );
	net_sm.payload       = htonl( payload           );
	net_sm.size          = htonl( sm->size          );
	net_sm.type          = htonl( sm->type          );
	net_sm.control_flags = htonl( sm->control_flags );
	net_sm.offset        = htonl( sm->offset        );

	io[0].iov_base = &net_sm;
	io[0].iov_len = sizeof(struct server_msg);

	// debug data on packet
	LEVEL_DEBUG("version=%u payload=%d size=%d type=%d SG=%X offset=%d",sm->version,payload,sm->size,sm->type,sm->control_flags,sm->offset);

	// Now do the actual write
	return writev(file_descriptor, io, nio) != (ssize_t) (payload + sizeof(struct server_msg) + tokens * sizeof(struct antiloop));
}

/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static int From_Server( struct server_connection_state * scs, struct client_msg *cm, char *msg, size_t size)
{
	size_t rtry;
	size_t actual_read ;
	struct timeval tv1 = { ow_Global.timeout_network + 1, 0, };
	struct timeval tv2 = { ow_Global.timeout_network + 1, 0, };

	do {						// read regular header, or delay (delay when payload<0)
		actual_read = tcp_read(scs->file_descriptor, (BYTE *) cm, sizeof(struct client_msg), &tv1);
		if (actual_read != sizeof(struct client_msg)) {
			cm->size = 0;
			cm->ret = -EIO;
			return -EIO;
		}

		cm->payload       = ntohl(cm->payload);
		cm->size          = ntohl(cm->size);
		cm->ret           = ntohl(cm->ret);
		cm->control_flags = ntohl(cm->control_flags);
		cm->offset        = ntohl(cm->offset);
		
	} while (cm->payload < 0);	// flag to show a delay message

	if (cm->payload == 0) {
		return 0;				// No payload, done.
	}
	rtry = cm->payload < (ssize_t) size ? (size_t) cm->payload : size;
	actual_read = tcp_read(scs->file_descriptor, (BYTE *) msg, rtry, &tv2);	// read expected payload now.
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

/* read from server, you must free return pointer if not Null */
/* Adds an extra null byte at end */
static void *From_ServerAlloc(struct server_connection_state * scs, struct client_msg *cm)
{
	BYTE *msg;
	struct timeval tv1 = { ow_Global.timeout_network + 1, 0, };
	struct timeval tv2 = { ow_Global.timeout_network + 1, 0, };
	size_t actual_size ;

	do {						/* loop until non delay message (payload>=0) */
		actual_size = tcp_read(scs->file_descriptor, (BYTE *) cm, sizeof(struct client_msg), &tv1 );
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
	if (cm->payload > MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE) {
		return NULL;
	}

	msg = malloc((size_t) cm->payload + 1) ;
	if ( msg != NULL ) {
		actual_size = tcp_read(scs->file_descriptor, msg, (size_t) (cm->payload), &tv2 );
		if ((ssize_t)actual_size != cm->payload) {
			cm->payload = 0;
			cm->offset = 0;
			cm->ret = -EIO;
			free(msg);
			msg = NULL;
		}
	}
	if(msg != NULL) {
		msg[cm->payload] = '\0';	// safety NULL
	}
	return msg;
}
