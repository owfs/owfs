/*
$Id$
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2004 Paul H Alfille

 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* owserver -- responds to requests over a network socket, and processes them on the 1-wire bus/
         Basic idea: control the 1-wire bus and answer queries over a network socket
         Clients can be owperl, owfs, owhttpd, etc...
         Clients can be local or remote
                 Eventually will also allow bounce servers.

         syntax:
                 owserver
                 -u (usb)
                 -d /dev/ttyS1 (serial)
                 -p tcp port
                 e.g. 3001 or 10.183.180.101:3001 or /tmp/1wire
*/

#include "owserver.h"
#include <pthread.h>

/* --- Prototypes ------------ */
static void Handler( int fd ) ;
static void PresenceHandler(struct server_msg *sm , struct client_msg *cm, const struct parsedname * pn ) ;
static void SizeHandler(struct server_msg *sm , struct client_msg *cm, const struct parsedname * pn ) ;
static void * ReadHandler( struct server_msg *sm, struct client_msg *cm, const struct parsedname *pn ) ;
static void WriteHandler(struct server_msg *sm, struct client_msg *cm, const unsigned char *data, const struct parsedname *pn ) ;
static void DirHandler(struct server_msg *sm, struct client_msg *cm, int fd, const struct parsedname * pn ) ;
static void * FromClientAlloc( int fd, struct server_msg *sm ) ;
static int ToClient( int fd, struct client_msg *cm, char *data ) ;

#ifdef OW_MT
pthread_t main_threadid ;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else
#define IS_MAINTHREAD 1
#endif

static void ow_exit( int e ) {
    if(IS_MAINTHREAD) {
        LibClose() ;
    }
    exit( e ) ;
}

static void exit_handler(int i) {
    return ow_exit( ((i<0) ? 1 : 0) ) ;
}

/* read from client, free return pointer if not Null */
static void * FromClientAlloc( int fd, struct server_msg * sm ) {
    char * msg ;
//printf("FromClientAlloc\n");
    if ( readn(fd, sm, sizeof(struct server_msg) ) != sizeof(struct server_msg) ) {
        sm->type = msg_error ;
        return NULL ;
    }

    sm->payload = ntohl(sm->payload) ;
    sm->size    = ntohl(sm->size)    ;
    sm->type    = ntohl(sm->type)    ;
    sm->sg      = ntohl(sm->sg)      ;
    sm->offset  = ntohl(sm->offset)  ;
//printf("FromClientAlloc payload=%d size=%d type=%d tempscale=%X offset=%d\n",sm->payload,sm->size,sm->type,sm->sg,sm->offset);
//printf("<%.4d|%.4d\n",sm->type,sm->payload);
    if ( sm->payload == 0 ) {
         msg = NULL ;
    } else if ( (sm->payload<0) || (sm->payload>MAXBUFFERSIZE) ) {
        sm->type = msg_error ;
        msg = NULL ;
    } else if ( (msg = (char *)malloc((size_t)sm->payload)) ) {
        if ( readn(fd,msg, (size_t)sm->payload) != sm->payload ) {
            sm->type = msg_error ;
            free(msg);
            msg = NULL ;
        }
    }
    return msg ;
}

static int ToClient( int fd, struct client_msg * cm, char * data ) {
    // note data should be (const char *) but iovec complains about const arguments 
    int nio = 1 ;
    int ret ;
    struct iovec io[] = {
        { cm, sizeof(struct client_msg), } ,
        { data, cm->payload, } ,
    } ;

    if ( data && cm->payload ) {
        ++nio ;
    }
//printf("ToClient payload=%d size=%d, ret=%d, sg=%X offset=%d payload=%s\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset,data?data:"");
//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
    //printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(cm->sg)));

    cm->payload = htonl(cm->payload) ;
    cm->size = htonl(cm->size) ;
    cm->offset = htonl(cm->offset) ;
    cm->ret = htonl(cm->ret) ;
    cm->sg  = htonl(cm->sg) ;

    ret = writev( fd, io, nio ) != (ssize_t)(io[0].iov_len+io[1].iov_len) ;

    cm->payload = ntohl(cm->payload) ;
    cm->size = ntohl(cm->size) ;
    cm->offset = ntohl(cm->offset) ;
    cm->ret = ntohl(cm->ret) ;
    cm->sg   = ntohl(cm->sg) ;

    return ret ;
}

