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
static int DS9490_sendback_byte( const unsigned char obyte , unsigned char * const ibyte ) ;
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

/* Open a DS9097 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
int DS9490_detect( int useusb ) {
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
                } else {
                    if ( (devusb=usb_open(dev)) ) {
                        if ( usb_set_configuration( devusb, 1 )==0 && usb_claim_interface( devusb, 0)==0 ) {
                            if ( usb_set_altinterface( devusb, 0)==0 ) {
                                syslog(LOG_INFO,"Opened USB 2490 adapter at %s/%s.\n",bus->dirname,dev->filename) ;
                                DS9490_setroutines( & iroutines ) ;
                                return DS9490_reset() ;
                            }
                            usb_release_interface( devusb, 0) ;
                        }
                        usb_close( devusb ) ;
                        devusb = 0 ;
                    }
                    syslog(LOG_INFO,"Failed to open/configure USB 2490 adapter at %s/%s.\n",bus->dirname,dev->filename) ;
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
        if ( (ret=usb_bulk_read(devusb,0x81,buffer,32,TIMEOUT_USB)) < 0 ) return ret ;
    } while ( (buffer[8]&0x20)==0 ) ;
{
int i ;
printf ("USBwait return buffer:\n") ;
for( i=0;i<8;++i) printf("%.2X: %.2X  |  %.2X: %.2X  ||  %.2X: %.2X  |  %.2X: %.2X\n",i,buffer[i],i+8,buffer[i+8],i+16,buffer[i+16],i+24,buffer[i+24]);
}
    return 0 ;
}

    /* Reset the bus */
static int DS9490_reset( void ) {
    int ret ;
    unsigned char buffer[32] ;
printf("9490RESET\n");
    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0043, 0x0000, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
    ) return ret ;

//    USBpowered = (buffer[8]&0x08) == 0x08 ;
    Version2480 = 8 ; /* dummy value */
    AnyDevices = !(buffer[16]&0x01) ;
printf("9490RESET=0\n");
    return 0 ;
}

static int DS9490_sendback_bit( const unsigned char obit , unsigned char * const ibit ) {
    int ret ;
    unsigned char buffer[32] ;

    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0021 | (obit<<3), 0x0000, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) return ret ;

    if ( usb_bulk_read(devusb,0x83,ibit,0x1, TIMEOUT_USB ) != -1 ) return 0 ;
    usb_clear_halt(devusb,0x83) ;
    return -EIO ;
}

static int DS9490_read_bits( unsigned char * const bits , const int length ) {
    int i ;
    int ret ;
    for ( i=0 ; i<length ; ++i ) {
        if ( (ret=DS9490_sendback_bit(0xFF,&bits[i])) ) return ret ;
printf("READBIT %d: ->%.2X\n",i,bits[i]);
    }
    return 0 ;
}

static int DS9490_sendback_bits( const unsigned char * const outbits , unsigned char * const inbits , const int length ) {
    int i ;
    int ret ;
    for ( i=0 ; i<length ; ++i ) {
        if ( (ret=DS9490_sendback_bit(outbits[i],&inbits[i])) ) return ret ;
printf("SENDBIT %d: %.2X->%.2X\n",i,outbits[i],inbits[i]);
    }
    return 0 ;
}

static int DS9490_sendback_data( const unsigned char * const data , unsigned char * const resp , const int len ) {
    int i ;
    int ret ;
    for ( i=0 ; i<len ; ++i ) {
        if ( (ret=DS9490_sendback_byte(data[i],&resp[i])) ) return ret ;
printf("SENDDATA %d: %.2X->%.2X\n",i,data[i],resp[i]);
    }
    return 0 ;
}

static int DS9490_sendback_byte( const unsigned char obyte , unsigned char * const ibyte ) {
    int ret ;
    unsigned char buffer[32] ;

    if ( (ret=usb_control_msg(devusb,0x40,COMM_CMD,0x0053, 0x0000|obyte, NULL, 0, TIMEOUT_USB ))<0
         ||
         (ret=DS9490wait(buffer))
          ) return ret ;

    if ( usb_bulk_read(devusb,0x83,ibyte,0x1, TIMEOUT_USB ) != -1 ) return 0 ;
    usb_clear_halt(devusb,0x83) ;
    return -EIO ;
}

static int DS9490_next_both(unsigned char * serialnumber, unsigned char search) {
    unsigned char search_direction = 0 ; /* set to 0 just to forestall compiler error, initialization not really needed */
    unsigned char bit_number ;
    unsigned char last_zero ;
    unsigned char bits[3] ;
    int ret ;

    // initialize for search
    last_zero = 0;

    // if the last call was not the last one
    if ( !AnyDevices ) LastDevice = 1 ;
    if ( LastDevice ) return -ENODEV ;

    /* Appropriate search command */
    if ( (ret=BUS_send_data(&search,1)) ) return ret ;
      // loop to do the search
    for ( bit_number=0; 1 ; ++bit_number ) {
        bits[1] = bits[2] = 0xFF ;
        if ( bit_number==0 ) {
            if ( (ret=DS9490_sendback_bits(&bits[1],&bits[1],2)) ) return ret ;
        } else {
            bits[0] = search_direction ;
            if ( bit_number<64 ) {
                if ( (ret=DS9490_sendback_bits(bits,bits,3)) ) return ret ;
            } else {
                if ( (ret=DS9490_sendback_bits(bits,bits,1)) ) return ret ;
                break ;
            }
        }
        if ( bits[1] ) {
            if ( bits[2] ) {
                break ; /* No devices respond */
            } else {
                search_direction = 1;  // bit write value for search
            }
        } else {
            if ( bits[2] ) {
                search_direction = 0;  // bit write value for search
            } else  if (bit_number < LastDiscrepancy) {
                // if this discrepancy if before the Last Discrepancy
                // on a previous next then pick the same as last time
                search_direction = UT_getbit(serialnumber,bit_number) ;
            } else if (bit_number == LastDiscrepancy) {
                search_direction = 1 ; // if equal to last pick 1, if not then pick 0
            } else {
                search_direction = 0 ;
                // if 0 was picked then record its position in LastZero
                last_zero = bit_number;
                // check for Last discrepancy in family
                if (last_zero < 9) LastFamilyDiscrepancy = last_zero;
            }
        }
        UT_setbit(serialnumber,bit_number,search_direction) ;

        // serial number search direction write bit
        //if ( (ret=DS9097_sendback_bits(&search_direction,bits,1)) ) return ret ;
    } // loop until through serial number bits

    if ( CRC8(serialnumber,8) || (bit_number<64) || (serialnumber[0] == 0)) return -EIO ;
      // if the search was successful then

    LastDiscrepancy = last_zero;
    LastDevice = (last_zero == 0);
    return 0 ;
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
