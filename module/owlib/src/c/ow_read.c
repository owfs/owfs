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

#include <sys/stat.h>
#include <string.h>

#include "owfs_config.h"
#include "ow.h"


/* ------- Prototypes ----------- */
static int FS_real_read(const char *path, char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_r_all(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_r_split(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;

static int FS_output_unsigned( unsigned int value, char * buf, const size_t size, const struct parsedname * pn ) ;
static int FS_output_int( int value, char * buf, const size_t size, const struct parsedname * pn ) ;
static int FS_output_float( float value, char * buf, const size_t size, const struct parsedname * pn ) ;
static int FS_output_unsigned_array( unsigned int * values, char * buf, const size_t size, const struct parsedname * pn ) ;
static int FS_output_integer_array( int * values, char * buf, const size_t size, const struct parsedname * pn ) ;
static int FS_output_float_array( float * values, char * buf, const size_t size, const struct parsedname * pn ) ;

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

int FS_read(const char *path, char *buf, const size_t size, const off_t offset) {
    struct parsedname pn ;
    struct stateinfo si ;
    size_t s = size ;
    int r ;
    pn.si = &si ;
    if ( FS_ParsedName( path , &pn ) ) {
        FS_ParsedName_destroy(&pn) ;
        return -ENOENT;
    } else if ( pn.dev==NULL || pn.ft == NULL ) {
        FS_ParsedName_destroy(&pn) ;
        return -EISDIR ;
    }
    ++ read_calls ; /* statistics */
    /* Check the cache (if not pn_uncached) */
    if ( offset!=0 || cacheavailable==0 ) {
        Lock_Get(&pn) ;
        r = FS_real_read( path, buf, size, offset, &pn ) ;
        Lock_Release(&pn) ;
    } else if ( pn.type==pn_uncached || Cache_Get( &pn, buf, &s ) ) {
//printf("Read didnt find %s(%d->%d)\n",path,size,s) ;
        Lock_Get(&pn) ;
        r = FS_real_read( path, buf, size, offset, &pn ) ;
        if ( r>= 0 ) Cache_Add( &pn, buf, r ) ;
        Lock_Release(&pn) ;
    } else {
//printf("Read found %s\n",path) ;
        ++read_cache ; /* statistics */
        read_cachebytes += s ; /* statistics */
        return s ;
    }

    if ( r>=0 ) {
        ++read_success ; /* statistics */
        read_bytes += r ; /* statistics */
    }
    FS_ParsedName_destroy(&pn) ;
    return r ;
}


/* Real read -- called from read
   Integrates with cache -- read not called if cached value alread set
*/
static int FS_real_read(const char *path, char *buf, const size_t size, const off_t offset, const struct parsedname * pn) {
    int r ;
//printf("RealRead path=%s size=%d, offset=%d, extension=%d\n",path,size,offset,pn->extension) ;
    /* Readable? */
    /* Do we exist? Only test static cases */
    if ( ( (pn->ft->read.v) == NULL ) || ( presencecheck && pn->ft->change==ft_static && CheckPresence(pn) ) ) return -ENOENT ;

    /* Array property? Read separately? Read together and separate? */
    if ( pn->ft->ag && pn->extension==-1 && pn->ft->ag->combined==ag_separate ) return FS_r_all(buf,size,offset,pn) ;
    if ( pn->ft->ag && pn->extension>= 0 && pn->ft->ag->combined==ag_aggregate ) return FS_r_split(buf,size,offset,pn) ;

    /* Normal read. Try three times */
    ++read_tries[0] ; /* statitics */
    if ( (r=FS_parse_read( buf, size, offset, pn )) >= 0 ) return r;
    ++read_tries[1] ; /* statitics */
    if ( (r=FS_parse_read( buf, size, offset, pn )) >= 0 ) return r;
    ++read_tries[2] ; /* statitics */
    r = FS_parse_read( buf, size, offset, pn ) ;
    if (r<0) syslog(LOG_INFO,"Read error on %s (size=%d)\n",path,size) ;
    return r ;
}

int FS_read_return( char *buf, const size_t size, const off_t offset , const char * src, const size_t length ) {
    size_t len = length-offset ;
    if ( offset>(off_t)length ) return -ERANGE ;
    if (len>size) len = size ;
    memcpy(buf, src + offset , len ) ;
    return len ;
}

static int FS_parse_read(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t elements = 1 ;
    int ret = 0 ;
    if ( pn->ft->ag && pn->ft->ag->combined==ag_aggregate ) elements = pn->ft->ag->elements ;
    if ( elements == 1 ) { /* read single values */
        switch( pn->ft->format ) {
        case ft_integer: {
            int i ;
            if ( offset ) return -EINVAL ;
            ret = (pn->ft->read.i)(&i,pn) ;
            if (ret < 0) return ret ;
            return FS_output_int( i , buf , size , pn ) ;
        }
        case ft_unsigned: {
            unsigned int u ;
            if ( offset ) return -EINVAL ;
            ret = (pn->ft->read.u)(&u,pn) ;
            if (ret < 0) return ret ;
            return FS_output_unsigned( u , buf , size , pn ) ;
            }
        case ft_float: {
            float f ;
            if ( offset ) return -EINVAL ;
            ret = (pn->ft->read.f)(&f,pn) ;
            if (ret < 0) return ret ;
            return FS_output_float( f , buf , size , pn ) ;
            }
        case ft_yesno: {
            int y ;
            if ( offset ) return -EINVAL ;
            if (size < 1) return -EMSGSIZE ;
            ret = (pn->ft->read.y)(&y,pn) ;
            if (ret < 0) return ret ;
            buf[0] = y ? '1' : '0' ;
            return 1 ;
            }
        case ft_ascii:
            return (pn->ft->read.a)(buf,size,offset,pn) ;
        case ft_binary:
            return (pn->ft->read.b)(buf,size,offset,pn) ;
        case ft_directory:
        case ft_subdir:
            return -ENOSYS ;
        }
    } else { /* array -- allocate off heap */
        switch( pn->ft->format ) {
        case ft_integer: {
            int * i = (int *) calloc( elements, sizeof(int) ) ;
            if ( i==NULL ) return -ENOMEM ;
            if ( offset ) {
                free( i ) ;
                return -EINVAL ;
            }
            ret = (pn->ft->read.i)(i,pn) ;
            if (ret >= 0) ret = FS_output_integer_array( i , buf , size , pn ) ;
            free( i ) ;
            return ret ;
        }
        case ft_unsigned: {
            unsigned int * u = (unsigned int *) calloc( elements, sizeof(unsigned int) ) ;
            if ( u==NULL ) return -ENOMEM ;
            if ( offset ) {
                free( u ) ;
                return -EINVAL ;
            }
            ret = (pn->ft->read.u)(u,pn) ;
            if (ret >= 0) ret = FS_output_unsigned_array( u , buf , size , pn ) ;
            free( u ) ;
            return ret ;
        }
        case ft_float: {
            float * f = (float *) calloc( elements, sizeof(float) ) ;
            if ( f==NULL ) return -ENOMEM ;
            if ( offset ) {
                free( f ) ;
                return -EINVAL ;
            }
            ret = (pn->ft->read.f)(f,pn) ;
            if (ret >= 0) ret = FS_output_float_array( f , buf , size , pn ) ;
            free( f ) ;
            return ret ;
        }
        case ft_yesno: {
            int * y = (int *) calloc( elements, sizeof(int) ) ;
            if ( y==NULL ) return -ENOMEM ;
            if ( offset ) {
                free( y ) ;
                return -EINVAL ;
            }
            ret = (pn->ft->read.y)(y,pn) ;
            if (ret >= 0) {
                int i ;
                for ( i=0 ; i<elements ; ++i ) {
                    buf[i*2] = y[i] ? '1' : '0' ;
                    if ( i<elements-1 ) buf[i*2+1] = ',' ;
                }
            }
            free( y ) ;
            return elements*2-1 ;
        }
        case ft_ascii:
            return (pn->ft->read.a)(buf,size,offset,pn) ;
        case ft_binary:
            return (pn->ft->read.b)(buf,size,offset,pn) ;
        case ft_directory:
        case ft_subdir:
            return -ENOSYS ;
        }
    }
    return -ENOENT ;
}

/* Read each array independently */
static int FS_r_all(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t left = size ;
    char * p = buf ;
    int r ;
    struct parsedname pname ;

    ++read_array ; /* statistics */
    memcpy( &pname , pn , sizeof(struct parsedname) ) ;
//printf("READALL(%p) %s size=%d\n",p,path,size) ;
    if ( offset ) return -ERANGE ;

    for ( pname.extension=0 ; pname.extension<pname.ft->ag->elements ; ++pname.extension ) {
        /* Add a separating comma if not the first element */
        if (pname.extension && pname.ft->format!=ft_binary) {
            if ( left == 0 ) return -ERANGE ;
            *p = ',' ;
            ++ p ;
            -- left ;
//printf("READALL(%p) comma\n",p) ;
        }
        if ( (r=FS_parse_read(p,left,0,&pname)) < 0 ) return r ;
        left -= r ;
        p += r ;
//printf("READALL(%p) %d->%d (%d->%d)\n",p,pname.extension,r,size,left) ;
    }
//printf("READALL return %d\n",size-left) ;
    ++ read_arraysuccess ; /* statistics */
    return size - left ;
}

/* read the combined data, and separate */
static int FS_r_split(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//    char * all ;
    int suglen = pn->ft->suglen ;
    int r ;
    size_t len ;
    int loc ;
    char * all ;
//printf("SPLIT %s\n",path) ;
    (void) offset ;
    ++ read_aggregate ; /* statistics */
    if( size < (size_t) suglen ) return -ERANGE ;

    if ( pn->ft->format == ft_binary ) {
        len = suglen * pn->ft->ag->elements ;
        loc = suglen * pn->extension ;
    } else { /* non-binary data, leave room for commas */
        len = (suglen+1) * pn->ft->ag->elements - 1 ;
        loc = (suglen+1) * pn->extension ;
    }
    all = (char *) malloc(len) ;
    if ( all==NULL ) return -ENOMEM ;
    r=FS_parse_read( all, len, 0, pn ) ;
    if ( r != len ) {
        free( all ) ;
        return -EINVAL ;
    } else {
           memcpy(buf,&all[loc],(size_t)suglen) ;
        ++ read_aggregatesuccess ; /* statistics */
        free( all ) ;
        return suglen ;
    }
}

static int FS_output_int( int value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = pn->ft->suglen ;
    char c[suglen+1] ;
    size_t len ;
    if ( suglen>size ) suglen=size ;
    len = snprintf(c,suglen+1,"%*d",suglen,value) ;
    if (len > suglen) return -EMSGSIZE ;
    memcpy( buf, c, len ) ;
    return len ;
}

static int FS_output_unsigned( unsigned int value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = pn->ft->suglen ;
    char c[suglen+1] ;
    size_t len ;
    if ( suglen>size ) suglen=size ;
    len = snprintf(c,suglen+1,"%*u",suglen,value) ;
    if (len > suglen) return -EMSGSIZE ;
    memcpy( buf, c, len ) ;
    return len ;
}

static int FS_output_float( float value, char * buf, const size_t size, const struct parsedname * pn ) {
    size_t suglen = pn->ft->suglen ;
    char c[suglen+1] ;
    size_t len ;
    if ( suglen>size ) suglen=size ;
    len = snprintf(c,suglen+1,"%*G",suglen,value) ;
    if (len > suglen) return -EMSGSIZE ;
    memcpy( buf, c, len ) ;
    return len ;
}

static int FS_output_integer_array( int * values, char * buf, const size_t size, const struct parsedname * pn ) {
    int len ;
    int left = size ;
    char * first = buf ;
    int i ;
    for ( i=0 ; i < pn->ft->ag->elements - 1 ; ++i ) {
        if ( (len=FS_output_int( values[i], first, left, pn )) < 0 ) return -EMSGSIZE ;
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

static int FS_output_unsigned_array( unsigned int * values, char * buf, const size_t size, const struct parsedname * pn ) {
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

static int FS_output_float_array( float * values, char * buf, const size_t size, const struct parsedname * pn ) {
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
