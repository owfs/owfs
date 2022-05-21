/*
 * part of owftpd By Paul H Alfille
 * The whole is GPLv2 licenced though the ftp code was more liberally licenced when first used.
 */

#include "owftpd.h"
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <stdarg.h>

/* space requirements */
#define ADDRPORT_STRLEN 58

/* prototypes */
static int invariant(const struct ftp_session_s *f);
static void reply(struct ftp_session_s *f, int code, const char *fmt, ...);
static void change_dir(struct ftp_session_s *f, const char *new_dir);
static FILE_DESCRIPTOR_OR_ERROR open_connection(struct ftp_session_s *f);
static int write_fully(FILE_DESCRIPTOR_OR_ERROR file_descriptor, const char *buf, int buflen);
static void init_passive_port(void);
static int get_passive_port(void);
static int convert_newlines(char *dst, const char *src, int srclen);
static void get_addr_str(const sockaddr_storage_t * s, char *buf, int bufsiz);
static void send_readme(const struct ftp_session_s *f, int code);
static void netscape_hack(FILE_DESCRIPTOR_OR_ERROR file_descriptor);
static void set_port(struct ftp_session_s *f, const sockaddr_storage_t * host_port);
static FILE_DESCRIPTOR_OR_ERROR set_pasv(struct ftp_session_s *f, sockaddr_storage_t * host_port);
//static int ip_equal(const sockaddr_storage_t *a, const sockaddr_storage_t *b);
static int ip_equal(const sockaddr_storage_t * a, const sockaddr_storage_t * b);
static void get_absolute_fname(char *fname, size_t fname_len, const char *dir, const char *file);
static void both_list(struct ftp_session_s *f, const struct ftp_command_s *cmd, enum file_list_e fle);

