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

/* These macros doesn't exist for Solaris */
/* Convenience macros for operations on timevals.
   NOTE: `timercmp' does not work for >= or <=.  */
#ifndef timerisset
# define timerisset(tvp)        ((tvp)->tv_sec || (tvp)->tv_usec)
#endif
#ifndef timerclear
# define timerclear(tvp)        ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif
#ifndef timercmp
# define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))
#endif
#ifndef timeradd
# define timeradd(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    if ((result)->tv_usec >= 1000000)                                         \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_usec -= 1000000;                                         \
      }                                                                       \
  } while (0)
#endif
#ifndef timersub
# define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif

#if OW_MT
  pthread_t main_threadid ;
  #define IS_MAINTHREAD (main_threadid == pthread_self())
  #define TOCLIENTLOCK(hd) pthread_mutex_lock( &((hd)->to_client) )
  #define TOCLIENTUNLOCK(hd) pthread_mutex_unlock( &((hd)->to_client) )
#else /* OW_MT */
  #define IS_MAINTHREAD 1
  #define TOCLIENTLOCK(hd)
  #define TOCLIENTUNLOCK(hd)
#endif /* OW_MT */

// this structure holds the data needed for the handler function called in a separate thread by the ping wrapper
struct handlerdata {
    int fd ;
#if OW_MT
    pthread_mutex_t to_client ;
#endif /* OW_MT */
    struct timeval tv ;
} ;


/* --- Prototypes ------------ */
static void Handler( int fd ) ; // Each new interaction from client
void * RealHandler( void * v ) ; // Called from ping wrapper
static void * ReadHandler( struct server_msg *sm, struct client_msg *cm, const struct parsedname *pn ) ;
static void WriteHandler(struct server_msg *sm, struct client_msg *cm, const BYTE *data, const struct parsedname *pn ) ;
static void DirHandler(struct server_msg *sm, struct client_msg *cm, struct handlerdata * hd, const struct parsedname * pn ) ;
static int FromClient( int fd, struct server_msg *sm, struct serverpackage * sp ) ;
static int ToClient( int fd, struct client_msg *cm, char *data ) ;
static void ow_exit( int e ) ;
static void SetupAntiloop( void ) ;


static void ow_exit( int e ) {
    LEVEL_DEBUG("ow_exit %d\n", e);
    if(IS_MAINTHREAD) {
        LibClose() ;
    }
#ifdef __UCLIBC__
    /* Process never die on WRT54G router with uClibc if exit() is used */
    _exit( e ) ;
#else
    exit( e ) ;
#endif
}

static void exit_handler(int signo, siginfo_t *info, void *context) {
    (void) context;
    if(info) {
      LEVEL_DEBUG ("exit_handler: for %d, errno %d, code %d, pid=%ld, self=%lu main=%lu\n", signo,
		   info->si_errno, info->si_code, (long int) info->si_pid,
		   pthread_self(), main_threadid);
    } else {
      LEVEL_DEBUG ("exit_handler: for %d, self=%lu, main=%lu\n", signo,
		   pthread_self(), main_threadid);
    }
#if OW_MT
    if(!shutdown_in_progress) {
      shutdown_in_progress = 1;
      
      if(info != NULL) {
	if(SI_FROMUSER(info)) {
	  LEVEL_DEBUG("exit_handler: kill from user\n");
	}
	if(SI_FROMKERNEL(info)) {
	  LEVEL_DEBUG("exit_handler: kill from kernel\n");
	}
      }
      if(!IS_MAINTHREAD) {
	LEVEL_DEBUG("exit_handler: kill mainthread %lu self=%d signo=%d\n", main_threadid, pthread_self(), signo);
	pthread_kill(main_threadid, signo);
      }
    }
#else
    shutdown_in_progress = 1;
#endif
    return;
}

