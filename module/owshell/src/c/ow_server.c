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

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include "owshell.h"

static int FromServer( int fd, struct client_msg * cm, char * msg, size_t size ) ;
static void * FromServerAlloc( int fd, struct client_msg * cm ) ;
//static int ToServer( int fd, struct server_msg * sm, char * path, char * data, size_t datasize ) ;
static int ToServer( int fd, struct server_msg * sm, struct serverpackage * sp ) ;
static void Server_setroutines( struct interface_routines * f ) ;
static void Zero_setroutines( struct interface_routines * f ) ;
static void Server_close( struct connection_in * in ) ;
static int ServerNOP( struct connection_in * in ) ;
static uint32_t SetupSemi( void ) ;

static void Server_setroutines( struct interface_routines * f ) {
    f->detect        = Server_detect ;
//    f->reset         =;
//    f->next_both  ;
//    f->overdrive = ;
//    f->testoverdrive = ;
//    f->PowerByte     = ;
//    f->ProgramPulse = ;
//    f->sendback_data = ;
//    f->sendback_bits = ;
//    f->select        = ;
    f->reconnect     = NULL          ;
    f->close         = Server_close  ;
}

static void Zero_setroutines( struct interface_routines * f ) {
    f->detect        = Server_detect ;
//    f->reset         =;
//    f->next_both  ;
//    f->overdrive = ;
//    f->testoverdrive = ;
//    f->PowerByte     = ;
//    f->ProgramPulse = ;
//    f->sendback_data = ;
//    f->sendback_bits = ;
//    f->select        = ;
    f->reconnect     = NULL          ;
    f->close         = Server_close  ;
}

int Server_detect( void ) {
    if ( indevice->name == NULL ) return -1 ;
    if ( ClientAddr( indevice->name, indevice ) ) return -1 ;
    indevice->Adapter = adapter_tcp ;
    indevice->adapter_name = "tcp" ;
    indevice->busmode = bus_server ;
    Server_setroutines( & (indevice->iroutines) ) ;
    return 0 ;
}

static void Server_close( struct connection_in * in ) {
    FreeClientAddr() ;
}

int ServerRead( char * buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    struct serverpackage sp = { pn->path_busless, NULL, 0, pn->tokenstring, pn->tokens, } ;
    int connectfd = ClientConnect() ;
    int ret = 0 ;

    if ( connectfd < 0 ) return -EIO ;
    //printf("ServerRead pn->path=%s, size=%d, offset=%u\n",pn->path,size,offset);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_read ;
    sm.size = size ;
    sm.sg = SetupSemi(pn) ;
    sm.offset = offset ;

    //printf("ServerRead path=%s\n", pn->path_busless);
    LEVEL_CALL("SERVER(%d)READ path=%s\n", pn->in->index, SAFESTRING(pn->path_busless));

    if ( ToServer( connectfd, &sm, &sp ) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, buf, size ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
    }
    close( connectfd ) ;
    return ret ;
}

int ServerPresence( const struct parsedname * pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    struct serverpackage sp = { pn->path_busless, NULL, 0, pn->tokenstring, pn->tokens, } ;
    int connectfd  = ClientConnect() ;
    int ret = 0 ;

    if ( connectfd < 0 ) return -EIO ;
    //printf("ServerPresence pn->path=%s\n",pn->path);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_presence ;

    sm.sg =  SetupSemi(pn) ;

    //printf("ServerPresence path=%s\n", pn->path_busless);
    LEVEL_CALL("SERVER(%d)PRESENCE path=%s\n", pn->in->index, SAFESTRING(pn->path_busless));

    if ( ToServer( connectfd, &sm, &sp) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, NULL, 0 ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
    }
    close( connectfd ) ;
    return ret ;
}

