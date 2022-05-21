/*
W! Announce -- daemon  for showing w1 busmasters using Avahi
Written 2008 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPLv2
Much thanks to Evgeniy Polyakov
*/

#if OW_W1

#ifndef OW_W1_H
#define OW_W1_H

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include "netlink.h"
#include "connector.h"
#include "w1_netlink.h"

// for some local types
#include "ow_fd.h"
#include "ow_localreturns.h"
#include "ow_w1_seq.h"

#define W1_NLM_LENGTH	16
#define W1_CN_LENGTH	20
#define W1_W1M_LENGTH	12
#define W1_W1C_LENGTH	4

enum Netlink_Read_Status {
	nrs_complete = 0 ,
	nrs_bad_send,
	nrs_nodev,
	nrs_timeout,
	nrs_error,
} ;

struct connection_in ;
struct parsedname ;

GOOD_OR_BAD w1_bind( struct connection_in * in ) ;

void RemoveW1Bus( int bus_master ) ;
void AddW1Bus( int bus_master ) ;
SEQ_OR_ERROR W1_send_msg( struct connection_in * in, struct w1_netlink_msg *msg, struct w1_netlink_cmd *cmd, const unsigned char * data) ;
GOOD_OR_BAD W1PipeSelect_timeout( FILE_DESCRIPTOR_OR_ERROR file_descriptor ) ;
void * W1_Dispatch( void * v ) ;

SEQ_OR_ERROR w1_list_masters( void ) ;

#define MAKE_NL_SEQ( bus, seq )  ((uint32_t)(( ((bus) & 0xFFFF) << 16 ) | ((seq) & 0xFFFF)))
#define NL_SEQ( seq )  ((uint32_t)((seq) & 0xFFFF))
#define NL_BUS( seq )  ((uint32_t)(((seq) >> 16 ) & 0xFFFF))

struct netlink_parse {
	struct nlmsghdr *	nlm ;
	struct cn_msg * 	cn ;
	struct w1_netlink_msg *	w1m ;
	struct w1_netlink_cmd *	w1c ;
	unsigned char *		data ;
	int			data_size ;
	__u8		follow[0] ;
} ;

//void * w1_master_command(struct netlink_parse * nlp) ;
void * w1_master_command(void * v) ;

void w1_parse_master_list(struct netlink_parse * nlp);
GOOD_OR_BAD Netlink_Parse_Get( struct netlink_parse * nlp ) ;
GOOD_OR_BAD Netlink_Parse_Buffer( struct netlink_parse * nlp ) ;
void Netlink_Print( struct nlmsghdr * nlm, struct cn_msg * cn, struct w1_netlink_msg * w1m, struct w1_netlink_cmd * w1c, unsigned char * data, int length ) ;
enum Netlink_Read_Status W1_Process_Response( void (* nrs_callback)( struct netlink_parse * nlp, void  *v, const struct parsedname * pn), SEQ_OR_ERROR seq, void * v, const struct parsedname * pn ) ;

#endif 	/* OW_W1_H */

#endif /* OW_W1 */