/* read from client, free return pointer if not Null */
static int FromClient( int fd, struct server_msg * sm, struct serverpackage * sp ) {
    char * msg ;
    ssize_t trueload ;
    struct timeval tv = { Global.timeout_server, 0, } ;
    //printf("FromClient\n");

    /* Clear return structure */
    memset( sp, 0, sizeof(struct serverpackage) ) ;
    
    if ( tcp_read(fd, sm, sizeof(struct server_msg), &tv ) != sizeof(struct server_msg) ) {
        sm->type = msg_error ;
        return -EIO ;
    }

    sm->version = ntohl(sm->version) ;
    sm->payload = ntohl(sm->payload) ;
    sm->size    = ntohl(sm->size)    ;
    sm->type    = ntohl(sm->type)    ;
    sm->sg      = ntohl(sm->sg)      ;
    sm->offset  = ntohl(sm->offset)  ;
    LEVEL_DEBUG("FromClient payload=%d size=%d type=%d tempscale=%X offset=%d\n",sm->payload,sm->size,sm->type,sm->sg,sm->offset);
    //printf("<%.4d|%.4d\n",sm->type,sm->payload);
    trueload = sm->payload ;
    if ( isServermessage(sm->version) ) trueload += sizeof(union antiloop) * Servertokens(sm->version) ;
    if ( trueload == 0 ) return 0 ;
    
    /* valid size? */
    if ( (sm->payload<0) || (trueload>MAXBUFFERSIZE) ) {
        sm->type = msg_error ;
        return -EMSGSIZE ;
    }

    /* Can allocate space? */
    if ( (msg = (char *)malloc(trueload)) == NULL ) { /* create a buffer */
        sm->type = msg_error ;
        return -ENOMEM ;
    }
    
    /* read in data */
    if ( tcp_read( fd, msg, trueload, &tv ) != trueload ) { /* read in the expected data */
        sm->type = msg_error ;
        free(msg);
        return -EIO ;
    }

    /* path has null termination? */
    if ( sm->payload ) {
        int pathlen ;
        if ( memchr( msg, 0, (size_t)sm->payload)==NULL ) {
            sm->type = msg_error ;
            free(msg);
            return -EINVAL ;
        }
        pathlen = strlen( msg ) + 1 ;
        sp->data = (BYTE*) &msg[pathlen] ;
        sp->datasize = sm->payload - pathlen ;
    } else {
        sp->data = NULL ;
        sp->datasize = 0 ;
    }
        
    if ( isServermessage(sm->version) ) { /* make sure no loop */
        size_t i ;
        char * p = &msg[sm->payload] ; // end of normal buffer
        sp->tokenstring = (BYTE *) p ;
        sp->tokens = Servertokens(sm->version) ;
        for ( i=0 ; i<sp->tokens ; ++i, p+=sizeof(union antiloop) ) {
            if ( memcmp( p, &(Global.Token), sizeof(union antiloop))==0 ) {
                free(msg) ;
                sm->type = msg_error ;
                LEVEL_DEBUG("owserver loop suppression\n") ;
                return -ELOOP ;
            }
        }
    }
    sp->path = msg ;
    return 0 ;
}

