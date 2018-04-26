// xt.c                Copyright (C) 2001, Real-Time Systems Inc.
//------------------------------------------ www.realtime.bc.ca -------------
//
//  Simple Terminal Emulator
//
//  Derived from miniterm.c (by Sven Goldt, goldt@math.tu.berlin.de).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of Version 2 of the GNU General Public License as
//  published by the Free Software Foundation
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//---------------------------------------------------------------------------

#include <sys/param.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __linux__
#include <pty.h>
#else
#include <util.h>
#endif
#include <errno.h>
#include <string.h>

//int openpty (int *, int *, char *, struct termios *, struct winsize *);
void printchar(char *buf, int length);
void print_outfile(int keyed, char *buf, int length);

// configuration

const int BUFLEN = 512;			// read/write buffer size


// global variables

char *argv0 = "xt";				// program name

struct termios oldStdinTio;		// old console settings
struct termios newStdinTio;		// new console settings

int devfd = 0;					// serial port file descriptor
struct termios oldSerialTio;	// old serial port settings
struct termios newSerialTio;	// new serial port settings

int ptyfd = 0;					// pseudo-tty master file descriptor
int ttyfd = 0;					// pseudo-tty slave file descriptor
struct termios oldMasterTio;	// old pty master settings
struct termios newMasterTio;	// new pty master settings
char *ttylink = NULL;			// name of symlink to slave

FILE *outfile = NULL;			// log file for hex display 

// find maximum

#define max(a, b) ((a) > (b) ? (a) : (b))


// clean up, exit

void quit(int code)
{
	if (outfile) {
		fclose(outfile);
		outfile = NULL;
	}

	if (ttylink)
		unlink(ttylink);

	if (ptyfd > 0) {
		tcsetattr(ptyfd, TCSANOW, &oldMasterTio);
		close(ptyfd);
	}

	if (devfd > 0) {
		tcsetattr(devfd, TCSANOW, &oldSerialTio);
		close(devfd);
	}

	tcsetattr(0, TCSANOW, &oldStdinTio);
	printf("\n");
	exit(code);
}


// show copyright message, exit

void copyright(void)
{
	printf("%s 3\n\n", argv0);
	printf("Copyright (C) 2002 Real-Time Systems, Inc. (www.realtime.bc.ca)\n");
	printf("This is free software, see the source for copying conditions.");
	printf("  There is NO\nwarranty; not even for MERCHANTABILITY or FITNESS");
	printf(" FOR A PARTICULAR PURPOSE.\n");
	printf("Heavily modified 4/2007 by Paul Alfille for sniffing 1-wire\n");
	printf("networks using the LINK\n");
	quit(0);
}


// show usage, exit

void usage(void)
{
	printf("Usage: %s -p port [-o logfile] [--version]", argv0);
	quit(1);
}


// show error message, exit

void error(const char *msg)
{
	if (errno) {
		printf("ERROR 1\n");
		printf("%s:        \n", argv0);
		printf("%s: %s:    \n", argv0, strerror(errno));
		printf("%s: %s: %s \n", argv0, strerror(errno), msg);
	} else {
		printf("ERROR 2\n");
		printf("%s: %s", argv0, msg);
	}
	quit(1);
}


// program mainline

