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

    //printf("FS_size_postparse pn->path=%s\n", pn->path);

    /* Those are stolen from FullFileLength just to avoid ServerSize()
     * beeing called */
    if (( pn2.type == pn_structure ) ||
	( pn2.ft && ((pn2.ft->format==ft_directory ) ||
		     ( pn2.ft->format==ft_subdir ) ||
		     ( pn2.ft->format==ft_bitfield &&  pn2.extension==-2 )))) {
      return FullFileLength(pn) ;
    }

    if ( (pn->type != pn_real )   /* stat, sys or set dir */
	 && (pn2.state & pn_bus) && (get_busmode(pn2.in) == bus_remote) ) {
      //printf("FS_size_postparse call ServerSize pn2.path=%s\n", pn2.path);
      ret = ServerSize(pn2.path, &pn2) ;
    } else {
      ret = FullFileLength( &pn2 ) ;
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

    //printf("FS_size: pid=%ld path=%s\n", pthread_self(), path);

    pn.si = &si ;
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
    
    //printf("FS_size_remote pid=%ld path=%s\n",pthread_self(), pn->path);

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */

    /* Those are stolen from FullFileLength just to avoid ServerSize()
     * beeing called */
    if (( pn2.type == pn_structure ) ||
	( pn2.ft && ((pn2.ft->format==ft_directory ) ||
		     ( pn2.ft->format==ft_subdir ) ||
		     ( pn2.ft->format==ft_bitfield &&  pn2.extension==-2 )))) {
      return FullFileLength(pn) ;
    }

    //printf("FS_size_remote pn2.type=%d pn2.state=%d busmode(pn2.in)=%d\n", pn2.type, pn2.state, get_busmode(pn2.in));

    if ( (pn2.type != pn_real )   /* stat, sys or set dir */
	 && ( (pn2.state & pn_bus) && (get_busmode(pn2.in) == bus_remote) )) {
      //printf("FS_size_remote call ServerSize pn2.path=%s\n", pn2.path);
      ret = ServerSize(pn2.path, &pn2) ;
    } else {
      ret = FullFileLength( &pn2 ) ;
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
	return (void *)eret;
    }
    if(!(pn->state & pn_bus)) {
      threadbad = pn->in==NULL || pn->in->next==NULL || pthread_create( &thread, NULL, Size2, (void *)pn ) ;
    }
#endif /* OW_MT */

    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pncopy, pn , sizeof( struct parsedname ) ) ; /*shallow copy */

    /* is this a remote bus? */
    if ( get_busmode(pncopy.in) == bus_remote ) {
        //printf("FS_size_seek call ServerSize pncopy.path=%s\n", pncopy.path);
        ret = ServerSize( pncopy.path, &pncopy ) ;
    } else { /* local bus */
        ret = FullFileLength( &pncopy ) ;
    }

#ifdef OW_MT
    /* See if next bus was also queried */
    if ( threadbad == 0 ) { /* was a thread created? */
        //printf("call pthread_join %ld\n", thread);
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

