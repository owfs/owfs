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

#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

static int FromServer( int fd, struct client_msg * cm, char * msg, size_t size ) ;
static void * FromServerAlloc( int fd, struct client_msg * cm ) ;
static int ToServer( int fd, struct server_msg * sm, char * path, char * data, size_t datasize ) ;
static int ConnectionError( const struct parsedname * pn ) ;
static void Server_setroutines( struct interface_routines * f ) ;
static void Server_close( struct connection_in * in ) ;

struct timeval tv = { 2, 0, } ;

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

int Server_detect( struct connection_in * in ) {
    if ( in->name == NULL ) return -1 ;
    if ( ClientAddr( in->name, in ) ) return -1 ;
    in->Adapter = adapter_tcp ;
    in->adapter_name = "tcp" ;
    in->busmode = bus_remote ;
    Server_setroutines( & (in->iroutines) ) ;
    return 0 ;
}

static void Server_close( struct connection_in * in ) {
    FreeClientAddr( in ) ;
}

int ServerSize( const char * path, const struct parsedname * pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    char *pathnow ;
    int connectfd = ClientConnect( pn->in ) ;
    int ret = 0 ;
    (void) path;  // not used anymore

    if ( connectfd < 0 ) return ConnectionError(pn) ;
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_size ;
    sm.sg =  SemiGlobal ;

    if( (pn->state & pn_bus) && pn->path_busless ) {
        pathnow = pn->path_busless ;
    } else {
        //printf("use path = %s\n", pn->path);
        pathnow = pn->path;
    }
    //printf("ServerSize pathnow=%s (path=%s)\n",pathnow, path);
    LEVEL_CALL("SERVERSIZE path=%s\n", SAFESTRING(pathnow));

    if ( ret ) {
    } else if ( ToServer( connectfd, &sm, pathnow, NULL, 0) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, NULL, 0 ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
    }
    close( connectfd ) ;
    return ret ;
}

