/*
 * part of owftpd By Paul H Alfille
 * The whole is GPLv2 licenced though the ftp code was more liberally licenced when first used.
 */

#include "owftpd.h"

/* characters to process */
#define SE         240
#define NOP        241
#define DATA_MARK  242
#define BRK        243
#define IP         244
#define AO         245
#define AYT        246
#define EC         247
#define EL         248
#define GA         249
#define SB         250
#define WILL       251
#define WONT       252
#define DO         253
#define DONT       254
#define IAC        255

/* input states */
#define NORMAL    0
#define GOT_IAC   1
#define GOT_WILL  2
#define GOT_WONT  3
#define GOT_DO    4
#define GOT_DONT  5
#define GOT_CR    6

/* prototypes */
static int invariant(const struct telnet_session_s *t);
static void process_data(struct telnet_session_s *t, int wait_flag);
static void read_incoming_data(struct telnet_session_s *t);
static void process_incoming_char(struct telnet_session_s *t, int c);
static void add_incoming_char(struct telnet_session_s *t, int c);
static int use_incoming_char(struct telnet_session_s *t);
static void write_outgoing_data(struct telnet_session_s *t);
static void add_outgoing_char(struct telnet_session_s *t, int c);
static int max_input_read(struct telnet_session_s *t);

/* initialize a telnet session */
void telnet_session_init(struct telnet_session_s *t, FILE_DESCRIPTOR_OR_ERROR in, FILE_DESCRIPTOR_OR_ERROR out)
{
	daemon_assert(t != NULL);
	daemon_assert(in >= 0);
	daemon_assert(out >= 0);

	t->in_fd = in;
	t->in_errno = 0;
	t->in_eof = 0;
	t->in_take = t->in_add = 0;
	t->in_buflen = 0;

	t->in_status = NORMAL;

	t->out_fd = out;
	t->out_errno = 0;
	t->out_eof = 0;
	t->out_take = t->out_add = 0;
	t->out_buflen = 0;

	process_data(t, 0);

	daemon_assert(invariant(t));
}

/* print output */
int telnet_session_print(struct telnet_session_s *t, const char *s)
{
	int len;
	int amt_printed;

	daemon_assert(invariant(t));

	len = strlen(s);
	if (len == 0) {
		daemon_assert(invariant(t));
		return 1;
	}

	amt_printed = 0;
	do {
		if ((t->out_errno != 0) || (t->out_eof != 0)) {
			daemon_assert(invariant(t));
			return 0;
		}
		while ((amt_printed < len) && (t->out_buflen < BUF_LEN)) {
			daemon_assert(s[amt_printed] != '\0');
			add_outgoing_char(t, s[amt_printed]);
			amt_printed++;
		}
		process_data(t, 1);
	} while (amt_printed < len);

	while (t->out_buflen > 0) {
		if ((t->out_errno != 0) || (t->out_eof != 0)) {
			daemon_assert(invariant(t));
			return 0;
		}
		process_data(t, 1);
	}

	daemon_assert(invariant(t));
	return 1;
}

/* print a line output */
int telnet_session_println(struct telnet_session_s *t, const char *s)
{
	daemon_assert(invariant(t));
	if (!telnet_session_print(t, s)) {
		daemon_assert(invariant(t));
		return 0;
	}
	if (!telnet_session_print(t, "\015\012")) {
		daemon_assert(invariant(t));
		return 0;
	}
	daemon_assert(invariant(t));
	return 1;
}

/* read a line */
int telnet_session_readln(struct telnet_session_s *t, char *buf, int buflen)
{
	int amt_read;

	daemon_assert(invariant(t));
	amt_read = 0;
	for (;;) {
		if ((t->in_errno != 0) || (t->in_eof != 0)) {
			daemon_assert(invariant(t));
			return 0;
		}
		while (t->in_buflen > 0) {
			if (amt_read == buflen - 1) {
				buf[amt_read] = '\0';
				daemon_assert(invariant(t));
				return 1;
			}
			daemon_assert(amt_read >= 0);
			daemon_assert(amt_read < buflen);
			buf[amt_read] = use_incoming_char(t);
			if (buf[amt_read] == '\012') {
				daemon_assert(amt_read + 1 >= 0);
				daemon_assert(amt_read + 1 < buflen);
				buf[amt_read + 1] = '\0';
				daemon_assert(invariant(t));
				return 1;
			}
			amt_read++;
		}
		process_data(t, 1);
	}
}

