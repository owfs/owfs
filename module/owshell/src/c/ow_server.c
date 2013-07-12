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

#include "owshell.h"

static int FromServer(int file_descriptor, struct client_msg *cm, char *msg, size_t size);
static void *FromServerAlloc(int file_descriptor, struct client_msg *cm);
static int ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp);
static uint32_t SetupSemi(void);
static void Write( char * buffer, int length ) ;

void Server_detect(void)
{
	if (count_inbound_connections == 0) {
		PRINT_ERROR("No owserver_connection defined\n");
		errno = ENOENT ;
		Exit(2);
	}
	if (owserver_connection->name == NULL || ClientAddr(owserver_connection->name)) {
		PRINT_ERROR("Could not connect with owserver %s\n", owserver_connection->name);
		errno = ENOENT ;
		Exit(2);
	}
}

int ServerRead(ASCII * path)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { path, NULL, 0, NULL, 0, };
	int connectfd = ClientConnect();
	int size = 65536;
	char buf[size + 1];
	int ret = 0;

	if (connectfd < 0) {
		return -EIO;
	}
	//printf("Read <%s>\n",path);
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = size;
	sm.offset = offset_into_data;

	if ( size_of_data >=0 && size_of_data <= 65536 ) {
		sm.size = size_of_data ;
	}

	if (ToServer(connectfd, &sm, &sp)) {
		PRINT_ERROR("ServerRead: Error sending request for %s\n", path);
		ret = -EIO;
	} else if (FromServer(connectfd, &cm, buf, size) < 0) {
		PRINT_ERROR("ServerRead: Error receiving data on %s\n", path);
		ret = -EIO;
	} else {
		ret = cm.ret;
		if (ret < 0 || ret > size ) {
			PRINT_ERROR("ServerRead: Data error on %s\n", path);
		} else {
			Write( buf, ret ) ;
		}
	}
	close(connectfd);
	return ret;
}

int ServerWrite(ASCII * path, ASCII * data)
{
	struct server_msg sm;
	struct client_msg cm;
	int size = strlen(data);
	struct serverpackage sp = { path, (BYTE *) data, size, NULL, 0, };
	int connectfd = ClientConnect();
	int ret = 0;

	if (connectfd < 0) {
		return -EIO;
	}
	//printf("Write <%s> <%s>\n",path,data);
	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_write;
	sm.size = size;
	sm.offset = offset_into_data;

	if (ToServer(connectfd, &sm, &sp)) {
		ret = -EIO;
	} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
		ret = -EIO;
	} else {
		ret = cm.ret;
	}
	close(connectfd);
	return ret;
}

int ServerDirall(ASCII * path)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { path, NULL, 0, NULL, 0, };
	char *path2;
	int connectfd = ClientConnect();

	if (connectfd < 0) {
		return -EIO;
	}
	//printf("DirALL <%s>\n",path ) ;
	
	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = slashflag ? msg_dirallslash : msg_dirall;

	if (ToServer(connectfd, &sm, &sp)) {
		cm.ret = -EIO;
	} else if ((path2 = FromServerAlloc(connectfd, &cm))) {
		char *p;
		path2[cm.payload - 1] = '\0';	/* Ensure trailing null */
		for (p = path2; *p; ++p)
			if (*p == ',')
				*p = '\n';
		printf("%s\n", path2);
		free(path2);
	}
	close(connectfd);
	return cm.ret;
}

int ServerDir(ASCII * path)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { path, NULL, 0, NULL, 0, };
	int connectfd = ClientConnect();

	if (connectfd < 0) {
		return -EIO;
	}
	//printf("Dir <%s>\n", path ) ;
	
	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_dir;

	if (ToServer(connectfd, &sm, &sp)) {
		cm.ret = -EIO;
	} else {
		char *path2;
		while ((path2 = FromServerAlloc(connectfd, &cm))) {
			path2[cm.payload - 1] = '\0';	/* Ensure trailing null */
			printf("%s\n", path2);
			free(path2);
		}
	}
	close(connectfd);
	return cm.ret;
}

