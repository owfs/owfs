/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
$ID: $
*/

#include "owfs_config.h"
#include "ow.h"

#include <sys/stat.h>
#include <string.h>

/* ------- Prototypes ----------- */
static int FS_read_seek(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) ;
static int FS_real_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_r_all(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_r_split(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_gamish(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_structure(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

int FS_read(const char *path, char *buf, const size_t size, const off_t offset) {
    struct parsedname pn ;
    struct stateinfo si ;
    int r ;

    pn.si = &si ;
    //printf("FS_read: pid=%d\n", getpid());

//    if ( indevice==NULL ) return -ENODEV ; /* probably unneeded */
    if ( FS_ParsedName( path , &pn ) ) {
        r = -ENOENT;
    } else if ( pn.dev==NULL || pn.ft == NULL ) {
        r = -EISDIR ;
    } else {
      //printf("FS_read: pid=%d call postparse FS_read_busmode=%d pn->type=%d\n", getpid(), pn.in->busmode, pn.type);
      r = FS_read_postparse(buf, size, offset, &pn ) ;
    }
    FS_ParsedName_destroy(&pn) ;
    return r ;
}

/* After parsing, but before sending to various devices. Will repeat 3 times if needed */
int FS_read_3times(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
  int r = 0 ;
  int i;
  //    if ( pn->in==NULL ) return -ENODEV ;
  /* Normal read. Try three times */
  STATLOCK
    AVERAGE_IN(&read_avg)
    AVERAGE_IN(&all_avg)
    STATUNLOCK
    for(i=0; i<3; i++) {
      STATLOCK
        ++read_tries[i] ; /* statitics */
      STATUNLOCK
        r = FS_read_postparse( buf, size, offset, pn ) ;
      if ( r>=0 ) break;
    }
  STATLOCK
    if ( r>=0 ) {
      ++read_success ; /* statistics */
      read_bytes += r ; /* statistics */
    }
  AVERAGE_OUT(&read_avg)
    AVERAGE_OUT(&all_avg)
    STATUNLOCK
    return r ;
}


/* Note on return values */
/* functions return the actual number of bytes read, */
/* or a negative value if an error */
/* negative values are of the form -EINVAL, etc */
/* the negative of a system errno */

/* Note on size and offset: */
/* Buffer length (and requested data) is size bytes */
/* reading should start after offset bytes in original data */
/* only date, binary, and ascii data support offset in single data points */
/* only binary supports offset in array data */
/* size and offset are vetted against specification data size and calls */
/*   outside of this module will not have buffer overflows */
/* I.e. the rest of owlib can trust size and buffer to be legal */

/* After parsing, choose special read based on path type */
int FS_read_postparse(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    int r = 0;
    //printf("FS_read_postparse: pid=%d busmode=%d pn->type=%d\n", getpid(), pn->in->busmode, pn->type);
  if(pn->in->busmode==bus_remote) {
    //printf("FS_read_postparse: pid=%d busmode=remote\n", getpid());
    switch (pn->type) {
    case pn_structure:
      /* Get structure data from local memory */
      r = FS_structure(buf,size,offset,pn) ;
      break;
    default:
      r = ServerRead(buf,size,offset,pn);
      break;
    }
    return r;
  }
#if 0
 else {
    //printf("FS_read_postparse: pid=%d busmode=local\n", getpid());

    STATLOCK
    AVERAGE_IN(&read_avg)
    AVERAGE_IN(&all_avg)
    ++ read_calls ; /* statistics */
    STATUNLOCK
#endif
    switch (pn->type) {
    case pn_system: /* use local data, generic bus (actually specified by extension) */
        //printf("FS_read_postparse: pid=%d call fs_real_read\n", getpid());
        r = FS_real_read( buf, size, offset, pn ) ;
	break;
    case pn_structure:
        /* Get structure data from local memory */
      //printf("FS_read_postparse: pid=%d call fs_structure\n", getpid());
        r = FS_structure(buf,size,offset,pn) ;
	break;
    case pn_settings:
    case pn_statistics:
      //printf("FS_read_postparse: pid=%d settings/statistics\n", getpid());
        /* Get internal data from first source */
        r = FS_real_read( buf, size, offset, pn ) ;
	break;
    default:
      //printf("FS_read_postparse: pid=%d call fs_read_seek\n", getpid());
        /* real data -- go through device chain */
        r = FS_read_seek(buf,size,offset,pn) ;
    }
#if 0
    STATLOCK
    if ( r>=0 ) {
      ++read_success ; /* statistics */
      read_bytes += r ; /* statistics */
    }
    AVERAGE_OUT(&read_avg)
    AVERAGE_OUT(&all_avg)
    STATUNLOCK
  }
#endif
  //printf("FS_read_postparse: pid=%d return %d\n", getpid(), r);
  return r;
}

/* Loop through input devices (busses) */
static int FS_read_seek(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    int r ;
#ifdef OW_MT
    pthread_t thread ;
    char buf2[size] ;
    /* Embedded function */
    void * Read2( void * vp ) {
        struct parsedname pn2 ;
        struct stateinfo si ;
        (void) vp ;
        memcpy( &pn2, pn , sizeof(struct parsedname) ) ;
        pn2.in = pn->in->next ;
        pn2.si = &si ;
	//printf("READSEEK fork new process pid=%d\n",getpid());
        return (void *) FS_read_seek(buf2,size,offset,&pn2) ;
    }
    int threadbad = pn->in==NULL || pn->in->next==NULL || pthread_create( &thread, NULL, Read2, NULL ) ;
#endif /* OW_MT */
//printf("READSEEK pid=%d path=%s index=%d\n",getpid(), pn->path,pn->in->index);
    if ( pn->in->busmode == bus_remote ) {
//printf("READSEEK0 pid=%d call ServerRead\n", getpid());
        r = ServerRead(buf,size,offset,pn) ;
//printf("READSEED0 pid=%d = %d\n",getpid(), r);
    } else {
        size_t s = size ;
	STATLOCK
	  ++ read_calls ; /* statistics */
	STATUNLOCK
	/* Check the cache (if not pn_uncached) */
        if ( offset!=0 || IsLocalCacheEnabled(pn)==0 ) {
//printf("READSEED1 pid=%d call FS_real_read\n",getpid());
            LockGet(pn) ;
	    r = FS_real_read(buf, size, offset, pn ) ;
            LockRelease(pn) ;
//printf("READSEEK1 pid=%d = %d\n",getpid(), r);
        } else if ( (pn->state & pn_uncached) || Cache_Get( buf, &s, pn ) ) {
    //printf("Read didnt find %s(%d->%d)\n",path,size,s) ;
//printf("READSEED2 pid=%d not found in cache\n",getpid());
            LockGet(pn) ;
	    r = FS_real_read( buf, size, offset, pn ) ;
	    if ( r>= 0 ) Cache_Add( buf, r, pn ) ;
            LockRelease(pn) ;
//printf("READSEED2 pid=%d = %d\n",getpid(), r);
        } else {
//printf("READSEEK3 pid=%d cached found\n",getpid()) ;
            STATLOCK
                ++read_cache ; /* statistics */
                read_cachebytes += s ; /* statistics */
            STATUNLOCK
            r = s ;
//printf("READSEED3 pid=%d = %d\n",getpid(), r);
        }
    }
#ifdef OW_MT
    if ( !threadbad ) { /* was a thread created? */
        void * v ;
        int rt ;
        if ( pthread_join( thread, &v ) ) return r ; /* wait for it (or return only this result) */
        rt = (int) v ;
//printf("READ 1st=%d 2nd=%d\n",r,rt);
        if ( rt >= 0 && rt > r ) { /* is it an error return? Then return this one */
//printf("READ from 2nd adapter\n") ;
            memcpy( buf, buf2, (size_t) rt ) ; /* Use the thread's result */
            return rt ;
        }
//printf("READ from 1st adapter\n") ;
    }
#endif /* OW_MT */
    return r ;
}

/* Real read -- called from read
   Integrates with cache -- read not called if cached value already set
*/
static int FS_real_read(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) {
//printf("RealRead pid=%d path=%s size=%d, offset=%d, extension=%d adapter=%d\n", getpid(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;
    /* Readable? */
    if ( (pn->ft->read.v) == NULL ) return -ENOENT ;
    /* Do we exist? Only test static cases */
    if ( ShouldCheckPresence(pn) && pn->ft->change==ft_static && Check1Presence(pn) ) return -ENODEV ;

    /* Array property? Read separately? Read together and manually separate? */
    if ( pn->ft->ag ) {
        switch(pn->ft->ag->combined) {
        case ag_separate: /* separate reads, artificially combined into a single array */
            if ( pn->extension==-1 ) return FS_r_all(buf,size,offset,pn) ;
            break ; /* fall through to normal read */
        case ag_mixed: /* mixed mode, ALL read handled differently */
            if ( pn->extension==-1 ) return FS_gamish(buf,size,offset,pn) ;
            break ; /* fall through to normal read */
        case ag_aggregate: /* natively an array */
            /* split apart if a single item requested */
            if ( pn->extension>-1 ) return FS_r_split(buf,size,offset,pn) ;
            /* return ALL if required */
            if ( pn->extension==-1 ) return FS_gamish(buf,size,offset,pn) ;
            break ; /* special bitfield case, extension==-2, read as a single entity */
        }
    }
    return FS_parse_read( buf, size, offset, pn ) ;
}

/* Structure file */
static int FS_structure(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) {
    char ft_format_char[] = "  iufabydy" ; /* return type */
    /* dir,dir,int,unsigned,float,ascii,binary,yesno,date,bitfield */
    int len ;
    struct parsedname pn2 ;

    if ( offset ) return -EADDRNOTAVAIL ;

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; /* shallow copy */
    pn2.type = pn_real ; /* "real" type to get return length, rather than "structure" length */

    UCLIBCLOCK
        len = snprintf(
            buf,
            size,
            "%c,%.6d,%.6d,%.2s,%.6d,",
            ft_format_char[pn->ft->format],
            (pn->ft->ag) ? pn->extension : 0 ,
            (pn->ft->ag) ? pn->ft->ag->elements : 1 ,
            (pn->ft->read.v) ?
                ( (pn->ft->write.v) ? "rw" : "ro" ) :
                ( (pn->ft->write.v) ? "wo" : "oo" ) ,
            FullFileLength(&pn2)
            ) ;
    UCLIBCUNLOCK
    return len;
}

/* read without artificial separation of combination */
static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int ret = 0 ;
//printf("ParseRead pid=%d path=%s size=%d, offset=%d, extension=%d adapter=%d\n",getpid(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;

    switch( pn->ft->format ) {
    case ft_integer: {
        int i ;
        if ( offset ) return -EADDRNOTAVAIL ;
        ret = (pn->ft->read.i)(&i,pn) ;
        if (ret < 0) return ret ;
        return FS_output_integer( i , buf , size , pn ) ;
    }
    case ft_bitfield:
    case ft_unsigned: {
        unsigned int u ;
        if ( offset ) return -EADDRNOTAVAIL ;
        ret = (pn->ft->read.u)(&u,pn) ;
        if (ret < 0) return ret ;
        return FS_output_unsigned( u , buf , size , pn ) ;
        }
    case ft_float: {
        FLOAT f ;
        if ( offset ) return -EADDRNOTAVAIL ;
        ret = (pn->ft->read.f)(&f,pn) ;
        if (ret < 0) return ret ;
        return FS_output_float( f , buf , size , pn ) ;
        }
    case ft_date: {
        DATE d ;
        if ( offset ) return -EADDRNOTAVAIL ;
        ret = (pn->ft->read.d)(&d,pn) ;
        if (ret < 0) return ret ;
        return FS_output_date( d , buf , size , pn ) ;
        }
    case ft_yesno: {
        int y ;
        if ( offset ) return -EADDRNOTAVAIL ;
        if (size < 1) return -EMSGSIZE ;
        ret = (pn->ft->read.y)(&y,pn) ;
        if (ret < 0) return ret ;
        buf[0] = y ? '1' : '0' ;
        return 1 ;
        }
    case ft_ascii: {
        size_t s = FileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.a)(buf,s,offset,pn) ;
        }
    case ft_binary: {
        size_t s = FileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.b)(buf,s,offset,pn) ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    }
    return -ENOENT ;
}

/* read an aggregation (returns an array from a single read) */
static int FS_gamish(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t elements = pn->ft->ag->elements ;
    int ret = 0 ;

    switch( pn->ft->format ) {
    case ft_integer:
        if (offset) {
            return -EADDRNOTAVAIL ;
        } else {
            int * i = (int *) calloc( elements, sizeof(int) ) ;
                if ( i==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.i)(i,pn) ;
                if (ret >= 0) ret = FS_output_integer_array( i , buf , size , pn ) ;
            free( i ) ;
            return ret ;
        }
    case ft_unsigned:
        if (offset) {
            return -EADDRNOTAVAIL ;
        } else {
            unsigned int * u = (unsigned int *) calloc( elements, sizeof(unsigned int) ) ;
                if ( u==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.u)(u,pn) ;
                if (ret >= 0) ret = FS_output_unsigned_array( u , buf , size , pn ) ;
            free( u ) ;
            return ret ;
        }
    case ft_float:
        if (offset) {
            return -EADDRNOTAVAIL ;
        } else {
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                if (ret >= 0) ret = FS_output_float_array( f , buf , size , pn ) ;
            free( f ) ;
            return ret ;
        }
    case ft_date:
        if (offset) {
            return -EADDRNOTAVAIL ;
        } else {
            DATE * d = (DATE *) calloc( elements, sizeof(DATE) ) ;
                if ( d==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.d)(d,pn) ;
                if (ret >= 0) ret = FS_output_date_array( d , buf , size , pn ) ;
            free( d ) ;
            return ret ;
        }
    case ft_yesno:
        if (offset) {
            return -EADDRNOTAVAIL ;
        } else {
            int * y = (int *) calloc( elements, sizeof(int) ) ;
                if ( y==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.y)(y,pn) ;
                if (ret >= 0) {
                    size_t i ;
                    for ( i=0 ; i<elements ; ++i ) {
                        buf[i*2] = y[i] ? '1' : '0' ;
                        if ( i<elements-1 ) buf[i*2+1] = ',' ;
                    }
                    ret = elements*2-1 ;
                }
            free( y ) ;
            return ret ; ;
        }
    case ft_bitfield:
        if (offset) {
            return -EADDRNOTAVAIL ;
        } else {
            unsigned int u ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret >= 0) {
                size_t i ;
                for ( i=0 ; i<elements ; ++i ) {
                    buf[i*2] = u&0x01 ? '1' : '0' ;
                    u = u>>1 ;
                    if ( i<elements-1 ) buf[i*2+1] = ',' ;
                }
                ret = elements*2-1 ;
            }
            return ret ;
        }
    case ft_ascii: {
        size_t s = FullFileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.a)(buf,s,offset,pn) ;
        }
    case ft_binary: {
        size_t s = FullFileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.b)(buf,s,offset,pn) ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    }
    return -ENOENT ;
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->ft->ag->combined==ag_separate */
static int FS_r_all(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t left = size ;
    char * p = buf ;
    int r ;
    struct parsedname pn2 ;

    STATLOCK
        ++read_array ; /* statistics */
    STATUNLOCK
    if ( offset ) return -EADDRNOTAVAIL ;

    /* shallow copy */
    memcpy( &pn2 , pn , sizeof(struct parsedname) ) ;
//printf("READALL(%p) %s size=%d\n",p,path,size) ;

    for ( pn2.extension=0 ; pn2.extension<pn2.ft->ag->elements ; ++pn2.extension ) {
        /* Add a separating comma if not the first element */
        if (pn2.extension && pn2.ft->format!=ft_binary) {
            if ( left == 0 ) return -ERANGE ;
            *p = ',' ;
            ++ p ;
            -- left ;
//printf("READALL(%p) comma\n",p) ;
        }
        if ( (r=FS_parse_read(p,left,0,&pn2)) < 0 ) return r ;
        left -= r ;
        p += r ;
//printf("READALL(%p) %d->%d (%d->%d)\n",p,pname.extension,r,size,left) ;
    }
//printf("READALL return %d\n",size-left) ;
    return size - left ;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->ft->ag->combined==ag_aggregate */
static int FS_r_split(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t elements = pn->ft->ag->elements ;
    int ret = 0 ;
    if (offset) {
        return -EADDRNOTAVAIL ;
    }
    switch( pn->ft->format ) {
    case ft_integer:
        {
            int * i = (int *) calloc( elements, sizeof(int) ) ;
                if ( i==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.i)(i,pn) ;
                if (ret >= 0) ret = FS_output_integer( i[pn->extension] , buf , size , pn ) ;
            free( i ) ;
            break ;
        }
    case ft_unsigned:
        {
            unsigned int * u = (unsigned int *) calloc( elements, sizeof(unsigned int) ) ;
                if ( u==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.u)(u,pn) ;
                if (ret >= 0) ret = FS_output_unsigned( u[pn->extension] , buf , size , pn ) ;
            free( u ) ;
            break ;
        }
    case ft_float:
        {
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                if (ret >= 0) ret = FS_output_float( f[pn->extension] , buf , size , pn ) ;
            free( f ) ;
            break ;
        }
    case ft_date:
        {
            DATE * d = (DATE *) calloc( elements, sizeof(DATE) ) ;
                if ( d==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.d)(d,pn) ;
                if (ret >= 0) ret = FS_output_date( d[pn->extension] , buf , size , pn ) ;
            free( d ) ;
            break ;
        }
    case ft_yesno:
        {
            int * y = (int *) calloc( elements, sizeof(int) ) ;
            if ( y==NULL ) return -ENOMEM ;
            ret = (pn->ft->read.y)(y,pn) ;
            if (ret >= 0) {
		buf[0] = y[pn->extension]?'1':'0' ;
		ret = 1;
	    }
            free( y ) ;
            break ;
        }
    case ft_bitfield:
        {
            unsigned int u ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret >= 0) {
                buf[0] = u & (1<<pn->extension) ? '1' : '0' ;
                ret = 1 ;
            }
            break ;
        }
    case ft_ascii: {
        size_t s = FullFileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.a)(buf,s,offset,pn) ;
        }
    case ft_binary: {
        size_t s = FullFileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.b)(buf,s,offset,pn) ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    }
    return ret ;
}

