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
    //printf("FS_read: pid=%ld path=%s\n", pthread_self(), path);

    if ( FS_ParsedName( path , &pn ) ) {
        r = -ENOENT;
    } else if ( pn.dev==NULL || pn.ft == NULL ) {
        r = -EISDIR ;
    } else {
      //printf("FS_read: pn->state=pn_bus=%c pn->bus_nr=%d\n", pn.state&pn_bus?'Y':'N', pn.bus_nr);
      //printf("FS_read: pn->path=%s pn->path_busless=%s\n", pn.path, pn.path_busless);
      //printf("FS_read: pid=%ld call postparse size=%ld pn->type=%d\n", pthread_self(), size, pn.type);
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
    //printf("FS_read_postparse: pid=%ld busmode=%d pn->type=%d size=%d\n", pthread_self(), get_busmode(pn->in), pn->type, size);

    STATLOCK
    AVERAGE_IN(&read_avg)
    AVERAGE_IN(&all_avg)
    STATUNLOCK

    switch (pn->type) {
    case pn_structure:
        /* Get structure data from local memory */
        //printf("FS_read_postparse: pid=%ld call fs_structure\n", pthread_self());
        r = FS_structure(buf,size,offset,pn) ;
        break;
    case pn_system:
    case pn_settings:
    case pn_statistics:
        //printf("FS_read_postparse: pid=%ld system/settings/statistics\n", pthread_self());
        if ( pn->state & pn_bus ) {
	    /* this will either call ServerDir or FS_real_read */
	    //printf("FS_read_postparse: call read_seek\n");
	    r = FS_read_seek(buf, size, offset, pn) ;
        } else {
	    /* Read from local memory */
	    //printf("FS_read_postparse: call real_read\n");
            r = FS_real_read( buf, size, offset, pn ) ;
	}
        break;
    default:
      //printf("FS_read_postparse: pid=%ld call fs_read_seek size=%ld\n", pthread_self(), size);

#if 0
      /* handle DeviceSimultaneous in some way */
        if(pn->dev == DeviceSimultaneous) {
	    printf("FS_read_postparse: DeviceSimultaneous %s\n", pn->path);
            r = FS_real_read(buf, size, offset, pn) ;
	} else
#endif
	{
	    /* real data -- go through device chain */
	    /* this will either call ServerDir or FS_real_read */
	    if((pn->type == pn_real) && !(pn->state & pn_bus)) {
	      struct parsedname pn2;
	      int bus_nr = -1;
	      if(Cache_Get_Device(&bus_nr, pn)) {
		//printf("Cache_Get_Device didn't find bus_nr\n");
		bus_nr = CheckPresence(pn);
		/* Cache_Add_Device() is called in FS_read_seek() */
	      }
	      if(bus_nr >= 0) {
		memcpy(&pn2, pn, sizeof(struct parsedname));
		/* fake that we read from only one indevice now! */
		pn2.in = find_connection_in(bus_nr);
		pn2.state |= pn_bus ;
		pn2.bus_nr = bus_nr ;
		//printf("read only from bus_nr=%d\n", bus_nr);
		r = FS_read_seek(buf, size, offset, &pn2) ;
	      } else {
		//printf("CheckPresence failed, no use to read\n");
		r = -ENOENT ;
	      }
	    } else {
	      r = FS_read_seek(buf, size, offset, pn) ;
	    }
	}
    }
    STATLOCK
        if ( r>=0 ) {
        ++read_success ; /* statistics */
        read_bytes += r ; /* statistics */
        }
        AVERAGE_OUT(&read_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK

//printf("FS_read_postparse: pid=%ld return %d\n", pthread_self(), r);
    return r;
}

/* Loop through input devices (busses) */
static int FS_read_seek(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    int r = 0;
#ifdef OW_MT
    pthread_t thread ;
    int threadbad = 1;
    char *buf2 ;
    void * v ;
    int rt ;
    size_t s ;

    /* Embedded function */
    void * Read2( void * vp ) {
        struct parsedname *pn2 = (struct parsedname *)vp ;
        struct parsedname pnnext ;
        struct stateinfo si ;
        int ret;
        memcpy( &pnnext, pn2 , sizeof(struct parsedname) ) ;
        /* we need a different state (search state) for a different bus -- subtle error */
        pnnext.si = &si ;
        pnnext.in = pn2->in->next ;
        ret = FS_read_seek(buf2,size,offset,&pnnext) ;
        pthread_exit((void *)ret);
	return (void *)ret;
    }
    //printf("READSEEK pid=%ld path=%s index=%d\n",pthread_self(), pn->path,pn->in->index);
    if( !(buf2 = malloc(size)) ) {
      return -ENOMEM;
    }
    if(!(pn->state & pn_bus)) {
      threadbad = pn->in==NULL || pn->in->next==NULL || pthread_create( &thread, NULL, Read2, (void *)pn ) ;
    }
#endif /* OW_MT */

    if ( (get_busmode(pn->in) == bus_remote) ) {
//printf("READSEEK0 pid=%ld call ServerRead\n", pthread_self());
        r = ServerRead(buf,size,offset,pn) ;
//printf("READSEEK0 pid=%ld r=%d\n",pthread_self(), r);
    } else {
        s = size ;
        STATLOCK
            ++ read_calls ; /* statistics */
        STATUNLOCK
        /* Check the cache (if not pn_uncached) */
        if ( offset!=0 || IsLocalCacheEnabled(pn)==0 ) {
//printf("READSEEK1 pid=%d call FS_real_read\n",getpid());
            if ( (r=LockGet(pn))==0 ) {
                r = FS_real_read(buf, size, offset, pn ) ;
//printf("READSEEK1 FS_real_read ret=%d\n", r);
                LockRelease(pn) ;
            }
//printf("READSEEK1 pid=%d = %d\n",getpid(), r);
        } else if ( (pn->state & pn_uncached) || Cache_Get( buf, &s, pn ) ) {
//printf("READSEEK2 pid=%d not found in cache\n",getpid());
            if ( (r=LockGet(pn))==0 ) {
//printf("READSEEK2 lock get size=%d offset=%d\n", size, offset);
                r = FS_real_read( buf, size, offset, pn ) ;
//printf("READSEEK2 FS_real_read ret=%d\n", r);
                if ( r>0 ) Cache_Add( buf, r, pn ) ;
                LockRelease(pn) ;
            }
//printf("READSEEK2 pid=%d = %d\n",getpid(), r);
        } else {
//printf("READSEEK3 pid=%ld cached found\n",pthread_self()) ;
            STATLOCK
                ++read_cache ; /* statistics */
                read_cachebytes += s ; /* statistics */
            STATUNLOCK
            r = s ;
//printf("READSEEK3 pid=%ld r=%d\n",pthread_self(), r);
        }
    }
    /* If sucessfully reading a device, we know it exists on a specific bus.
     * Update the cache content */
    if((pn->type == pn_real) && (r >= 0)) {
      Cache_Add_Device(pn->in->index, pn);
    }

#ifdef OW_MT
    if ( threadbad == 0 ) { /* was a thread created? */
        //printf("READ call pthread_join\n");
        if ( pthread_join( thread, &v ) ) {
            free(buf2);
            return r ; /* wait for it (or return only this result) */
        }
        rt = (int) v ;
//printf("READ 1st=%d 2nd=%d\n",r,rt);
        if ( (rt >= 0) && (rt > r) ) { /* is it an error return? Then return this one */
//printf("READ from 2nd adapter\n") ;
            memcpy( buf, buf2, (size_t) rt ) ; /* Use the thread's result */
            free(buf2);
            return rt ;
        }
//printf("READ from 1st adapter\n") ;
    }
    free(buf2);
#endif /* OW_MT */
    return r ;
}

/* Real read -- called from read
   Integrates with cache -- read not called if cached value already set
*/
static int FS_real_read(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) {
    int r;
    //printf("RealRead pid=%ld path=%s size=%d, offset=%d, extension=%d adapter=%d\n", pthread_self(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;
    /* Readable? */
    if ( (pn->ft->read.v) == NULL ) return -ENOENT ;

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
       /* Normal read. Try three times */
    ++read_tries[0] ; /* statitics */
    r = FS_parse_read( buf, size, offset, pn ) ;
    //printf("RealRead path=%s size=%d, offset=%d, extension=%d adapter=%d result=%d\n",pn->path,size,(int)offset,pn->extension,pn->in->index,r) ;
    //    ++read_tries[1] ; /* statitics */
    //    if ( (r=FS_parse_read( buf, size, offset, pn )) >= 0 ) return r;
    //    ++read_tries[2] ; /* statitics */
    //    r = FS_parse_read( buf, size, offset, pn ) ;
    //    if (r<0) syslog(LOG_INFO,"Read error on %s (size=%d)\n",pn->path,(int)size) ;
    return r;
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
            (int)FullFileLength(&pn2)
            ) ;
    UCLIBCUNLOCK
    return len;
}

/* read without artificial separation of combination */
static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int ret = 0 ;
//printf("ParseRead pid=%ld path=%s size=%d, offset=%d, extension=%d adapter=%d\n",pthread_self(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;

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