void Handler( int fd ) {
    char * retbuffer = NULL ;
    struct server_msg sm ;
    struct client_msg cm ;
    struct parsedname pn ;
    struct stateinfo si ;
    char * path = FromClientAlloc( fd, &sm ) ;

    memset(&cm, 0, sizeof(struct client_msg));

    switch( (enum msg_classification) sm.type ) {
    case msg_size:
    case msg_read:
    case msg_write:
    case msg_dir:
    case msg_presence:
        if ( (path==NULL) || (memchr( path, 0, (size_t)sm.payload)==NULL) ) { /* Bad string -- no trailing null */
            cm.ret = -EBADMSG ;
        } else {
            memset(&si, 0, sizeof(struct stateinfo));
            pn.si = &si ;
            //printf("Handler: path=%s\n",path);
            /* Parse the path string */

            if ( (cm.ret=FS_ParsedName( path, &pn )) ) {
	        FS_ParsedName_destroy(&pn) ;
                break ;
            }

            /* Use client persistent settings (temp scale, discplay mode ...) */
            si.sg = sm.sg ;
            //printf("Handler: sm.sg=%X pn.state=%X\n", sm.sg, pn.state);
            //printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(sm.sg)));

            switch( (enum msg_classification) sm.type ) {
            case msg_presence:
                if(pn.dev && pn.ft) {
                    PresenceHandler( &sm, &cm, &pn ) ;
                    //printf("msg_presence: PresenceHandler returned cm.ret=%d\n", cm.ret);
                } else {
                    cm.ret = 0;
                    //printf("msg_presence: cm.ret=%d\n", cm.ret);
                }
                break ;
            case msg_size:
                if(pn.dev && pn.ft)
                    SizeHandler( &sm, &cm, &pn ) ;
                else
                    cm.ret = 0;
                break ;
            case msg_read:
                retbuffer = ReadHandler( &sm , &cm, &pn ) ;
                break ;
            case msg_write: {
                    int pathlen = strlen( path ) + 1 ; /* include trailing null */
                    int datasize = sm.payload - pathlen ;
                    if ( (datasize<=0) || (datasize<sm.size) ) {
                        cm.ret = -EMSGSIZE ;
                    } else {
                        unsigned char * data = &path[pathlen] ;
                        WriteHandler( &sm, &cm, data, &pn ) ;
                    }
                }
                break ;
            case msg_dir:
                DirHandler( &sm, &cm, fd, &pn ) ;
                break ;
            default: // never reached
                break ;
            }
            FS_ParsedName_destroy(&pn) ;
        }
        /* Fall through */
    case msg_nop:
        break ;
    default:
        cm.ret = -EIO ;
        break ;
    }

    if (path) free(path) ;
    ToClient( fd, &cm, retbuffer ) ;
    if ( retbuffer ) free(retbuffer) ;
}

