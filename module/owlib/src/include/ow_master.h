/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.


*/

#ifndef OW_MASTER_H			/* tedious wrapper */
#define OW_MASTER_H

/* included in ow_connection.h as the bus-master specific portion of the connection_in structure */

struct master_server {
	char *type;					// for zeroconf
	char *domain;				// for zeroconf
	char *name;					// zeroconf name
	int no_dirall;				// flag that server doesn't support DIRALL
} ;

struct master_serial {
	enum ds2480b_mode { ds2480b_data_mode, ds2480b_command_mode, } mode ;
	int reverse_polarity ;
};

struct master_fake {
	int index;
	_FLOAT templow;
	_FLOAT temphigh;

	// For adapters that maintain dir-at-once (or dirgulp):
	struct dirblob main;        /* main directory */
	struct dirblob alarm;       /* alarm directory */
};

struct master_usb {
#if OW_USB
	struct usb_device *dev;
	struct usb_dev_handle *usb;
	int usb_bus_number;
	int usb_dev_number;
	int datasampleoffset;
	int writeonelowtime;
	int pulldownslewrate;
	int timeout;
#endif /* OW_USB */
	int specific_usb_address ; // only a specific address requested?

	/* "Name" of the device, like "8146572300000051"
	 * This is set to the first DS1420 id found on the 1-wire adapter which
	 * exists on the DS9490 adapters. If user only have a DS2490 chip, there
	 * are no such DS1420 device available. It's used to find the 1-wire adapter
	 * if it's disconnected and later reconnected again.
	 */
	BYTE ds1420_address[SERIAL_NUMBER_SIZE];
};

struct master_i2c {
	int channels;
	int index;
	int i2c_address;
	int i2c_index ;
	BYTE configreg;
	BYTE configchip;
	/* only one per chip, the bus entries for the other 7 channels point to the first one */
	pthread_mutex_t all_channel_lock;	// second level mutex for the entire chip */
	int current;
	struct connection_in *head;
};

struct master_ha7 {
	ASCII lock[10];
	int locked;
	int found;
};

enum e_link_t_mode { e_link_t_unknown, e_link_t_extra, e_link_t_none } ;

struct master_link {
	enum e_link_t_mode tmode ; // extra ',' before tF0 command
	enum e_link_t_mode qmode ; //extra '?' after b command
};

struct master_ha5 {
	int checksum ;              /* flag to use checksum byte in communication */
	char channel ;
	pthread_mutex_t all_channel_lock;	// second level mutex for the entire chip */
	struct connection_in *head;
};

struct master_w1 {
#if OW_W1
	// bus master name kept in name
	SEQ_OR_ERROR seq ;
	int id ; // equivalent to the number part of w1_bus_master23
	FILE_DESCRIPTOR_OR_ERROR netlink_pipe[2] ;
	enum enum_w1_slave_order { w1_slave_order_unknown, w1_slave_order_forward, w1_slave_order_reversed } w1_slave_order ;
#endif /* OW_W1 */
};

struct master_w1_monitor {
#if OW_W1
	// netlink fd kept in file_descriptor
	SEQ_OR_ERROR seq ; // seq number to netlink
	pid_t pid ;
	struct timeval last_read ;

	pthread_mutex_t seq_mutex;	// mutex for w1 sequence number */
	pthread_mutex_t read_mutex;  // mutex for w1 netlink read time
#endif /* OW_W1 */
};

struct master_usb_monitor {
	FILE_DESCRIPTOR_OR_ERROR shutdown_pipe[2] ;
};

struct master_browse {
#if OW_ZERO
	DNSServiceRef bonjour_browse;

#if !OW_CYGWIN
	AvahiServiceBrowser *avahi_browser ;
	AvahiSimplePoll *avahi_poll ;
	AvahiClient *avahi_client ;
#if __HAS_IPV6__
	char avahi_host[INET6_ADDRSTRLEN+1] ;
#else
	char avahi_host[INET_ADDRSTRLEN+1] ;
#endif
	char avahi_service[10] ;
#endif							/* OW_CYGWIN */
#endif /* OW_ZERO */
};

union master_union {
	struct master_serial serial;
	struct master_link link;
	struct master_server server ;
	struct master_usb usb;
	struct master_i2c i2c;
	struct master_fake fake;
	struct master_fake tester;
	struct master_fake mock;
	struct master_ha5 ha5;
	struct master_ha7 ha7;
	struct master_w1 w1;
	struct master_w1_monitor w1_monitor ;
	struct master_browse browse;
	struct master_usb_monitor usb_monitor ;
};


#endif							/* OW_MASTER_H */
