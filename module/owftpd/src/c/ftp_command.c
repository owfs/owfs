/*
 * $Id$
 */

#include "owfs_config.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "ftp_command.h"
#include "af_portability.h"
#include "daemon_assert.h"

/* argument types */
#define ARG_NONE              0
#define ARG_STRING            1
#define ARG_OPTIONAL_STRING   2
#define ARG_HOST_PORT         3
#define ARG_TYPE              4
#define ARG_STRUCTURE         5
#define ARG_MODE              6
#define ARG_OFFSET            7
#define ARG_HOST_PORT_LONG    8
#define ARG_HOST_PORT_EXT     9
#define ARG_OPTIONAL_NUMBER  10

#define NUM_COMMAND (sizeof(command_def) / sizeof(struct command_struct))

struct command_struct {
    char *name;
    int arg_type;
    unsigned int len;
} ;

/* prototypes */
static const char *copy_string(char *dst, const char *src);
static const char *parse_host_port(struct sockaddr_in *addr, const char *s);
static const char *parse_number(int *num, const char *s, int max_num);
static const char *parse_offset(off_t *ofs, const char *s);
static const char *parse_host_port_long(sockaddr_storage_t *sa, const char *s);
static const char *parse_host_port_ext(sockaddr_storage_t *sa, const char *s); 
static int commandsort( const void * a , const void * b ) ;
static int commandcmp( const void * a , const void * b ) ;

static int commandsort( const void * a , const void * b ) {
    return strcmp( ((const struct command_struct *)a)->name , ((const struct command_struct *)b)->name ) ;
}

static int commandcmp( const void * a , const void * b ) {
    return strncmp( (const char *)a , ((const struct command_struct *)b)->name, ((const struct command_struct *)b)->len ) ;
}