int ServerRead( char * buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    char *pathnow ;
    int connectfd = ClientConnect( pn->in ) ;
    int ret = 0 ;

    if ( connectfd < 0 ) return ConnectionError(pn) ;
    //printf("ServerRead pn->path=%s, size=%d, offset=%u\n",pn->path,size,offset);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_read ;
    sm.size = size ;
    sm.sg =  SemiGlobal ;
    sm.offset = offset ;

    if( (pn->state & pn_bus) && pn->path_busless ) {
        pathnow = pn->path_busless ;
    } else {
        //printf("use path = %s\n", pn->path);
        pathnow = pn->path;
    }
    //printf("ServerRead path=%s\n", pathnow);
    LEVEL_CALL("SERVERREAD path=%s\n", SAFESTRING(pathnow));

    if ( ret ) {
    } else if ( ToServer( connectfd, &sm, pathnow, NULL, 0) ) {
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
    char *pathnow ;
    int connectfd = ClientConnect( pn->in ) ;
    int ret = 0 ;

    if ( connectfd < 0 ) return ConnectionError(pn) ;
    //printf("ServerPresence pn->path=%s\n",pn->path);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_presence ;
    sm.sg =  SemiGlobal ;

    if( (pn->state & pn_bus) && pn->path_busless ) {
        pathnow = pn->path_busless ;
    } else {
        //printf("use path = %s\n", pn->path);
        pathnow = pn->path;
    }
    //printf("ServerPresence path=%s\n", pathnow);
    LEVEL_CALL("SERVERPRESENCE path=%s\n", SAFESTRING(pathnow));

    if ( ret ) {
    } else if ( ToServer( connectfd, &sm, pathnow, NULL, 0) ) {
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
    char *pathnow = NULL ;
    int connectfd = ClientConnect( pn->in ) ;
    int ret = 0 ;

    if ( connectfd < 0 ) return ConnectionError(pn) ;
    //printf("ServerWrite path=%s, buf=%*s, size=%d, offset=%d\n",path,size,buf,size,offset);
    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_write ;
    sm.size = size ;
    sm.sg =  SemiGlobal ;
    sm.offset = offset ;

    if( (pn->state & pn_bus) && pn->path_busless ) {
        //printf("use path_busless = %s\n", pn->path_busless);
        pathnow = pn->path_busless ;
    } else {
        //printf("use path = %s\n", pn->path);
        pathnow = pn->path;
    }
    //printf("ServerRead path=%s\n", pathnow);
    LEVEL_CALL("SERVERWRITE path=%s\n", SAFESTRING(pathnow));

    if ( ret ) {
    } else if ( ToServer( connectfd, &sm, pathnow, buf, size) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, NULL, 0 ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
        if ( SemiGlobal != cm.sg ) {
            //printf("ServerRead: cm.sg changed!  SemiGlobal=%X cm.sg=%X\n", SemiGlobal, cm.sg);
            CACHELOCK;
                SemiGlobal = cm.sg ;
            CACHEUNLOCK;
        }
    }
    close( connectfd ) ;
    return ret ;
}

int ServerDir( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn, uint32_t * flags ) {
    struct server_msg sm ;
    struct client_msg cm ;
    char * path2 ;
    char *pathnow = NULL ;
    int ret = 0 ;
    int connectfd = ClientConnect( pn->in ) ;
    struct parsedname pn2 ;
    int dindex = 0 ;

    if ( connectfd < 0 ) return ConnectionError(pn) ;

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */

    memset(&sm, 0, sizeof(struct server_msg));
    sm.type = msg_dir ;

    sm.sg = SemiGlobal ;
    //if((pn->state & pn_bus) && FS_RemoteBus(pn)) {
    if((pn->state & pn_bus) && (get_busmode(pn->in) == bus_remote)) {
        sm.sg |= (1<<BUSRET_BIT) ; // make sure it returns bus-list
        LEVEL_DEBUG("ServerDir: path_busless=%p [%s]\n", pn->path_busless, SAFESTRING(pn->path_busless))
        // if pn_bus is set, then path_busless is automatically set too in Parse_Bus().
        pathnow = pn->path_busless ;
    } else {
        LEVEL_DEBUG("ServerDir: path=%p [%s]\n", pn->path, SAFESTRING(pn->path))
        pathnow = pn->path;
    }

    LEVEL_CALL("SERVERDIR path=%s\n", SAFESTRING(pathnow));

    if (ret) {
        cm.ret = ret ;
    } else if ( ToServer( connectfd, &sm, pathnow, NULL, 0) ) {
        cm.ret = -EIO ;
    } else {
        int got_entry = 0 ;
        while((path2 = FromServerAlloc( connectfd, &cm))) {
            if(got_entry)
                FS_ParsedName_destroy( &pn2 ) ;  // destroy the last parsed name
            else
                got_entry = 1 ;
            path2[cm.payload-1] = '\0' ; /* Ensure trailing null */

            LEVEL_DEBUG("ServerDir: got=[%s]\n", path2);
            ret = FS_ParsedName_Remote( path2, &pn2 ) ;

            if ( ret ) {
                cm.ret = -EINVAL ;
                //LEVEL_DEBUG("ServerDir: error parsing [%s] ret=%d\n", path2, ret);
                free(path2) ;
                break ;
            } else {
                unsigned char sn[8] ;
                /* we got a device on bus_nr = pn->in->index. Cache it so we
                    find it quicker next time we want to do read values from the
                    the actual device
                */
                if(pn2.dev && (pn2.type == pn_real)) {
                    /* If we get a device then cache the bus_nr */
                    Cache_Add_Device(pn->in->index, &pn2);
                }

                if(pn2.dev && (pn2.type == pn_real)) {
                    /* If we get a device then cache it */
                    //FS_LoadPath(sn, &pn2);
                    memcpy(sn, pn2.sn, 8);
                    pn2.in = pn->in ;  // reuse the current pn->in->index
                    Cache_Add_Dir(sn,dindex,&pn2) ;
                    ++dindex ;
                }

                DIRLOCK;
                    dirfunc(&pn2) ;
                DIRUNLOCK;

                free(path2) ;
            }
        }
        if(got_entry) {
            // we got at least one entry. FS_ParsedName_destroy() mustn't be
            // called before Cache_Del_Dir()
            Cache_Del_Dir(dindex,&pn2) ;  // end with a null entry
            FS_ParsedName_destroy( &pn2 ) ;
        }
        DIRLOCK;
            /* flags are sent back in "offset" of final blank entry */
            flags[0] |= cm.offset ;
        DIRUNLOCK;
    }
    close( connectfd ) ;
    return cm.ret ;
}

/* read from server, free return pointer if not Null */
static void * FromServerAlloc( int fd, struct client_msg * cm ) {
    char * msg ;
    int ret;

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

    rtry = cm->payload<size ? cm->payload : size ;
    ret = readn(fd, msg, rtry, &tv ); // read expected payload now.
    if ( ret != rtry ) {
        cm->ret = -EIO ;
        return -EIO ;
    }

    if ( cm->payload > size ) { // Uh oh. payload bigger than expected. read it in and discard
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
static int ToServer( int fd, struct server_msg * sm, char * path, char * data, size_t datasize ) {
    int nio = 1 ;
    int payload = 0 ;
    struct iovec io[] = {
        { sm, sizeof(struct server_msg), } ,
        { path, 0, } ,
        { data, datasize, } ,
    } ;
    if ( path ) {
        ++ nio ;
        io[1].iov_len = payload = strlen(path) + 1 ;
        if ( data && datasize ) {
            ++nio ;
            payload += datasize ;
        }
    }

//printf("ToServer payload=%d size=%d type=%d tempscale=%X offset=%d\n",payload,sm->size,sm->type,sm->sg,sm->offset);
//printf("<%.4d|%.4d\n",sm->type,payload);
    //printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(sm->sg)));

    sm->payload = htonl(payload)       ;
    sm->size    = htonl(sm->size)      ;
    sm->type    = htonl(sm->type)      ;
    sm->sg      = htonl(sm->sg)        ;
    sm->offset  = htonl(sm->offset)    ;

    return writev( fd, io, nio ) != (payload + sizeof(struct server_msg)) ;
}

static int ConnectionError( const struct parsedname * pn ) {
    (void) pn ;
    LEVEL_CONNECT("Unable to open socket\n") ;
    return -EIO ;
}
