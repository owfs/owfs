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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

/* ---------------------------------------------- */
/* raw COM port interface routines                */
/* ---------------------------------------------- */

//
//open serial port
// set global pn->si->file_descriptor and devport
/* return 0=good
 *        -errno = cannot opon
 *        -EFAULT = already open
 */
/* return 0 for success, 1 for failure */
int COM_open(struct connection_in *in)
{
	struct termios newSerialTio;	/*new serial port settings */
	if (in == NULL) {
		LEVEL_DEBUG("Attempt to open a NULL serial device\n");
		return -ENODEV;
	}
//    if ((in->file_descriptor = open(in->name, O_RDWR | O_NONBLOCK )) < 0) {
	if ((in->file_descriptor = open(in->name, O_RDWR | O_NONBLOCK | O_NOCTTY)) < 0) {
		ERROR_DEFAULT("Cannot open port: %s\n", SAFESTRING(in->name));
		return -ENODEV;
	}
	// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
	memset(&(in->oldSerialTio), 0, sizeof(struct termios));
	if ((tcgetattr(in->file_descriptor, &in->oldSerialTio) < 0)) {
		ERROR_CONNECT("Cannot get old port attributes: %s\n", SAFESTRING(in->name));
	}

	// set baud in structure
	COM_speed( B9600, in ) ;
	memset(&newSerialTio, 0, sizeof(struct termios));
	if ((tcgetattr(in->file_descriptor, &newSerialTio) < 0)) {
		ERROR_CONNECT("Cannot get new port attributes: %s\n", SAFESTRING(in->name));
	}
	
	// Set to non-canonical mode, and no RTS/CTS handshaking
	newSerialTio.c_iflag &= ~(BRKINT | ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXON | IXOFF | PARMRK);
	newSerialTio.c_iflag |= IGNBRK | IGNPAR;
	newSerialTio.c_oflag &= ~(OPOST);
	newSerialTio.c_cflag &= ~(CRTSCTS | CSIZE | HUPCL | PARENB);
	newSerialTio.c_cflag |= (CLOCAL | CS8 | CREAD);
	newSerialTio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | IEXTEN | ISIG);
	newSerialTio.c_cc[VMIN] = 0;
	newSerialTio.c_cc[VTIME] = 3;
	if (tcsetattr(in->file_descriptor, TCSAFLUSH, &newSerialTio)) {
		ERROR_CONNECT("Cannot set port attributes: %s\n", SAFESTRING(in->name));
		return -EIO;
	}
	tcflush(in->file_descriptor, TCIOFLUSH);
	//fcntl(pn->si->file_descriptor, F_SETFL, fcntl(pn->si->file_descriptor, F_GETFL, 0) & ~O_NONBLOCK);
	return 0;
}

void COM_close(struct connection_in *in)
{
	// restore tty settings
	if (in->file_descriptor > -1) {
		LEVEL_DEBUG("COM_close: flush\n");
		tcflush(in->file_descriptor, TCIOFLUSH);
		LEVEL_DEBUG("COM_close: restore\n");
		if (tcsetattr(in->file_descriptor, TCSANOW, &in->oldSerialTio) < 0) {
			ERROR_CONNECT("Cannot restore port attributes: %s\n", SAFESTRING(in->name));
		}
		LEVEL_DEBUG("COM_close: close\n");
		close(in->file_descriptor);
		in->file_descriptor = -1;
	}
}

void COM_flush(struct connection_in *in)
{
	tcflush(in->file_descriptor, TCIOFLUSH);
}

void COM_break(struct connection_in *in)
{
	tcsendbreak(in->file_descriptor, 0);
}

void COM_speed(speed_t new_baud, struct connection_in *in)
{
	struct termios t;

	// read the attribute structure
	// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
	memset(&t, 0, sizeof(struct termios));
	if (tcgetattr(in->file_descriptor, &t) < 0) {
		ERROR_CONNECT("Could not get com port attributes: %s\n", SAFESTRING(in->name));
		return;
	}
	// set baud in structure
	if (cfsetospeed(&t, new_baud) < 0 || cfsetispeed(&t, new_baud) < 0) {
		ERROR_CONNECT("Trouble setting port speed: %s\n", SAFESTRING(in->name));
	}
	// change baud on port
	in->baud = new_baud ;
	if (tcsetattr(in->file_descriptor, TCSAFLUSH, &t) < 0) {
		ERROR_CONNECT("Could not set com port attributes: %s\n", SAFESTRING(in->name));
		if (new_baud != B9600) { // avoid infinite recursion
			COM_speed(B9600, in);
		}
	}
}
