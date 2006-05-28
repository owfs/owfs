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
// set global pn->si->fd and devport
/* return 0=good
 *        -errno = cannot opon
 *        -EFAULT = already open
 */
/* return 0 for success, 1 for failure */
int COM_open( struct connection_in * in ) {
    struct termios newSerialTio; /*new serial port settings*/
    if(!in) return -ENODEV;

    if ((in->fd = open(in->name, O_RDWR | O_NONBLOCK)) < 0) {
        ERROR_DEFAULT("Cannot open port: %s\n",SAFESTRING(in->name));
        STAT_ADD1_BUS(BUS_open_errors,in) ;
        return -ENODEV;
    }

    if ( tcgetattr(in->fd, &in->connin.serial.oldSerialTio) < 0 ) {
        ERROR_CONNECT("Cannot get old port attributes: %s\n",SAFESTRING(in->name)) ;
    }
    memcpy(&newSerialTio, &in->connin.serial.oldSerialTio, sizeof(struct termios));
    in->connin.serial.speed = B9600 ;
     // set baud in structure
    if ( cfsetospeed(&newSerialTio, in->connin.serial.speed)<0 || cfsetispeed(&newSerialTio, in->connin.serial.speed)<0 ) {
        ERROR_CONNECT("Trouble setting port speed: %s\n",SAFESTRING(in->name)) ;
    }

    // Set to non-canonical mode, and no RTS/CTS handshaking
    newSerialTio.c_iflag = IGNBRK|IGNPAR|IXANY;
    newSerialTio.c_oflag &= ~(OPOST);
    newSerialTio.c_cflag = CLOCAL|CS8|CREAD;
    newSerialTio.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ICANON|IEXTEN|ISIG);
    newSerialTio.c_cc[VMIN] = 0;
    newSerialTio.c_cc[VTIME] = 3;
    if (tcsetattr(in->fd, TCSAFLUSH, &newSerialTio)) {
        ERROR_CONNECT("Cannot set port attributes: %s\n",SAFESTRING(in->name));
        STAT_ADD1_BUS(BUS_tcsetattr_errors,in) ;
        return -EIO ;
    }

    //fcntl(pn->si->fd, F_SETFL, fcntl(pn->si->fd, F_GETFL, 0) & ~O_NONBLOCK);
    return 0 ;
}

void COM_close( struct connection_in * in ) {
    int fd ;
    if(!in) return;
    fd = in->fd ;
    // restore tty settings
    if ( fd > -1 ) {
        if (tcsetattr(fd, TCSAFLUSH, &in->connin.serial.oldSerialTio)) {
            ERROR_CONNECT("Cannot restore port attributes: %s\n",SAFESTRING(in->name));
            STAT_ADD1_BUS(BUS_tcsetattr_errors,in) ;
        }
        tcflush(fd, TCIOFLUSH);
        close(fd);
        in->fd = -1 ;
    }
}

void COM_flush( const struct parsedname * pn ) {
    if(!pn || !pn->in || (pn->in->fd < 0)) return;
    tcflush(pn->in->fd, TCIOFLUSH);
}

void COM_break( const struct parsedname * pn ) {
    if(!pn || !pn->in || (pn->in->fd < 0)) return;
    tcsendbreak(pn->in->fd, 0);
}

void COM_speed(speed_t new_baud, const struct parsedname * pn) {
     struct termios t;
     if(!pn || !pn->in || (pn->in->fd < 0)) return;

     // read the attribute structure
     if (tcgetattr(pn->in->fd, &t) < 0) {
         ERROR_CONNECT("Could not get com port attributes: %s\n",SAFESTRING(pn->in->name)) ;
         return;
     }

     // set baud in structure
     if ( cfsetospeed(&t, new_baud)<0 || cfsetispeed(&t, new_baud)<0 ) {
         ERROR_CONNECT("Trouble setting port speed: %s\n",SAFESTRING(pn->in->name)) ;
     }

     // change baud on port
     if (tcsetattr(pn->in->fd, TCSAFLUSH, &t) < 0) {
         ERROR_CONNECT("Could not set com port attributes: %s\n",SAFESTRING(pn->in->name)) ;
         if ( new_baud != B9600 ) COM_speed(B9600,pn) ;
         return ;
     }
     pn->in->connin.serial.speed = new_baud ;
     return ;
}
