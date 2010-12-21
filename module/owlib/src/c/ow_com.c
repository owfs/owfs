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

//open serial port
// return 0=good
GOOD_OR_BAD COM_open(struct connection_in *connection)
{
	struct termios newSerialTio;	/*new serial port settings */
	FILE_DESCRIPTOR_OR_ERROR fd ;

	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to open a NULL serial device");
		return gbBAD;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return gbBAD ;
		case ct_telnet:
		case ct_tcp:
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_serial:
			break ;
	}

	switch ( SOC(connection)->state ) {
		case cs_good:
			LEVEL_DEBUG("Attempt to reopen a good connection?");
			return gbGOOD ;
		case cs_virgin:
		case cs_bad:
			break ;
	}

	fd = open( SOC(connection)->devicename, O_RDWR | O_NONBLOCK | O_NOCTTY) ;
	connection->soc.file_descriptor = fd ;
	if ( FILE_DESCRIPTOR_NOT_VALID( fd ) ) {
		// state doesn't change
		ERROR_DEFAULT("Cannot open port: %s Permissions problem?", SAFESTRING(SOC(connection)->devicename));
		return gbBAD;
	}

	// assume state is bad
	SOC(connection)->state = cs_bad ;

	if ( SOC(connection)->state == cs_virgin ) {
		// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
		memset( &(SOC(connection)->dev.serial.oldSerialTio), 0, sizeof(struct termios));
		if ((tcgetattr( fd, &(SOC(connection)->dev.serial.oldSerialTio) ) < 0)) {
			ERROR_CONNECT("Cannot get old port attributes: %s", SAFESTRING(SOC(connection)->devicename));
			// proceed anyway
		}
	}

	// set baud in structure
	COM_speed( B9600, connection ) ;
	memset(&newSerialTio, 0, sizeof(struct termios));
	if ((tcgetattr( fd, &newSerialTio) < 0)) {
		ERROR_CONNECT("Cannot get new port attributes: %s", SAFESTRING(SOC(connection)->devicename));
	}
	
	// Set to non-canonical mode, and no RTS/CTS handshaking
	newSerialTio.c_iflag &= ~(BRKINT | ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXON | IXOFF | PARMRK);
	newSerialTio.c_iflag |= IGNBRK | IGNPAR;
	newSerialTio.c_oflag &= ~(OPOST);
	switch( SOC(connection)->dev.serial.flow_control ) {
		case flow_hard:
			newSerialTio.c_cflag &= ~(CSIZE | HUPCL | PARENB);
			newSerialTio.c_cflag |= (CRTSCTS | CLOCAL | CS8 | CREAD);
			break ;
		case flow_none:
			newSerialTio.c_cflag &= ~(CRTSCTS | CSIZE | HUPCL | PARENB);
			newSerialTio.c_cflag |= (CLOCAL | CS8 | CREAD);
			break ;
		case flow_soft:
		default:
			LEVEL_DEBUG("Unsupported COM port flow control");
			return -ENOTSUP ;
	}
	newSerialTio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | IEXTEN | ISIG);
	newSerialTio.c_cc[VMIN] = 0;
	newSerialTio.c_cc[VTIME] = 3;
	if (tcsetattr( fd, TCSAFLUSH, &newSerialTio)) {
		ERROR_CONNECT("Cannot set port attributes: %s", SAFESTRING(SOC(connection)->devicename));
		return gbBAD;
	}
	tcflush( fd, TCIOFLUSH);
	//fcntl(pn->si->file_descriptor, F_SETFL, fcntl(pn->si->file_descriptor, F_GETFL, 0) & ~O_NONBLOCK);
	SOC(connection)->state = cs_good ;
	return gbGOOD;
}

void COM_close(struct connection_in *connection)
{
	FILE_DESCRIPTOR_OR_ERROR fd ;

	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to close a NULL serial device");
		return ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
		case ct_usb:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return ;
		case ct_telnet:
		case ct_tcp:
		case ct_i2c:
		case ct_netlink:
			SOC(connection)->state = cs_bad ;
			Test_and_Close( &( SOC(connection)->file_descriptor) ) ;
			LEVEL_DEBUG("Unimplemented!!!");
			return ;
		case ct_serial:
			break ;
	}

	fd = SOC(connection)->file_descriptor ;

	switch ( SOC(connection)->state ) {
		case cs_good:
			break ;
		case cs_virgin:
			return ;
		case cs_bad:
			// reopen to restore attributes
			fd = open( SOC(connection)->devicename, O_RDWR | O_NONBLOCK | O_NOCTTY) ;
			break ;
	}

	// restore tty settings
	if ( FILE_DESCRIPTOR_VALID( fd ) ) {
		LEVEL_DEBUG("COM_close: flush");
		tcflush( fd, TCIOFLUSH);
		LEVEL_DEBUG("COM_close: restore");
		if ( tcsetattr( fd, TCSANOW, &(SOC(connection)->dev.serial.oldSerialTio) ) < 0) {
			ERROR_CONNECT("Cannot restore port attributes: %s", SAFESTRING(SOC(connection)->devicename));
		}
		LEVEL_DEBUG("COM_close: close");
		close( fd );
		SOC(connection)->file_descriptor = FILE_DESCRIPTOR_BAD;
		SOC(connection)->state = cs_bad ;
	}
}

void COM_flush( const struct connection_in *connection)
{
	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to flush a NULL device");
		return ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return ;
		case ct_telnet:
		case ct_tcp:
			tcp_read_flush( SOC(connection)->file_descriptor) ;
			break ;
		case ct_netlink:
		case ct_i2c:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return ;
		case ct_serial:
			tcflush( SOC(connection)->file_descriptor, TCIOFLUSH);
			break ;
	}
}

void COM_break(struct connection_in *in)
{
	tcsendbreak(SOC(in)->file_descriptor, 0);
}

void COM_speed(speed_t new_baud, struct connection_in *in)
{
	struct termios t;

	// read the attribute structure
	// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
	memset(&t, 0, sizeof(struct termios));
	if (tcgetattr(SOC(in)->file_descriptor, &t) < 0) {
		ERROR_CONNECT("Could not get com port attributes: %s", SAFESTRING(SOC(in)->devicename));
		return;
	}
	// set baud in structure
	if (cfsetospeed(&t, new_baud) < 0 || cfsetispeed(&t, new_baud) < 0) {
		ERROR_CONNECT("Trouble setting port speed: %s", SAFESTRING(SOC(in)->devicename));
	}
	// change baud on port
	in->soc.dev.serial.baud = new_baud ;
	if (tcsetattr(SOC(in)->file_descriptor, TCSAFLUSH, &t) < 0) {
		ERROR_CONNECT("Could not set com port attributes: %s", SAFESTRING(SOC(in)->devicename));
		if (new_baud != B9600) { // avoid infinite recursion
			COM_speed(B9600, in);
		}
	}
}
