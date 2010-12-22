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
GOOD_OR_BAD serial_open(struct connection_in *connection)
{
	struct termios newSerialTio;	/*new serial port settings */
	FILE_DESCRIPTOR_OR_ERROR fd ;

	fd = open( SOC(connection)->devicename, O_RDWR | O_NONBLOCK | O_NOCTTY) ;
	SOC(connection)->file_descriptor = fd ;
	if ( FILE_DESCRIPTOR_NOT_VALID( fd ) ) {
		// state doesn't change
		ERROR_DEFAULT("Cannot open port: %s Permissions problem?", SAFESTRING(SOC(connection)->devicename));
		return gbBAD;
	}

	if ( SOC(connection)->state == cs_virgin ) {
		// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
		memset( &(SOC(connection)->dev.serial.oldSerialTio), 0, sizeof(struct termios));
		if ((tcgetattr( fd, &(SOC(connection)->dev.serial.oldSerialTio) ) < 0)) {
			ERROR_CONNECT("Cannot get old port attributes: %s", SAFESTRING(SOC(connection)->devicename));
			// proceed anyway
		}
		SOC(connection)->state = cs_deflowered ;
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
	return gbGOOD;
}
