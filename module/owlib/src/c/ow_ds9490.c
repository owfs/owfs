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

/* DS9490R-W USB 1-Wire master

   USB parameters:
       Vendor ID: 04FA
       ProductID: 2490

   Dallas controller DS2490

*/

#ifdef OW_USB /* conditional inclusion of USB */

#include "owfs_config.h"
#include "ow.h"
#include <usb.h>

static int DS9490_reset( void ) ;
static int DS9490wait(unsigned char * const buffer) ;
void DS9490_setroutines( struct interface_routines * const f ) ;
static int DS9490_next_both(unsigned char * serialnumber, unsigned char search) ;
static int DS9490_sendback_bit( const unsigned char obit , unsigned char * const ibit ) ;
//static int DS9490_sendback_byte( const unsigned char obyte , unsigned char * const ibyte ) ;
static int DS9490_read_bits( unsigned char * const bits , const int length ) ;
static int DS9490_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length ) ;
static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len ) ;
static int DS9490_level(int new_level) ;

#define TIMEOUT_USB	5000 /* 5 seconds */

#define CONTROL_CMD     0x00
#define COMM_CMD        0x01
#define MODE_CMD        0x02
#define TEST_CMD        0x03

#define CTL_RESET_DEVICE        0x0000
#define CTL_START_EXE           0x0001
#define CTL_RESUME_EXE          0x0002
#define CTL_HALT_EXE_IDLE       0x0003
#define CTL_HALT_EXE_DONE       0x0004
#define CTL_FLUSH_COMM_CMDS     0x0007
#define CTL_FLUSH_CV_BUFFER     0x0008
#define CTL_FLUSH_CMT_BUFFER    0x0009
#define CTL_GET_COMM_CMDS       0x000A

#define MOD_PULSE_EN            0x0000
#define MOD_SPEED_CHANGE_EN     0x0001
#define MOD_1WIRE_SPEED         0x0002
#define MOD_STRONG_PU_DURATION  0x0003
#define MOD_PULLDOWN_SLEWRATE   0x0004
#define MOD_PROG_PULSE_DURATION 0x0005
#define MOD_WRITE1_LOWTIME      0x0006
#define MOD_DSOW0_TREC          0x0007

/** EP1 -- control read */
#define DS2490_EP1              0x81
/** EP2 -- bulk write */
#define DS2490_EP2              0x02
/** EP3 -- bulk read */
#define DS2490_EP3              0x83

/* Open a DS9097 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS9490_detect( void ) {
    struct usb_bus *bus ;
    struct usb_device *dev ;
    int usbnum = 0 ;

    /* Set up low-level routines */
    DS9097_setroutines( & iroutines ) ;

    usb_init() ;
    usb_find_busses() ;
    usb_find_devices() ;
    for ( bus = usb_get_busses() ; bus ; bus=bus->next ) {
        for ( dev=bus->devices ; dev ; dev=dev->next ) {
            if ( dev->descriptor.idVendor==0x04FA && dev->descriptor.idProduct==0x2490 ) {
                if ( ++usbnum < useusb ) {
                    syslog(LOG_INFO,"USB 2490 adapter at %s/%s passed over.\n",bus->dirname,dev->filename) ;
                } else if ( (devport=(char *)malloc(strlen(bus->dirname)+strlen(dev->filename)+2) ) == NULL ) {
                    return -ENOMEM ;
                } else {
                    strcpy(devport,bus->dirname) ;
                    strcat(devport,"/") ;
                    strcat(devport,dev->filename) ;
                    if ( (devusb=usb_open(dev)) ) {
                        if ( usb_set_configuration( devusb, 1 )==0 && usb_claim_interface( devusb, 0)==0 ) {
                            if ( usb_set_altinterface( devusb, 3)==0 ) {
                                syslog(LOG_INFO,"Opened USB 2490 adapter at %s.\n",devport) ;
                                DS9490_setroutines( & iroutines ) ;
                                Version2480 = 8 ; /* dummy value */
                                return DS9490_reset() ;
                            }
                            usb_release_interface( devusb, 0) ;
                        }
                        usb_close( devusb ) ;
                        devusb = 0 ;
                    }
                    syslog(LOG_INFO,"Failed to open/configure USB 2490 adapter at %s.\n",devport) ;
                    return 1 ;
                }
            }
        }
    }
    syslog(LOG_INFO,"No USB 2490 adapter found\n") ;
    return 1 ;
}