int ServerWrite( const char * buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    struct serverpackage sp = { pn->path_busless, buf, size, pn->tokenstring, pn->tokens, } ;
    int connectfd  = ClientConnect() ;
    int ret = 0 ;

    if ( connectfd < 0 ) return -EIO ;
    //printf("ServerWrite path=%s, buf=%*s, size=%d, offset=%d\n",path,size,buf,size,offset);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_write ;
    sm.size = size ;
    sm.sg =  SetupSemi(pn) ;
    sm.offset = offset ;

    //printf("ServerRead path=%s\n", pn->path_busless);
    LEVEL_CALL("SERVER(%d)WRITE path=%s\n", pn->in->index, SAFESTRING(pn->path_busless));

    if ( ToServer( connectfd, &sm, &sp) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, NULL, 0 ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
        if ( SemiGlobal != cm.sg ) {
            //printf("ServerRead: cm.sg changed!  SemiGlobal=%X cm.sg=%X\n", SemiGlobal, cm.sg);
            CACHELOCK;
            SemiGlobal = cm.sg & (~BUSRET_MASK) ;
            CACHEUNLOCK;
        }
    }
    close( connectfd ) ;
    return ret ;
}

/* Null "ping" message */
/* Note, uses connection_in rather than full parsedname structure */
static int ServerNOP( struct connection_in * in ) {
    struct server_msg sm ;
    struct client_msg cm ;
    struct serverpackage sp = { "", NULL, 0, NULL, 0, } ;
    int connectfd  = ClientConnect() ;
    int ret = 0 ;

    if ( connectfd < 0 ) return -EIO ;
    //printf("ServerWrite path=%s, buf=%*s, size=%d, offset=%d\n",path,size,buf,size,offset);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_nop ;
    sm.size = 0 ;
    sm.sg =  0 ;
    sm.offset = 0 ;

    //printf("ServerRead path=%s\n", pn->path_busless);
    LEVEL_CALL("SERVER(%d)NOP\n", in->index );

    if ( ToServer( connectfd, &sm, &sp) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, NULL, 0 ) < 0 ) {
        ret = -EIO ;
    }
    close( connectfd ) ;
    return ret ;
}

int ServerDir( ASCII * path ) {
    struct server_msg sm ;
    struct client_msg cm ;
    struct serverpackage sp = { path, NULL, 0, NULL, 0, } ;
    int connectfd  = ClientConnect() ;
    
    if ( connectfd < 0 ) return -EIO ;

    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_dir ;

    sm.sg =  SetupSemi() ;

    if ( ToServer( connectfd, &sm, &sp) ) {
        cm.ret = -EIO ;
    } else {
        char * path2 ;
        while((path2 = FromServerAlloc( connectfd, &cm))) {
            path2[cm.payload-1] = '\0' ; /* Ensure trailing null */
            print("%s\n",path2) ;
            free(path2) ;
        }
    }
    close( connectfd ) ;
    return cm.ret ;
}

/* read from server, free return pointer if not Null */
static void * FromServerAlloc( int fd, struct client_msg * cm ) {
    char * msg ;
    int ret;
    struct timeval tv = { Global.timeout_network+1, 0, } ;

    do { /* loop until non delay message (payload>=0) */
        //printf("OW_SERVER loop1\n");
        ret = readn(fd, cm, sizeof(struct client_msg), &tv );
        if ( ret != sizeof(struct client_msg) ) {
            memset(cm, 0, sizeof(struct client_msg)) ;
            cm->ret = -EIO ;
            return NULL ;
        }
        cm->payload = ntohl(cm->payload) ;
        cm->size = ntohl(cm->size) ;
        cm->ret = ntohl(cm->ret) ;
        cm->sg = ntohl(cm->sg) ;
        cm->offset = ntohl(cm->offset) ;
    } while ( cm->payload < 0 ) ;
    //printf("OW_SERVER loop1 done\n");

//printf("FromServerAlloc payload=%d size=%d ret=%d sg=%X offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
    if ( cm->payload == 0 ) return NULL ;
    if ( cm->ret < 0 ) return NULL ;
    if ( cm->payload > 65000 ) {
//printf("FromServerAlloc payload too large\n");
        return NULL ;
    }

    if ( (msg=(char *)malloc((size_t)cm->payload)) ) {
        ret = readn(fd,msg,(size_t)(cm->payload), &tv );
        if ( ret != cm->payload ) {
//printf("FromServer couldn't read payload\n");
            cm->payload = 0 ;
            cm->offset = 0 ;
            cm->ret = -EIO ;
            free(msg);
            msg = NULL ;
        }
//printf("FromServer payload read ok\n");
    }
    return msg ;
}
/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static int FromServer( int fd, struct client_msg * cm, char * msg, size_t size ) {
    size_t rtry ;
    size_t ret;
    struct timeval tv = { Global.timeout_network+1, 0, } ;

    do { // read regular header, or delay (delay when payload<0)
        //printf("OW_SERVER loop2\n");
        ret = readn(fd, cm, sizeof(struct client_msg), &tv );
        if ( ret != sizeof(struct client_msg) ) {
            //printf("OW_SERVER loop2 bad\n");
            cm->size = 0 ;
            cm->ret = -EIO ;
            return -EIO ;
        }

        cm->payload = ntohl(cm->payload) ;
        cm->size = ntohl(cm->size) ;
        cm->ret = ntohl(cm->ret) ;
        cm->sg = ntohl(cm->sg) ;
        cm->offset = ntohl(cm->offset) ;
    } while ( cm->payload < 0 ) ; // flag to show a delay message
    //printf("OW_SERVER loop2 done\n");

//printf("FromServer payload=%d size=%d ret=%d sg=%d offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
    if ( cm->payload==0 ) return 0 ; // No payload, done.

    rtry = cm->payload<(ssize_t)size ? (size_t)cm->payload : size ;
    ret = readn(fd, msg, rtry, &tv ); // read expected payload now.
    if ( ret != rtry ) {
        cm->ret = -EIO ;
        return -EIO ;
    }

    if ( cm->payload > (ssize_t)size ) { // Uh oh. payload bigger than expected. read it in and discard
        size_t d = cm->payload - size ;
        char extra[d] ;
        ret = readn(fd,extra,d,&tv);
        if ( ret != d ) {
            cm->ret = -EIO ;
            return -EIO ;
        }
        return size ;
    }
    return cm->payload ;
}

