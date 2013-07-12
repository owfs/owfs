/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ENET2 (OW-SERVER-ENET-2) discovery code */
/* Basically udp broadcast of "D" to port 30303 */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "jsmn.h"

/* from http://stackoverflow.com/questions/337422/how-to-udp-broadcast-with-c-in-linux */
static void Setup_ENET_hint( struct addrinfo * hint )
{
	memset(hint, 0, sizeof(struct addrinfo));
#ifdef AI_NUMERICSERV
	hint->ai_flags = AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV;
#else
	hint->ai_flags = AI_CANONNAME | AI_NUMERICHOST;
#endif
	hint->ai_family = AF_INET;
//    hint->ai_socktype = SOCK_DGRAM | SOCK_CLOEXEC ; // udp
    hint->ai_socktype = SOCK_DGRAM ; // udp
	hint->ai_protocol = 0;
}

#define RESPONSE_BUFFER_LENGTH 512

static GOOD_OR_BAD send_ENET_discover( FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct addrinfo *now )
{
    char * message = "D";

	if (sendto(file_descriptor, message, strlen(message), 0, now->ai_addr, now->ai_addrlen) < 0) {
		ERROR_CONNECT("Trouble sending broadcast message");
		return gbBAD ;
	}
	return gbGOOD ;
}

