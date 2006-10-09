/* File: ow.i */
/* $Id$ */

%module OW

%include "typemaps.i"

%init %{
#include "../../../src/include/config.h"
#include "owfs_config.h"
#include "ow.h"
LibSetup(opt_swig) ;
%}

%{
#include "../../../src/include/config.h"
#include "owfs_config.h"
#include "ow.h"

#if OW_MT
    pthread_t main_threadid ;
    #define IS_MAINTHREAD (main_threadid == pthread_self())
#else /* OW_MT */
    #define IS_MAINTHREAD 1
#endif /* OW_MT */

char *version( ) {
    return VERSION;
}


int init( const char * dev ) {
    int ret = 1 ; /* Good initialization */
    //    LibSetup(opt_swig) ; /* Done in %init section */
    if ( OWLIB_can_init_start() || owopt_packed(dev) ) {
        ret = 0 ; // error
    } else {
        LibStart() ;
    }
    OWLIB_can_init_end() ;
    return ret ; 
}

int put( const char * path, const char * value ) {
    int s ;
    int ret = 1 ; /* good result */
    /* Overall flag for valid setup */
    if ( value==NULL) return 0 ;
    if ( OWLIB_can_access_start() ) {
        ret = 0 ;
    } else {
        s = strlen(value) ;
        if ( FS_write(path,value,s,0) ) ret = 0 ;
    }
    OWLIB_can_access_end() ;
    return ret ;
}


#ifdef NO_NESTED_FUNCTIONS

#if OW_MT
pthread_mutex_t owigetmutex = PTHREAD_MUTEX_INITIALIZER ;
#endif /* OW_MT */

char * owigetbuf = NULL ;
int owigetsz ; /* current buffer size */
int owigets ; /* current string length */

void owigetdirectory( const struct parsedname * const pn2 ) {
    int sn = owigets+OW_FULLNAME_MAX+2 ; /* next buffer limit */
    if ( owigetsz<sn ) {
        void * temp = owigetbuf ;
        owigetsz = sn ;
        owigetbuf = realloc( temp, sn ) ;
        //printf("Realloc buf pointer=%p,%p\n",buf,&buf);
        if ( owigetbuf==NULL && temp ) free(temp) ;
    }
    if ( owigetbuf ) {
        if ( owigets ) strcpy( &owigetbuf[owigets++], "," ) ;
        FS_DirName( &owigetbuf[owigets], OW_FULLNAME_MAX, pn2 ) ;
        if ( IsDir(pn2) ) strcat( &owigetbuf[owigets], "/" );
        owigets = strlen( owigetbuf ) ;
        //printf("buf=%s len=%d\n", owigetbuf, owigets);
    }
}

char * get( const char * path ) {
#if OW_MT
    pthread_mutex_lock(&owigetmutex) ;
#endif /* OW_MT */
    struct parsedname pn ;

    if ( OWLIB_can_access_start() ) { /* Prior init */
        // owigetbuf = NULL ;
    } else if ( FS_ParsedName( path, &pn ) ) { /* Parse the input string */
        // owigetbuf = NULL ;
    } else {
        //printf("path=%s dev=%p ft=%p subdir=%p format=%d\n",pathcpy,pn.dev,pn.ft,pn.subdir,pn.ft?pn.ft->format:-1) ;

//        if ( pn.dev==NULL || pn.ft == NULL || pn.subdir ) { /* A directory of some kind */
        if ( IsDir(&pn) ) { /* A directory of some kind */
            //printf("Directory\n");
            owigets=owigetsz=0 ;
            FS_dir( owigetdirectory, &pn ) ;
        } else { /* A regular file */
            //printf("File %s\n",path);
            owigets = FullFileLength(&pn) ;
            //printf("File len=%d, %s\n",owigets,path);
            if ( (owigetbuf=(char *) malloc( owigets+1 )) ) {
                int r =  FS_read_postparse( owigetbuf, owigets, 0, &pn ) ;
                if ( r<0 ) {
                    free(owigetbuf) ;
                    owigetbuf = NULL;
                } else {
                    owigetbuf[owigets] = '\0' ;
                    // if (r!=owigets) printf("Mismatch path=%s read=%d len=%d\n",path,r,owigets);
                }
            }
        }
        //printf("End GET\n");
        FS_ParsedName_destroy(&pn) ;
        if(!owigetbuf) owigetbuf = strdup("") ; // have to return empty string on error
    }
    OWLIB_can_access_end() ;
#if OW_MT
    pthread_mutex_unlock(&owigetmutex) ;
#endif /* OW_MT */
    return owigetbuf ;
}

#else /* NO_NESTED_FUNCTIONS */

char * get( const char * path ) {
    struct parsedname pn ;
    char * buf = NULL ;
    int sz ; /* current buffer size */
    int s ; /* current string length */
    /* Embedded callback function */
    void directory( const struct parsedname * const pn2 ) {
        int sn = s+OW_FULLNAME_MAX+2 ; /* next buffer limit */
        if ( sz<sn ) {
            void * temp = buf ;
            sz = sn ;
            buf = realloc( temp, sn ) ;
            //printf("Realloc buf pointer=%p,%p\n",buf,&buf);
            if ( buf==NULL && temp ) free(temp) ;
        }
        if ( buf ) {
            if ( s ) strcpy( &buf[s++], "," ) ;
            FS_DirName( &buf[s], OW_FULLNAME_MAX, pn2 ) ;
            if ( IsDir(pn2) ) strcat( &buf[s], "/" );
            s = strlen( buf ) ;
            //printf("buf=%s len=%d\n",buf,s);
        }
    }
    
    if ( OWLIB_can_access_start() ) { /* Prior init */
        // buf = NULL ;
    } else if ( FS_ParsedName( path, &pn ) ) { /* Parse the input string */
        // buf = NULL ;
    } else {
        //printf("path=%s dev=%p ft=%p subdir=%p format=%d\n",pathcpy,pn.dev,pn.ft,pn.subdir,pn.ft?pn.ft->format:-1) ;

//        if ( pn.dev==NULL || pn.ft == NULL || pn.subdir ) { /* A directory of some kind */
        if ( IsDir(&pn) ) { /* A directory of some kind */
            //printf("Directory\n");
            s=sz=0 ;
            FS_dir( directory, &pn ) ;
        } else { /* A regular file */
            //printf("File %s\n",path);
            s = FullFileLength(&pn) ;
            //printf("File len=%d, %s\n",s,path);
            if ( (buf=(char *) malloc( s+1 )) ) {
                int r =  FS_read_postparse( buf, s, 0, &pn ) ;
                if ( r<0 ) {
                    free(buf) ;
                    buf = NULL;
                } else {
                    buf[s] = '\0' ;
                    // if (r!=s) printf("Mismatch path=%s read=%d len=%d\n",path,r,s);
                }
            }
        }
        //printf("End GET\n");
        FS_ParsedName_destroy(&pn) ;
        if(!buf) buf = strdup("") ; // have to return empty string on error
    }
    OWLIB_can_access_end() ;
    return buf ;
}

#endif /* NO_NESTED_FUNCTIONS */


void finish( void ) {
    OWLIB_can_finish_start() ;
    LibClose() ;
    OWLIB_can_finish_end() ;
}

%}
%typemap(newfree) char * { if ($1) free($1) ; }
%newobject get ;

extern char *version( );
extern int init( const char * dev ) ;
extern char * get( const char * path ) ;
extern int put( const char * path, const char * value ) ;
extern void finish( void ) ;

//extern int error_print;
//extern int error_level ;
