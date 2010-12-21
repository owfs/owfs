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
	GOOD_OR_BAD ret ;

	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to open a NULL serial device");
		return gbBAD;
	}

	switch ( SOC(connection)->state ) {
		case cs_good:
			LEVEL_DEBUG("Attempt to reopen a good connection?");
			return gbGOOD ;
		case cs_virgin:
		case cs_bad:
			break ;
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
			ret = serial_open( connection ) ;
			break ;
	}

	if ( GOOD(ret) ) {
		SOC(connection)->state = cs_good ;
	} else {
		SOC(connection)->state = cs_bad ;
	}
	return ret ;
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