int ftp_command_parse(const char *input, struct ftp_command_t *cmd)
{
    struct ftp_command_t tmp;
    int c;
    const char *optional_number;
    int no_args ; /* Is there a space after the command for an argument? */
    static int first_time = 1 ;
/* our FTP commands */
    struct command_struct * command_found ;
    static struct command_struct command_def[] = {
	{ "USER", ARG_STRING          ,4,},
	{ "PASS", ARG_STRING          ,4,},
	{ "CWD",  ARG_STRING          ,3,},
	{ "CDUP", ARG_NONE            ,4,},
	{ "QUIT", ARG_NONE            ,4,},
	{ "PORT", ARG_HOST_PORT       ,4,},
	{ "LPRT", ARG_HOST_PORT_LONG  ,4,},
	{ "EPRT", ARG_HOST_PORT_EXT   ,4,},
	{ "PASV", ARG_NONE            ,4,},
	{ "LPSV", ARG_NONE            ,4,},
	{ "EPSV", ARG_OPTIONAL_NUMBER ,4,},
	{ "TYPE", ARG_TYPE            ,4,},
	{ "STRU", ARG_STRUCTURE       ,4,},
	{ "MODE", ARG_MODE            ,4,},
	{ "RETR", ARG_STRING          ,4,},
	{ "STOR", ARG_STRING          ,4,},
	{ "PWD",  ARG_NONE            ,3,},
	{ "LIST", ARG_OPTIONAL_STRING ,4,},
	{ "NLST", ARG_OPTIONAL_STRING ,4,},
	{ "SYST", ARG_NONE            ,4,},
	{ "HELP", ARG_OPTIONAL_STRING ,4,},
	{ "NOOP", ARG_NONE            ,4,},
	{ "REST", ARG_OFFSET          ,4,},
	{ "SIZE", ARG_STRING          ,4,},
	{ "MDTM", ARG_STRING          ,4,}
    };
    static unsigned int num_commands = (sizeof(command_def) / sizeof(struct command_struct)) ;

    if ( first_time ) {
	first_time = 0 ;
	qsort(command_def,num_commands,sizeof(struct command_struct),commandsort);
    }

    daemon_assert(input != NULL);
    daemon_assert(cmd != NULL);

    /* see if our input starts with a valid command */
    if ( (command_found=bsearch(input,command_def,num_commands,sizeof(struct command_struct), commandcmp)) == NULL )
	return 0 ;

    /* copy our command */
    strcpy(tmp.command, command_found->name);

    /* advance input past the command */
    input += command_found->len;

    tmp.num_arg = 0;
    if ( *input == ' ' ) {
        no_args = 0 ;
        ++input ;
    } else {
        no_args = 1 ;
    }
    
    /* now act based on the command */
    switch (command_found->arg_type) {

    case ARG_NONE:
        if (!no_args) return 0 ;
        break;

    case ARG_STRING:
        if (no_args) return 0 ;
        input = copy_string(tmp.arg[0].string, input);
        if (input != NULL) tmp.num_arg++;
        break;

    case ARG_OPTIONAL_STRING:
        if (!no_args) {
            input = copy_string(tmp.arg[0].string, input);
	    if (input != NULL) tmp.num_arg++;
        }
        break;

    case ARG_HOST_PORT:
        if (no_args) return 0 ;
            /* parse the host & port information (if any) */
        input = parse_host_port(&tmp.arg[0].host_port, input);
        if (input == NULL) return 0 ;
        tmp.num_arg++;
        break;

    case ARG_HOST_PORT_LONG:
        if (no_args) return 0 ;
        /* parse the host & port information (if any) */
        input = parse_host_port_long(&tmp.arg[0].host_port, input);
        if (input == NULL) return 0 ;
        tmp.num_arg++;
        break;    

    case ARG_HOST_PORT_EXT:
        if (no_args) return 0 ;
        /* parse the host & port information (if any) */
        input = parse_host_port_ext(&tmp.arg[0].host_port, input);
        if (input == NULL) return 0 ;
        tmp.num_arg++;
        break;
 
        /* the optional number may also be "ALL" */
    case ARG_OPTIONAL_NUMBER:
        if (!no_args) {
            optional_number = parse_number(&tmp.arg[0].num, input, 255);
            if (optional_number != NULL) {
                input = optional_number;
            } else if (strncasecmp(input,"ALL",3)==0) {
                tmp.arg[0].num = EPSV_ALL;
                input += 3;
            } else {
                return 0;
            }
	    if (input != NULL) tmp.num_arg++;
        }
        break;     

    case ARG_TYPE:
        if (no_args) return 0 ;
        /* Get a char (AEIL), put in args, and process further */
        c = toupper(*input);
        ++input ;
        tmp.arg[0].string[0] = c;
        tmp.arg[0].string[1] = '\0';
	tmp.num_arg = 1;
        
        switch(c) {
        case 'A':
        case 'E':
            if (*input == ' ') {
                input++;
                c = toupper(*input);
                if ((c != 'N') && (c != 'T') && (c != 'C')) return 0 ;
                tmp.arg[1].string[0] = c;
                tmp.arg[1].string[1] = '\0';
                input++;
                tmp.num_arg = 2;
            }
            break ;
        case 'I':
            break ;
        case 'L':
            input = parse_number(&tmp.arg[1].num, input, 255);
            if (input == NULL) return 0 ;
            tmp.num_arg = 2;
            break ;
        default:
            return 0;
        }
        break;

    case ARG_STRUCTURE:
        if (no_args) return 0 ;
        c = toupper(*input);
        if ((c != 'F') && (c != 'R') && (c != 'P')) return 0 ;
        input++;
        tmp.arg[0].string[0] = c;
        tmp.arg[0].string[1] = '\0';
        tmp.num_arg++;
        break;

    case ARG_MODE:
        if (no_args) return 0 ;
        c = toupper(*input);
        if ((c != 'S') && (c != 'B') && (c != 'C')) return 0 ;
        input++;
        tmp.arg[0].string[0] = c;
        tmp.arg[0].string[1] = '\0';
        tmp.num_arg++;
        break;

    case ARG_OFFSET:
        if (no_args)  return 0;
        input = parse_offset(&tmp.arg[0].offset, input);
        if (input == NULL) return 0 ;
        tmp.num_arg++;
        break;

    default:
        daemon_assert(0);
    } 

    /* check for our ending newline */
    if (*input != '\n') return 0 ;

    /* return our result */
    *cmd = tmp;
    return 1;
}

