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

/* --- Prototypes ------------ */
static int Handler( int fd ) ;
static void * FromClientAlloc( int fd, struct server_msg * sm ) ;
static int ToClient( int fd, struct client_msg * cm, const char * data ) ;
static void * Acceptor( int listenfd ) ;
static void ow_exit( int e ) ;

/* ----- Globals ------------- */
#ifdef OW_MT
    pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER ;
    #define ACCEPTLOCK       pthread_mutex_lock(  &accept_mutex) ;
    #define ACCEPTUNLOCK     pthread_mutex_unlock(&accept_mutex) ;
#else /* OW_MT */
    #define ACCEPTLOCK
    #define ACCEPTUNLOCK
#endif /* OW_MT */


/* read from client, free return pointer if not Null */
static void * FromClientAlloc( int fd, struct server_msg * sm ) {
    char * msg ;
//printf("FromClientAlloc\n");
    if ( readn(fd, sm, sizeof(struct server_msg) ) != sizeof(struct server_msg) ) {
        sm->size = -1 ;
        return NULL ;
    }

    sm->payload = ntohl(sm->payload) ;
    sm->size    = ntohl(sm->size)    ;
    sm->type    = ntohl(sm->type)    ;
    sm->sg      = ntohl(sm->sg)      ;
    sm->offset  = ntohl(sm->offset)  ;
//printf("FromClientAlloc payload=%d size=%d type=%d tempscale=%X offset=%d\n",sm->payload,sm->size,sm->type,sm->sg,sm->offset);
printf("<%.4d|%.4d\n",sm->type,sm->payload);
    if ( sm->payload <= 0 ) {
         msg = NULL ;
    } else if ( sm->payload > MAXBUFFERSIZE ) {
        sm->size = -1 ;
        msg = NULL ;
    } else if ( (msg=malloc(sm->payload)) ) {
        if ( readn(fd,msg,sm->payload) != sm->payload ) {
            sm->size = -1 ;
            free(msg);
            msg = NULL ;
        }
    }
    return msg ;
}

static int ToClient( int fd, struct client_msg * cm, const char * data ) {
    int nio = 1 ;
    int ret ;
    struct iovec io[] = {
        { cm, sizeof(struct client_msg), } ,
        { data, cm->payload, } ,
    } ;

    if ( data && cm->payload ) {
        ++nio ;
    }
//printf("ToClient payload=%d size=%d, ret=%d, sg=%X offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
printf(">%.4d|%.4d\n",cm->ret,cm->payload);
    cm->payload = htonl(cm->payload) ;
    cm->size = htonl(cm->size) ;
    cm->offset = htonl(cm->offset) ;
    cm->ret = htonl(cm->ret) ;
    cm->sg  = htonl(cm->sg) ;
    ret = writev( fd, io, nio ) != io[0].iov_len+io[1].iov_len ;
    cm->payload = ntohl(cm->payload) ;
    cm->size = ntohl(cm->size) ;
    cm->offset = ntohl(cm->offset) ;
    cm->ret = ntohl(cm->ret) ;
    cm->sg   = ntohl(cm->sg) ;
    return ret ;
}

