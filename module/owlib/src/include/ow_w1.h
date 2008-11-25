/*
$Id$
W! Announce -- daemon  for showing w1 busmasters using Avahi
Written 2008 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPLv2
Much thanks to Evgeniy Polyakov
*/


#ifndef W1_REPEATER_H
#define W1_REPEATER_H

extern int nl_file_descriptor ;

int w1_bind( void ) ;
void RemoveBus( int bus_master ) ;
void AddBus( int bus_master ) ;
int Avahi_link( void ) ;
int w1_scan( void ) ;
int w1_load_sysfs( const char * directory ) ;
int Announce_Control_init( int allocated ) ;

struct netlink_parse {
	struct nlmsghdr *	nlm ;
	struct cn_msg * 	cn ;
	struct w1_netlink_msg *	w1m ;
	struct w1_netlink_cmd *	w1c ;
	unsigned char *		data ;
	int			data_size ;
} ;

void Netlink_Parse_Destroy( struct netlink_parse * nlp ) ;
int Netlink_Parse_Get( struct netlink_parse * nlp ) ;

#endif 	/* W1_REPEATER_H */