/* copy a string terminated with a newline */
static const char *copy_string(char *dst, const char *src)
{
    int i;

    daemon_assert(dst != NULL);
    daemon_assert(src != NULL);

    for (i=0; (i<=MAX_STRING_LEN) && (src[i]!='\0') && (src[i]!='\n'); i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';

    return src+i;
}


static const char *parse_host_port(struct sockaddr_in *addr, const char *s)
{
    int i;
    int octets[6];
    char addr_str[16];
    int port;
    struct in_addr in_addr;

    daemon_assert(addr != NULL);
    daemon_assert(s != NULL);

    /* scan in 5 pairs of "#," */
    for (i=0; i<5; i++) {
        s = parse_number(&octets[i], s, 255);
    if (s == NULL) {
        return NULL;
    }
    if (*s != ',') {
        return NULL;
    }
    s++;
    }

    /* scan in ending "#" */
    s = parse_number(&octets[5], s, 255);
    if (s == NULL) {
        return NULL;
    }

    daemon_assert(octets[0] >= 0);
    daemon_assert(octets[0] <= 255);
    daemon_assert(octets[1] >= 0);
    daemon_assert(octets[1] <= 255);
    daemon_assert(octets[2] >= 0);
    daemon_assert(octets[2] <= 255);
    daemon_assert(octets[3] >= 0);
    daemon_assert(octets[3] <= 255);

    /* convert our number to a IP/port */
    sprintf(addr_str, "%d.%d.%d.%d", 
            octets[0], octets[1], octets[2], octets[3]);
    port = (octets[4] << 8) | octets[5];
#ifdef HAVE_INET_ATON
    if (inet_aton(addr_str, &in_addr) == 0) {
        return NULL;
    }
#else
    in_addr.s_addr = inet_addr(addr_str);
    if (in_addr.s_addr == -1) {
        return NULL;
    }
#endif
    addr->sin_family = AF_INET;
    addr->sin_addr = in_addr;
    addr->sin_port = htons(port);

    /* return success */
    return s;
}

/* note: returns success even for unknown address families */
/*       this is okay, as long as subsequent uses VERIFY THE FAMILY first */
static const char *parse_host_port_long(sockaddr_storage_t *sa, const char *s)
{
    unsigned int i;
    int family;
    int tmp;
    unsigned int addr_len;
    unsigned char addr[255];
    int port_len;
    unsigned char port[255];

    /* we are family */
    s = parse_number(&family, s, 255);
    if (s == NULL) {
        return NULL;
    }
    if (*s != ',') {
        return NULL;
    }
    s++;

    /* parse host length */
    s = parse_number(&addr_len, s, 255);
    if (s == NULL) {
        return NULL;
    }
    if (*s != ',') {
        return NULL;
    }
    s++;

    /* parse address */
    for (i=0; i<addr_len; i++) {
    daemon_assert(i < sizeof(addr)/sizeof(addr[0]));
        s = parse_number(&tmp, s, 255);
    addr[i] = tmp;
    if (s == NULL) {
        return NULL;
    }
    if (*s != ',') {
        return NULL;
    }
        s++;
    }

    /* parse port length */
    s = parse_number(&port_len, s, 255);
    if (s == NULL) {
        return NULL;
    }

    /* parse port */
    for (i=0; i<port_len; i++) {
        if (*s != ',') {
        return NULL;
    }
    s++;
    daemon_assert(i < sizeof(port)/sizeof(port[0]));
    s = parse_number(&tmp, s, 255);
        port[i] = tmp;
    }

    /* okay, everything parses, load the address if possible */
    if (family == 4) {
        SAFAM(sa) = AF_INET;
    if (addr_len != sizeof(struct in_addr)) {
        return NULL;
    }
    if (port_len != 2) {
        return NULL;
    }
    memcpy(&SINADDR(sa), addr, addr_len);
    SINPORT(sa) = htons((port[0] << 8) + port[1]);
    }
#ifdef INET6
    else if (family == 6) {
        SAFAM(sa) = AF_INET6;
    if (addr_len != sizeof(struct in6_addr)) {
        return NULL;
    }
    if (port_len != 2) {
        return NULL;
    }
    memcpy(&SIN6ADDR(sa), addr, addr_len);
    SINPORT(sa) = htons((port[0] << 8) + port[1]);
    }
#endif
    else {
        SAFAM(sa) = -1;
    }

    /* return new pointer */
    return s;
}

static const char *parse_host_port_ext(sockaddr_storage_t *sa, const char *s)
{
    int delimeter;
    int family;
    char *p;
    unsigned int len;
    char addr_str[256];
    int port;

    /* get delimeter character */
    if ((*s < 33) || (*s > 126)) {
        return NULL;
    }
    delimeter = *s;
    s++;

    /* get address family */
    if (*s == '1') {
        family = AF_INET;
    } 
#ifdef INET6
    else if (*s == '2') {
        family = AF_INET6;
    }
#endif
    else {
        return NULL;
    }
    s++;
    if (*s != delimeter) { 
        return NULL;
    }
    s++;

    /* get address string */
    p = strchr(s, delimeter);
    if (p == NULL) {
        return NULL;
    }
    len = p - s;
    if (len >= sizeof(addr_str)) {
        return NULL;
    }
    memcpy(addr_str, s, len);
    addr_str[len] = '\0';
    s = p+1;

    /* get port */
    s = parse_number(&port, s, 65535);
    if (s == NULL) {
        return NULL;
    }
    if (*s != delimeter) {
        return NULL;
    }
    s++;

    /* now parse the value passed */
#ifndef INET6
    {
        struct in_addr in_addr;

#ifdef HAVE_INET_ATON
        if (inet_aton(addr_str, &in_addr) == 0) {
            return NULL;
        }
#else
        in_addr.s_addr = inet_addr(addr_str);
        if (in_addr.s_addr == -1) {
            return NULL;
        }
#endif /* HAVE_INET_ATON */

        SIN4ADDR(sa) = in_addr;
    }
#else
    {
        struct addrinfo hints;
    struct *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = family;

    if (getaddrinfo(addr_str, NULL, &hints, &res) != 0) {
        return NULL;
    }

    memcpy(sa, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);
    }
#endif /* INET6 */

    SAFAM(sa) = family;
    SINPORT(sa) = htons(port);

    /* return new pointer */
    return s;
}

/* scan the string for a number from 0 to max_num */
/* returns the next non-numberic character */
/* returns NULL if not at least one digit */
static const char *parse_number(int *num, const char *s, int max_num)
{
    int tmp;
    int cur_digit;
    
    daemon_assert(s != NULL);
    daemon_assert(num != NULL);
    
    /* handle first character */
    if (!isdigit(*s)) {
        return NULL;
    }
    tmp = (*s - '0');
    s++;

    /* handle subsequent characters */
    while (isdigit(*s)) {
        cur_digit = (*s - '0');

        /* check for overflow */
        if ((max_num - cur_digit) < (tmp * 10)) {
        return NULL;
    }

        tmp *= 10;
    tmp += cur_digit;
        s++;
    }

    daemon_assert(tmp >= 0);
    daemon_assert(tmp <= max_num);

    /* return the result */
    *num = tmp;
    return s;
}

static const char *parse_offset(off_t *ofs, const char *s)
{
    off_t tmp_ofs;
    int cur_digit;
    off_t max_ofs;

    daemon_assert(ofs != NULL);
    daemon_assert(s != NULL);

    /* calculate maximum allowable offset based on size of off_t */
    if (sizeof(off_t) == 8) {
        max_ofs = (off_t)9223372036854775807LL;
    } else if (sizeof(off_t) == 4) {
        max_ofs = (off_t)2147483647LL;
    } else if (sizeof(off_t) == 2) {
        max_ofs = (off_t)32767LL;
    } else {
        max_ofs = 0;
    }
    daemon_assert(max_ofs != 0);
    
    /* handle first character */
    if (!isdigit(*s)) {
        return NULL;
    }
    tmp_ofs = (*s - '0');
    s++;

    /* handle subsequent characters */
    while (isdigit(*s)) {
        cur_digit = (*s - '0');

        /* check for overflow */
    if ((max_ofs - cur_digit) < (tmp_ofs * 10)) {
        return NULL;
    }

        tmp_ofs *= 10;
    tmp_ofs += cur_digit;
        s++;
    }

    /* return */
    *ofs = tmp_ofs;
    return s;
}