void DS9490_close(void) {
    if ( devusb ) {
        usb_release_interface( devusb, 0) ;
        usb_close( devusb ) ;
        devusb = 0 ;
    }
}

static int DS9490wait(unsigned char * const buffer) {
    int ret ;
    do {
        if ( (ret=usb_bulk_read(devusb,DS2490_EP1,buffer,32,TIMEOUT_USB)) < 0 ) return ret ;
    } while ( !(buffer[8]&0x20) ) ;
//{
//int i ;
//printf ("USBwait return buffer:\n") ;
//for( i=0;i<8;++i) printf("%.2X: %.2X  |  %.2X: %.2X  ||  %.2X: %.2X  |  %.2X: %.2X\n",i,buffer[i],i+8,buffer[i+8],i+16,buffer[i+16],i+24,buffer[i+24]);
//}
    return 0 ;
}

    /* Reset the bus */
static int DS9490_reset( void ) {
    int ret ;
    unsigned char buffer[32] ;
//printf("9490RESET\n");
    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0043, 0x0000, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
    ) {
        return ret ;
    }
//    USBpowered = (buffer[8]&0x08) == 0x08 ;
    AnyDevices = !(buffer[16]&0x01) ;
//printf("9490RESET=0 anydevices=%d\n",AnyDevices);
    return 0 ;
}

static int DS9490_sendback_bit( const unsigned char obit , unsigned char * const ibit ) {
    int ret ;
    unsigned char buffer[32] ;

    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0021 | (obit<<3), 0x0000, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) return ret ;

    if ( usb_bulk_read(devusb,DS2490_EP3,ibit,0x1, TIMEOUT_USB ) > 0 ) {
//printf("USB bit %.2X->%.2X\n",obit,*ibit) ;
        return 0 ;
    }
//printf("DS9490_sendback_bit error \n");
    usb_clear_halt(devusb,DS2490_EP3) ;
    return -EIO ;
}

static int DS9490_read_bits( unsigned char * const bits , const int length ) {
    int i ;
    int ret ;
    for ( i=0 ; i<length ; ++i ) {
        if ( (ret=DS9490_sendback_bit(0xFF,&bits[i])) ) return ret ;
//printf("READBIT %d: ->%.2X\n",i,bits[i]);
    }
    return 0 ;
}

static int DS9490_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length ) {
    int i ;
    int ret ;
    for ( i=0 ; i<length ; ++i ) {
        if ( (ret=DS9490_sendback_bit(outbits[i]&0x01,&inbits[i])) ) return ret ;
        inbits[i] &= 0x01 ;
//printf("SENDBIT %d: %.2X->%.2X\n",i,outbits[i],inbits[i]);
    }
    return 0 ;
}

/*
static int DS9490_sendback_byte( const unsigned char obyte , unsigned char * const ibyte ) {
    int ret ;
    unsigned char buffer[32] ;

    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0053, 0x0000|obyte, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) return ret ;

    if ( usb_bulk_read(devusb,DS2490_EP3,ibyte,0x1, TIMEOUT_USB ) > 0 ) {
//printf("USB byte %.2X->%.2X\n",obyte,*ibyte) ;
        return 0 ;
    }
    usb_clear_halt(devusb,DS2490_EP3) ;
    return -EIO ;
}
*/

static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len ) {
    int ret ;
    unsigned char buffer[32] ;

    if ( len > USB_FIFO_EACH ) {
//printf("USBsendback splitting\n");
        return DS9490_sendback_data(data,resp,USB_FIFO_EACH)
            || DS9490_sendback_data(&data[USB_FIFO_EACH],&resp[USB_FIFO_EACH],len-USB_FIFO_EACH) ;
    }

    if ( (ret=usb_bulk_write(devusb,DS2490_EP2,data,len, TIMEOUT_USB )) < len ) {
//printf("USBsendback bulk write problem = %d\n",ret);
        usb_clear_halt(devusb,DS2490_EP2) ;
        return -EIO ;
    }

    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0075, len, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) {
//printf("USBsendback control problem\n");
        return ret ;
    }

    if ( usb_bulk_read(devusb,DS2490_EP3,resp,len, TIMEOUT_USB ) > 0 ) return 0 ;