int FS_output_integer( int value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = FileLength(pn) ;
    char c[suglen+2] ;
    /* should only need suglen+1, but uClibc's snprintf()
       seem to trash 'len' if not increased */
    int len ;
    if ( suglen>size ) suglen=size ;
    UCLIBCLOCK
        len = snprintf(c,suglen+1,"%*d",(int)suglen,value) ;
    UCLIBCUNLOCK
    if ( (len<0) || (len>suglen) ) return -EMSGSIZE ;
    memcpy( buf, c, (size_t)len ) ;
    return len ;
}

int FS_output_unsigned( unsigned int value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = FileLength(pn) ;
    char c[suglen+2] ;
    /* should only need suglen+1, but uClibc's snprintf()
       seem to trash 'len' if not increased */
    int len ;
    if ( suglen>size ) suglen=size ;
    UCLIBCLOCK
        len = snprintf(c,suglen+1,"%*u",(int)suglen,value) ;
    UCLIBCUNLOCK
    if ((len<0) || (len>suglen) ) return -EMSGSIZE ;
    memcpy( buf, c, (size_t)len ) ;
    return len ;
}

int FS_output_float( FLOAT value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = FileLength(pn) ;
    char c[suglen+2] ;
    /* should only need suglen+1, but uClibc's snprintf()
       seem to trash 'len' if not increased */
    int len ;
    if ( suglen>size ) suglen=size ;
    UCLIBCLOCK
        len = snprintf(c,suglen+1,"%*G",(int)suglen,value) ;
    UCLIBCUNLOCK
    if ((len<0) || (len>suglen) ) return -EMSGSIZE ;
    memcpy( buf, c, (size_t)len ) ;
    return len ;
}

int FS_output_date( DATE value, char * buf, const size_t size, const struct parsedname * pn ) {
    char c[26] ;
    (void) pn ;
    if ( size < 24 ) return -EMSGSIZE ;
    ctime_r( &value, c) ;
    memcpy( buf, c, 24 ) ;
    return 24 ;
}

int FS_output_integer_array( int * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
        if ( (len=FS_output_integer( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_unsigned( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}

int FS_output_unsigned_array( unsigned int * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
//printf("OUA i=%d left=%d size=%d values=%u,%5u,%6u\n",i,left,size,values[i],values[i],values[i]);
        if ( (len=FS_output_unsigned( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_unsigned( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}

int FS_output_float_array( FLOAT * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
        if ( (len=FS_output_float( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_float( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}

int FS_output_date_array( DATE * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
        if ( (len=FS_output_date( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_date( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}