void telnet_session_destroy(struct telnet_session_s *t)
{
	daemon_assert(invariant(t));

	Test_and_Close( & t->in_fd);
	Test_and_Close( & t->out_fd);
}

/* receive any incoming data, send any pending data */
static void process_data(struct telnet_session_s *t, int wait_flag)
{
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	struct timeval tv_zero;
	struct timeval *tv;

	/* set up our select() variables */
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	if (wait_flag) {
		tv = NULL;
	} else {
		tv_zero.tv_sec = 0;
		tv_zero.tv_usec = 0;
		tv = &tv_zero;
	}

	/* only check for input if we can accept input */
	if ((t->in_errno == 0) && (t->in_eof == 0) && (max_input_read(t) > 0)) {
		FD_SET(t->in_fd, &readfds);
		FD_SET(t->in_fd, &exceptfds);
	}

	/* only check for output if we have pending output */
	if ((t->out_errno == 0) && (t->out_eof == 0) && (t->out_buflen > 0)) {
		FD_SET(t->out_fd, &writefds);
		FD_SET(t->out_fd, &exceptfds);
	}

	/* see if there's anything to do */
	if (select(FD_SETSIZE, &readfds, &writefds, &exceptfds, tv) > 0) {

		if (FD_ISSET(t->in_fd, &exceptfds)) {
			t->in_eof = 1;
		} else if (FD_ISSET(t->in_fd, &readfds)) {
			read_incoming_data(t);
		}

		if (FD_ISSET(t->out_fd, &exceptfds)) {
			t->out_eof = 1;
		} else if (FD_ISSET(t->out_fd, &writefds)) {
			write_outgoing_data(t);
		}
	}
}

static void read_incoming_data(struct telnet_session_s *t)
{
	ssize_t read_ret;
	char buf[BUF_LEN];
	ssize_t i;

	/* read as much data as we have buffer space for */
	daemon_assert(max_input_read(t) <= BUF_LEN);
	read_ret = read(t->in_fd, buf, max_input_read(t));

	/* handle three possible read results */
	if (read_ret == -1) {
		t->in_errno = errno;
	} else if (read_ret == 0) {
		t->in_eof = 1;
	} else {
		for (i = 0; i < read_ret; i++) {
			process_incoming_char(t, (unsigned char) buf[i]);
		}
	}
}


/* process a single character */
static void process_incoming_char(struct telnet_session_s *t, int c)
{
	switch (t->in_status) {
	case GOT_IAC:
		switch (c) {
		case WILL:
			t->in_status = GOT_WILL;
			break;
		case WONT:
			t->in_status = GOT_WONT;
			break;
		case DO:
			t->in_status = GOT_DO;
			break;
		case DONT:
			t->in_status = GOT_DONT;
			break;
		case IAC:
			add_incoming_char(t, IAC);
			t->in_status = NORMAL;
			break;
		default:
			t->in_status = NORMAL;
		}
		break;
	case GOT_WILL:
		add_outgoing_char(t, IAC);
		add_outgoing_char(t, DONT);
		add_outgoing_char(t, c);
		t->in_status = NORMAL;
		break;
	case GOT_WONT:
		t->in_status = NORMAL;
		break;
	case GOT_DO:
		add_outgoing_char(t, IAC);
		add_outgoing_char(t, WONT);
		add_outgoing_char(t, c);
		t->in_status = NORMAL;
		break;
	case GOT_DONT:
		t->in_status = NORMAL;
		break;
	case GOT_CR:
		add_incoming_char(t, '\012');
		if (c != '\012') {
			add_incoming_char(t, c);
		}
		t->in_status = NORMAL;
		break;
	case NORMAL:
		if (c == IAC) {
			t->in_status = GOT_IAC;
		} else if (c == '\015') {
			t->in_status = GOT_CR;
		} else {
			add_incoming_char(t, c);
		}
	}
}

/* add a single character, wrapping buffer if necessary (should never occur) */
static void add_incoming_char(struct telnet_session_s *t, int c)
{
	daemon_assert(t->in_add >= 0);
	daemon_assert(t->in_add < BUF_LEN);
	t->in_buf[t->in_add] = c;
	t->in_add++;
	if (t->in_add == BUF_LEN) {
		t->in_add = 0;
	}
	if (t->in_buflen == BUF_LEN) {
		t->in_take++;
		if (t->in_take == BUF_LEN) {
			t->in_take = 0;
		}
	} else {
		t->in_buflen++;
	}
}

