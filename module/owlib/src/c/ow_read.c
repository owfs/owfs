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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Prototypes ----------- */
static int FS_read_seek(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) ;
static int FS_real_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_r_all(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_r_split(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_gamish(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_structure(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;

/*
Change in strategy 6/2006:
Now use CheckPresence as primary method of finding correct bus
Can break down cases into:
1. bad ParsedName -- no read possible
2. structure -- read from 1st bus (local)
3. specified bus (picked up in ParsedName) -- use that
4. statistics, settings, Simultaneous, Thermostat -- use first or specified
5. real -- use caced, if error, delete cache entry and try twice more.
*/

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

int FS_read(const char *path, char *buf, const size_t size, const off_t offset) {
    struct parsedname pn ;
    int r ;

    LEVEL_CALL("READ path=%s size=%d offset=%d\n", SAFESTRING(path), (int)size, (int)offset )

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
int FS_read_postparse(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    struct parsedname pn2 ;
    ssize_t r = 0 ;
    //    if ( pn->in==NULL ) return -ENODEV ;
    /* Normal read. Try three times */

    LEVEL_DEBUG("READ_POSTPARSE %s\n", pn->path);
    STATLOCK;
    AVERAGE_IN(&read_avg);
    AVERAGE_IN(&all_avg);
    STATUNLOCK;

    /* First try */
    /* in and bus_nr already set */
    STAT_ADD1(read_tries[0]) ;
    r = FS_read_postpostparse( buf, size, offset, pn ) ;

    /* Second Try */
    /* if not a specified bus, relook for chip location */
    if ( r < 0 ) {
        memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
        STAT_ADD1(read_tries[1]) ;
        if ( server_mode ) {
            Cache_Del_Device( pn ) ;
        } else if ( pn->state & pn_buspath ) {
            r = TestConnection(pn) ? -ECONNABORTED : FS_read_postpostparse( buf, size, offset, pn ) ;
        } else if ( (r = CheckPresence( pn )) >= 0 ) {
            pn2.in = find_connection_in(r);
            pn2.state |= pn_bus ;
            pn2.bus_nr = r ;
            Cache_Add_Device( r, pn ) ;
            r = FS_read_postpostparse( buf, size, offset, &pn2 ) ;
        } else {
            r = -ENOENT ;
        }
    }

    /* Third try */
    /* if not a specified bus, relook for chip location */
    if ( !server_mode && (r < 0) ) {
        STAT_ADD1(read_tries[2]) ;
        if ( pn->state & pn_buspath ) {
            r = TestConnection(pn) ? -ECONNABORTED : FS_read_postpostparse( buf, size, offset, pn ) ;
        } else if ( (r = CheckPresence( pn )) >= 0 ) {
            pn2.in = find_connection_in(r);
            pn2.state |= pn_bus ;
            pn2.bus_nr = r ;
            Cache_Add_Device( r, pn ) ;
            r = FS_read_postpostparse( buf, size, offset, &pn2 ) ;
        } else {
            Cache_Del_Device(pn) ;
            r = -ENOENT ;
        }
    }
    STATLOCK;
    if ( r>=0 ) {
      ++read_success ; /* statistics */
      read_bytes += r ; /* statistics */
    }
    AVERAGE_OUT(&read_avg);
    AVERAGE_OUT(&all_avg);
    STATUNLOCK;
    LEVEL_DEBUG("READ_POSTPARSE %s return %d\n", pn->path, r);
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
int FS_read_postpostparse(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    int r = 0;
    //printf("FS_read_postpostparse: pid=%ld busmode=%d pn->type=%d size=%d\n", pthread_self(), get_busmode(pn->in), pn->type, size);

    LEVEL_DEBUG("READ_POSTPOSTPARSE %s\n",pn->path) ;
    STATLOCK;
    AVERAGE_IN(&read_avg);
    AVERAGE_IN(&all_avg);
    STATUNLOCK;

    switch (pn->type) {
        case pn_structure:
            /* Get structure data from local memory */
            //printf("FS_read_postpostparse: pid=%ld call fs_structure\n", pthread_self());
            r = FS_structure(buf,size,offset,pn) ;
            break;
        default:
            r = FS_read_seek( buf, size, offset, pn ) ;
            break;
    }
    STATLOCK;
        if ( r>=0 ) {
            ++read_success ; /* statistics */
            read_bytes += r ; /* statistics */
        }
        AVERAGE_OUT(&read_avg);
        AVERAGE_OUT(&all_avg);
    STATUNLOCK;

LEVEL_DEBUG("READ_POSTPOSTPARSE: %s return %d\n", pn->path, r);
//printf("FS_read_postpostparse: pid=%ld return %d\n", pthread_self(), r);
    return r;
}

static int FS_read_seek(char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) {
    int r = 0;
    //printf("READSEEK\n");
    LEVEL_DEBUG("READSEEK\n");

    if ( (pn->state & pn_bus) && (get_busmode(pn->in) == bus_remote) ) {
        LEVEL_DEBUG("READSEEK0 pid=%ld call ServerRead\n", pthread_self());
        //printf("READSEEK0 pid=%ld call ServerRead\n", pthread_self());
        r = ServerRead(buf,size,offset,pn) ;
        //printf("READSEEK0 pid=%ld r=%d\n",pthread_self(), r);
    } else {
        size_t s = size ;
        STAT_ADD1(read_calls) ; /* statistics */
        /* Check the cache (if not pn_uncached) */
        if ( offset!=0 || IsLocalCacheEnabled(pn)==0 ) {
	    LEVEL_DEBUG("READSEEK1 pid=%d call FS_real_read\n",getpid());
            //printf("READSEEK1 pid=%d call FS_real_read\n",getpid());
            if ( (r=LockGet(pn))==0 ) {
                r = FS_real_read(buf, size, offset, pn ) ;
                //printf("READSEEK1 FS_real_read ret=%d\n", r);
                LockRelease(pn) ;
            }
            //printf("READSEEK1 pid=%d = %d\n",getpid(), r);
        } else if ( IsUncachedDir(pn) || Cache_Get( buf, &s, pn ) ) {
            LEVEL_DEBUG("READSEEK2 pid=%d not found in cache\n",getpid());
            //printf("READSEEK2 pid=%d not found in cache\n",getpid());
            if ( (r=LockGet(pn))==0 ) {
                //printf("READSEEK2 lock get size=%d offset=%d\n", size, offset);
                r = FS_real_read( buf, size, offset, pn ) ;
                //printf("READSEEK2 FS_real_read ret=%d\n", r);
                if ( r>0 ) Cache_Add( buf, (const size_t)r, pn ) ;
                LockRelease(pn) ;
            }
            //printf("READSEEK2 pid=%d = %d\n",getpid(), r);
        } else {
	    LEVEL_DEBUG("READSEEK3 pid=%ld cached found\n",pthread_self()) ;
            //printf("READSEEK3 pid=%ld cached found\n",pthread_self()) ;
            STATLOCK;
                ++read_cache ; /* statistics */
                read_cachebytes += s ; /* statistics */
            STATUNLOCK;
            r = s ;
            //printf("READSEEK3 pid=%ld r=%d\n",pthread_self(), r);
        }
    }
    return r ;
}

/* Real read -- called from read
   Integrates with cache -- read not called if cached value already set
*/
static int FS_real_read(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) {
    int r;
    //printf("RealRead pid=%ld path=%s size=%d, offset=%d, extension=%d adapter=%d\n", pthread_self(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;
    /* Readable? */
    if ( (pn->ft->read.v) == NULL ) return -ENOTSUP ;

    /* Special case for "fake" adapter */
    if ( pn->in->Adapter==adapter_fake && pn->ft->change!=ft_static ) return FS_read_fake( buf, size, offset, pn ) ;

    /* Array property? Read separately? Read together and manually separate? */
    if ( pn->ft->ag ) { /* array property */
        if ( pn->extension==-1) {
            switch(pn->ft->ag->combined) {
                case ag_separate: /* separate reads, artificially combined into a single array */
                    return FS_r_all(buf,size,offset,pn) ;
                case ag_mixed: /* mixed mode, ALL read handled differently */
                    return FS_gamish(buf,size,offset,pn) ;
                case ag_aggregate: /* natively an array */
                    /* return ALL if required   (comma separated)*/
                    return FS_gamish(buf,size,offset,pn) ;
            }
        } else if ( pn->extension>-1 && pn->ft->ag->combined==ag_aggregate ) {
            /* split apart if a single item requested */
            return FS_r_split(buf,size,offset,pn) ;
        }
    }
     
    /* Normal read. */
    r = FS_parse_read( buf, size, offset, pn ) ;
    //printf("RealRead path=%s size=%d, offset=%d, extension=%d adapter=%d result=%d\n",pn->path,size,(int)offset,pn->extension,pn->in->index,r) ;
    //    ++read_tries[1] ; /* statitics */
    //    if ( (r=FS_parse_read( buf, size, offset, pn )) >= 0 ) return r;
    //    ++read_tries[2] ; /* statitics */
    //    r = FS_parse_read( buf, size, offset, pn ) ;
    //    if (r<0) LEVEL_DATA("Read error on %s (size=%d)\n",SAFESTRING(pn->path),(int)size)
    return r;
}

/* Structure file */
static int FS_structure(char *buf, const size_t size, const off_t offset, const struct parsedname * pn) {
    char ft_format_char[] = "  iufabydy" ; /* return type */
    /* dir,dir,int,unsigned,float,ascii,binary,yesno,date,bitfield */
    int len ;
    struct parsedname pn2 ;

    size_t s = SimpleFullFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; /* shallow copy */
    pn2.type = pn_real ; /* "real" type to get return length, rather than "structure" length */

    UCLIBCLOCK;
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
            (int)SimpleFullFileLength(&pn2)
            ) ;
    UCLIBCUNLOCK;

    if((len > 0) && offset) {
      memcpy(buf, &buf[offset], (size_t)len - (size_t)offset);
      return len - offset;
    }
    return len;
}

/* read without artificial separation or combination */
static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int ret = 0 ;
    int sz ;
    size_t s = 0 ;
    enum ft_format format = pn->ft->format ;
//printf("ParseRead pid=%ld path=%s size=%d, offset=%d, extension=%d adapter=%d\n",pthread_self(), pn->path,size,(int)offset,pn->extension,pn->in->index) ;

    LEVEL_CALL("FS_parse_read: format=%d s=%d offset=%d\n", (int)pn->ft->format, (int)size, (int)offset )

    /* Mounting fuse with "direct_io" will cause a second read with offset
     * at end-of-file... Just return 0 if offset == size */
    s = SimpleFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;

    /* Special for *.BYTE -- treat as a single value */
    if ( format==ft_bitfield && pn->extension==-2 ) format = ft_unsigned ;
    
    switch( format ) {
        case ft_integer: {
            int i ;
            ret = (pn->ft->read.i)(&i,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (integer) %d\n", i ) ;
            sz = FS_output_integer( i , buf , size , pn ) ;
            break;
            }
        case ft_bitfield: {
            UINT u ;
            if (size < 1) return -EMSGSIZE ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (bitfield) %u\n", u ) ;
            buf[0] = UT_getbit((void*)(&u),pn->extension) ? '1' : '0' ;
            return 1 ;
            }
        case ft_unsigned: {
            UINT u ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (unsigned) %u\n", u ) ;
            sz = FS_output_unsigned( u , buf , size , pn ) ;
            break ;
            }
        case ft_float: {
            FLOAT f ;
            ret = (pn->ft->read.f)(&f,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (float) %G\n", f ) ;
            sz = FS_output_float( f , buf , size , pn ) ;
            break ;
            }
        case ft_temperature: {
            FLOAT f ;
            ret = (pn->ft->read.f)(&f,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (temperature) %G\n", f ) ;
            sz = FS_output_float( Temperature(f,pn) , buf , size , pn ) ;
            break ;
            }
        case ft_tempgap: {
            FLOAT f ;
            ret = (pn->ft->read.f)(&f,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (tempgap) %G\n", f ) ;
            sz = FS_output_float( TemperatureGap(f,pn) , buf , size , pn ) ;
            break ;
            }
        case ft_date: {
            DATE d ;
            ret = (pn->ft->read.d)(&d,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (date) %lu\n", (unsigned long int) d ) ;
            sz = FS_output_date( d , buf , size , pn ) ;
            break;
            }
        case ft_yesno: {
            int y ;
            if (size < 1) return -EMSGSIZE ;
            ret = (pn->ft->read.y)(&y,pn) ;
            if (ret < 0) return ret ;
            LEVEL_DEBUG("FS_parse_read: (yesno) %d\n", y ) ;
            buf[0] = y ? '1' : '0' ;
            return 1 ;
            }
        case ft_vascii:
        case ft_ascii: {
            //size_t s = SimpleFileLength(pn) ;
            //if ( offset > s ) return -ERANGE ;
            s -= offset ;
            if ( s > size ) s = size ;
            return (pn->ft->read.a)(buf,s,offset,pn) ;
            }
        case ft_binary: {
            //size_t s = SimpleFileLength(pn) ;
            //if ( offset > s ) return -ERANGE ;
            s -= offset ;
            if ( s > size ) s = size ;
            return (pn->ft->read.b)(buf,s,offset,pn) ;
            }
        case ft_directory:
        case ft_subdir:
            return -ENOSYS ;
        default:
            return -ENOENT ;
    }

    /* Return correct buffer according to offset for most data-types here */
    if((sz > 0) && offset) {
      memcpy(buf, &buf[offset], (size_t)sz - (size_t)offset);
      return sz - offset;
    }
    return sz ;
}

/* read an aggregation (returns an array from a single read) */
static int FS_gamish(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t elements = pn->ft->ag->elements ;
    int ret = 0 ;
    size_t s = 0 ;
    
    /* Mounting fuse with "direct_io" will cause a second read with offset
     * at end-of-file... Just return 0 if offset == size */
    s = SimpleFullFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;

    switch( pn->ft->format ) {
    case ft_integer:
        {
            int * i = (int *) calloc( elements, sizeof(int) ) ;
                if ( i==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.i)(i,pn) ;
                if (ret >= 0) ret = FS_output_integer_array( i , buf , size , pn ) ;
            free( i ) ;
            break;
        }
    case ft_unsigned:
        {
            UINT * u = (UINT *) calloc( elements, sizeof(UINT) ) ;

                if ( u==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.u)(u,pn) ;
                if (ret >= 0) ret = FS_output_unsigned_array( u , buf , size , pn ) ;
            free( u ) ;
            break;
        }
    case ft_float:
        {
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                if (ret >= 0) ret = FS_output_float_array( f , buf , size , pn ) ;
            free( f ) ;
            break ;
        }
    case ft_temperature:
        {
            size_t i ;
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                for ( i=0; i<elements ; ++i ) f[i] = Temperature(f[i],pn) ;
                if (ret >= 0) ret = FS_output_float_array( f , buf , size , pn ) ;
            free( f ) ;
            break ;
        }
    case ft_tempgap:
        {
            size_t i ;
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                for ( i=0; i<elements ; ++i ) f[i] = TemperatureGap(f[i],pn) ;
                if (ret >= 0) ret = FS_output_float_array( f , buf , size , pn ) ;
            free( f ) ;
            break ;
        }
    case ft_date:
        {
            DATE * d = (DATE *) calloc( elements, sizeof(DATE) ) ;
                if ( d==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.d)(d,pn) ;
                if (ret >= 0) ret = FS_output_date_array( d , buf , size , pn ) ;
            free( d ) ;
            break ;
        }
    case ft_yesno:
        {
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
            break ;
        }
    case ft_bitfield:
        {
            UINT u ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret >= 0) {
                size_t i ;
                size_t j = 0 ;
                for ( i=0 ; i<elements ; ++i ) {
                    buf[j++] = u&0x01 ? '1' : '0' ;
                    u = u>>1 ;
                    if ( i<elements-1 ) buf[j++] = ',' ;
                }
                ret = j ;
            }
            break ;
        }
    case ft_vascii:
    case ft_ascii: {
        //size_t s = SimpleFullFileLength(pn) ;
        //if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.a)(buf,s,offset,pn) ;
        }
    case ft_binary: {
        //size_t s = SimpleFullFileLength(pn) ;
        //if ( offset > s ) return -ERANGE ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.b)(buf,s,offset,pn) ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    default:
	return -ENOENT ;
    }

    if((size_t)ret != s) {
      /* Read error since we didn't get all bytes */
      LEVEL_DEBUG("FS_gamish: error ret=%d s=%d\n", ret, s);
      return -ENOENT ;
    }

    /* Return correct buffer according to offset for most data-types here */
    if((ret > 0) && offset) {
      memcpy(buf, &buf[offset], (size_t)ret - (size_t)offset);
      return ret - offset;
    }
    return ret ;
}

/* Read each array element independently, but return as one long string */
/* called when pn->extension==-1 (ALL) and pn->ft->ag->combined==ag_separate */
static int FS_r_all(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t left = size ;
    char * p = buf ;
    int r ;
    struct parsedname pn2 ;
    size_t s, sz;

    STAT_ADD1(read_array) ; /* statistics */

    s = SimpleFullFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;

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
        if ( (r=FS_parse_read(p,left,(const off_t)0,&pn2)) < 0 ) return r ;
        left -= r ;
        p += r ;
        //printf("READALL(%p) %d->%d (%d->%d)\n",p,pname.extension,r,size,left) ;
    }
    //printf("READALL return %d\n",size-left) ;

    sz = size - left ;
#if 0
    // /var/1wire/system/adapter/address.ALL is 512 long
    // but will only return 10 bytes or something
    if(sz != s) {
      LEVEL_DEBUG("FS_r_all: error sz=%d s=%d\n", sz, s);
      return -ENOENT ;
    }
#endif

    LEVEL_DEBUG("FS_r_all: size=%d left=%d sz=%d\n", size, left, sz);
    if((sz > 0) && offset) {
        memcpy(buf, &buf[offset], sz - (size_t)offset);
        return sz - offset;
    }
    return size - left ;
}

/* read the combined data, and then separate */
/* called when pn->extension>0 (not ALL) and pn->ft->ag->combined==ag_aggregate */
static int FS_r_split(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t elements = pn->ft->ag->elements ;
    int ret = 0 ;
    size_t s = 0 ;
    
    /* Mounting fuse with "direct_io" will cause a second read with offset
     * at end-of-file... Just return 0 if offset == size */
    s = SimpleFileLength(pn) ;
    if ( offset > s ) return -ERANGE ;
    if ( offset == s ) return 0 ;

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
            UINT * u = (UINT *) calloc( elements, sizeof(UINT) ) ;
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
    case ft_temperature:
        {
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                if (ret >= 0) ret = FS_output_float( Temperature(f[pn->extension],pn) , buf , size , pn ) ;
            free( f ) ;
            break ;
        }
    case ft_tempgap:
        {
            FLOAT * f = (FLOAT *) calloc( elements, sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = (pn->ft->read.f)(f,pn) ;
                if (ret >= 0) ret = FS_output_float( TemperatureGap(f[pn->extension],pn) , buf , size , pn ) ;
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
            UINT u ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret >= 0) {
                buf[0] = u & (1<<pn->extension) ? '1' : '0' ;
                ret = 1 ;
            }
            break ;
        }
    case ft_vascii:
    case ft_ascii: {
        s = SimpleFullFileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        if ( offset == s ) return 0 ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.a)(buf,s,offset,pn) ;
        }
    case ft_binary: {
        s = SimpleFullFileLength(pn) ;
        if ( offset > s ) return -ERANGE ;
        if ( offset == s ) return 0 ;
        s -= offset ;
        if ( s > size ) s = size ;
        return (pn->ft->read.b)(buf,s,offset,pn) ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    default:
        return -ENOENT ;
    }

    if((size_t)ret != s) {
      /* Read error since we didn't get all bytes */
      LEVEL_DEBUG("FS_r_split: error ret=%d s=%d\n", ret, s);
      return -ENOENT ;
    }

    LEVEL_DEBUG("FS_r_split: size=%d sz=%d\n", size, ret);
    if((ret > 0) && offset) {
      memcpy(buf, &buf[offset], (size_t)ret - (size_t)offset);
      return ret - offset;
    }

    return ret ;
}

int FS_output_integer( int value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = SimpleFileLength(pn) ;
    /* should only need suglen+1, but uClibc's snprintf()
       seem to trash 'len' if not increased */
    int len ;
    char *c;
    if(!(c = (char *)malloc(suglen+2))) {
      return -ENOMEM;
    }
    if ( suglen>size ) suglen=size ;
    UCLIBCLOCK;
        len = snprintf(c,suglen+1,"%*d",(int)suglen,value) ;
    UCLIBCUNLOCK;
    if ( (len<0) || ((size_t)len>suglen) ) {
      free(c) ;
      return -EMSGSIZE ;
    }
    memcpy( buf, c, (size_t)len ) ;
    free(c) ;
    return len ;
}

int FS_output_unsigned( UINT value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = SimpleFileLength(pn) ;
    int len ;
    /*
      char c[suglen+2] ;
      defining this REALLY doesn't work on gcc 3.3.2 either...
      It will corrupt "const size_t size"! after returning from this function
    */
    /* should only need suglen+1, but uClibc's snprintf()
       seem to trash 'len' if not increased */
    char *c;
    if(!(c = (char *)malloc(suglen+2))) {
      return -ENOMEM;
    }
    if ( suglen>size ) suglen=size ;
    UCLIBCLOCK;
        len = snprintf(c,suglen+1,"%*u",(int)suglen,value) ;
    UCLIBCUNLOCK;
    if ((len<0) || ((size_t)len>suglen) ) {
      free(c) ;
      return -EMSGSIZE ;
    }
    memcpy( buf, c, (size_t)len ) ;
    free(c) ;
    return len ;
}

int FS_output_float( FLOAT value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = SimpleFileLength(pn) ;
    /* should only need suglen+1, but uClibc's snprintf()
       seem to trash 'len' if not increased */
    int len ;
    char *c;
    if(!(c = (char *)malloc(suglen+2))) {
      return -ENOMEM;
    }
    if ( suglen>size ) suglen=size ;
    UCLIBCLOCK;
        len = snprintf(c,suglen+1,"%*G",(int)suglen,value) ;
    UCLIBCUNLOCK;
    if ((len<0) || ((size_t)len>suglen) ) {
      free(c) ;
      return -EMSGSIZE ;
    }
    memcpy( buf, c, (size_t)len ) ;
    free(c) ;
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
        if ( (len=FS_output_integer( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_integer( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}

int FS_output_unsigned_array( UINT * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
//printf("OUA i=%d left=%d size=%d values=%u,%5u,%6u\n",i,left,size,values[i],values[i],values[i]);
        if ( (len=FS_output_unsigned( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_unsigned( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}

int FS_output_float_array( FLOAT * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
        if ( (len=FS_output_float( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_float( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}

int FS_output_date_array( DATE * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
        if ( (len=FS_output_date( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
        left -= len ;
        first += len ;
        if ( left<1 ) return -EMSGSIZE ;
        first[0] = ',' ;
        ++first ;
        --left ;
    }
    if ( (len=FS_output_date( values[i], first, (size_t)left, pn )) < 0 ) return -EMSGSIZE ;
    return size-(left-len) ;
}
