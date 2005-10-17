/* File: ow.i */
/* $Id$ */

%module OW

%include "typemaps.i"

%init %{
#include "owfs_config.h"
#include "ow.h"
LibSetup() ;
%}

%{
#include "owfs_config.h"
#include "ow.h"

#ifdef OW_MT
    pthread_t main_threadid ;
#define IS_MAINTHREAD (main_threadid == pthread_self())

    pthread_mutex_t capi_mutex ;
#define CAPILOCK       pthread_mutex_lock(  &capi_mutex   )
#define CAPIUNLOCK     pthread_mutex_unlock(  &capi_mutex   )
#else /* OW_MT */
#define IS_MAINTHREAD 1
    
#define CAPIINITLOCK
#define CAPILOCK
#define CAPIUNLOCK
#endif /* OW_MT */

char *version( ) {
    return VERSION;
}


int init( const char * dev ) {
//    LibSetup() ;
    background = 0 ;
    pid_file = 0 ;
    if ( !strcmp(dev,"u") ) {
        OW_ArgUSB(NULL) ;
    } else {
        OW_ArgGeneric( dev ) ;
    }
    if ( LibStart() ) return 0 ;
//printf("Libstart good\n");
    return 1 ; /* Good initialization */
}

int put( const char * path, const char * value ) {
    int s ;
    int indev ;
    /* Check for prior init */
    CAPILOCK ;
        indev = indevices ;
    CAPIUNLOCK ;
    if ( indev==0 ) return 0 ;
    /* Overall flag for valid setup */
    if ( value==NULL) return 0 ;
    s = strlen(value) ;
    if ( FS_write(path,value,s,0) ) return 0 ;
    return 1 ;
}

char * get( const char * path ) {
    struct parsedname pn ;
    struct stateinfo si ;
    char * buf = NULL ;
    int sz ; /* current buffer size */
    int s ; /* current string length */
    int indev ;
    /* Embedded callback function */
    void directory( const struct parsedname * const pn2 ) {
        int sn = s+OW_FULLNAME_MAX+2 ; /* next buffer limit */
        if ( sz<sn ) {
            sz = sn ;
            buf = realloc( buf, sn ) ;
//printf("Realloc buf pointer=%p,%p\n",buf,&buf);
        }
        if ( buf ) {
            if ( s ) strcpy( &buf[s++], "," ) ;
            FS_DirName( &buf[s], OW_FULLNAME_MAX, pn2 ) ;
            if (
                pn2->dev ==NULL
                || pn2->ft ==NULL
                || pn2->ft->format ==ft_subdir
                || pn2->ft->format ==ft_directory
            ) strcat( &buf[s], "/" );
            s = strlen( buf ) ;
//printf("buf=%s len=%d\n",buf,s);
        }
    }
    /* Check for prior init */
    CAPILOCK ;
        indev = indevices ;
    CAPIUNLOCK ;
    if ( indev==0 ) return NULL ;

    /* Parse the input string */
    pn.si = &si ;
    if ( FS_ParsedName( path, &pn ) ) return NULL ;
//printf("path=%s dev=%p ft=%p subdir=%p format=%d\n",pathcpy,pn.dev,pn.ft,pn.subdir,pn.ft?pn.ft->format:-1) ;

    if ( pn.dev==NULL || pn.ft == NULL || pn.subdir ) { /* A directory of some kind */
//printf("Directory\n");
        s=sz=0 ;
        FS_dir( directory, &pn ) ;
    } else { /* A regular file */
//printf("File %s\n",path);
        s = FS_size_postparse(&pn) ;
//printf("File len=%d, %s\n",s,path);
        if ( (buf=(char *) malloc( s+1 )) ) {
            int r =  FS_read_3times( buf, s, 0, &pn ) ;
            if ( r<0 ) {
                free(buf) ;
                buf = NULL;
            } else {
                buf[s] = '\0' ;
//              if (r!=s) printf("Mismatch path=%s read=%d len=%d\n",path,r,s);
            }
        }
    }
//printf("End GET\n");
    FS_ParsedName_destroy(&pn) ;
    if(!buf) return strdup("") ; // have to return empty string on error
    return buf ;
}

void finish( void ) {
    LibClose() ;
}

%}
%typemap(newfree) char * { if ($1) free($1) ; }
%newobject get ;

extern char *version( );
extern int init( const char * dev ) ;
extern char * get( const char * path ) ;
extern int put( const char * path, const char * value ) ;
extern void finish( void ) ;

extern int error_print;
extern int error_level ;
