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
#include "ow.h"

/* hex-digit to number */
unsigned char char2num( const char * s ) {
    /* This is much faster compared to switch() */
    if((*s <= '9') && (*s >= '0')) return (*s) - '0' ;
    if((*s <= 'F') && (*s >= 'A')) return 10 + (*s) - 'A' ;
    if((*s <= 'f') && (*s >= 'a')) return 10 + (*s) - 'a' ;
    return 0 ;
}

/* 2 hex digits to number */
unsigned char string2num( const char * s ) {
    if ( s == NULL ) return 0 ;
    return char2num(&s[0])<<4 | char2num(&s[1]) ;
}

/* number to a hex digit */
char num2char( const unsigned char n ) {
    switch(n) {
    case  0: return '0' ;
    case  1: return '1' ;
    case  2: return '2' ;
    case  3: return '3' ;
    case  4: return '4' ;
    case  5: return '5' ;
    case  6: return '6' ;
    case  7: return '7' ;
    case  8: return '8' ;
    case  9: return '9' ;
    case 10: return 'A' ;
    case 11: return 'B' ;
    case 12: return 'C' ;
    case 13: return 'D' ;
    case 14: return 'E' ;
    case 15: return 'F' ;
    default: return ' ' ;
    }
}

/* number to 2 hex digits */
void num2string( char * s , const unsigned char n ) {
    s[0] = num2char((unsigned char)(n>>4)) ;
    s[1] = num2char((unsigned char)(n&0xF)) ;
}

/* 2x hex digits to x number bytes */
void string2bytes( const char * str , unsigned char * b , const int bytes ) {
    int i ;
    for ( i=0 ; i<bytes ; ++i ) b[i]=string2num(&str[i<<1]) ;
}
/* number(x bytes) to 2x hex digits */
void bytes2string( char * str , const unsigned char * b , const int bytes ) {
    int i ;
    for ( i=0 ; i<bytes ; ++i ) num2string(&str[i<<1],b[i]) ;
}

// #define UT_getbit(buf, loc)	( ( (buf)[(loc)>>3]>>((loc)&0x7) ) &0x01 )
int UT_getbit(const unsigned char * buf, const int loc) {
    return ( ( (buf[loc>>3]) >> (loc&0x7) ) & 0x01 ) ;
}
int UT_get2bit(const unsigned char * buf, const int loc) {
    return ( ( (buf[loc>>2]) >> ((loc&0x3)<<1) ) & 0x03 ) ;
}
void UT_setbit( unsigned char * buf, const int loc , const int bit ) {
    if (bit) {
        buf[loc>>3] |= 0x01 << (loc&0x7) ;
    } else {
        buf[loc>>3] &= ~( 0x01 << (loc&0x7) ) ;
    }
}
void UT_set2bit( unsigned char * buf, const int loc , const int bits ) {
    unsigned char * p = &buf[loc>>2] ;
    switch (loc&3) {
    case 0 :
        *p = (*p & 0xFC) | bits ;
        return ;
    case 1 :
        *p = (*p & 0xF3) | (bits<<2) ;
        return ;
    case 2 :
        *p = (*p & 0xCF) | (bits<<4) ;
        return ;
    case 3 :
        *p = (*p & 0x3F) | (bits<<6) ;
        return ;
    }
}

void UT_fromDate( const DATE d, unsigned char * data) {
    data[0] = d & 0xFF ;
    data[1] = (d>>8) & 0xFF ;
    data[2] = (d>>16) & 0xFF ;
    data[3] = (d>>24) & 0xFF ;
}

DATE UT_toDate( const unsigned char * data ) {
    return (((((((unsigned int) data[3])<<8)|data[2])<<8)|data[1])<<8)|data[0] ;
}

#include <features.h>
#if defined(__UCLIBC__)
 #if (__UCLIBC_MAJOR__ << 16)+(__UCLIBC_MINOR__ << 8)+(__UCLIBC_SUBLEVEL__) <= 0x000913
/*
  uClibc older than 0.9.19 is missing tdestroy() (don't know exactly when
  it was added) I added a replacement to this, just to be able to compile
  owfs for WRT54G without any patched uClibc.
*/

typedef struct node_t {
    void        *key;
    struct node_t *left, *right;
} node;

static void tdestroy_recurse_ (node *root, void *freefct) {
    if (root->left != NULL)
        tdestroy_recurse_ (root->left, freefct);
    if (root->right != NULL)
        tdestroy_recurse_ (root->right, freefct);
    //(*freefct) ((void *) root->key);
    free((void *) root->key);
    /* Free the node itself.  */
    free (root);
}

void tdestroy_(void *vroot, void *freefct) {
    node *root = (node *) vroot;
    if (root != NULL) {
        tdestroy_recurse_ (root, freefct);
    }
}
 #endif /* Older than 0.9.19 */
#endif /* __UCLIBC__ */
