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

#include <sys/stat.h>
#include <string.h>

#include "owfs_config.h"
#include "ow.h"

/* ------- Prototypes ----------- */
static int FS_real_write(const char * const path, const char * const buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
static int FS_w_all(const char * const buf, const size_t size, const off_t offset , const struct parsedname * const pn) ;
static int FS_w_split(const char * const buf, const size_t size, const off_t offset , const struct parsedname * const pn) ;
static int FS_parse_write(const char * const buf, const size_t size, const off_t offset , const struct parsedname * const pn) ;

static int FS_input_yesno( int * const result, const char * const buf, const size_t size ) ;
static int FS_input_integer( int * const result, const char * const buf, const size_t size ) ;
static int FS_input_unsigned( unsigned int * const result, const char * const buf, const size_t size ) ;
static int FS_input_float( FLOAT * const result, const char * const buf, const size_t size ) ;
static int FS_input_date( DATE * const result, const char * const buf, const size_t size ) ;

static int FS_input_yesno_array( int * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) ;
static int FS_input_unsigned_array( unsigned int * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) ;
static int FS_input_integer_array( int * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) ;
static int FS_input_float_array( FLOAT * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) ;
static int FS_input_date_array( DATE * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) ;

/* ---------------------------------------------- */
/* Filesystem callback functions                  */
/* ---------------------------------------------- */

/* Note on return values: */
/* Top level FS_write will return size if ok, else a negative number */
/* Each lower level function called will return 0 if ok, else non-zero */

/* Note on size and offset: */
/* Buffer length (and requested data) is size bytes */
/* writing should start after offset bytes in original data */
/* only binary, and ascii data support offset in single data points */
/* only binary supports offset in array data */
/* size and offset are vetted against specification data size and calls */
/*   outside of this module will not have buffer overflows */
/* I.e. the rest of owlib can trust size and buffer to be legal */

/* return size if ok, else negative */
int FS_write(const char *path, const char *buf, const size_t size, const off_t offset) {
    struct parsedname pn ;
    struct stateinfo si ;
    int r ;
    pn.si = &si ;
//printf("WRITE path=%s size=%d offset=%d\n",path,(int)size,(int)offset);

    /* if readonly exit */
    if ( readonly ) return -EROFS ;

    STATLOCK
        AVERAGE_IN(&write_avg)
        AVERAGE_IN(&all_avg)
    STATUNLOCK
    if ( FS_ParsedName( path , &pn ) ) {
        r = -ENOENT;
    } else if ( pn.dev==NULL || pn.ft == NULL ) {
        r = -EISDIR ;
    } else if (pn.type == pn_structure ) { /* structure is read-only */
        r = -ENOTSUP ;
    } else {
        STATLOCK
            ++ write_calls ; /* statistics */
        STATUNLOCK
        LockGet(&pn) ;
        r = FS_real_write( path, buf, size, offset, &pn ) ;
        LockRelease(&pn) ;

        if ( offset  || r ) {
            Cache_Del( &pn ) ;
        } else {
    //printf("Write adding %s\n",path) ;
            Cache_Add( buf, size, &pn ) ;
        }
        if ( r == 0 ) {
            STATLOCK
                ++write_success ; /* statistics */
                write_bytes += size ; /* statistics */
            STATUNLOCK
        }
    }

    FS_ParsedName_destroy(&pn) ;
    STATLOCK
        AVERAGE_OUT(&write_avg)
        AVERAGE_OUT(&all_avg)
    STATUNLOCK
    return (r) ? r : size ; /* here's where the size is used! */
}

/* return 0 if ok */
static int FS_real_write(const char * const path, const char * const buf, const size_t size, const off_t offset, const struct parsedname * pn) {
    int r ;
//printf("REAL_WRITE\n");

    /* Writable? */
    if ( (pn->ft->write.v) == NULL ) return -ENOTSUP ;

    /* Do we exist? Only test static cases */
    if ( presencecheck && pn->ft->change==ft_static && CheckPresence(pn) ) return -ENOENT ;

    /* Array properties? Write all together if aggregate */
    if ( pn->ft->ag && pn->extension==-1 && pn->ft->ag->combined==ag_separate ) return FS_w_all(buf,size,offset,pn) ;
    if ( pn->ft->ag && pn->extension>=0 && pn->ft->ag->combined==ag_aggregate ) return FS_w_split(buf,size,offset,pn) ;

    /* Norml write. Triplicate attempt */
    STATLOCK
        ++ write_tries[0] ; /* statistics */
    STATUNLOCK
    if ( FS_parse_write( buf, size, offset, pn ) == 0 ) return 0;
    STATLOCK
        ++ write_tries[1] ; /* statistics */
    STATUNLOCK
    if ( FS_parse_write( buf, size, offset, pn ) == 0 ) return 0;
    STATLOCK
        ++ write_tries[2] ; /* statistics */
    STATUNLOCK
    r = FS_parse_write( buf, size, offset, pn ) ;
    if (r) syslog(LOG_INFO,"Write error on %s (size=%d)\n",path,(int)size) ;
    return r ;
}

/* return 0 if ok */
static int FS_parse_write(const char * const buf, const size_t size, const off_t offset , const struct parsedname * const pn) {
    size_t elements = 1 ;
    int ret ;
    /* We will allocate memory for array variables off heap, but not single vars for efficiency */
//printf("PARSE_WRITE\n");
    if ( pn->ft->ag && ( pn->ft->ag->combined==ag_aggregate || ( pn->ft->ag->combined==ag_mixed && pn->extension==-1 ) ) )
        elements = pn->ft->ag->elements ;
    switch( pn->ft->format ) {
    case ft_integer: {
        int I ;
        int * i = &I ;
        if ( offset ) return -EADDRNOTAVAIL ;
        if ( elements>1 ) {
            i = (int *) calloc( elements , sizeof(int) ) ;
            if ( i==NULL ) return -ENOMEM ;
            ret = FS_input_integer_array( i, buf, size, pn ) ;
        } else {
            ret = FS_input_integer( i, buf, size ) ;
        }
        ret |= (pn->ft->write.i)(i,pn) ;
        if ( elements>1) free(i) ;
        return ret ;
    }
    case ft_unsigned: {
        unsigned int U ;
        unsigned int * u = &U ;
        if ( offset ) return -EADDRNOTAVAIL ;
        if ( elements>1 ) {
            u = (unsigned int *) calloc( elements , sizeof(unsigned int) ) ;
            if ( u==NULL ) return -ENOMEM ;
            ret = FS_input_unsigned_array( u, buf, size, pn ) ;
        } else {
            ret = FS_input_unsigned( u, buf, size ) ;
        }
        ret |= (pn->ft->write.u)(u,pn) ;
        if ( elements>1) free(u) ;
        return ret ;
    }
    case ft_float: {
        FLOAT F ;
        FLOAT * f = &F ;
        if ( offset ) return -EADDRNOTAVAIL ;
        if ( elements>1 ) {
            f = (FLOAT *) calloc( elements , sizeof(FLOAT) ) ;
            if ( f==NULL ) return -ENOMEM ;
            ret = FS_input_float_array( f, buf, size, pn ) ;
        } else {
            ret = FS_input_float( f, buf, size ) ;
        }
        ret |= (pn->ft->write.f)(f,pn) ;
        if ( elements>1) free(f) ;
        return ret ;
    }
    case ft_date: {
        DATE D ;
        DATE * d = &D ;
        if ( offset ) return -EADDRNOTAVAIL ;
        if ( elements>1 ) {
            d = (DATE *) calloc( elements , sizeof(DATE) ) ;
            if ( d==NULL ) return -ENOMEM ;
            ret = FS_input_date_array( d, buf, size, pn ) ;
        } else {
            ret = FS_input_date( d, buf, size ) ;
        }
        ret |= (pn->ft->write.d)(d,pn) ;
        if ( elements>1) free(d) ;
        return ret ;
    }
    case ft_yesno: {
        int Y ;
        int * y = &Y ;
        if ( offset ) return -EADDRNOTAVAIL ;
        if ( elements>1 ) {
            y = (int *) calloc( elements , sizeof(int) ) ;
            if ( y==NULL ) return -ENOMEM ;
            ret = FS_input_yesno_array( y, buf, size, pn ) ;
        } else {
            ret = FS_input_yesno( y, buf, size ) ;
        }
        ret |= (pn->ft->write.y)(y,pn) ;
        if ( elements>1) free(y) ;
        return ret ;
    }
    case ft_ascii:
        {
            size_t s = FileLength(pn) ;
            if ( offset > s ) return -ERANGE ;
            s -= offset ;
            if ( s > size ) s = size ;
            return (pn->ft->write.a)(buf,s,offset,pn) ;
        }
    case ft_binary:
        {
            size_t s = FileLength(pn) ;
            if ( offset > s ) return -ERANGE ;
            s -= offset ;
            if ( s > size ) s = size ;
            return (pn->ft->write.b)(buf,s,offset,pn) ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    }
    return -EINVAL ; /* unknown data type */
}

/* Non-combined input  field, so treat  as several separate tranactions */
/* return 0 if ok */
static int FS_w_all(const char * const buf, const size_t size, const off_t offset , const struct parsedname * const pn) {
    size_t left = size ;
    const char * p = buf ;
    int r ;
    struct parsedname pname ;
//printf("WRITE_ALL\n");

    STATLOCK
        ++ write_array ; /* statistics */
    STATUNLOCK
    memcpy( &pname , pn , sizeof(struct parsedname) ) ;
//printf("WRITEALL(%p) %s\n",p,path) ;
    if ( offset ) return -ERANGE ;

    if ( pname.ft->format==ft_binary ) { /* handle binary differently, no commas */
        int suglen = pname.ft->suglen ;
        for ( pname.extension=0 ; pname.extension < pname.ft->ag->elements ; ++pname.extension ) {
            if ( (int) left < suglen ) return -ERANGE ;
            if ( (r=FS_parse_write(p,(size_t) suglen,0,&pname)) ) return r ;
            p += suglen ;
            left -= suglen ;
        }
    } else { /* comma separation */
        for ( pname.extension=0 ; pname.extension < pname.ft->ag->elements ; ++pname.extension ) {
            char * c = memchr( p , ',' , left ) ;
            if ( c==NULL ) {
                if ( (r=FS_parse_write(p,left,0,&pname)) ) return r ;
                p = buf + size ;
                left = 0 ;
            } else {
                if ( (r=FS_parse_write(p,(size_t)(c-p),0,&pname)) ) return r ;
                p = c + 1 ;
                left = size - (buf-p) ;
            }
        }
    }
    return 0 ;
}

/* Combined field, so read all, change the relevant field, and write back */
/* return 0 if ok */
static int FS_w_split(const char * const buf, const size_t size, const off_t offset , const struct parsedname * const pn) {
    size_t elements = pn->ft->ag->elements ;
    int ret = 0;
//printf("WRITE_SPLIT\n");

    (void) offset ;

    switch( pn->ft->format ) {
    case ft_yesno:
        if ( offset ) {
            return -EADDRNOTAVAIL ;
        } else {
            int * y = (int *) calloc( elements , sizeof(int) ) ;
                if ( y==NULL ) return -ENOMEM ;
                ret = ((pn->ft->read.y)(y,pn)<0) || FS_input_yesno(&y[pn->extension],buf,size) || (pn->ft->write.y)(y,pn)  ;
            free( y ) ;
            break ;
        }
    case ft_integer:
        if ( offset ) {
            return -EADDRNOTAVAIL ;
        } else {
            int * i = (int *) calloc( elements , sizeof(int) ) ;
                if ( i==NULL ) return -ENOMEM ;
                ret = ((pn->ft->read.i)(i,pn)<0) || FS_input_integer(&i[pn->extension],buf,size) || (pn->ft->write.i)(i,pn) ;
            free( i ) ;
            break ;
        }
    case ft_unsigned:
        if ( offset ) {
            return -EADDRNOTAVAIL ;
        } else {
            int * u = (unsigned int *) calloc( elements , sizeof(unsigned int) ) ;
                if ( u==NULL ) return -ENOMEM ;
                ret = ((pn->ft->read.u)(u,pn)<0) || FS_input_unsigned(&u[pn->extension],buf,size) || (pn->ft->write.u)(u,pn) ;
            free( u ) ;
            break ;
        }
    case ft_float:
        if ( offset ) {
            return -EADDRNOTAVAIL ;
        } else {
            FLOAT * f = (FLOAT *) calloc( elements , sizeof(FLOAT) ) ;
                if ( f==NULL ) return -ENOMEM ;
                ret = ((pn->ft->read.f)(f,pn)<0) || FS_input_float(&f[pn->extension],buf,size) || (pn->ft->write.f)(f,pn) ;
            free(f) ;
            break ;
        }
    case ft_date: {
        if ( offset ) {
            return -EADDRNOTAVAIL ;
        } else {
            DATE * d = (DATE *) calloc( elements , sizeof(DATE) ) ;
                if ( d==NULL ) return -ENOMEM ;
                ret = ((pn->ft->read.d)(d,pn)<0) || FS_input_date(&d[pn->extension],buf,size) || (pn->ft->write.d)(d,pn) ;
            free(d) ;
            break ;
        }
    case ft_binary: {
        unsigned char * all ;
        int suglen = pn->ft->suglen ;
        size_t s = suglen ;
        size_t len = suglen*elements ;
        if ( offset > suglen ) return -ERANGE ;
        s -= offset ;
        if ( s>size ) s = size ;
        if ( (all = (unsigned char *) malloc( len ) ) ) { ;
            if ( (ret = (pn->ft->read.b)(all,len,0,pn))==0 ) {
                memcpy(&all[suglen*pn->extension+offset],buf,s) ;
                ret = (pn->ft->write.b)(all,len,0,pn) ;
            }
            free( all ) ;
        } else {
            return -ENOMEM ;
        }
        break ;
        }
    case ft_ascii:
        if ( offset ) {
            return -EADDRNOTAVAIL ;
        } else {
            char * all ;
            int suglen = pn->ft->suglen ;
            size_t s = suglen ;
            size_t len = (suglen+1)*elements - 1 ;
            if ( s>size ) s = size ;
            if ( (all=(char *) malloc(len)) ) {
                if ((ret = (pn->ft->read.a)(all,len,0,pn))==0 ) {
                    memcpy(&all[(suglen+1)*pn->extension],buf,s) ;
                    ret = (pn->ft->write.a)(all,len,0,pn) ;
                }
                free( all ) ;
            } else
                return -ENOMEM ;
            }
            break ;
        }
    case ft_directory:
    case ft_subdir:
        return -ENOSYS ;
    }
    return ret ? -EINVAL : 0 ;
}

/* return 0 if ok */
static int FS_input_yesno( int * const result, const char * const buf, const size_t size ) {
//printf("yesno size=%d, buf=%s\n",size,buf);
    if ( size ) {
        if ( buf[0]=='1' || strncasecmp("on",buf,2)==0 || strncasecmp("yes",buf,2)==0 ) {
            *result = 1 ;
//printf("YESno\n");
            return 0 ;
        }
        if ( buf[0]=='0' || strncasecmp("off",buf,2)==0 || strncasecmp("no",buf,2)==0 ) {
            *result = 0 ;
//printf("yesNO\n") ;
            return 0 ;
        }
    }
    return 1 ;
}

/* return 0 if ok */
static int FS_input_integer( int * const result, const char * const buf, const size_t size ) {
    char cp[size+1] ;
    char * end ;

    memcpy( cp, buf, size ) ;
    cp[size] = '\0' ;
    errno = 0 ;
    * result = strtol( cp,&end,10) ;
    return end==cp || errno ;
}

/* return 0 if ok */
static int FS_input_unsigned( unsigned int * const result, const char * const buf, const size_t size ) {
    char cp[size+1] ;
    char * end ;

    memcpy( cp, buf, size ) ;
    cp[size] = '\0' ;
    errno = 0 ;
    * result = strtoul( cp,&end,10) ;
//printf("UI str=%s, val=%u\n",cp,*result) ;
    return end==cp || errno ;
}

/* return 0 if ok */
static int FS_input_float( FLOAT * const result, const char * const buf, const size_t size ) {
    char cp[size+1] ;
    char * end ;

    memcpy( cp, buf, size ) ;
    cp[size] = '\0' ;
    errno = 0 ;
    * result = strtod( cp,&end) ;
    return end==cp || errno ;
}

/* return 0 if ok */
static int FS_input_date( DATE * const result, const char * const buf, const size_t size ) {
    struct tm tm ;
    if ( size==0 || buf[0]=='\0' ) {
        *result = time(NULL) ;
    } else if ( strptime(buf,"%a %b %d %T %Y",&tm) || strptime(buf,"%b %d %T %Y",&tm) || strptime(buf,"%c",&tm) || strptime(buf,"%D %T",&tm) ) {
        *result = mktime(&tm) ;
    } else {
        return -EINVAL ;
    }
    return 0 ;
}

/* returns 0 if ok */
static int FS_input_yesno_array( int * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) {
    int i ;
    int last = pn->ft->ag->elements - 1 ;
    const char * first ;
    const char * end = buf + size - 1 ;
    const char * next = buf ;
    for ( i=0 ; i<=last ; ++i ) {
        if ( next <= end ) {
            first = next ;
            if ( (next=memchr( first, ',' , first-end+1 )) == NULL ) next = end ;
            if ( FS_input_yesno( &results[i], first, next-first ) ) results[i]=0 ;
            ++next ; /* past comma */
        } else { /* assume "no" for absent values */
            results[i] = 0 ;
        }
    }
    return 0 ;
}

/* returns number of valid integers, or negative for error */
static int FS_input_integer_array( int * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) {
    int i ;
    int last = pn->ft->ag->elements - 1 ;
    const char * first ;
    const char * end = buf + size - 1 ;
    const char * next = buf ;
    for ( i=0 ; i<=last ; ++i ) {
        if ( next <= end ) {
            first = next ;
            if ( (next=memchr( first, ',' , first-end+1 )) == NULL ) next = end ;
            if ( FS_input_integer( &results[i], first, next-first ) ) results[i]=0 ;
            ++next ; /* past comma */
        } else { /* assume 0 for absent values */
            results[i] = 0 ;
        }
    }
    return 0 ;
}

/* returns 0, or negative for error */
static int FS_input_unsigned_array( unsigned int * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) {
    int i ;
    int last = pn->ft->ag->elements - 1 ;
    const char * first ;
    const char * end = buf + size - 1 ;
    const char * next = buf ;
    for ( i=0 ; i<=last ; ++i ) {
        if ( next <= end ) {
            first = next ;
            if ( (next=memchr( first, ',' , first-end+1 )) == NULL ) next = end ;
            if ( FS_input_unsigned( &results[i], first, next-first ) ) results[i]=0 ;
            ++next ; /* past comma */
        } else { /* assume 0 for absent values */
            results[i] = 0 ;
        }
    }
    return 0 ;
}

/* returns 0, or negative for error */
static int FS_input_float_array( FLOAT * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) {
    int i ;
    int last = pn->ft->ag->elements - 1 ;
    const char * first ;
    const char * end = buf + size - 1 ;
    const char * next = buf ;
    for ( i=0 ; i<=last ; ++i ) {
        if ( next <= end ) {
            first = next ;
            if ( (next=memchr( first, ',' , first-end+1 )) == NULL ) next = end ;
            if ( FS_input_float( &results[i], first, next-first ) ) results[i]=0. ;
            ++next ; /* past comma */
        } else { /* assume 0. for absent values */
            results[i] = 0. ;
        }
    }
    return 0 ;
}

/* returns 0, or negative for error */
static int FS_input_date_array( DATE * const results, const char * const buf, const size_t size, const struct parsedname * const pn ) {
    int i ;
    int last = pn->ft->ag->elements - 1 ;
    const char * first ;
    const char * end = buf + size - 1 ;
    const char * next = buf ;
    DATE now = time(NULL) ;
    for ( i=0 ; i<=last ; ++i ) {
        if ( next <= end ) {
            first = next ;
            if ( (next=memchr( first, ',' , first-end+1 )) == NULL ) next = end ;
            if ( FS_input_date( &results[i], first, next-first ) ) results[i]=now ;
            ++next ; /* past comma */
        } else { /* assume now for absent values */
            results[i] = now ;
        }
    }
    return 0 ;
}