static int Handler( int fd ) {
    char * data = NULL ;
    int datasize ;
    int pathlen ;
    char * retbuffer = NULL ;
    int ret = 0 ;
    struct server_msg sm ;
    struct client_msg cm = {0,0,0,0,0,0,} ; /* default values */
    char * path = FromClientAlloc( fd, &sm ) ;
    struct parsedname pn ;
    struct stateinfo si ;
    /* nested function -- callback for directory entries */
    /* return the full path length, including current entry */
    void directory( const struct parsedname * const pn2 ) {
        /* Note, path preloaded into retbuffer */
//printf("Handler: DIR preloop %s\n",retbuffer);
        FS_DirName( &retbuffer[pathlen-1], OW_FULLNAME_MAX, pn2 ) ;
//printf("Handler: DIR loop %s\n",retbuffer);
	cm.size = strlen(retbuffer) ;
        cm.ret = 0 ;
        ToClient(fd,&cm,retbuffer) ;
    }
    /* error reading message */
    if ( sm.size < 0 ) {
        /* Nothing allocated */
        cm.ret = -EIO ;
        return ToClient( fd, &cm, NULL ) ;
    }

    /* No payload -- only ok if msg_nop */
    if ( path==NULL ) {
        /* Nothing allocated */
        cm.ret = (sm.type==msg_nop)?0:-EBADMSG ;
        return ToClient( fd, &cm, NULL ) ;
    }

    /* Now parse path and locate memory */
    if ( memchr( path, 0, sm.payload) == NULL ) { /* Bad string -- no trailing null */
        if ( path ) free(path) ;
        cm.ret = -EBADMSG ;
        return ToClient( fd, &cm, NULL ) ;
    }
    pathlen = strlen( path ) + 1 ; /* include trailing null */

    /* Locate post-path buffer as data */
    datasize = sm.payload - pathlen ;
    if ( datasize ) data = &path[pathlen] ;

    /* Parse the path string */
    pn.si = &si ;
    if ( (ret=FS_ParsedName( path , &pn )) ) {
        if ( path ) free(path) ;
        cm.ret = ret ;
        cm.payload = 0 ;
        return ToClient( fd, &cm, NULL ) ;
    }
    pn.si->sg.int32 = sm.sg ;

    /* First pass -- figure out return size */
    switch( (enum msg_type) sm.type ) {
    case msg_read:
//printf("Handler: READ \n");
        cm.payload = FullFileLength(&pn) ;
        if ( cm.payload > sm.size ) cm.payload = sm.size ;
        cm.offset = sm.offset ;
        break ;
    case msg_write:
//printf("Handler: WRITE \n");
        cm.payload = 0 ;
        cm.offset = 0 ;
        break ;
    case msg_dir:
//printf("Handler: DIR \n");
        cm.payload = pathlen + OW_FULLNAME_MAX + 2 ;
        cm.offset = 0 ;
        break ;
    default:
//printf("Handler: OTHER \n");
        FS_ParsedName_destroy(&pn) ;
        if ( path ) free(path) ;
        cm.payload = 0 ;
        cm.ret = -ENOTSUP ;
        return ToClient( fd, &cm, NULL ) ;
    }

    /* Allocate return buffer */
//printf("Handler: Allocating buffer size=%d\n",cm.payload);
    if ( cm.payload>0 && cm.payload <MAXBUFFERSIZE && (retbuffer = malloc(cm.payload))==NULL ) {
        FS_ParsedName_destroy(&pn) ;
        if ( path ) free(path) ;
        cm.payload = 0 ;
        cm.ret = -ENOBUFS ;
        return ToClient( fd, &cm, NULL ) ;
    }

    /* Second pass -- do actual function */
    switch( (enum msg_type) sm.type ) {
    case msg_read:
        ret = FS_read_postparse(path,retbuffer,cm.payload,cm.offset,&pn) ;
        if ( ret<0 ) {
//printf("Handler: READ done path=%s, err = %d cm.size=%d cm.payload=%d\n",path,ret,cm.size,cm.payload);
            cm.size = cm.payload = 0 ;
        } else {
//printf("Handler: READ done string = %s\n",retbuffer);
            cm.size = ret ;
        }
        break ;
    case msg_write:
        ret = FS_write_postparse(path,data,cm.size,cm.offset,&pn) ;
//printf("Handler: WRITE done\n");
        if ( ret<0 ) {
            cm.size = 0 ;
            cm.sg = sm.sg ;
        } else {
            cm.size = ret ;
            cm.sg = pn.si->sg.int32 ;
        }
       break ;
    case msg_dir:
        cm.sg = sm.sg ;
//printf("Handler: DIR retbuffer=%p\n",retbuffer);
	/* return buffer holds max file length */
        /* copy current path into return buffer */
        memcpy(retbuffer, path, pathlen ) ;
	/* Add a trailing '/' */
	if ( (pathlen <2) || (retbuffer[pathlen-2] !='/') ) {
	    strcpy( &retbuffer[pathlen-1] , "/" ) ;
	    ++pathlen ;
	}
        FS_dir( directory, path, &pn ) ;
//printf("Handler: DIR done\n");
        /* Now null entry to show end of directy listing */
        ret = 0 ;
        cm.payload = cm.size = 0 ;
        break ;
    }
    
    /* Clean up and return result */
    FS_ParsedName_destroy(&pn) ;
    if (path) free(path) ;
    cm.ret = ret ;
    ret = ToClient( fd, &cm, retbuffer ) ;
    if ( retbuffer ) free(retbuffer) ;
    return 0 ;
}

static void * ThreadedAccept( void * pv ) {
    return (void *) Acceptor( ((struct network_work *)pv)->listenfd ) ;
}

static void * Acceptor( int listenfd ) {
    int fd ;
//printf("ACCEPT thread=%ld waiting\n",pthread_self()) ;
    fd = accept( listenfd, NULL, NULL ) ;
//printf("ACCEPT thread=%ld accepted fd=%d\n",pthread_self(),fd) ;
    ACCEPTUNLOCK
//printf("ACCEPT thread=%ld unlocked\n",pthread_self()) ;
    if ( fd>=0 ) Handler( fd ) ;
    return NULL ;
}

int main( int argc , char ** argv ) {
    char c ;
#ifdef OW_MT
    pthread_t thread ;
    pthread_attr_t attr ;
    
    pthread_attr_init(&attr) ;
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
#endif /* OW_MT */

    LibSetup() ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
            "Usage: %s ttyDevice -p tcpPort [options] \n"
            "   or: %s [options] -d ttyDevice -p tcpPort \n"
            "    -p port   -- tcp port for server process (e.g. 3001)\n" ,
            argv[0],argv[0] ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc-1 ) {
        ComSetup(argv[optind]) ;
        ++optind ;
    }

    if ( devfd==-1 && devusb==0 ) {
        fprintf(stderr, "No device port specified (-d or -u)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if ( portname==NULL ) {
        fprintf(stderr, "No TCP port specified (-p)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if ( ServerAddr( portname, &server ) || (ServerListen( &server )<0) ) {
        fprintf(stderr,"Cannot start server.\n");
        exit(1);
    }

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

    for(;;) {
//printf("MAIN prelock\n");
        ACCEPTLOCK
//printf("MAIN postlock\n");
#ifdef OW_MT
        if ( pthread_create( &thread, &attr, ThreadedAccept, &server ) ) ow_exit(1) ;
#else /* OW_MT */
        Acceptor( server.listenfd ) ;
#endif /* OW_MT */
    }
}

static void ow_exit( int e ) {
    LibClose() ;
    closelog() ;
    exit( e ) ;
}

