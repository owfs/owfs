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

#include <linux/limits.h>
#include <termios.h>
#include <syslog.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "owfs_config.h"
#include "ow.h"

/* ---------------------------------------------- */
/* raw COM port interface routines                */
/* ---------------------------------------------- */

struct termios oldSerialTio;    /*old serial port settings*/
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
    int fd = in->fd ;

    tcgetattr(fd, &oldSerialTio);
    tcgetattr(fd, &newSerialTio);
    in->speed = B9600 ;
    cfsetospeed(&newSerialTio, in->speed);
    cfsetispeed(&newSerialTio, in->speed);

    // Set to non-canonical mode, and no RTS/CTS handshaking
    newSerialTio.c_iflag = IGNBRK|IGNPAR|IXANY;
    newSerialTio.c_oflag &= ~(OPOST);
    newSerialTio.c_cflag = CLOCAL|CS8|CREAD;
    newSerialTio.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ICANON|IEXTEN|ISIG);
    newSerialTio.c_cc[VMIN] = 0;
    newSerialTio.c_cc[VTIME] = 3;
    tcsetattr(fd, TCSAFLUSH, &newSerialTio);

//        fcntl(pn->si->fd, F_SETFL, fcntl(pn->si->fd, F_GETFL, 0) & ~O_NONBLOCK);
    return 0 ;
}

void COM_close( struct connection_in * in ) {
    int fd = in->fd ;
    // restore tty settings
    if ( fd > -1 ) {
        tcsetattr(fd, TCSAFLUSH, &oldSerialTio);
//        COM_flush(pn);
        tcflush(fd, TCIOFLUSH);
        close(fd);
        in->fd=-1 ;
    }
}

void COM_flush( const struct parsedname * pn ) {
    tcflush(pn->in->fd, TCIOFLUSH);
}

void COM_break( const struct parsedname * pn ) {
    tcsendbreak(pn->in->fd, 0);
}

void COM_speed(speed_t new_baud, const struct parsedname * pn) {
     struct termios t;

     // read the attribute structure
     if (tcgetattr(pn->in->fd, &t) < 0) return;

     // set baud in structure
     cfsetospeed(&t, new_baud);
     cfsetispeed(&t, new_baud);

     // change baud on port
     if (tcsetattr(pn->in->fd, TCSAFLUSH, &t) < 0) {
         if ( new_baud != B9600 ) COM_speed(B9600,pn) ;
         return ;
     }
     pn->in->speed = new_baud ;
     return ;
}