// should be const char * data but iovec has problems with const arguments
//static int ToServer( int fd, struct server_msg * sm, const char * path, const char * data, int datasize ) {
static int ToServer( int fd, struct server_msg * sm, struct serverpackage * sp ) {
    int payload = 0 ;
    struct iovec io[5] = {
        { sm, sizeof(struct server_msg), } , // send "server message" header structure
    } ;
    if ( (io[1].iov_base=sp->path) ) { // send path (if not null)
        io[1].iov_len = payload = strlen(sp->path) + 1 ;
    } else {
        io[1].iov_len = payload = 0 ;
    }
    if ( (io[2].iov_base=sp->data) ) { // send data (if datasize not zero)
        payload += ( io[2].iov_len=sp->datasize)  ;
    } else {
        io[2].iov_len = 0 ;
    }
    if ( Global.opt != opt_server ) { // if not being called from owserver, that's it
        io[3].iov_base = io[4].iov_base = NULL ;
        io[3].iov_len  = io[4].iov_len  = 0 ;
        sp->tokens = 0 ;
        sm->version = 0 ;
    } else {
        if ( sp->tokens > 0 ) { // owserver: send prior tags
            io[3].iov_base = sp->tokenstring ;
            io[3].iov_len  = sp->tokens * sizeof(union antiloop) ;
        } else {
            io[3].iov_base = NULL ;
            io[3].iov_len  = 0 ;
        }
        ++sp->tokens ;
        sm->version = Servermessage + (sp->tokens) ;
        io[4].iov_base = &(Global.Token) ; // owserver: add our tag
        io[4].iov_len  = sizeof(union antiloop) ;
    }

    //printf("ToServer payload=%d size=%d type=%d tempscale=%X offset=%d\n",payload,sm->size,sm->type,sm->sg,sm->offset);
    //printf("<%.4d|%.4d\n",sm->type,payload);
    //printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(sm->sg)));

    // encode in network order (just the header)
    sm->version = htonl(sm->version)   ;
    sm->payload = htonl(payload)       ;
    sm->size    = htonl(sm->size)      ;
    sm->type    = htonl(sm->type)      ;
    sm->sg      = htonl(sm->sg)        ;
    sm->offset  = htonl(sm->offset)    ;

    return writev( fd, io, 5 ) != (ssize_t)(payload + sizeof(struct server_msg) + sp->tokens * sizeof(union antiloop) ) ;
}

/* flag the sg for "virtual root" -- the remote bus was specifically requested */
static uint32_t SetupSemi( void ) {
    uint32_t sg = SemiGlobal | (1<<BUSRET_BIT) ;
    return sg ;
}