/* Read, called from Handler with the following caveates: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Read, will return: */
/* cm fully constructed */
/* a malloc'ed string, that must be free'd by Handler */
/* The length of string in cm.payload */
/* If cm.payload is 0, then a NULL string is returned */
/* cm.ret is also set to an error <0 or the read length */
static void * ReadHandler(struct server_msg *sm , struct client_msg *cm, const struct parsedname * pn ) {
    char * retbuffer = NULL ;
//printf("ReadHandler:\n");
    if ( ( pn->type != pn_real )   /* stat, sys or set dir */
     && ( (pn->state & pn_bus) && (get_busmode(pn->in) == bus_remote) )) {
        //printf("ReadHandler: call ServerSize pn->path=%s\n", pn->path);
        cm->payload = ServerSize(pn->path, pn) ;
    } else {
        cm->payload = FullFileLength( pn ) ;
    }
    if ( cm->payload > sm->size ) cm->payload = sm->size ;
    cm->offset = sm->offset ;

    if ( cm->payload <= 0 ) {
        cm->payload = 0 ;
        cm->ret = 0 ;
    } else if ( cm->payload > MAXBUFFERSIZE ) {
        cm->payload = 0 ;
        cm->ret = -EMSGSIZE ;
#ifdef VALGRIND
    } else if ( (retbuffer = (char *)calloc(1, (size_t)cm->payload))==NULL ) {
#else
    } else if ( (retbuffer = (char *)malloc((size_t)cm->payload))==NULL ) {
#endif
    /* Allocate return buffer */
//printf("Handler: Allocating buffer size=%d\n",cm.payload);
        cm->payload = 0 ;
        cm->ret = -ENOBUFS ;
    } else if ( (cm->ret = FS_read_postparse(retbuffer,(size_t)cm->payload,(off_t)cm->offset,pn)) <= 0 ) {
        cm->payload = 0 ;
        free(retbuffer) ;
        retbuffer = NULL ;
    } else {
        cm->size = cm->ret ;
//printf("Handler: READ done string = %s\n",retbuffer);
    }
    return retbuffer ;
}

/* Write, called from Handler with the following caveates: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* data is what will be written, of sm->size length */
/* Read, will return: */
/* cm fully constructed */
/* cm.ret is also set to an error <0 or the written length */
static void WriteHandler(struct server_msg *sm, struct client_msg *cm, const unsigned char *data, const struct parsedname *pn ) {
    int ret = FS_write_postparse(data,(size_t)sm->size,(off_t)sm->offset,pn) ;
//printf("Handler: WRITE done\n");
    if ( ret<0 ) {
        cm->size = 0 ;
        cm->sg = sm->sg ;
    } else {
        cm->size = ret ;
        cm->sg = pn->si->sg ;
    }
}

/* Dir, called from Handler with the following caveates: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Dir, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
static void DirHandler(struct server_msg *sm , struct client_msg *cm, int fd, const struct parsedname * pn ) {
    uint32_t flags = 0 ;
    /* embedded function -- callback for directory entries */
    /* return the full path length, including current entry */
    void directory( const struct parsedname * const pn2 ) {
        char *retbuffer ;
        size_t _pathlen ;
        char *path ;

        if ((pn->state & pn_bus) && (get_busmode(pn->in)==bus_remote)) {
            path = pn->path_busless ;
        } else {
            path = pn->path ;
        }
        _pathlen = strlen(path);
#ifdef VALGRIND
        if( (retbuffer = (char *)calloc(1, _pathlen + 1 + OW_FULLNAME_MAX + 3)) == NULL) return;
#else
        if( (retbuffer = (char *)malloc(_pathlen + 1 + OW_FULLNAME_MAX + 2)) == NULL) return;
#endif

        if ( pn2->dev==NULL ) {
            if ( pn2->type != pn_real ) {
                //printf("DirHandler: call FS_dirname_type\n");
                FS_dirname_type(retbuffer,OW_FULLNAME_MAX,pn2);
            } else if ( pn2->state ) {
                FS_dirname_state(retbuffer,OW_FULLNAME_MAX,pn2) ;
                //printf("DirHandler: call FS_dirname_state\n");
            }
        } else {
//printf("DirHandler: call FS_DirName pn2->dev=%p  Nodevice=%p\n", pn2->dev, NoDevice);
            strcpy(retbuffer, path);
            if ( (_pathlen == 0) || (retbuffer[_pathlen-1] !='/') ) {
                retbuffer[_pathlen] = '/' ;
                ++_pathlen ;
            }
            retbuffer[_pathlen] = '\000' ;
            /* make sure path ends with a / */
            FS_DirName( &retbuffer[_pathlen], OW_FULLNAME_MAX, pn2 ) ;
        }

        cm->size = strlen(retbuffer) ;
//printf("DirHandler: loop size=%d [%s]\n",cm->size, retbuffer);
        cm->ret = 0 ;
        ToClient(fd, cm, retbuffer) ;
        free(retbuffer);
    }

    cm->payload = strlen(pn->path) + 1 + OW_FULLNAME_MAX + 2 ;
    cm->sg = sm->sg ;

    //printf("DirHandler: pn->path=%s\n", pn->path);

    cm->ret = FS_dir_remote( directory, pn, &flags ) ;
    cm->offset = flags ; /* send the flags in the offset slot */
    //printf("DirHandler: DIR done ret=%d flags=%d\n", cm->ret, flags);
    /* Now null entry to show end of directy listing */
    cm->payload = cm->size = 0 ;
}

