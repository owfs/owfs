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

#include "owshell.h"

static int FromServer(int file_descriptor, struct client_msg *cm, char *msg,
					  size_t size);
static void *FromServerAlloc(int file_descriptor, struct client_msg *cm);
static int ToServer(int file_descriptor, struct server_msg *sm,
					struct serverpackage *sp);
static uint32_t SetupSemi(void);

void Server_detect(void)
{
	if (head_inbound_list == NULL) {
		fprintf(stderr, "No head_inbound_list defined\n");
		exit(1);
	}
	if (head_inbound_list->name == NULL || ClientAddr(head_inbound_list->name, head_inbound_list)) {
		fprintf(stderr, "Could not connect with owserver %s\n",
				head_inbound_list->name);
		exit(1);
	}
}

int ServerRead(ASCII * path)
{
	struct server_msg sm;
	struct client_msg cm;
	struct serverpackage sp = { path, NULL, 0, NULL, 0, };
	int connectfd = ClientConnect();
	int size = 8096;
	char buf[size + 1];
	int ret = 0;

	if (connectfd < 0)
		return -EIO;
	//printf("ServerRead pn->path=%s, size=%d, offset=%u\n",pn->path,size,offset);
	memset(&sm, 0, sizeof(struct server_msg));
	memset(&cm, 0, sizeof(struct client_msg));
	sm.type = msg_read;
	sm.size = size;
	sm.sg = SetupSemi();
	sm.offset = 0;

	if (ToServer(connectfd, &sm, &sp)) {
		fprintf(stderr, "Error sending request for %s\n", path);
		ret = -EIO;
	} else if (FromServer(connectfd, &cm, buf, size) < 0) {
		fprintf(stderr, "Error receiving data on %s\n", path);
		ret = -EIO;
	} else {
		ret = cm.ret;
		if (ret <= 0 || ret > size) {
			fprintf(stderr, "Data error on %s\n", path);
		} else {
			buf[ret] = '\0';
			printf("%s", buf);
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

	if (connectfd < 0)
		return -EIO;
	//printf("ServerWrite path=%s, buf=%*s, size=%d, offset=%d\n",path,size,buf,size,offset);
	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_write;
	sm.size = size;
	sm.sg = SetupSemi();
	sm.offset = 0;

	//printf("ServerRead path=%s\n", pn->path_busless);
	//LEVEL_CALL("SERVER(%d)WRITE path=%s\n", pn->selected_connection->index, SAFESTRING(pn->path_busless));

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
	//printf("DirALL <%s>\n",path ) ;

	if (connectfd < 0)
		return -EIO;

	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_dirall;

	sm.sg = SetupSemi();

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

	//printf("Dir <%s>\n", path ) ;
	if (connectfd < 0)
		return -EIO;

	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_dir;

	sm.sg = SetupSemi();

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

	if (connectfd < 0)
		return -EIO;
	//printf("ServerPresence pn->path=%s\n",pn->path);
	memset(&sm, 0, sizeof(struct server_msg));
	sm.type = msg_presence;

	sm.sg = SetupSemi();

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
	struct timeval tv = { Global.timeout_network + 1, 0, };

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
	if (cm->payload > 65000) {
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
static int FromServer(int file_descriptor, struct client_msg *cm, char *msg,
					  size_t size)
{
	size_t rtry;
	size_t ret;
	struct timeval tv = { Global.timeout_network + 1, 0, };

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
//static int ToServer( int file_descriptor, struct server_msg * sm, const char * path, const char * data, int datasize ) {
static int ToServer(int file_descriptor, struct server_msg *sm,
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

	return writev(file_descriptor, io,
				  5) !=
		(ssize_t) (payload + sizeof(struct server_msg) +
				   sp->tokens * sizeof(union antiloop));
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupSemi(void)
{
	uint32_t sg = SemiGlobal | BUSRET_MASK;
	return sg;
}
