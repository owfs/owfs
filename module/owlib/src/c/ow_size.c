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

#include "owfs_config.h"
#include "ow_devices.h"

static int FS_size_seek( const struct parsedname * const pn ) ;

/* Calls dirfunc() for each element in directory */
/* void * data is arbitrary user data passed along -- e.g. output file descriptor */
/* pn -- input:
    pn->dev == NULL -- root directory, give list of all devices
    pn->dev non-null, -- device directory, give all properties
    branch aware
    cache aware

   pn -- output (with each call to dirfunc)
    ROOT
    pn->dev set
    pn->sn set appropriately
    pn->ft not set

    DEVICE
    pn->dev and pn->sn still set
    pn->ft loops through
*/

int FS_size_postparse( const struct parsedname * const pn ) {
    int ret = 0 ;
    struct parsedname pn2 ;

    if ( pn == NULL || pn->in==NULL ) return -ENODEV ;

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */
    pn2.badcopy = 1 ;

    //printf("FS_size_postparse pn->path=%s\n",pn->path);

    if ( pn->type != pn_real ) {  /* stat, sys or set dir */
      if ( pn2.state & pn_bus && pn2.in->busmode == bus_remote) {
	  ret = ServerSize(pn2.path, &pn2) ;
      } else {
	  ret = FullFileLength( &pn2 ) ;
      }
    }
    else {
      ret = FS_size_seek( &pn2 ) ;
    }
    return ret ;
}


/* path is the path which "pn" parses */
/* FS_size produces the "invariant" portion of the directory, passing on to
   FS_size_seek the variable part */
int FS_size( const char *path ) {
    struct parsedname pn ;
    struct stateinfo si ;
    int r ;

    pn.si = &si ;
    //printf("FS_size: pid=%ld path=%s\n", pthread_self(), path);

    if ( FS_ParsedName( path , &pn ) ) {
        r = -ENOENT;
    } else if ( pn.dev==NULL || pn.ft == NULL ) {
        r = -EISDIR ;
    } else {
      //printf("FS_size: pn->state=pn_bus=%c pn->bus_nr=%d\n", pn.state&pn_bus?'Y':'N', pn.bus_nr);
      //printf("FS_size: pn->path=%s pn->path_busless=%s\n", pn.path, pn.path_busless);
      //printf("FS_size: pid=%ld call postparse pn->type=%d\n", pthread_self(), pn.type);
      r = FS_size_postparse( &pn ) ;
    }
    FS_ParsedName_destroy(&pn) ;
    return r ;
}

/* path is the path which "pn" parses */
/* FS_size_remote is the entry into FS_size_seek from ServerSize */
int FS_size_remote( const struct parsedname * const pn ) {
    int ret = 0 ;
    struct parsedname pn2 ;
    
    if ( pn == NULL || pn->in==NULL ) return -ENODEV ;
    
    //printf("FS_size_remote pid=%ld path=%s\n",pthread_self(), pn->path); UT_delay(100);

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */
    pn2.badcopy = 1 ;

    if ( pn->type != pn_real ) {  /* stat, sys or set dir */
      if ( pn2.state & pn_bus && pn2.in->busmode == bus_remote) {
	  ret = ServerSize(pn2.path, &pn2) ;
      } else {
	  ret = FullFileLength( &pn2 ) ;
      }
    } else {
      if ( pn2.state & pn_bus ) {
	if(pn2.in->busmode == bus_remote) {
	  ret = ServerSize(pn2.path, &pn2) ;
	} else {
	  ret = FullFileLength( &pn2 ) ;
	}
      } else {
	ret = FS_size_seek( &pn2 ) ;
      }
    }
    //printf("FS_size_remote ret=%d\n", ret);
    return ret ;
}

/* path is the path which "pn" parses */
/* FS_size_seek produces the data that can vary: device lists, etc. */
static int FS_size_seek( const struct parsedname * const pn ) {
    int ret = 0 ;
#ifdef OW_MT
    pthread_t thread ;
    struct parsedname pncopy ;
    int threadbad = 1;
    void * v ;
    int rt ;

    /* Embedded function */
    void * Size2( void * vp ) {
        struct parsedname *pn2 = (struct parsedname *)vp ;
        struct parsedname pnnext ;
        struct stateinfo si ;
        int eret;
        memcpy( &pnnext, pn2 , sizeof(struct parsedname) ) ;
        /* we need a different state (search state) for a different bus -- subtle error */
        pnnext.si = &si ;
        pnnext.in = pn2->in->next ;
        eret = FS_size_seek( &pnnext ) ;
        pthread_exit((void *)eret);
    }
    if(!(pn->state & pn_bus)) {
      threadbad = pn->in==NULL || pn->in->next==NULL || pthread_create( &thread, NULL, Size2, (void *)pn ) ;
    } else {
      //printf("size_seek: Only dir bus %d\n", pn->bus_nr);
    }
#endif /* OW_MT */

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pncopy, pn , sizeof( struct parsedname ) ) ; /*shallow copy */
    pncopy.badcopy = 1 ;

    /* is this a remote bus? */
    if ( pncopy.in->busmode == bus_remote ) {
        ret = ServerSize( pncopy.path, &pncopy ) ;
    } else { /* local bus */
      ret = FullFileLength( &pncopy ) ;
    }

#ifdef OW_MT
    /* See if next bus was also queried */
    if ( threadbad == 0 ) { /* was a thread created? */
        //printf("call pthread_join %ld\n", thread); UT_delay(1000);
        if ( pthread_join( thread, &v ) ) {
//printf("pthread_join returned error\n");
            return ret ; /* wait for it (or return only this result) */
        }
//printf("pthread_join returned ok\n");
        rt = (int) v ;
        if ( rt >= 0 ) return rt ; /* is it an error return? Then return this one */
    }
#endif /* OW_MT */
    return ret ;
}