int main(int argc,				// argument count
		 char *argv[]			// argument vector
	)
{
	char *devname = NULL;		// serial port device name
	speed_t speed = B9600;		// serial port speed
	int stopbit = 1;			// number of stop bits, can be 1 or 2

	char ttyname[PATH_MAX] = "";	// pseudo-tty slave name

	int maxfd = 0;				// maximum file descriptor, for select()
	int done, i;
	char c;
	char initstring[] = "\r \\\\";

	// capture current terminal settings

	tcgetattr(0, &oldStdinTio);

	// check argument count

	argv0 = argv[0];
	if (argc < 2)
		usage();

	// parse command line

	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "--version"))
			copyright();

	done = 0;
	while (!done) {
		switch (getopt(argc, argv, "p:o:")) {
		case 'p':				// set device
			devname = optarg;
			break;

		case 'o':				// ALFILLE create output file for binary
			outfile = fopen(optarg, "w");
			if (outfile == NULL)
				error("cannot open file");
			break;

		case '?':				// unknown argument
			usage();

		case -1:				// no more arguments
			done = 1;
			break;

		}
	}

	if (!devname)
		usage();

	// set console into raw mode

	tcgetattr(0, &newStdinTio);
	newStdinTio.c_lflag = 0;
	tcsetattr(0, TCSAFLUSH, &newStdinTio);
	maxfd = max(maxfd, STDIN_FILENO);

	// open serial device, set to transparent mode

	if ((devfd = open(devname, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
		error(devname);
	maxfd = max(maxfd, devfd);

	tcgetattr(devfd, &oldSerialTio);
	tcgetattr(devfd, &newSerialTio);
	newSerialTio.c_cflag = CS8 | CLOCAL | CREAD;
	if (stopbit == 2)
		newSerialTio.c_cflag |= CSTOPB;
	newSerialTio.c_iflag = IGNPAR;
	newSerialTio.c_oflag = 0;
	newSerialTio.c_lflag = 0;
	cfsetspeed(&newSerialTio, B9600);
	tcsetattr(devfd, TCSAFLUSH, &newSerialTio);
	fcntl(devfd, F_SETFL, fcntl(devfd, F_GETFL, 0) & ~O_NONBLOCK);

	// send a carriage return to elicit a prompt

	write(devfd, initstring, sizeof(initstring));

	// main loop

	while (1) {
		fd_set readset;
		int result;

		// wait for data

		do {
			struct timeval timeout = { 1, 0 };
			FD_ZERO(&readset);
			FD_SET(STDIN_FILENO, &readset);
			FD_SET(devfd, &readset);
			if (ptyfd)
				FD_SET(ptyfd, &readset);
			result = select(maxfd + 1, &readset, NULL, NULL, &timeout);
		}
		while (result < 0 && errno == EINTR);

		if (result == 0)
			break;				// timeout

		if (result < 0)
			error("can't select");

		// copy stdin to serial port

		if (FD_ISSET(STDIN_FILENO, &readset)) {
			int count, put, get;
			char inbuf[BUFLEN];
			char outbuf[BUFLEN];

			if ((count = read(STDIN_FILENO, inbuf, BUFLEN)) < 1)
				quit(0);

			for (get = 0, put = 0; put < count; put++) {
				switch (inbuf[put]) {
				case 0x1B:		// escape character?  exit
					if (put - get) {
						print_outfile(1, outbuf + get, put - get);
						write(devfd, outbuf + get, put - get);
					}
					tcdrain(devfd);
					quit(0);
					break;

				case 0x02:		// ^B?  send line break
					if (put - get) {
						print_outfile(1, outbuf + get, put - get);
						write(devfd, outbuf + get, put - get);
					}
					tcdrain(devfd);
					tcsendbreak(devfd, 0);
					get = put + 1;
					break;

				case 0x7F:		// map del to backspace
					outbuf[put] = 'H' - 0x40;
					break;

				case '\n':		// map newline to carriage return
					outbuf[put] = '\r';
					break;

				default:		// otherwise just send character
					outbuf[put] = inbuf[put];
					break;
				}
			}
			if (put - get) {
				print_outfile(1, outbuf + get, put - get);
				write(devfd, outbuf + get, put - get);
			}
		}
		// copy serial port to stdout and pseudo-tty
		if (FD_ISSET(devfd, &readset)) {
			int count;
			char buf[BUFLEN];

			count = read(devfd, buf, BUFLEN);
			print_outfile(0, buf, count);
			write(STDOUT_FILENO, buf, count);
			if (ptyfd)
				write(ptyfd, buf, count);
		}
		// copy pseudo-tty to serial port

		if (ptyfd && FD_ISSET(ptyfd, &readset)) {
			int count;
			char buf[BUFLEN];

			count = read(ptyfd, buf, BUFLEN);
			write(devfd, buf, count);
		}
	}

	// reset to new speed for sniffing
	cfsetspeed(&newSerialTio, B57600);
	tcsetattr(devfd, TCSAFLUSH, &newSerialTio);
	//fcntl(devfd, F_SETFL, fcntl(devfd, F_GETFL, 0) & ~O_NONBLOCK);

	// main loop
	printf("\nSniffing:\n");

	while (1) {
		fd_set readset;
		int result;

		// wait for data

		do {
			FD_ZERO(&readset);
			FD_SET(STDIN_FILENO, &readset);
			FD_SET(devfd, &readset);
			if (ptyfd)
				FD_SET(ptyfd, &readset);
			result = select(maxfd + 1, &readset, NULL, NULL, NULL);
		}
		while (result < 0 && errno == EINTR);
		if (result < 0)
			error("can't select");

		// copy stdin to serial port

		if (FD_ISSET(STDIN_FILENO, &readset)) {
			int count, put, get;
			char inbuf[BUFLEN];
			char outbuf[BUFLEN];

			if ((count = read(STDIN_FILENO, inbuf, BUFLEN)) < 1)
				quit(0);

			for (get = 0, put = 0; put < count; put++) {
				switch (inbuf[put]) {
				case 0x1B:		// escape character?  exit
					if (put - get) {
						print_outfile(1, outbuf + get, put - get);
						write(devfd, outbuf + get, put - get);
					}
					tcdrain(devfd);
					quit(0);
					break;

				case 0x02:		// ^B?  send line break
					if (put - get) {
						print_outfile(1, outbuf + get, put - get);
						write(devfd, outbuf + get, put - get);
					}
					tcdrain(devfd);
					tcsendbreak(devfd, 0);
					get = put + 1;
					break;

				case 0x7F:		// map del to backspace
					outbuf[put] = 'H' - 0x40;
					break;

				case '\n':		// map newline to carriage return
					outbuf[put] = '\r';
					break;

				default:		// otherwise just send character
					outbuf[put] = inbuf[put];
					break;
				}
			}
			if (put - get) {
				print_outfile(1, outbuf + get, put - get);
				write(devfd, outbuf + get, put - get);
			}
		}
		// copy serial port to stdout and pseudo-tty

		if (FD_ISSET(devfd, &readset)) {
			int count;
			char buf[BUFLEN];

			count = read(devfd, buf, BUFLEN);
			print_outfile(0, buf, count);
			write(STDOUT_FILENO, buf, count);
			if (ptyfd)
				write(ptyfd, buf, count);
		}
		// copy pseudo-tty to serial port

		if (ptyfd && FD_ISSET(ptyfd, &readset)) {
			int count;
			char buf[BUFLEN];

			count = read(ptyfd, buf, BUFLEN);
			write(devfd, buf, count);
		}
	}
}

void printchar(char *buf, int length)
{
	int i;
	for (i = 0; i < length; ++i) {
		char c = buf[i];
		if (c >= ' ' && c <= '~') {
			fprintf(outfile, "%c", c);
		} else {
			fprintf(outfile, ".");
		}
	}
}

void print_outfile(int keyed, char *buf, int length)
{
	int left = length;
	char *loc = buf;
	char direction = keyed ? '>' : '<';

	if (outfile == NULL)
		return;

	if (length == 0)
		return;

	while (left > 0) {
		int printlength = (left > 16) ? 16 : left;
		int i;
		fprintf(outfile, "%c ", direction);
		for (i = 0; i < 16; ++i) {
			if (i < printlength) {
				fprintf(outfile, " %.2X", loc[i]);
			} else {
				fprintf(outfile, "   ");
			}
		}
		fprintf(outfile, " # ");
		printchar(loc, printlength);
		fprintf(outfile, "\n");
		left -= printlength;
		loc += printlength;
	}
}
