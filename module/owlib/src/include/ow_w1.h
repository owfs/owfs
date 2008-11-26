/*
$Id$
W! Announce -- daemon  for showing w1 busmasters using Avahi
Written 2008 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPLv2
Much thanks to Evgeniy Polyakov
*/

#if OW_W1
#ifndef OW_W1_H
#define OW_W1_H

#include <stdint.h>
#include "netlink.h"
#include "connector.h"
#include "w1_netlink.h"

int w1_bind( void ) ;
void w1_unbind( void ) ;

void RemoveW1Bus( int bus_master ) ;
void AddW1Bus( int bus_master ) ;
int W1_send_msg( struct w1_netlink_msg *msg) ;
int W1Select( void ) ;

int W1NLScan( void ) ;
int W1NLList( void ) ;

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

#endif 	/* OW_W1_H */
#endif /* OW_W1 */