static int ToClient( int fd, struct client_msg * cm, char * data ) {
    // note data should be (const char *) but iovec complains about const arguments 
    int nio = 1 ;
    int ret ;
    struct iovec io[] = {
        { cm, sizeof(struct client_msg), } ,
        { data, cm->payload, } ,
    } ;

    /* If payload==0, no extra data
       If payload <0, flag to show a delay message, again no data
    */
    if ( data && cm->payload>0 ) {
        ++nio ;
    }
    LEVEL_DEBUG("ToClient payload=%d size=%d, ret=%d, sg=%X offset=%d \n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
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

/*
 * Main routine for actually handling a request
 * deals with a connection
 */
static void Handler( int fd ) {
    struct handlerdata hd = {
        fd,
#if OW_MT
        PTHREAD_MUTEX_INITIALIZER ,
#endif /* OW_MT */
        {0,0},
    } ;

#if OW_MT
    struct client_msg ping_cm ;
    struct timeval now ; // timer calculation
    struct timeval delta = { Global.timeout_network, 500000 } ; // 1.5 seconds ping interval
    struct timeval result ; // timer calculation
    pthread_t thread ; // hanler thread id (not used)
    int loop = 1 ; // ping loop flap

    memset(&ping_cm, 0, sizeof(struct client_msg));
    ping_cm.payload = -1 ; /* flag for delay message */
    gettimeofday( &(hd.tv), NULL ) ;

    //printf("OWSERVER pre-create\n");
    // PTHREAD_CREATE_DETACHED doesn't work for older uclibc... call pthread_detach() instead.

    if ( pthread_create( &thread, NULL, RealHandler, &hd ) )
      {
	LEVEL_DEBUG("OWSERVER:handler() can't create new thread\n");
	return;
      }

    do { // ping loop
#ifdef HAVE_NANOSLEEP
      struct timespec nano = {0, 100000000} ; // .1 seconds (Note second element NANOsec)
      nanosleep( &nano, NULL ) ;
#else
      usleep((unsigned long)100000);
#endif
      TOCLIENTLOCK( &hd ) ;
      if ( ! timerisset( &(hd.tv) ) ) { // flag that the other thread is done
	loop = 0 ;
      } else { // check timing -- ping if expired
	gettimeofday( &now, NULL ) ; // current time
	timersub( &now, &delta, &result ) ; // less delay
	if ( timercmp( &(hd.tv), &result, < ) ) { // test against last message time
	  char * c = NULL ; // dummy argument
	  ToClient( hd.fd, &ping_cm, c ) ; // send the ping
	  //printf("OWSERVER ping\n") ;
	  gettimeofday( &(hd.tv), NULL ) ; // reset timer
	}
      }                
      TOCLIENTUNLOCK( &hd ) ;
    } while ( loop ) ;
#else /* OW_MT */
    RealHandler( &hd ) ;
#endif /* OW_MT */
    //printf("OWSERVER handler done\n" ) ;
}

/*
 * Main routine for actually handling a request
 * deals with a conncection
 */
void * RealHandler( void * v ) {
    struct handlerdata * hd = (struct handlerdata *) v ;
    char * retbuffer = NULL ;
    struct server_msg sm ;
    struct client_msg cm ;
    struct parsedname pn ;
    struct serverpackage sp ;

#if OW_MT
    pthread_detach( pthread_self() );
#endif

#if OW_CYGWIN
    /* Random generator seem to need initialization for each new thread
     * If not, seed will be reset and rand() will return 0 the first call.
     */
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      srand((unsigned int)tv.tv_usec);
    }
#endif

    //printf("OWSERVER message type = %d\n",sm.type ) ;
    memset(&cm, 0, sizeof(struct client_msg));

    cm.ret = FromClient( hd->fd, &sm, &sp ) ;

    switch( (enum msg_classification) sm.type ) { // outer switch
        case msg_read: // good message
        case msg_write: // good message
        case msg_dir: // good message
        case msg_presence: // good message
            if ( sm.payload==0 ) { /* Bad string -- no trailing null */
                cm.ret = -EBADMSG ;
            } else {
                //printf("Handler: path=%s\n",path);
                /* Parse the path string */

                LEVEL_CALL("owserver: parse path=%s\n",sp.path);
                if ( (cm.ret=FS_ParsedName( sp.path, &pn )) ) break ;

                /* Use client persistent settings (temp scale, display mode ...) */
                pn.sg = sm.sg ;
                /* Antilooping tags */
                pn.tokens = sp.tokens ;
                pn.tokenstring = sp.tokenstring ;
                //printf("Handler: sm.sg=%X pn.state=%X\n", sm.sg, pn.state);
                //printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(sm.sg)));

                switch( (enum msg_classification) sm.type ) {
                    case msg_presence:
                        LEVEL_CALL("Presence message on %s bus_nr=%d\n",SAFESTRING(pn.path),pn.bus_nr) ;
                        // Basically, if we were able to ParsedName it's here!
                        cm.size = cm.payload = 0 ;
                        cm.ret = 0 ; // good answer
                        break ;
                    case msg_read:
                        LEVEL_CALL("Read message\n") ;
                        retbuffer = ReadHandler( &sm , &cm, &pn ) ;
                        LEVEL_DEBUG("Read message done retbuffer=%p\n", retbuffer) ;
                        break ;
                    case msg_write: {
                            LEVEL_CALL("Write message\n") ;
                            if ( (sp.datasize<=0) || ((int)sp.datasize<sm.size) ) {
                                cm.ret = -EMSGSIZE ;
                            } else {
                                WriteHandler( &sm, &cm, sp.data, &pn ) ;
                            }
                        }
                        break ;
                    case msg_dir:
                        LEVEL_CALL("Directory message\n") ;
                        DirHandler( &sm, &cm, hd, &pn ) ;
                        break ;
                    default: // never reached
                        break ;
                }
                FS_ParsedName_destroy(&pn) ;
                LEVEL_DEBUG("RealHandler: FS_ParsedName_destroy done\n");
            }
            break ;
        case msg_nop: // "bad" message
            LEVEL_CALL("NOP message\n") ;
            cm.ret = 0 ;
            break ;
        case msg_size: // no longer used
        case msg_error:
        default: // "bad" message
            LEVEL_CALL("No message\n") ;
            break ;
    }
    LEVEL_DEBUG("RealHandler: cm.ret=%d\n", cm.ret);

    TOCLIENTLOCK(hd) ;
    if ( cm.ret != -EIO ) ToClient( hd->fd, &cm, retbuffer ) ;
    timerclear(&(hd->tv)) ;
    TOCLIENTUNLOCK(hd) ;
    if (sp.path) {
      free(sp.path) ;
      sp.path = NULL;
    }
    if ( retbuffer ) free(retbuffer) ;
    LEVEL_DEBUG("RealHandler: done\n");
    return NULL ;
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
    ssize_t ret ;

    //printf("ReadHandler:\n");
    LEVEL_DEBUG("ReadHandler: cm->payload=%d cm->size=%d cm->offset=%d\n", cm->payload, cm->size, cm->offset);
    LEVEL_DEBUG("ReadHandler: sm->payload=%d sm->size=%d sm->offset=%d\n", sm->payload, sm->size, sm->offset);
    cm->size = 0 ;
    cm->payload = 0 ;
    cm->offset = 0 ;

    if ( ( sm->size <= 0 ) || ( sm->size > MAXBUFFERSIZE ) ) {
        cm->ret = -EMSGSIZE ;
#ifdef VALGRIND
    } else if ( (retbuffer = (char *)calloc(1, (size_t)sm->size))==NULL ) { // allocate return buffer
#else
    } else if ( (retbuffer = (char *)malloc((size_t)sm->size))==NULL ) { // allocate return buffer
#endif
        cm->ret = -ENOBUFS ;
    } else if ( (ret = FS_read_postparse(retbuffer,(size_t)sm->size,(off_t)sm->offset,pn)) <= 0 ) {
        free(retbuffer) ;
        retbuffer = NULL ;
        cm->ret = ret ;
    } else {
        cm->payload = sm->size ;
        cm->offset = sm->offset ;
        cm->size = ret ;
        cm->ret = ret ;
    }
    LEVEL_DEBUG("ReadHandler: return size=%d [%*s]\n",sm->size, sm->size, retbuffer);
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
static void WriteHandler(struct server_msg *sm, struct client_msg *cm, const BYTE *data, const struct parsedname *pn ) {
    int ret = FS_write_postparse((const ASCII *)data,(size_t)sm->size,(off_t)sm->offset,pn) ;
    //printf("Handler: WRITE done\n");
    if ( ret<0 ) {
        cm->size = 0 ;
        cm->sg = sm->sg ;
    } else {
        cm->size = ret ;
        cm->sg = pn->sg ;
    }
}

#ifdef NO_NESTED_FUNCTIONS

#if OW_MT
pthread_mutex_t Dirhandlermutex = PTHREAD_MUTEX_INITIALIZER ;
#endif /* OW_MT */

struct server_msg  *DirHandlersm = NULL;
struct client_msg  *DirHandlercm = NULL;
struct handlerdata *DirHandlerhd = NULL;
const struct parsedname  *DirHandlerpn = NULL;

void DirHandlerdirectory( const struct parsedname * const pn2 ) {
    char *retbuffer ;
    size_t _pathlen ;
    char *path = ((DirHandlerpn->state & pn_bus) && (is_servermode(DirHandlerpn->in))) ? DirHandlerpn->path_busless : DirHandlerpn->path ;

    _pathlen = strlen(path);
#ifdef VALGRIND
    if( (retbuffer = (char *)calloc(1, _pathlen + 1 + OW_FULLNAME_MAX + 3)) == NULL) {
#else
    if( (retbuffer = (char *)malloc(_pathlen + 1 + OW_FULLNAME_MAX + 2)) == NULL) {
#endif
        return ;
    }
    LEVEL_DEBUG("owserver dir path = %s\n",SAFESTRING(pn2->path)) ;
    if ( pn2->dev==NULL ) {
        if ( NotRealDir(pn2) ) {
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

    DirHandlercm->size = strlen(retbuffer) ;
    //printf("DirHandler: loop size=%d [%s]\n",cm->size, retbuffer);
    DirHandlercm->ret = 0 ;
    TOCLIENTLOCK(DirHandlerhd) ;
    ToClient(DirHandlerhd->fd, DirHandlercm, retbuffer) ; // send this directory element
    gettimeofday( &(DirHandlerhd->tv), NULL ) ; // reset timer
    TOCLIENTUNLOCK(DirHandlerhd) ;
    free(retbuffer);
}

/* Dir, called from Handler with the following caveats: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Dir, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
static void DirHandler(struct server_msg *sm , struct client_msg *cm, struct handlerdata * hd, const struct parsedname * pn ) {
#if OW_MT
    pthread_mutex_lock(&Dirhandlermutex) ;
#endif /* OW_MT */

    uint32_t flags = 0 ;
    /* embedded function -- callback for directory entries */
    /* return the full path length, including current entry */

    //printf("DirHandler: pn->path=%s\n", pn->path);
    
    DirHandlersm = sm;
    DirHandlercm = cm;
    DirHandlerhd = hd;
    DirHandlerpn = pn;

    // Settings for all directory elements
    cm->payload = strlen(pn->path) + 1 + OW_FULLNAME_MAX + 2 ;
    cm->sg = sm->sg ;

    // Now generate the directory (using the embedded callback function above for each element
    LEVEL_DEBUG("OWSERVER SpecifiedBus=%d pn->bus_nr=%d\n",SpecifiedBus(pn),pn->bus_nr);
    LEVEL_DEBUG("owserver dir pre = %s\n",SAFESTRING(pn->path)) ;
    cm->ret = FS_dir_remote( DirHandlerdirectory, pn, &flags ) ;
    LEVEL_DEBUG("owserver dir post = %s\n",SAFESTRING(pn->path)) ;

    // Finished -- send some flags and set up for a null element to tell client we're done
    cm->offset = flags ; /* send the flags in the offset slot */
    /* Now null entry to show end of directory listing */
    cm->payload = cm->size = 0 ;
    //printf("DirHandler: DIR done ret=%d flags=%d\n", cm->ret, flags);

#if OW_MT
    pthread_mutex_unlock(&Dirhandlermutex) ;
#endif /* OW_MT */
}

#else /* NO_NESTED_FUNCTIONS */

/* Dir, called from Handler with the following caveats: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Dir, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
static void DirHandler(struct server_msg *sm , struct client_msg *cm, struct handlerdata * hd, const struct parsedname * pn ) {
    uint32_t flags = 0 ;
    /* embedded function -- callback for directory entries */
    /* return the full path length, including current entry */
    void directory( const struct parsedname * const pn2 ) {
        char *retbuffer ;
        size_t _pathlen ;
        char *path = ((pn->state & pn_bus) && (is_servermode(pn->in))) ? pn->path_busless : pn->path ;

        _pathlen = strlen(path);
#ifdef VALGRIND
        if( (retbuffer = (char *)calloc(1, _pathlen + 1 + OW_FULLNAME_MAX + 3)) == NULL) {
#else
        if( (retbuffer = (char *)malloc(_pathlen + 1 + OW_FULLNAME_MAX + 2)) == NULL) {
#endif
            return ;
        }
        LEVEL_DEBUG("owserver dir path = %s\n",SAFESTRING(pn2->path)) ;
        if ( pn2->dev==NULL ) {
            if ( NotRealDir(pn2) ) {
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
        TOCLIENTLOCK(hd) ;
            ToClient(hd->fd, cm, retbuffer) ; // send this directory element
            gettimeofday( &(hd->tv), NULL ) ; // reset timer
        TOCLIENTUNLOCK(hd) ;
        free(retbuffer);
    }

    //printf("DirHandler: pn->path=%s\n", pn->path);
    
    // Settings for all directory elements
    cm->payload = strlen(pn->path) + 1 + OW_FULLNAME_MAX + 2 ;
    cm->sg = sm->sg ;

    // Now generate the directory (using the embedded callback function above for each element
    LEVEL_DEBUG("OWSERVER SpecifiedBus=%d pn->bus_nr=%d\n",SpecifiedBus(pn),pn->bus_nr);
    LEVEL_DEBUG("owserver dir pre = %s\n",SAFESTRING(pn->path)) ;
    cm->ret = FS_dir_remote( directory, pn, &flags ) ;
    LEVEL_DEBUG("owserver dir post = %s\n",SAFESTRING(pn->path)) ;

    // Finished -- send some flags and set up for a null element to tell client we're done
    cm->offset = flags ; /* send the flags in the offset slot */
    /* Now null entry to show end of directory listing */
    cm->payload = cm->size = 0 ;
    //printf("DirHandler: DIR done ret=%d flags=%d\n", cm->ret, flags);
}

#endif /* NO_NESTED_FUNCTIONS */


int main(int argc , char ** argv) {
    int c ;

    /* Set up owlib */
    LibSetup(opt_server) ;
    
    /* grab our executable name */
    if (argc > 0) Global.progname = strdup(argv[0]);

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        default:
            break;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    while ( optind < argc ) {
        OW_ArgGeneric(argv[optind]) ;
        ++optind ;
    }

    if ( outdevices==0 ) {
        if ( Global.announce_off ) {
            LEVEL_DEFAULT("No TCP port specified (-p)\n%s -h for help\n",argv[0]) ;
            ow_exit(1);
        }
        OW_ArgServer("0") ; // make an ephemeral assignment
    }

    set_signal_handlers(exit_handler);

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;
#if OW_MT
    main_threadid = pthread_self() ;
    LEVEL_DEBUG("main_threadid = %lu\n", (unsigned long int)main_threadid);
#endif

    /* Set up "Antiloop" -- a unique token */
    SetupAntiloop() ;

    ServerProcess( Handler, ow_exit ) ;
    return 0 ;
}

static void SetupAntiloop( void ) {
    struct tms t ;
    Global.Token.simple.pid = getpid() ;
    Global.Token.simple.clock = times(&t) ;
}
