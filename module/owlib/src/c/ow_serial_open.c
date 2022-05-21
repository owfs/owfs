/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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

// Fix an OpenWRT error found by Patryk
#ifndef CMSPAR
  #define CMSPAR 010000000000
#endif

/* ---------------------------------------------- */
/* raw COM port interface routines                */
/* ---------------------------------------------- */

//open serial port ( called on head of connection_in group from com_open )
GOOD_OR_BAD serial_open(struct connection_in *connection)
{
	struct port_in * pin = connection->pown ;
	FILE_DESCRIPTOR_OR_ERROR fd = open( DEVICENAME(connection), O_RDWR | O_NONBLOCK | O_NOCTTY) ;
	pin->file_descriptor = fd ;
	if ( FILE_DESCRIPTOR_NOT_VALID( fd ) ) {
		// state doesn't change
		ERROR_DEFAULT("Cannot open port: %s Permissions problem?", SAFESTRING(DEVICENAME(connection)));
		return gbBAD;
	}

	if ( pin->state == cs_virgin ) {
		// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
		memset( &(pin->dev.serial.oldSerialTio), 0, sizeof(struct termios));
		if ((tcgetattr( fd, &(pin->dev.serial.oldSerialTio) ) < 0)) {
			ERROR_CONNECT("Cannot get old port attributes: %s", SAFESTRING(DEVICENAME(connection)));
			// proceed anyway
		}
		pin->state = cs_deflowered ;
	}

	return serial_change( connection ) ;
}

//change serial port settings
GOOD_OR_BAD serial_change(struct connection_in *connection)
{
	struct port_in * pin = connection->pown ;
	struct termios newSerialTio;	/*new serial port settings */
	FILE_DESCRIPTOR_OR_ERROR fd = pin->file_descriptor ;
	size_t baud = pin->baud ;

	// read the attribute structure
	// valgrind warns about uninitialized memory in tcsetattr(), so clear all.
	memset(&newSerialTio, 0, sizeof(struct termios));
	if ((tcgetattr( fd, &newSerialTio) < 0)) {
		ERROR_CONNECT("Cannot get existing port attributes: %s", SAFESTRING(DEVICENAME(connection)));
	}
	
	// set baud in structure
	if (cfsetospeed(&newSerialTio, baud) < 0 || cfsetispeed(&newSerialTio, baud) < 0) {
		ERROR_CONNECT("Trouble setting port speed: %s", SAFESTRING(DEVICENAME(connection)));
		cfsetospeed(&newSerialTio, B9600) ;
		cfsetispeed(&newSerialTio, B9600) ;
		pin->baud = B9600 ;
	}

	// Set to non-canonical mode, and no RTS/CTS handshaking
	newSerialTio.c_iflag &= ~(BRKINT | ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXON | IXOFF | PARMRK);
	newSerialTio.c_iflag |= IGNBRK | IGNPAR;

	newSerialTio.c_oflag &= ~(OPOST);

	newSerialTio.c_cflag &= ~ HUPCL ;
	newSerialTio.c_cflag |= (CLOCAL | CREAD);

	switch( pin->flow ) {
		case flow_hard:
			newSerialTio.c_cflag |= CRTSCTS ;
			break ;
		case flow_none:
			newSerialTio.c_cflag &= ~CRTSCTS;
			break ;
		case flow_soft:
		default:
			LEVEL_DEBUG("Unsupported COM port flow control");
			return -ENOTSUP ;
	}

	// set bit length
	newSerialTio.c_cflag &= ~ CSIZE ;
	switch (pin->bits) {
		case 5:
			newSerialTio.c_cflag |= CS5 ;
			break ;
		case 6:
			newSerialTio.c_cflag |= CS6 ;
			break ;
		case 7:
			newSerialTio.c_cflag |= CS7 ;
			break ;
		case 8:
		default:
			newSerialTio.c_cflag |= CS8 ;
			break ;
	}

	// parity
	switch (pin->parity) {
		case parity_none:
			newSerialTio.c_cflag &= ~PARENB ;
			break ;
		case parity_even:
			newSerialTio.c_cflag |= PARENB ;
			newSerialTio.c_cflag &= ~( PARODD | CMSPAR ) ;
			break ;
		case parity_odd:
			newSerialTio.c_cflag |= PARENB | PARODD;
			newSerialTio.c_cflag &= ~CMSPAR ;
			break ;
		case parity_mark:
			newSerialTio.c_cflag |= PARENB | PARODD | CMSPAR;
			break ;
	}

	// stop bits
	switch (pin->stop) {
		case stop_15:
			LEVEL_DEBUG("1.5 Stop bits not supported");
			pin->stop = stop_1 ;
			// fall through
		case stop_1:
			newSerialTio.c_cflag &= ~CSTOPB ;
			break ;
		case stop_2:
			newSerialTio.c_cflag |= CSTOPB ;
			break ;
	}


	newSerialTio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | IEXTEN | ISIG);
	newSerialTio.c_cc[VMIN] = pin->vmin;
	newSerialTio.c_cc[VTIME] = pin->vtime;
	if (tcsetattr( fd, TCSAFLUSH, &newSerialTio)) {
		ERROR_CONNECT("Cannot set port attributes: %s", SAFESTRING(DEVICENAME(connection)));
		return gbBAD;
	}
	tcflush( fd, TCIOFLUSH);
	return gbGOOD;
}

//change set to 0 baud for 1 second
GOOD_OR_BAD serial_powercycle(struct connection_in *connection)
{
	struct port_in * pin = connection->pown ;
	FILE_DESCRIPTOR_OR_ERROR fd = pin->file_descriptor ;
	size_t baud = pin->baud ;

	if ( pin->type != ct_serial ) {
		return gbGOOD ;
	}

	if ( FILE_DESCRIPTOR_NOT_VALID(fd) ) {
		LEVEL_DEBUG("Cannot power cycle a closed serial port");
		return gbBAD ;
	}
	
	// temporary
	pin->baud = B0 ;
	
	if ( GOOD(serial_change( connection )) ) {
		LEVEL_DEBUG("Sleep after setting DTR/RTS pins off");
		sleep(2) ;
	}

	// restore
	pin->baud = baud ;
	
	// Close for good measure
	Test_and_Close( &( pin->file_descriptor) ) ;
	return serial_open( connection ) ;
}