/* Size, called from Handler with the following caveates: */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Size, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
static void SizeHandler(struct server_msg *sm , struct client_msg *cm, const struct parsedname * pn ) {

    cm->payload = 0 ;
    cm->sg = sm->sg ;

    //printf("SizeHandler: pn->path=[%s] bus=%d\n", pn->path, pn->bus_nr);

    cm->ret = FS_size_remote( pn ) ;
    //printf("Handler: SIZE done ret=%d flags=%ul\n", cm->ret, flags);

    cm->payload = cm->size = 0 ;
}

/* Presence, called from Handler with the following caveates: */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Presence, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
static void PresenceHandler(struct server_msg *sm , struct client_msg *cm, const struct parsedname * pn ) {
    int bus_nr = -1;
    cm->payload = 0 ;
    cm->sg = sm->sg ;

    //printf("PresenceHandler: pn->path=[%s] state=%d bus_nr=%d\n", pn->path, pn->state, pn->bus_nr);

    if((pn->type == pn_real) && !(pn->state & pn_bus)) {
      if(Cache_Get_Device(&bus_nr, pn)) {
    //printf("Cache_Get_Device didn't find bus_nr\n");
    bus_nr = CheckPresence(pn);
    if(bus_nr >= 0) {
      //printf("PresenceHandler(%s) found bus_nr %d (add to cache)\n", pn->path, bus_nr);
      Cache_Add_Device(bus_nr, pn);
    } else {
      //printf("PresenceHandler(%s) didn't find device\n", pn->path);
    }
      } else {
    //printf("Cache_Get_Device found bus! %d\n", bus_nr);
      }
      cm->ret = bus_nr ;
    } else {
      cm->ret = pn->bus_nr ;
    }
    cm->payload = cm->size = 0 ;
}


int main( int argc , char ** argv ) {
    int c ;

    /* grab our executable name */
    if (argc > 0) progname = strdup(argv[0]);

    /* Set up owlib */
    LibSetup() ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        default:
            break;
        }
        if ( owopt(c,optarg,opt_server) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    while ( optind < argc ) {
        OW_ArgGeneric(argv[optind]) ;
        ++optind ;
    }

    if ( outdevices==0 ) {
        LEVEL_DEFAULT("No TCP port specified (-p)\n%s -h for help\n",argv[0])
        ow_exit(1);
    }

    set_signal_handlers(exit_handler);

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

#ifdef OW_MT
    main_threadid = pthread_self() ;
#endif

    /* this row will give a very strange output??
     * indevice->busmode is not working for me?
     * main7: indevice=0x9d314d0 indevice->busmode==858692 2 */
    //printf("main7: indevice=%p indevice->busmode==%d %d\n", indevice, indevice->busmode, get_busmode(indevice));

    ServerProcess( Handler, ow_exit ) ;
    ow_exit(0) ;
    return 0 ;
}