//printf("USBsendback bulk read problem\n");
    usb_clear_halt(devusb,DS2490_EP3) ;
    return -EIO ;
}

static int DS9490_next_both(unsigned char * serialnumber, unsigned char search) {
    unsigned char buffer[32] ;
    int ret ;
    int i ;

//printf("DS9490_next_both SN in: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
    // if the last call was not the last one
    if ( !AnyDevices ) LastDevice = 1 ;
    if ( LastDevice ) return -ENODEV ;

    /** Play LastDescrepancy games with bitstream */
    memcpy( combuffer,serialnumber,8) ; /* set bufferto zeros */
    if ( LastDiscrepancy > -1 ) UT_setbit(combuffer,LastDiscrepancy,1) ;
    for ( i=LastDiscrepancy+1;i<64;i++) {
        UT_setbit(combuffer,i,0) ;
    }
//printf("DS9490_next_both EP2: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",combuffer[0],combuffer[1],combuffer[2],combuffer[3],combuffer[4],combuffer[5],combuffer[6],combuffer[7]) ;

//printf("USBnextboth\n");
    if ( (ret=usb_bulk_write(devusb,DS2490_EP2,combuffer,8, TIMEOUT_USB )) < 8 ) {
//printf("USBnextboth bulk write problem = %d\n",ret);
        usb_clear_halt(devusb,DS2490_EP2) ;
        return -EIO ;
    }

    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x48FD, 0x0100|search, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) {
//printf("USBnextboth control problem\n");
        return ret ;
    }

    if ( (ret=usb_bulk_read(devusb,DS2490_EP3,combuffer,16, TIMEOUT_USB )) >0 ) {
        memcpy(serialnumber,combuffer,8) ;
//printf("DS9490_next_both SN out: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
//printf("DS9490_next_both rest: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",combuffer[8],combuffer[9],combuffer[10],combuffer[11],combuffer[12],combuffer[13],combuffer[14],combuffer[15]) ;
//        LastDevice = !(combuffer[8]||combuffer[9]||combuffer[10]||combuffer[11]||combuffer[12]||combuffer[13]||combuffer[14]||combuffer[15]) ;
        LastDevice = (ret==8) ;
//printf("DS9490_next_both lastdevice=%d bytes=%d\n",LastDevice,ret) ;

        for ( i=63 ; i>=0 ; i-- ) {
            if ( UT_getbit(combuffer,i+64) && (UT_getbit(combuffer,i)==0) ) {
                LastDiscrepancy = i ;
//printf("DS9490_next_both lastdiscrepancy=%d\n",LastDiscrepancy) ;
                break ;
            }
        }

        return CRC8(serialnumber,8) || (serialnumber[0] == 0) ? -EIO:0 ;
    }
//printf("USBnextboth bulk read problem error=%d\n",ret);
    usb_clear_halt(devusb,DS2490_EP3) ;
    return -EIO ;
}

/* Set the 1-Wire Net line level.  The values for new_level are
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:    0 GOOD, !0 Error
   Actually very simple for passive adapter
*/
/* return 0=good
  -EIO not supported
 */
static int DS9490_level(int new_level) {
    int ret ;
    unsigned char buffer[32] ;
    int lev ;
    if (new_level == ULevel) {     // check if need to change level
        return 0 ;
    }
    switch (new_level) {
    case MODE_NORMAL:
        lev = 0x0000;
        break ;
    case MODE_BREAK:
        return 1 ;
    case MODE_STRONG5:
        lev = 0x0002 ;
        break ; ;
    case MODE_PROGRAM:
        lev = 0x0001 ;
        break ;
    default:
        return 1 ;
    }

    if ( (ret=usb_control_msg(devusb,0x40,MODE_CMD,MOD_PULSE_EN, lev, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) return ret ;

    ULevel = new_level ;
    return 0 ;
}

void DS9490_setroutines( struct interface_routines * const f ) {
    f->write = NULL ;
    f->read  = NULL ;
    f->reset = DS9490_reset ;
    f->next_both = DS9490_next_both ;
    f->level = DS9490_level ;
    f->PowerByte = NULL ;
    f->ProgramPulse = NULL ;
    f->sendback_data = DS9490_sendback_data ;
}

#endif /* OW_USB */
