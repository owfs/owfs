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
GOOD_OR_BAD COM_open(struct connection_in *in)
{
	struct termios newSerialTio;	/*new serial port settings */
	if (in == NULL) {
		LEVEL_DEBUG("Attempt to open a NULL serial device");
		return gbBAD;
	}
//    if ((in->file_descriptor = open(in->name, O_RDWR | O_NONBLOCK )) < 0) {
	in->file_descriptor = open(in->name, O_RDWR | O_NONBLOCK | O_NOCTTY) ;
	if ( FILE_DESCRIPTOR_NOT_VALID( in->file_descriptor ) ) {
		ERROR_DEFAULT("Cannot open port: %s Permissions problem?", SAFESTRING(in->name));
		return gbBAD;
	}
	// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
	memset(&(in->oldSerialTio), 0, sizeof(struct termios));
	if ((tcgetattr(in->file_descriptor, &in->oldSerialTio) < 0)) {
		ERROR_CONNECT("Cannot get old port attributes: %s", SAFESTRING(in->name));
	}

	// set baud in structure
	COM_speed( B9600, in ) ;
	memset(&newSerialTio, 0, sizeof(struct termios));
	if ((tcgetattr(in->file_descriptor, &newSerialTio) < 0)) {
		ERROR_CONNECT("Cannot get new port attributes: %s", SAFESTRING(in->name));
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
		ERROR_CONNECT("Cannot set port attributes: %s", SAFESTRING(in->name));
		return gbBAD;
	}
	tcflush(in->file_descriptor, TCIOFLUSH);
	//fcntl(pn->si->file_descriptor, F_SETFL, fcntl(pn->si->file_descriptor, F_GETFL, 0) & ~O_NONBLOCK);
	return gbGOOD;
}

void COM_close(struct connection_in *in)
{
	// restore tty settings
	if ( FILE_DESCRIPTOR_VALID( in->file_descriptor ) ) {
		LEVEL_DEBUG("COM_close: flush");
		tcflush(in->file_descriptor, TCIOFLUSH);
		LEVEL_DEBUG("COM_close: restore");
		if (tcsetattr(in->file_descriptor, TCSANOW, &in->oldSerialTio) < 0) {
			ERROR_CONNECT("Cannot restore port attributes: %s", SAFESTRING(in->name));
		}
		LEVEL_DEBUG("COM_close: close");
		close(in->file_descriptor);
		in->file_descriptor = FILE_DESCRIPTOR_BAD;
	}
}

void COM_flush( const struct connection_in *in)
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
		ERROR_CONNECT("Could not get com port attributes: %s", SAFESTRING(in->name));
		return;
	}
	// set baud in structure
	if (cfsetospeed(&t, new_baud) < 0 || cfsetispeed(&t, new_baud) < 0) {
		ERROR_CONNECT("Trouble setting port speed: %s", SAFESTRING(in->name));
	}
	// change baud on port
	in->baud = new_baud ;
	if (tcsetattr(in->file_descriptor, TCSAFLUSH, &t) < 0) {
		ERROR_CONNECT("Could not set com port attributes: %s", SAFESTRING(in->name));
		if (new_baud != B9600) { // avoid infinite recursion
			COM_speed(B9600, in);
		}
	}
}