/* command handlers */
static void do_user(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_pass(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_cwd(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_cdup(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_quit(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_port(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_pasv(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_type(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_stru(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_mode(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_retr(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_stor(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_pwd(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_nlst(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_list(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_syst(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_noop(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_rest(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_lprt(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_lpsv(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_eprt(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_epsv(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_size(struct ftp_session_s *f, const struct ftp_command_s *cmd);
static void do_mdtm(struct ftp_session_s *f, const struct ftp_command_s *cmd);

static struct {
	char *name;
	void (*func) (struct ftp_session_s * f, const struct ftp_command_s * cmd);
} command_func[] = {
	{
	"USER", do_user}, {
	"PASS", do_pass}, {
	"CWD", do_cwd}, {
	"CDUP", do_cdup}, {
	"QUIT", do_quit}, {
	"PORT", do_port}, {
	"PASV", do_pasv}, {
	"LPRT", do_lprt}, {
	"LPSV", do_lpsv}, {
	"EPRT", do_eprt}, {
	"EPSV", do_epsv}, {
	"TYPE", do_type}, {
	"STRU", do_stru}, {
	"MODE", do_mode}, {
	"RETR", do_retr}, {
	"STOR", do_stor}, {
	"PWD", do_pwd}, {
	"NLST", do_nlst}, {
	"LIST", do_list}, {
	"SYST", do_syst}, {
	"NOOP", do_noop}, {
	"REST", do_rest}, {
	"SIZE", do_size}, {
	"MDTM", do_mdtm}
};

#define NUM_COMMAND_FUNC (sizeof(command_func) / sizeof(command_func[0]))

int ftp_session_init(struct ftp_session_s *f,
					 const sockaddr_storage_t * client_addr, const sockaddr_storage_t * server_addr, struct telnet_session_s *t, const char *dir)
{
	daemon_assert(f != NULL);
	daemon_assert(client_addr != NULL);
	daemon_assert(server_addr != NULL);
	daemon_assert(t != NULL);
	daemon_assert(dir != NULL);
	daemon_assert(strlen(dir) <= PATH_MAX);

#ifdef INET6
	/* if the control connection is on IPv6, we need to get an IPv4 address */
	/* to bind the socket to */
	if (SSFAM(server_addr) == AF_INET6) {
		struct addrinfo hints;
		struct addrinfo *res;
		int errcode;

		/* getaddrinfo() does the job nicely */
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;
		if (getaddrinfo(NULL, "ftp", &hints, &res) != 0) {
			ERROR_CONNECT("Unable to determing IPv4 address; %s", gai_strerror(errcode));
			return 0;
		}

		/* let's sanity check */
		daemon_assert(res != NULL);
		daemon_assert(sizeof(f->server_ipv4_addr) >= res->ai_addrlen);
		daemon_assert(SSFAM(host_port) == AF_INET);

		/* copy the result and free memory as necessary */
		memcpy(&f->server_ipv4_addr, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
	} else {
		daemon_assert(SSFAM(host_port) == AF_INET);
		f->server_ipv4_addr = *server_addr;
	}
#else
	f->server_ipv4_addr = *server_addr;
#endif

	f->session_active = 1;
	f->command_number = 0;

	f->data_type = TYPE_ASCII;
	f->file_structure = STRU_FILE;

	f->file_offset = 0;
	f->file_offset_command_number = ULONG_MAX;

	f->epsv_all_set = 0;

	f->client_addr = *client_addr;
	get_addr_str(client_addr, f->client_addr_str, sizeof(f->client_addr_str));

	f->server_addr = *server_addr;

	f->telnet_session = t;
	daemon_assert(strlen(dir) < sizeof(f->dir));
	strncpy(f->dir, dir, sizeof(f->dir) - 1);

	f->data_channel = DATA_PORT;
	f->data_port = *client_addr;
	f->server_fd = FILE_DESCRIPTOR_BAD;

	daemon_assert(invariant(f));

	return 1;
}

void ftp_session_drop(struct ftp_session_s *f, const char *reason)
{
	daemon_assert(invariant(f));
	daemon_assert(reason != NULL);

	/* say goodbye */
	reply(f, 421, "%s.", reason);

	daemon_assert(invariant(f));
}

void ftp_session_run(struct ftp_session_s *f, struct watched_s *watched)
{
	char buf[2048];
	int len;
	struct ftp_command_s cmd;
	size_t i;

	daemon_assert(invariant(f));
	daemon_assert(watched != NULL);

	/* record our watchdog */
	f->watched = watched;

	/* say hello */
	send_readme(f, 220);
	reply(f, 220, "Service ready for new user.");

	/* process commands */
	while (f->session_active && telnet_session_readln(f->telnet_session, buf, sizeof(buf))) {

		/* delay our timeout based on this input */
		watchdog_defer_watched(f->watched);

		/* increase our command count */
		if (f->command_number == ULONG_MAX) {
			f->command_number = 0;
		} else {
			f->command_number++;
		}

		/* make sure we read a whole line */
		len = strlen(buf);
		if (buf[len - 1] != '\n') {
			reply(f, 500, "Command line too long.");
			while (telnet_session_readln(f->telnet_session, buf, sizeof(buf))) {
				len = strlen(buf);
				if (buf[len - 1] == '\n') {
					break;
				}
			}
			goto next_command;
		}

		syslog(LOG_DEBUG, "%s %s", f->client_addr_str, buf);

		/* parse the line */
		if (!ftp_command_parse(buf, &cmd)) {
			reply(f, 500, "Syntax error, command unrecognized.");
			goto next_command;
		}

		/* dispatch the command */
		for (i = 0; i < NUM_COMMAND_FUNC; i++) {
			if (strcmp(cmd.command, command_func[i].name) == 0) {
				(command_func[i].func) (f, &cmd);
				goto next_command;
			}
		}

		/* oops, we don't have this command (shouldn't happen - shrug) */
		reply(f, 502, "Command not implemented.");

	  next_command:{
		}
	}

	daemon_assert(invariant(f));
}

void ftp_session_destroy(struct ftp_session_s *f)
{
	daemon_assert(invariant(f));

	Test_and_Close( & f->server_fd ) ;
}

#ifndef NDEBUG
static int invariant(const struct ftp_session_s *f)
{
	int len;

	if (f == NULL) {
		return 0;
	}
	if ((f->session_active != 0) && (f->session_active != 1)) {
		return 0;
	}
	if ((f->data_type != TYPE_ASCII) && (f->data_type != TYPE_IMAGE)) {
		return 0;
	}
	if ((f->file_structure != STRU_FILE)
		&& (f->file_structure != STRU_RECORD)) {
		return 0;
	}
	if (f->file_offset < 0) {
		return 0;
	}
	if ((f->epsv_all_set != 0) && (f->epsv_all_set != 1)) {
		return 0;
	}
	len = strlen(f->client_addr_str);
	if ((len <= 0) || (len >= ADDRPORT_STRLEN)) {
		return 0;
	}
	if (f->telnet_session == NULL) {
		return 0;
	}
	switch (f->data_channel) {
	case DATA_PORT:
		/* If the client specifies a port, verify that it is from the   */
		/* host the client connected from.  This prevents a client from */
		/* using the server to open connections to arbritrary hosts.    */
		if (!ip_equal(&f->client_addr, &f->data_port)) {
			return 0;
		}
		if ( FILE_DESCRIPTOR_NOT_VALID(f->server_fd)) {
			return 0;
		}
		break;
	case DATA_PASSIVE:
		if ( FILE_DESCRIPTOR_NOT_VALID(f->server_fd)) {
			return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}
#endif							/* NDEBUG */

static void reply(struct ftp_session_s *f, int code, const char *fmt, ...)
{
	char buf[256];
	va_list ap;

	daemon_assert(invariant(f));
	daemon_assert(code >= 100);
	daemon_assert(code <= 559);
	daemon_assert(fmt != NULL);

	/* prepend our code to the buffer */
	sprintf(buf, "%d", code);
	buf[3] = ' ';

	/* add the formatted output of the caller to the buffer */
	va_start(ap, fmt);
	vsnprintf(buf + 4, sizeof(buf) - 4, fmt, ap);
	va_end(ap);

	/* log our reply */
	LEVEL_DATA("%s %s", f->client_addr_str, buf);

	/* send the output to the other side */
	telnet_session_println(f->telnet_session, buf);

	daemon_assert(invariant(f));
}

static void do_user(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const char *user;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	user = cmd->arg[0].string;
	if (strcasecmp(user, "ftp") && strcasecmp(user, "anonymous")) {
		LEVEL_CONNECT("%s attempted to log in as \"%s\"", f->client_addr_str, user);
		//reply(f, 530, "Only anonymous FTP supported.");
		reply(f, 331, "Force Anonymous. Send e-mail address as password.");
	} else {
		reply(f, 331, "Send e-mail address as password.");
	}
	daemon_assert(invariant(f));
}

static void do_pass(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const char *password;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	password = cmd->arg[0].string;
	LEVEL_CONNECT("%s reports e-mail address \"%s\"", f->client_addr_str, password);
	reply(f, 230, "User logged in, proceed.");

	daemon_assert(invariant(f));
}

#ifdef INET6
static void get_addr_str(const sockaddr_storage_t * s, char *buf, int bufsiz)
{
	int port;
	int error;
	int len;

	daemon_assert(s != NULL);
	daemon_assert(buf != NULL);

	/* buf must be able to contain (at least) a string representing an
	 * ipv4 addr, followed by the string " port " (6 chars) and the port
	 * number (which is 5 chars max), plus the '\0' character. */
	daemon_assert(bufsiz >= (INET_ADDRSTRLEN + 12));

	error = getnameinfo(client_addr, sizeof(sockaddr_storage_t), buf, bufsiz, NULL, 0, NI_NUMERICHOST);
	/* getnameinfo() should never fail when called with NI_NUMERICHOST */
	daemon_assert(error == 0);

	len = strlen(buf);
	daemon_assert(bufsiz >= len + 12);
	snprintf(buf + len, bufsiz - len, " port %d", ntohs(SINPORT(&f->client_addr)));
}
#else
static void get_addr_str(const sockaddr_storage_t * s, char *buf, int bufsiz)
{
	unsigned int addr;
	int port;

	daemon_assert(s != NULL);
	daemon_assert(buf != NULL);

	/* buf must be able to contain (at least) a string representing an
	 * ipv4 addr, followed by the string " port " (6 chars) and the port
	 * number (which is 5 chars max), plus the '\0' character. */
	daemon_assert(bufsiz >= (INET_ADDRSTRLEN + 12));

	addr = ntohl(s->sin_addr.s_addr);
	port = ntohs(s->sin_port);
	snprintf(buf, bufsiz, "%d.%d.%d.%d port %d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff, port);
}
#endif

static void do_cwd(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const char *new_dir;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	new_dir = cmd->arg[0].string;
	change_dir(f, new_dir);

	daemon_assert(invariant(f));
}

static void do_cdup(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	change_dir(f, "..");

	daemon_assert(invariant(f));
}

static void change_dir(struct ftp_session_s *f, const char *new_dir)
{
	struct cd_parse_s cps;

	strcpy(cps.buffer, (ASCII *) f->dir);
	cps.rest = new_dir;
	cps.pse = parse_status_init;

	LEVEL_DEBUG("CD dir=%s, file=%s", SAFESTRING(cps.buffer), SAFESTRING(new_dir));
	FileLexCD(&cps);

	switch (cps.solutions) {
	case 0:
		cps.ret = -ENOENT;
	case 1:
		break;
	default:
		cps.ret = -EFAULT;
	}
	if (cps.dir == NULL) {
		cps.ret = -ENOMEM;
	}

	if (cps.ret) {
		reply(f, 550, "Error changing directory. %s", strerror(-cps.ret));
	} else {
		f->dir[PATH_MAX] = '\0' ; // trailing null
		strncpy(f->dir, cps.dir, PATH_MAX);
		reply(f, 250, "Directory change successful.");
	}

	if (cps.dir) {
		owfree(cps.dir);
	}
}

static void do_quit(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	reply(f, 221, "Service closing control connection.");
	f->session_active = 0;

	daemon_assert(invariant(f));
}

/* support for the various port setting functions */
static void set_port(struct ftp_session_s *f, const sockaddr_storage_t * host_port)
{
	daemon_assert(invariant(f));
	daemon_assert(host_port != NULL);

	if (f->epsv_all_set) {
		reply(f, 500, "After EPSV ALL, only EPSV allowed.");
	} else if (!ip_equal(&f->client_addr, host_port)) {
		reply(f, 500, "Port must be on command channel IP.");
	} else if (ntohs(cSINPORT(host_port)) < IPPORT_RESERVED) {
		reply(f, 500, "Port may not be less than 1024, which is reserved.");
	} else {
		/* close any outstanding PASSIVE port */
		if (f->data_channel == DATA_PASSIVE) {
			Test_and_Close( & f->server_fd ) ;
		}

		f->data_channel = DATA_PORT;
		f->data_port = *host_port;
		reply(f, 200, "Command okay.");
	}

	daemon_assert(invariant(f));
}

/* set IP and port for client to receive data on */
static void do_port(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const sockaddr_storage_t *host_port;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	host_port = &cmd->arg[0].host_port;
	daemon_assert(cSSFAM(host_port) == AF_INET);

	set_port(f, host_port);

	daemon_assert(invariant(f));
}

/* set IP and port for client to receive data on, transport independent */
static void do_lprt(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const sockaddr_storage_t *host_port;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	host_port = &cmd->arg[0].host_port;

#ifdef INET6
	if ((cSSFAM(host_port) != AF_INET) && (cSSFAM(host_port) != AF_INET6)) {
		reply(f, 521, "Only IPv4 and IPv6 supported, address families (4,6)");
	}
#else
	if (cSSFAM(host_port) != AF_INET) {
		reply(f, 521, "Only IPv4 supported, address family (4)");
	}
#endif

	set_port(f, host_port);

	daemon_assert(invariant(f));
}

/* set IP and port for the client to receive data on, IPv6 extension */
/*                                                                   */
/* RFC 2428 specifies that if the data connection is going to be on  */
/* the same IP as the control connection, EPSV must be used.  Since  */
/* that is the only mode of transfer we support, we reject all EPRT  */
/* requests.                                                         */
static void do_eprt(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	reply(f, 500, "EPRT not supported, use EPSV.");

	daemon_assert(invariant(f));
}

/* support for the various pasv setting functions */
/* returns the file descriptor of the bound port, or -1 on error */
/* note: the "host_port" parameter will be modified, having its port set */
static FILE_DESCRIPTOR_OR_ERROR set_pasv(struct ftp_session_s *f, sockaddr_storage_t * bind_addr)
{
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	int port;

	daemon_assert(invariant(f));
	daemon_assert(bind_addr != NULL);

	socket_fd = socket(SSFAM(bind_addr), SOCK_STREAM, 0);
	if (FILE_DESCRIPTOR_NOT_VALID(socket_fd)) {
		reply(f, 500, "Error creating server socket; %s.", strerror(errno));
		return FILE_DESCRIPTOR_BAD;
	}
//	fcntl (socket_fd, F_SETFD, FD_CLOEXEC); // for safe forking

	for (;;) {
		port = get_passive_port();
		SINPORT(bind_addr) = htons(port);
		if (bind(socket_fd, (struct sockaddr *) bind_addr, sizeof(struct sockaddr)) == 0) {
			break;
		}
		if (errno != EADDRINUSE) {
			reply(f, 500, "Error binding server port; %s.", strerror(errno));
			Test_and_Close(&socket_fd);
			return FILE_DESCRIPTOR_BAD;
		}
	}

	if (listen(socket_fd, 1) != 0) {
		reply(f, 500, "Error listening on server port; %s.", strerror(errno));
		Test_and_Close(&socket_fd);
		return FILE_DESCRIPTOR_BAD;
	}

	return socket_fd;
}

/* pick a server port to listen for connection on */
static void do_pasv(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	unsigned int addr;
	int port;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	if (f->epsv_all_set) {
		reply(f, 500, "After EPSV ALL, only EPSV allowed.");
		goto exit_pasv;
	}

	socket_fd = set_pasv(f, &f->server_ipv4_addr);
	if (FILE_DESCRIPTOR_NOT_VALID(socket_fd)) {
		goto exit_pasv;
	}

	/* report port to client */
	addr = ntohl(f->server_ipv4_addr.sin_addr.s_addr);
	port = ntohs(f->server_ipv4_addr.sin_port);
	reply(f, 227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).",
		  addr >> 24, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff, port >> 8, port & 0xff);

	/* close any outstanding PASSIVE port */
	if (f->data_channel == DATA_PASSIVE) {
		Test_and_Close( & f->server_fd);
	}
	f->data_channel = DATA_PASSIVE;
	f->server_fd = socket_fd;

  exit_pasv:
	daemon_assert(invariant(f));
}

/* pick a server port to listen for connection on, including IPv6 */
static void do_lpsv(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	char addr[80];
	uint8_t *a;
	uint8_t *p;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	if (f->epsv_all_set) {
		reply(f, 500, "After EPSV ALL, only EPSV allowed.");
		goto exit_lpsv;
	}

	socket_fd = set_pasv(f, &f->server_addr);
	if (FILE_DESCRIPTOR_NOT_VALID(socket_fd)) {
		goto exit_lpsv;
	}

	/* report address and port to client */
#ifdef INET6
	if (SSFAM(&f->server_addr) == AF_INET6) {
		a = (uint8_t *) & SIN6ADDR(&f->server_addr);
		p = (uint8_t *) & SIN6PORT(&f->server_addr);
		snprintf(addr, sizeof(addr),
				 "(6,16,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,2,%d,%d)",
				 a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], p[0], p[1]);
	} else
#endif
	{
		a = (uint8_t *) & SIN4ADDR(&f->server_addr);
		p = (uint8_t *) & SIN4PORT(&f->server_addr);
		snprintf(addr, sizeof(addr), "(4,4,%d,%d,%d,%d,2,%d,%d)", a[0], a[1], a[2], a[3], p[0], p[1]);
	}

	reply(f, 228, "Entering Long Passive Mode %s", addr);

	/* close any outstanding PASSIVE port */
	if (f->data_channel == DATA_PASSIVE) {
		Test_and_Close( & f->server_fd);
	}
	f->data_channel = DATA_PASSIVE;
	f->server_fd = socket_fd;

  exit_lpsv:
	daemon_assert(invariant(f));
}

/* pick a server port to listen for connection on, new IPv6 method */
static void do_epsv(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	sockaddr_storage_t *addr;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert((cmd->num_arg == 0) || (cmd->num_arg == 1));

	/* check our argument, if any,  and use the appropriate address */
	if (cmd->num_arg == 0) {
		addr = &f->server_addr;
	} else {
		switch (cmd->arg[0].num) {
			/* EPSV_ALL is a special number indicating the client sent */
			/* the command "EPSV ALL" - this is not a request to assign */
			/* a new passive port, but rather to deny all future port */
			/* assignment requests other than EPSV */
		case EPSV_ALL:
			f->epsv_all_set = 1;
			reply(f, 200, "EPSV ALL command successful.");
			goto exit_epsv;
		case 1:
			addr = (sockaddr_storage_t *) & f->server_ipv4_addr;
			break;
#ifdef INET6
		case 2:
			addr = &f->server_addr;
			break;
		default:
			reply(f, 522, "Only IPv4 and IPv6 supported, use (1,2)");
			goto exit_epsv;
#else
		default:
			reply(f, 522, "Only IPv4 supported, use (1)");
			goto exit_epsv;
#endif
		}
	}

	/* bind port and so on */
	socket_fd = set_pasv(f, addr);
	if (FILE_DESCRIPTOR_NOT_VALID(socket_fd)) {
		goto exit_epsv;
	}

	/* report port to client */
	reply(f, 229, "Entering Extended Passive Mode (|||%d|)", ntohs(SINPORT(&f->server_addr)));

	/* close any outstanding PASSIVE port */
	if (f->data_channel == DATA_PASSIVE) {
		Test_and_Close( & f->server_fd ) ;
	}
	f->data_channel = DATA_PASSIVE;
	f->server_fd = socket_fd;

  exit_epsv:
	daemon_assert(invariant(f));
}

/* seed the random number generator used to pick a port */
static void init_passive_port()
{
	struct timeval tv;
	unsigned short int seed[3];

	gettimeofday(&tv, NULL);
	seed[0] = (tv.tv_sec >> 16) & 0xFFFF;
	seed[1] = tv.tv_sec & 0xFFFF;
	seed[2] = tv.tv_usec & 0xFFFF;
	seed48(seed);
}

/* pick a port to try to bind() for passive FTP connections */
static int get_passive_port()
{
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	int port;

	/* initialize the random number generator the first time we're called */
	pthread_once(&once_control, init_passive_port);

	/* pick a random port between 1024 and 65535, inclusive */
	_MUTEX_LOCK(mutex);
	port = (lrand48() % 64512) + 1024;
	_MUTEX_UNLOCK(mutex);

	return port;
}

static void do_type(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	char type;
	char form;
	int cmd_okay;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg >= 1);
	daemon_assert(cmd->num_arg <= 2);

	type = cmd->arg[0].string[0];
	if (cmd->num_arg == 2) {
		form = cmd->arg[1].string[0];
	} else {
		form = 0;
	}

	cmd_okay = 0;
	if (type == 'A') {
		if ((cmd->num_arg == 1) || ((cmd->num_arg == 2) && (form == 'N'))) {
			f->data_type = TYPE_ASCII;
			cmd_okay = 1;
		}
	} else if (type == 'I') {
		f->data_type = TYPE_IMAGE;
		cmd_okay = 1;
	}

	if (cmd_okay) {
		reply(f, 200, "Command okay.");
	} else {
		reply(f, 504, "Command not implemented for that parameter.");
	}

	daemon_assert(invariant(f));
}

static void do_stru(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	char structure;
	int cmd_okay;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	structure = cmd->arg[0].string[0];
	cmd_okay = 0;
	if (structure == 'F') {
		f->file_structure = STRU_FILE;
		cmd_okay = 1;
	} else if (structure == 'R') {
		f->file_structure = STRU_RECORD;
		cmd_okay = 1;
	}

	if (cmd_okay) {
		reply(f, 200, "Command okay.");
	} else {
		reply(f, 504, "Command not implemented for that parameter.");
	}

	daemon_assert(invariant(f));
}

static void do_mode(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	char mode;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	mode = cmd->arg[0].string[0];
	if (mode == 'S') {
		reply(f, 200, "Command okay.");
	} else {
		reply(f, 504, "Command not implemented for that parameter.");
	}

	daemon_assert(invariant(f));
}

/* convert the user-entered file name into a full path on our local drive */
static void get_absolute_fname(char *fname, size_t fname_len, const char *dir, const char *file)
{
	daemon_assert(fname != NULL);
	daemon_assert(dir != NULL);
	daemon_assert(file != NULL);

	if (*file == '/') {

		/* absolute path, use as input */
		daemon_assert(strlen(file) < fname_len);
		strcpy(fname, file);

	} else {

		/* construct a file name based on our current directory */
		daemon_assert(strlen(dir) + 1 + strlen(file) < fname_len);
		strncpy(fname, dir, fname_len - 1);

		/* add a seperating '/' if we're not at the root */
		if (fname[1] != '\0') {
			strcat(fname, "/");
		}

		/* and of course the actual file name */
		strcat(fname, file);

	}
}

static void do_retr(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const char *file_name;
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	ASCII *buf2 = NULL;
	ASCII *bufwrite;
	struct timeval start_timestamp;
	struct timeval end_timestamp;
	struct timeval transfer_time;
	struct one_wire_query owq;
	size_t size_write;
	SIZE_OR_ERROR returned_length;
	off_t offset = 0;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	/* set up for exit */
	socket_fd = FILE_DESCRIPTOR_BAD;

	/* create an absolute name for our file */
	file_name = cmd->arg[0].string;

	/* if the last command was a REST command, restart at the */
	/* requested position in the file                         */
	if ( f->file_offset_command_number == (f->command_number - 1) ) {
		offset = f->file_offset;
	}

	/* Can we parse the name? */
	if ( BAD( OWQ_create_plus(f->dir, file_name, &owq) ) ) {
		reply(f, 550, "File does not exist.");
		goto exit_retr;
	}

	if (IsDir(PN(&owq))) {
		reply(f, 550, "Error, file is a directory.");
		goto exit_retr;
	}

	if (OWQ_pn(&owq).selected_filetype->read == NO_READ_FUNCTION) {
		reply(f, 550, "Error, file is write-only.");
		goto exit_retr;
	}

	if ((OWQ_pn(&owq).selected_filetype->format == ft_binary)
		&& (f->data_type == TYPE_ASCII)) {
		reply(f, 550, "Error, binary file (type ascii).");
		goto exit_retr;
	}

	if ( BAD( OWQ_allocate_read_buffer(&owq)) ) {
		reply(f, 550, "Error, file too large.");
		goto exit_retr;
	}
	OWQ_offset(&owq) = offset ;

	returned_length = FS_read_postparse(&owq);
	if (returned_length < 0) {
		reply(f, 550, "Error reading from file; %s.", strerror(-returned_length));
		goto exit_retr;
	}

	if (f->data_type == TYPE_IMAGE) {
		bufwrite = OWQ_buffer(&owq);
		size_write = returned_length;
		goto good_retr;
	}

	buf2 = owmalloc(2 * returned_length);
	if (buf2 == NULL) {
		reply(f, 550, "Error, file too large.");
		goto exit_retr;
	}
	// TYPE_ASCII
	size_write = convert_newlines(buf2, OWQ_buffer(&owq), returned_length);
	bufwrite = buf2;

  good_retr:
	/* ready to transfer */
	reply(f, 150, "About to open data connection.");

	/* mark start time */
	gettimeofday(&start_timestamp, NULL);

	/* open data path */
	socket_fd = open_connection(f);
	if (FILE_DESCRIPTOR_NOT_VALID(socket_fd)) {
		goto exit_retr;
	}

	/* we're golden, send the file */
	if (write_fully(socket_fd, bufwrite, size_write) == -1) {
		reply(f, 550, "Error writing to data connection; %s.", strerror(errno));
		goto exit_retr;
	}

	watchdog_defer_watched(f->watched);

	/* disconnect */
	Test_and_Close(&socket_fd);

	/* hey, it worked, let the other side know */
	reply(f, 226, "File transfer complete.");

	/* mark end time */
	gettimeofday(&end_timestamp, NULL);

	/* calculate transfer rate */
	timersub( &end_timestamp, &start_timestamp, &transfer_time ) ;

	/* note the transfer */
	LEVEL_DATA("%s retrieved \"%s\", %ld bytes in "TVformat,
			   f->client_addr_str, OWQ_pn(&owq).path, size_write, TVvar(&transfer_time) );

  exit_retr:
	OWQ_destroy(&owq); // safe at any speed
	if (buf2) {
		owfree(buf2);
	}
	f->file_offset = 0;
	Test_and_Close(&socket_fd) ;
	daemon_assert(invariant(f));
}

/* Write to 1-wire device */
static void do_stor(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	struct timeval start_timestamp;
	struct timeval end_timestamp;
	struct timeval transfer_time;
	struct timeval limit_time = { Globals.timeout_ftp, 0 };

	size_t size_read;
	off_t offset = 0 ;
	char * data_in = NULL ;
	struct parsedname *pn;
	OWQ_allocate_struct_and_pointer(owq);

	pn = PN(owq);

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	/* set up for exit */
	socket_fd = FILE_DESCRIPTOR_BAD;

	/* create an absolute name for our file */
	if ( BAD( OWQ_create_plus(f->dir, cmd->arg[0].string, owq) ) ) {
		reply(f, 550, "File does not exist.");
		goto exit_stor;
	}

	/* if the last command was a REST command, restart at the */
	/* requested position in the file                         */
	if ( f->file_offset_command_number == (f->command_number - 1) ) {
		offset = f->file_offset;
	}

	if (IsDir(pn)) {
		reply(f, 550, "Error, file is a directory.");
		goto exit_stor;
	}

	if (pn->selected_filetype->write == NO_WRITE_FUNCTION) {
		reply(f, 550, "Error, file is read-only.");
		goto exit_stor;
	}

	if ((pn->selected_filetype->format == ft_binary)
		&& (f->data_type == TYPE_ASCII)) {
		reply(f, 550, "Error, binary file (type ascii).");
		goto exit_stor;
	}

	/* ready to transfer */
	reply(f, 150, "About to open data connection.");

	/* mark start time */
	gettimeofday(&start_timestamp, NULL);

	/* open data path */
	socket_fd = open_connection(f);
	if ( FILE_DESCRIPTOR_NOT_VALID(socket_fd) ) {
		goto exit_stor;
	}

	size_read = FullFileLength(PN(owq)) - OWQ_offset(owq) + 100;
	data_in = owmalloc(size_read);
	if (data_in == NULL) {
		reply(f, 550, "Out of memory.");
		goto exit_stor;
	} else {
		/* we're golden, read the file */
		size_t size_actual ;
		int read_return = tcp_read(socket_fd, (BYTE *) data_in, size_read, &limit_time, &size_actual) ;
		if (read_return != 0 && read_return != -EAGAIN) {
			reply(f, 550, "Error reading from data connection; %s.", strerror(errno));
			goto exit_stor;
		}
		if ( BAD( OWQ_allocate_write_buffer( data_in, size_actual, offset, owq )) ) {
			reply(f, 550, "Out of memory.");
			goto exit_stor;
		}
	}

	watchdog_defer_watched(f->watched);

	/* disconnect */
	Test_and_Close(&socket_fd) ;

	/* hey, it worked, let the other side know */
	reply(f, 226, "File transfer complete.");

	/* mark end time */
	gettimeofday(&end_timestamp, NULL);

	/* calculate transfer rate */
	timersub( &end_timestamp, &start_timestamp, &transfer_time ) ;

	{
		/* write to one-wire */
		int one_wire_error = FS_write_postparse(owq);
		if (one_wire_error < 0) {
			reply(f, 550, "Error writing to file; %s.", strerror(-one_wire_error));
			goto exit_stor;
		}
	}

	/* note the transfer */
	LEVEL_DATA("%s stored \"%s\", %ld bytes in "TVformat,
			   f->client_addr_str, pn->path, OWQ_size(owq), TVvar(&transfer_time) );


	/* Fall through */
  exit_stor:
	OWQ_destroy(owq);

	f->file_offset = 0;

	/* Free memory */
	if ( data_in != NULL ) {
		owfree(data_in) ;
	} 

	/* disconnect */
	Test_and_Close(&socket_fd) ;

	daemon_assert(invariant(f));
}

static FILE_DESCRIPTOR_OR_ERROR open_connection(struct ftp_session_s *f)
{
	FILE_DESCRIPTOR_OR_ERROR socket_fd;
	struct sockaddr_in addr;
	unsigned addr_len;

	daemon_assert((f->data_channel == DATA_PORT) || (f->data_channel == DATA_PASSIVE));

	if (f->data_channel == DATA_PORT) {
		socket_fd = socket(SSFAM(&f->data_port), SOCK_STREAM, 0);
		if ( FILE_DESCRIPTOR_NOT_VALID(socket_fd) ) {
			reply(f, 425, "Error creating socket; %s.", strerror(errno));
			return FILE_DESCRIPTOR_BAD ;
		}
//		fcntl (socket_fd, F_SETFD, FD_CLOEXEC); // for safe forking
		if (connect(socket_fd, (struct sockaddr *) &f->data_port, sizeof(sockaddr_storage_t)) != 0) {
			reply(f, 425, "Error connecting; %s.", strerror(errno));
			Test_and_Close(&socket_fd);
			return FILE_DESCRIPTOR_BAD;
		}
	} else {
		daemon_assert(f->data_channel == DATA_PASSIVE);

		addr_len = sizeof(struct sockaddr_in);
		socket_fd = accept(f->server_fd, (struct sockaddr *) &addr, &addr_len);
		if ( FILE_DESCRIPTOR_NOT_VALID(socket_fd) ) {
			reply(f, 425, "Error accepting connection; %s.", strerror(errno));
			return FILE_DESCRIPTOR_BAD;
		}
#ifdef INET6
		/* in IPv6, the client can connect to a channel using a different */
		/* protocol - in that case, we'll just blindly let the connection */
		/* through, otherwise verify addresses match */
		if (SAFAM(addr) == SSFAM(&f->client_addr)) {
			if (memcmp(&SINADDR(&f->client_addr), &SINADDR(&addr), sizeof(SINADDR(&addr)))) {
				reply(f, 425, "Error accepting connection; connection from invalid IP.");
				Test_and_Close(&socket_fd);
				return FILE_DESCRIPTOR_BAD;
			}
		}
#else
		if (memcmp(&f->client_addr.sin_addr, &addr.sin_addr, sizeof(struct in_addr))) {
			reply(f, 425, "Error accepting connection; connection from invalid IP.");
			Test_and_Close(&socket_fd);
			return FILE_DESCRIPTOR_BAD;
		}
#endif
	}
	return socket_fd;
}

/* convert any '\n' to '\r\n' */
/* destination should be twice the size of the source for safety */
static int convert_newlines(char *dst, const char *src, int srclen)
{
	int i;
	int dstlen;

	daemon_assert(dst != NULL);
	daemon_assert(src != NULL);

	dstlen = 0;
	for (i = 0; i < srclen; i++) {
		if (src[i] == '\n') {
			dst[dstlen++] = '\r';
		}
		dst[dstlen++] = src[i];
	}
	return dstlen;
}

static int write_fully(FILE_DESCRIPTOR_OR_ERROR file_descriptor, const char *buf, int buflen)
{
	int amt_written;
	int write_ret;

	amt_written = 0;
	while (amt_written < buflen) {
		write_ret = write(file_descriptor, buf + amt_written, buflen - amt_written);
		if (write_ret <= 0) {
			return -1;
		}
		amt_written += write_ret;
	}
	return amt_written;
}

static void do_pwd(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	reply(f, 257, "\"%s\" is current directory.", f->dir);

	daemon_assert(invariant(f));
}

static void both_list(struct ftp_session_s *f, const struct ftp_command_s *cmd, enum file_list_e fle)
{
	struct file_parse_s fps;

	strcpy(fps.buffer, f->dir);
	fps.rest = NULL;
	fps.pse = parse_status_init;
	fps.fle = fle;
	fps.out = FILE_DESCRIPTOR_BAD;


	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert((cmd->num_arg == 0) || (cmd->num_arg == 1));

	/* figure out what parameters to use */
	if (cmd->num_arg == 0) {
		fps.rest = strdup("*");
	} else {
		daemon_assert(cmd->num_arg == 1);

		/* ignore attempts to send options to "ls" by silently dropping */
		/* We don't use "ls" so can just pass on literal text */
		fps.rest = strdup(cmd->arg[0].string);
	}

	/* ready to list */
	reply(f, 150, "About to send name list.");

	/* open our data connection */
	fps.out = open_connection(f);
	if (FILE_DESCRIPTOR_NOT_VALID(fps.out)) {
		goto exit_blst;
	}

	/* send any files */
	FileLexParse(&fps);

	/* strange handshake for Netscape's benefit */
	netscape_hack(fps.out);

	if (fps.ret == 0) {
		reply(f, 226, "Transfer complete.");
	} else {
		reply(f, 451, "Error sending name list. %s", strerror(-fps.ret));
	}

	/* clean up and exit */
  exit_blst:
	Test_and_Close( & fps.out ) ;
	daemon_assert(invariant(f));
}

static void do_nlst(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	both_list(f, cmd, file_list_nlst);
}

static void do_list(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	both_list(f, cmd, file_list_list);
}

static void do_syst(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	reply(f, 215, "UNIX");

	daemon_assert(invariant(f));
}


static void do_noop(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 0);

	reply(f, 200, "Command okay.");

	daemon_assert(invariant(f));
}

static void do_rest(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	if (f->data_type != TYPE_IMAGE) {
		reply(f, 555, "Restart not possible in ASCII mode.");
	} else if (f->file_structure != STRU_FILE) {
		reply(f, 555, "Restart only possible with FILE structure.");
	} else {
		f->file_offset = cmd->arg[0].offset;
		f->file_offset_command_number = f->command_number;
		reply(f, 350, "Restart okay, awaiting file retrieval request.");
	}

	daemon_assert(invariant(f));
}

static void do_size(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	off_t filesize;
	struct parsedname pn;

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	if (f->data_type != TYPE_IMAGE) {
		reply(f, 550, "Size cannot be determined in ASCII mode.");
	} else if (f->file_structure != STRU_FILE) {
		reply(f, 550, "Size cannot be determined with FILE structure.");
	} else {

		/* get the file information */
		if ( FS_ParsedNamePlus(f->dir, cmd->arg[0].string, &pn) != 0 ) {
			reply(f, 550, "Bad file specification");
		} else {

			/* verify that the file is not a directory */
			if (pn.selected_device == NO_DEVICE || pn.selected_filetype == NO_FILETYPE) {
				reply(f, 550, "File is a directory, SIZE command not valid.");
			} else {
				filesize = FullFileLength(&pn);
				/* output the size */
				if (sizeof(off_t) == 8) {
					reply(f, 213, "%llu", filesize);
				} else {
					reply(f, 213, "%lu", filesize);
				}
			}
			FS_ParsedName_destroy(&pn);
		}
	}
	daemon_assert(invariant(f));
}

/* if no gmtime_r() is available, provide one */
#ifndef HAVE_GMTIME_R
struct tm *gmtime_r(const time_t * timep, struct tm *timeptr)
{
	static pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

	_MUTEX_LOCK(time_lock);
	*timeptr = *(gmtime(timep));
	_MUTEX_UNLOCK(time_lock);
	return timeptr;
}
#endif							/* HAVE_GMTIME_R */

static void do_mdtm(struct ftp_session_s *f, const struct ftp_command_s *cmd)
{
	const char *file_name;
	char full_path[PATH_MAX + 1 + MAX_STRING_LEN];
	struct stat stat_buf;
	struct tm mtime;
	char time_buf[16];

	daemon_assert(invariant(f));
	daemon_assert(cmd != NULL);
	daemon_assert(cmd->num_arg == 1);

	/* create an absolute name for our file */
	file_name = cmd->arg[0].string;
	get_absolute_fname(full_path, sizeof(full_path), f->dir, file_name);

	/* get the file information */
	if (FS_fstat(full_path, &stat_buf) < 0) {
		reply(f, 550, "Error getting file status; %s.", strerror(errno));
	} else {
		gmtime_r(&stat_buf.st_mtime, &mtime);
		strftime(time_buf, sizeof(time_buf), "%Y%m%d%H%M%S", &mtime);
		reply(f, 213, time_buf);
	}

	daemon_assert(invariant(f));
}


static void send_readme(const struct ftp_session_s *f, int code)
{
	char code_str[8];

	daemon_assert(invariant(f));
	daemon_assert(code >= 100);
	daemon_assert(code <= 559);


	/* convert our code to a buffer */
	daemon_assert(code >= 100);
	daemon_assert(code <= 999);
	sprintf(code_str, "%03d-", code);

	telnet_session_print(f->telnet_session, code_str);
	telnet_session_println(f->telnet_session, "owftpd 1-wire ftp server -- Paul H Alfille");
	telnet_session_print(f->telnet_session, code_str);
	telnet_session_println(f->telnet_session, "Version: " VERSION " see http://www.owfs.org");

}

/* hack which prevents Netscape error in file list */
static void netscape_hack(FILE_DESCRIPTOR_OR_ERROR file_descriptor)
{
	fd_set readfds;
	struct timeval ns_timeout = {15, 0 } ; // 15 seconds
	int select_ret;
	char c;

	daemon_assert(FILE_DESCRIPTOR_VALID(file_descriptor));

	shutdown(file_descriptor, 1);
	FD_ZERO(&readfds);
	FD_SET(file_descriptor, &readfds);
	select_ret = select(file_descriptor + 1, &readfds, NULL, NULL, &ns_timeout);
	if (select_ret > 0) {
		ignore_result = read(file_descriptor, &c, 1); // ignore warning
	}
}

/* compare two addresses to see if they contain the same IP address */
//static int ip_equal(const sockaddr_storage_t *a, const sockaddr_storage_t *b) {
static int ip_equal(const sockaddr_storage_t * a, const sockaddr_storage_t * b)
{
	daemon_assert(a != NULL);
	daemon_assert(b != NULL);
#ifdef AF_INET6
	daemon_assert((cSSFAM(a) == AF_INET) || (cSSFAM(a) == AF_INET6));
	daemon_assert((cSSFAM(b) == AF_INET) || (cSSFAM(b) == AF_INET6));
#else
	daemon_assert((cSSFAM(a) == AF_INET));
	daemon_assert((cSSFAM(b) == AF_INET));
#endif

	if (cSSFAM(a) != cSSFAM(b))
		return 0;
	if (memcmp(&cSINADDR(a), &cSINADDR(b), sizeof(cSINADDR(a))) != 0)
		return 0;
	return 1;
}