/* remove a single character */
static int use_incoming_char(struct telnet_session_s *t)
{
	int c;

	daemon_assert(t->in_take >= 0);
	daemon_assert(t->in_take < BUF_LEN);
	c = t->in_buf[t->in_take];
	t->in_take++;
	if (t->in_take == BUF_LEN) {
		t->in_take = 0;
	}
	t->in_buflen--;

	return c;
}

/* add a single character, hopefully will never happen :) */
static void add_outgoing_char(struct telnet_session_s *t, int c)
{
	daemon_assert(t->out_add >= 0);
	daemon_assert(t->out_add < BUF_LEN);
	t->out_buf[t->out_add] = c;
	t->out_add++;
	if (t->out_add == BUF_LEN) {
		t->out_add = 0;
	}
	if (t->out_buflen == BUF_LEN) {
		t->out_take++;
		if (t->out_take == BUF_LEN) {
			t->out_take = 0;
		}
	} else {
		t->out_buflen++;
	}
}


static void write_outgoing_data(struct telnet_session_s *t)
{
	ssize_t write_ret;

	if (t->out_take < t->out_add) {
		/* handle a buffer that looks like this:       */
		/*     |-- empty --|-- data --|-- empty --|    */
		daemon_assert(t->out_take >= 0);
		daemon_assert(t->out_take < BUF_LEN);
		daemon_assert(t->out_buflen > 0);
		daemon_assert(t->out_buflen + t->out_take <= BUF_LEN);
		write_ret = write(t->out_fd, t->out_buf + t->out_take, t->out_buflen);
	} else {
		/* handle a buffer that looks like this:       */
		/*     |-- data --|-- empty --|-- data --|     */
		daemon_assert(t->out_take >= 0);
		daemon_assert(t->out_take < BUF_LEN);
		daemon_assert((BUF_LEN - t->out_take) > 0);
		write_ret = write(t->out_fd, t->out_buf + t->out_take, BUF_LEN - t->out_take);
	}

	/* handle three possible write results */
	if (write_ret == -1) {
		t->out_errno = errno;
	} else if (write_ret == 0) {
		t->out_eof = 1;
	} else {
		t->out_buflen -= write_ret;
		t->out_take += write_ret;
		if (t->out_take >= BUF_LEN) {
			t->out_take -= BUF_LEN;
		}
	}
}

/* return the amount of a read */
static int max_input_read(struct telnet_session_s *t)
{
	int max_in;
	int max_out;

	daemon_assert(invariant(t));

	/* figure out how much space is available in the input buffer */
	if (t->in_buflen < BUF_LEN) {
		max_in = BUF_LEN - t->in_buflen;
	} else {
		max_in = 0;
	}

	/* worry about space in the output buffer (for DONT/WONT replies) */
	if (t->out_buflen < BUF_LEN) {
		max_out = BUF_LEN - t->out_buflen;
	} else {
		max_out = 0;
	}

	daemon_assert(invariant(t));

	/* return the minimum of the two values */
	return (max_in < max_out) ? max_in : max_out;
}

#ifndef NDEBUG
static int invariant(const struct telnet_session_s *t)
{
	if (t == NULL) {
		return 0;
	}
	if (FILE_DESCRIPTOR_NOT_VALID(t->in_fd)) {
		return 0;
	}
	if ((t->in_take < 0) || (t->in_take >= BUF_LEN)) {
		return 0;
	}
	if ((t->in_add < 0) || (t->in_add >= BUF_LEN)) {
		return 0;
	}
	if ((t->in_buflen < 0) || (t->in_buflen > BUF_LEN)) {
		return 0;
	}

	switch (t->in_status) {
	case NORMAL:
		break;
	case GOT_IAC:
		break;
	case GOT_WILL:
		break;
	case GOT_WONT:
		break;
	case GOT_DO:
		break;
	case GOT_DONT:
		break;
	case GOT_CR:
		break;
	default:
		return 0;
	}

	if (FILE_DESCRIPTOR_NOT_VALID(t->out_fd)) {
		return 0;
	}
	if ((t->out_take < 0) || (t->out_take >= BUF_LEN)) {
		return 0;
	}
	if ((t->out_add < 0) || (t->out_add >= BUF_LEN)) {
		return 0;
	}
	if ((t->out_buflen < 0) || (t->out_buflen > BUF_LEN)) {
		return 0;
	}

	return 1;
}
#endif							/* NDEBUG */