static GOOD_OR_BAD set_ENET_broadcast( FILE_DESCRIPTOR_OR_ERROR file_descriptor )
{
	int broadcastEnable=1;

	if (setsockopt(file_descriptor, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1) {
		ERROR_DEBUG("Cannot set socket option for broadcast.");
		return gbBAD ;
	}
	
	return gbGOOD ;
}

// maximun number of JSON tokens
#define jsmnTOKnum	50
#define ENET_PARAM_SIZE 100

static void parse_ENET_response( char * response_buffer, int * found_version, char * found_ip, char * found_port )
{
	// use jsmn JSON parsing code
	jsmn_parser parser ;
	jsmntok_t tokens[jsmnTOKnum] ;
	int token_index ;
	enum { jsp_key, jsp_value, jsp_version, jsp_ip, jsp_port } jsp_string  = jsp_key ;
	
	jsmn_init( &parser ) ;
	switch ( jsmn_parse( &parser, response_buffer, tokens, jsmnTOKnum ) ) {
		case JSMN_ERROR_NOMEM:
			LEVEL_DEBUG("Not enough JSON tokens were provided");
			break ;
		case JSMN_ERROR_INVAL:
			LEVEL_DEBUG("Invalid character inside JSON string");
			break ;
		case JSMN_ERROR_PART:
			LEVEL_DEBUG("The string is not a full JSON packet, more bytes expected");
			break ;
		case JSMN_SUCCESS:
			break ;
	}
	//printf("PARSED tokens=%d\n",parser.toknext);
	for ( token_index = 0 ; token_index < parser.toknext ; ++token_index ) {
		int length = tokens[token_index].end - tokens[token_index].start ;
		char * tok_start = &response_buffer[tokens[token_index].start] ;
		char temp_null = tok_start[length] ;
		tok_start[length] = '\0' ; // null terminate temporarily

		// printf("Type %d Start %d End %d Size %d Length %d\n",tokens[token_index].type, tokens[token_index].start, tokens[token_index].end, tokens[token_index].size, length ) ;

		switch ( tokens[token_index].type ) {
			case JSMN_PRIMITIVE:
				//printf("Primative ");
				jsp_string = jsp_key ; // expect a key next
				break ;
			case JSMN_OBJECT:
				//printf("Object ");
				jsp_string = jsp_key ; // expect a key next
				break ;
			case JSMN_ARRAY:
				//printf("Array ");
				jsp_string = jsp_key ; // expect a key next
				break ;
			case JSMN_STRING:
				switch ( jsp_string ) {
					case jsp_key:
						// see if this is a special (interesting) key
						//printf("Key <%s>\n",tok_start);
						if ( strcmp( "IP", tok_start ) == 0 ) {
							//printf("IP match\n");
							jsp_string = jsp_ip ; // IP address next
						} else if ( strcmp( "TCPIntfPort", tok_start ) == 0 ) {
							//printf("PORT match\n");
							jsp_string = jsp_port ; // telnet port next
						} else if ( strcmp( "Product", tok_start ) == 0 ) {
							//printf("Product match\n");
							jsp_string = jsp_version ; // telnet port next
						} else {
							//printf("No match\n");
							jsp_string = jsp_value ; // value follows key (but not an interesting one)
						}
						break ;
					case jsp_value:
						//printf("Value <%s>\n",tok_start);
						jsp_string = jsp_key ; // new key follow value
						break ;
					case jsp_ip:
						//printf("IP <%s>\n",tok_start);
						jsp_string = jsp_key ; // new key follow value
						if ( ENET_PARAM_SIZE > length ) {
							strcpy( found_ip, tok_start ) ;
						}
						break ;
					case jsp_port:
						//printf("Port <%s>\n",tok_start);
						jsp_string = jsp_key ; // new key follow value
						if ( ENET_PARAM_SIZE > length ) {
							strcpy( found_port, tok_start ) ;
						}
						break ;
					case jsp_version:
						//printf("Product <%s>\n",tok_start);
						jsp_string = jsp_key ; // new key follow value
						if ( strstr( tok_start, "v2" ) != NULL ) {
							found_version[0] = 2 ; 
						} else {
							found_version[0] = 1 ;
						}
						break ;
				}
				break ;
		}
		tok_start[length] = temp_null ; // restore char
	}
}

static GOOD_OR_BAD Get_ENET_response( int multiple, struct enet_list * elist, struct addrinfo *now )
{
	struct timeval tv = { 2, 0 };
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	char response_buffer[RESPONSE_BUFFER_LENGTH] ;
	
	int at_least_one = 0 ;

	struct sockaddr_in from ;
	socklen_t fromlen = sizeof(struct sockaddr_in) ;

	file_descriptor = socket(now->ai_family, now->ai_socktype, now->ai_protocol) ;
	if ( FILE_DESCRIPTOR_NOT_VALID(file_descriptor) ) {
		ERROR_DEBUG("Cannot get socket file descriptor for broadcast.");
		return 1;
	}

	if ( multiple ) {
		RETURN_BAD_IF_BAD( set_ENET_broadcast( file_descriptor ) ) ;
	}
	
	RETURN_BAD_IF_BAD( send_ENET_discover( file_descriptor, now ) ) ;

	/* now read */
	do {
		ssize_t response = udp_read(file_descriptor, response_buffer, RESPONSE_BUFFER_LENGTH-1, &tv, &from, &fromlen) ;
		char enet_ip[ENET_PARAM_SIZE] ;
		char enet_port[ENET_PARAM_SIZE] ;
		int enet_version = 0 ;
		
		if ( response < 0 ) {
			if ( at_least_one == 0 ) {
				LEVEL_CONNECT("ENET response timeout");
			}
			break ;
		}
		at_least_one = 1 ;
		response_buffer[response] = '\0' ; // null terminated string
		enet_ip[0] = '\0' ;
		enet_port[0] = '\0' ;
		parse_ENET_response( response_buffer, &enet_version, enet_ip, enet_port ) ;
		enet_list_add( enet_ip, enet_port, enet_version, elist ) ;
		//printf( "------- %d bytes from ENET\n%*s\n", (int)response, (int)response, response_buffer ) ;
		//printf(" IP=%s PORT=%s Version=%d\n", enet_ip, enet_port, enet_version ) ;
		if ( multiple == 0 ) {
			return gbGOOD ;
		}
	} while (1) ;

	return gbBAD ;
}

void Find_ENET_all( struct enet_list * elist )
{
	struct addrinfo *ai;
	struct addrinfo hint;
	struct addrinfo *now;
	int getaddr_error ;

	Setup_ENET_hint( &hint ) ;

	if ((getaddr_error = getaddrinfo("255.255.255.255", "30303", &hint, &ai))) {
		LEVEL_CONNECT("Couldn't set up ENET broadcast message %s", gai_strerror(getaddr_error));
		return;
	}

	for (now = ai; now; now = now->ai_next) {
		if ( Get_ENET_response( 1, elist, now ) ) {
			continue ;
		}
	}
	freeaddrinfo(ai);
}

void Find_ENET_Specific( char * addr, struct enet_list * elist )
{
	struct addrinfo *ai;
	struct addrinfo hint;
	struct addrinfo *now;
	int getaddr_error ;

	Setup_ENET_hint( &hint ) ;

	if ((getaddr_error = getaddrinfo(addr, "30303", &hint, &ai))) {
		LEVEL_CONNECT("Couldn't set up ENET broadcast message %s", gai_strerror(getaddr_error));
		return;
	}

	for (now = ai; now; now = now->ai_next) {
		if ( Get_ENET_response( 0, elist, now ) ) {
			continue ;
		}
	}
	freeaddrinfo(ai);
}

void enet_list_init( struct enet_list * elist )
{
	elist->members = 0 ;
	elist->head = NULL ;
}

void enet_list_kill( struct enet_list * elist )
{
	while ( elist->head ) {
		struct enet_member * head = elist->head ;
		elist->head = head->next ;
		owfree( head ) ;
		--elist->members ;
	}
}
 
void enet_list_add( char * ip, char * port, int version, struct enet_list * elist )
{
	struct enet_member * new = (struct enet_member *) owmalloc( sizeof( struct enet_member ) + 2 + strlen( ip ) + strlen( port ) ) ;

	if ( new == NULL ) {
		return ;
	}
	
	if ( strcmp( port, "0" ) == 0 ) {
		LEVEL_CALL("ENET at %s has 1-wire telnet access disabled.\n--> Use the Web configuration http://%s '1-Wire Setup'",ip,ip) ;
		return ;
	}
	
	new->version = version ;
	strcpy( new->name, ip ) ;
	strcat( new->name, ":" ) ;
	strcat( new->name, port ) ;
	new->next = elist->head ;
	++elist->members ;
	elist->head = new ;
}