int ServerPresence(ASCII * path)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { path, NULL, 0, NULL, 0, };
	int connectfd = ClientConnect();
	int ret = 0;

	if (connectfd < 0) {
		return -EIO;
	}
	//printf("Presence <%s>\n",path);
	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_presence;

	if (ToServer(connectfd, &sm, &sp)) {
		ret = -EIO;
	} else if (FromServer(connectfd, &cm, NULL, 0) < 0) {
		ret = -EIO;
	} else {
		ret = cm.ret;
	}
	close(connectfd);
	printf((ret >= 0) ? "1" : "0");
	return ret;
}

/* read from server, free return pointer if not Null */
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

	//printf("FromServerAlloc payload=%d size=%d ret=%d sg=%X offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
	//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
	if (cm->payload == 0)
		return NULL;
	if (cm->ret < 0)
		return NULL;
	if (cm->payload > MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE) {
		//printf("FromServerAlloc payload too large\n");
		return NULL;
	}

	if ((msg = (char *) malloc((size_t) cm->payload))) {
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

// should be const char * data but iovec has problems with const arguments
static int ToServer(int file_descriptor, struct server_msg *sm, struct serverpackage *sp)
{
	int payload = 0;
	int nio = 0;
	struct iovec io[5] = { {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, };

	// Set up flags
	sm->sg = SetupSemi() ;

	// First block to send, the header
	io[nio].iov_base = sm;
	io[nio].iov_len = sizeof(struct server_msg);
	nio++;

	// Next block, the path
	if ((io[nio].iov_base = sp->path)) {	// send path (if not null)
		io[nio].iov_len = payload = strlen(sp->path) + 1;
		nio++;
		//printf("ToServer path=%s\n", sp->path);
	}

	// next block, data (only for writes)
	if ((sp->datasize>0) && (io[nio].iov_base = sp->data)) {	// send data only for writes (if datasize not zero)
		payload += (io[nio].iov_len = sp->datasize);
		nio++;
        //LEVEL_DEBUG("ToServer data=%s\n", sp->data);
	}

	sp->tokens = 0;
	sm->version = 0; // shouldn't this be MakeServerprotocol(OWSERVER_PROTOCOL_VERSION);

	//printf("ToServer payload=%d size=%d type=%d tempscale=%X offset=%d\n",payload,sm->size,sm->type,sm->sg,sm->offset);
	//printf("<%.4d|%.4d\n",sm->type,payload);

	// encode in network order (just the header)
	sm->version = htonl(sm->version);
	sm->payload = htonl(payload);
	sm->size = htonl(sm->size);
	sm->type = htonl(sm->type);
	sm->sg = htonl(sm->sg);
	sm->offset = htonl(sm->offset);

	//Debug_Writev(io, nio);
	return writev(file_descriptor, io, nio) != (ssize_t) (payload + sizeof(struct server_msg) + sp->tokens * sizeof(union antiloop));
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
/* Also ALIAS_REQUEST means that aliases will be applied */
static uint32_t SetupSemi(void)
{
	uint32_t sg = SHOULD_RETURN_BUS_LIST ;

	// Device format
	sg |= (device_format) << DEVFORMAT_BIT ;
	// Pressure scale
	sg |= (pressure_scale) << PRESSURESCALE_BIT ;
	// Temperature scale
	sg |= (temperature_scale) << TEMPSCALE_BIT ;
	// Uncached
	sg |= uncached ? UNCACHED : 0 ;
	// Unaliased
	sg |= unaliased ? 0 : ALIAS_REQUEST ;
	// OWNet flag or Presence check
	sg |= OWNET ;

	return sg;
}

static void Write( char * buffer, int length)
 {
	int i ;
	char *fmt;

	/*
 	 * Write binary (raw) or hex-formatted ASCII depending on
	 * the value of "hexflag" (set through the CLI argument
	 * --hex).
	 */
	fmt = hexflag ? "%.2X" : "%c";

	for ( i=0 ; i<length ; ++i ) {
		printf( fmt, (unsigned char) buffer[i]);
	}
 }
