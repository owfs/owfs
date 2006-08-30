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
/* i2c support for the DS2482-100 and DS2482-800 1-wire host adapters */
/* Stolen shamelessly from Ben Gardners kernel module */
/* Actually, Dallas datasheet has the information, 
   the module code showed a nice implementation,
   the eventual format is owfs-specific (using similar primatives, data structures)
   Testing by Jan Kandziora and Daniel Höper.
 */
/**
 * ds2482.c - provides i2c to w1-master bridge(s)
 * Copyright (C) 2005  Ben Gardner <bgardner@wabtec.com>
 *
 * The DS2482 is a sensor chip made by Dallas Semiconductor (Maxim).
 * It is a I2C to 1-wire bridge.
 * There are two variations: -100 and -800, which have 1 or 8 1-wire ports.
 * The complete datasheet can be obtained from MAXIM's website at:
 *   http://www.maxim-ic.com/quick_view2.cfm/qv_pk/4382
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * MODULE_AUTHOR("Ben Gardner <bgardner@wabtec.com>");
 * MODULE_DESCRIPTION("DS2482 driver");
 * MODULE_LICENSE("GPL");
 */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

#if OW_I2C
// Header taken from lm-sensors code
// specifically lm-sensors-2.10.0
#include "i2c-dev.h"

static int DS2482_next_both(struct device_search * ds, const struct parsedname * pn) ;
static int DS2482_triple( BYTE * bits, int direction, int fd ) ;
static int DS2482_send_and_get( int fd, const BYTE wr, BYTE * rd ) ;
static int DS2482_reset( const struct parsedname * pn ) ;
static int DS2482_sendback_data( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) ;
static void DS2482_setroutines( struct interface_routines * f ) ;
static int HeadChannel( struct connection_in * in ) ;
static int CreateChannels( struct connection_in * in ) ;
static int DS2482_channel_select( const struct parsedname * pn ) ;
static int DS2482_readstatus( BYTE * c, int fd, unsigned long int min_usec, unsigned long int max_usec ) ;
static int SetConfiguration( BYTE c, struct connection_in * in ) ;
static void DS2482_close( struct connection_in * in ) ;
static int DS2482_redetect( const struct parsedname * pn ) ;
static int DS2482_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) ;

/**
 * The DS2482 registers - there are 3 registers that are addressed by a read
 * pointer. The read pointer is set by the last command executed.
 *
 * To read the data, issue a register read for any address
 */
#define DS2482_CMD_RESET               0xF0    /* No param */
#define DS2482_CMD_SET_READ_PTR        0xE1    /* Param: DS2482_PTR_CODE_xxx */
#define DS2482_CMD_CHANNEL_SELECT      0xC3    /* Param: Channel byte - DS2482-800 only */
#define DS2482_CMD_WRITE_CONFIG        0xD2    /* Param: Config byte */
#define DS2482_CMD_1WIRE_RESET         0xB4    /* Param: None */
#define DS2482_CMD_1WIRE_SINGLE_BIT    0x87    /* Param: Bit byte (bit7) */
#define DS2482_CMD_1WIRE_WRITE_BYTE    0xA5    /* Param: Data byte */
#define DS2482_CMD_1WIRE_READ_BYTE     0x96    /* Param: None */
/* Note to read the byte, Set the ReadPtr to Data then read (any addr) */
#define DS2482_CMD_1WIRE_TRIPLET       0x78    /* Param: Dir byte (bit7) */

/* Values for DS2482_CMD_SET_READ_PTR */
#define DS2482_PTR_CODE_STATUS         0xF0
#define DS2482_PTR_CODE_DATA           0xE1
#define DS2482_PTR_CODE_CHANNEL        0xD2    /* DS2482-800 only */
#define DS2482_PTR_CODE_CONFIG         0xC3

/**
 * Configure Register bit definitions
 * The top 4 bits always read 0.
 * To write, the top nibble must be the 1's compl. of the low nibble.
 */
#define DS2482_REG_CFG_1WS     0x08
#define DS2482_REG_CFG_SPU     0x04
#define DS2482_REG_CFG_PPM     0x02
#define DS2482_REG_CFG_APU     0x01

/**
 * Status Register bit definitions (read only)
 */
#define DS2482_REG_STS_DIR     0x80
#define DS2482_REG_STS_TSB     0x40
#define DS2482_REG_STS_SBR     0x20
#define DS2482_REG_STS_RST     0x10
#define DS2482_REG_STS_LL      0x08
#define DS2482_REG_STS_SD      0x04
#define DS2482_REG_STS_PPD     0x02
#define DS2482_REG_STS_1WB     0x01

/* Device-specific functions */
static void DS2482_setroutines( struct interface_routines * f ) {
    f->detect        = DS2482_detect         ;
    f->reset         = DS2482_reset          ;
    f->next_both     = DS2482_next_both      ;
//    f->overdrive = ;
//    f->testoverdrive = ;
    f->PowerByte     = DS2482_PowerByte      ;
//    f->ProgramPulse = ;
    f->sendback_data = DS2482_sendback_data  ;
    f->sendback_bits = NULL                  ;
    f->select        = NULL                  ;
    f->reconnect     = DS2482_redetect       ;
    f->close         = DS2482_close          ;
    f->transaction   = NULL                  ;
    f->flags         = ADAP_FLAG_overdrive   ;
}

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */
/* Open a DS2482 */
/* Try to see if there is a DS2482 device on the specified i2c bus */
int DS2482_detect( struct connection_in * in ) {
    int test_address[8] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, } ; // the last 4 are -800 only
    int i ;
    int fd ;
    
    /* open the i2c port */
    fd = open(in->name,O_RDWR) ;
    if ( fd < 0 ) {
        ERROR_CONNECT("Could not open i2c device %s\n",in->name) ;
        return -ENODEV ;
    }

    /* Set up low-level routines */
    DS2482_setroutines( & (in->iroutines) ) ;

    /* cycle though the possible addresses */
    for ( i=0 ; i<8 ; ++i ) {
        /* set the candidate address */
        if ( ioctl(fd,I2C_SLAVE,test_address[i]) < 0 ) {
            ERROR_CONNECT("Cound not set trial i2c address to %.2X\n",test_address[i]) ;
        } else {
            BYTE c ;
            LEVEL_CONNECT("Found an i2c device at %s address %d\n",in->name, test_address[i]) ;
            /* Provisional setup as a DS2482-100 ( 1 channel ) */
            in->connin.i2c.fd = fd ;
            in->connin.i2c.index = 0 ;
            in->connin.i2c.channels = 1 ;
            in->connin.i2c.current = 0 ;
            in->connin.i2c.head = in ;
            in->adapter_name = "DS2482-100" ;
            in->connin.i2c.i2c_address = test_address[i] ;
            in->connin.i2c.configreg = 0x00 ; // default configuration setting
#if OW_MT
            pthread_mutex_init(&(in->connin.i2c.i2c_mutex), pmattr);
#endif /* OW_MT */
            in->busmode = bus_i2c ;
            in->Adapter = adapter_DS2482_100 ;

            /* write the RESET code */
            if (
               i2c_smbus_write_byte( fd,  DS2482_CMD_RESET )  // reset
               || DS2482_readstatus(&c,fd,1,2) // pause .5 usec then read status
               || ( c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST) ) // make sure status is properly set
              ) {
                LEVEL_CONNECT("i2c device at %s address %d cannot be reset. Not a DS2482.\n",in->name, test_address[i]) ;
                continue ; ;
            }
            LEVEL_CONNECT("i2c device at %s address %d appears to be DS2482-x00\n",in->name, test_address[i]) ;
            in->connin.i2c.configchip = 0x00 ; // default configuration register after RESET
            // Note, only the lower nibble of the device config stored

            /* Now see if DS2482-100 or DS2482-800 */
            return HeadChannel(in) ;
        }
    }
    /* fellthough, no device found */
    close(fd) ;
    in->connin.i2c.fd = -1 ;
    return -ENODEV ;
}

/* Re-open a DS2482 */
static int DS2482_redetect( const struct parsedname * pn ) {
    struct connection_in * head = pn->in->connin.i2c.head ;
    int address = head->connin.i2c.i2c_address ;
    int fd ;
    
    /* open the i2c port */
    fd = open(head->name,O_RDWR) ;
    if ( fd < 0 ) {
        ERROR_CONNECT("Could not open i2c device %s\n",head->name) ;
        return -ENODEV ;
    }

    /* address is known */
    if ( ioctl(fd,I2C_SLAVE,head->connin.i2c.i2c_address) < 0 ) {
        ERROR_CONNECT("Cound not set i2c address to %.2X\n",address) ;
    } else {
        BYTE c ;
        /* write the RESET code */
        if (
           i2c_smbus_write_byte( fd,  DS2482_CMD_RESET )  // reset
           || DS2482_readstatus(&c,fd,1,2) // pause .5 usec then read status
           || ( c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST) ) // make sure status is properly set
          ) {
            LEVEL_CONNECT("i2c device at %s address %d cannot be reset. Not a DS2482.\n",head->name, address) ;
        } else {
            head->connin.i2c.current = 0 ;
            head->connin.i2c.fd = fd ;
            head->connin.i2c.configchip = 0x00 ; // default configuration register after RESET
            LEVEL_CONNECT("i2c device at %s address %d reset successfully\n",head->name, address) ;
            if ( head->connin.i2c.channels > 1 ) { // need to reset other 8 channels?
                /* loop through devices, matching those that have the same "head" */
                /* BUSLOCK also locks the sister channels for this */
                struct connection_in * in ;
                for ( in=pn->indevice ; in ; in=in->next ) {
                    if ( in==head ) in->reconnect_state = reconnect_ok ;
                }
            }
            return 0 ;
        }
    }
    /* fellthough, no device found */
    close(fd) ;
    return -ENODEV ;
}

/* read status register */
/* should already be set to read from there */
/* will read at min time, avg time, max time, and another 50% */
/* returns 0 good, 1 bad */
/* tests to make sure bus not busy */
static int DS2482_readstatus( BYTE * c, int fd, unsigned long int min_usec, unsigned long int max_usec ) {
    unsigned long int delta_usec = (max_usec-min_usec+1)/2 ;
    int i = 0 ;
    UT_delay_us( min_usec ) ; // at least get minimum out of the way
    do {
        int ret = i2c_smbus_read_byte( fd ) ;
        if ( ret < 0 ) {
            LEVEL_DEBUG("Reading DS2482 status problem min=%lu max=%lu i=%d ret=%d\n",min_usec,max_usec,i,ret) ;
            return 1 ;
        }
        if ( ( ret & DS2482_REG_STS_1WB ) == 0x00 ) {
            c[0] = (BYTE) ret ;
            LEVEL_DEBUG("DS2482 read status ok\n") ;
            return 0 ;
        }
        if ( i++ == 3 ) {
            LEVEL_DEBUG("Reading DS2482 status still busy min=%lu max=%lu i=%d ret=%d\n",min_usec,max_usec,i,ret) ;
            return 1 ;
        }
       UT_delay_us( delta_usec ) ; // increment up to three times
    } while ( 1 ) ;
}

/* uses the "Triple" primative for faster search */
static int DS2482_next_both(struct device_search * ds, const struct parsedname * pn) {
    int search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    int bit_number ;
    int last_zero = -1 ;
    int fd = pn->in->connin.i2c.head->connin.i2c.fd ;
    BYTE bits[3] ;
    int ret ;

    // initialize for search
    // if the last call was not the last one
    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    /* Make sure we're using the correct channel */
    /* Appropriate search command */
    if ( (ret=BUS_send_data(&(ds->search),1,pn)) ) return ret ;

      // loop to do the search
    for ( bit_number=0 ; bit_number<64 ; ++bit_number ) {
        LEVEL_DEBUG("DS2482 search bit number %d\n",bit_number);
        /* Set the direction bit */
        if ( bit_number < ds->LastDiscrepancy ) {
            search_direction = UT_getbit(ds->sn,bit_number);
        } else {
            search_direction = (bit_number==ds->LastDiscrepancy) ? 1 : 0 ;
        }
        /* Appropriate search command */
        if ( (ret=DS2482_triple(bits, search_direction, fd)) ) return ret ;
        if ( bits[0] || bits[1] || bits[2] ) {
            if ( bits[0] && bits[1] ) { /* 1,1 */
                break ; /* No devices respond */
            }
        } else { /* 0,0,0 */
            last_zero = bit_number ;
        }
        UT_setbit(ds->sn,bit_number,bits[2]) ;
    } // loop until through serial number bits

    if ( CRC8(ds->sn,8) || (bit_number<64) || (ds->sn[0] == 0)) {
      /* Unsuccessful search or error -- possibly a device suddenly added */
        return -EIO ;
    }
    if((ds->sn[0] & 0x7F) == 0x04) {
        /* We found a DS1994/DS2404 which require longer delays */
        pn->in->ds2404_compliance = 1 ;
    }
    // if the search was successful then

    ds->LastDiscrepancy = last_zero;
//    printf("Post, lastdiscrep=%d\n",si->LastDiscrepancy) ;
    ds->LastDevice = (last_zero < 0);
    LEVEL_DEBUG("DS2482_next_both SN found: "SNformat"\n",SNvar(ds->sn)) ;
    return 0 ;
}

/* DS2482 Reset -- A little different from DS2480B */
// return 1 shorted, 0 ok, <0 error
static int DS2482_reset( const struct parsedname * pn ) {
    BYTE c;
    int fd = pn->in->connin.i2c.head->connin.i2c.fd ;

    /* Make sure we're using the correct channel */
    if ( DS2482_channel_select(pn) ) return -1 ;

    /* write the RESET code */
    if( i2c_smbus_write_byte( fd,  DS2482_CMD_1WIRE_RESET ) ) return -1 ;

    /* wait */
    // rstl+rsth+.25 usec

    /* read status */
    if ( DS2482_readstatus( &c, fd, 1125, 1250 ) ) return -1 ; // 8 * Tslot

    pn->in->AnyDevices = (c & DS2482_REG_STS_PPD) != 0 ;
    LEVEL_DEBUG("DS2482 Reset\n");
    return 0 ;
}

static int DS2482_sendback_data( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) {
    int fd = pn->in->connin.i2c.head->connin.i2c.fd ;
    size_t i ;

    /* Make sure we're using the correct channel */
    if ( DS2482_channel_select(pn) ) return -1 ;

    for ( i = 0 ; i < len ; ++i ) {
        if ( DS2482_send_and_get( fd, data[i], &resp[i] ) ) return 1 ;
    }
    return 0 ;
}

/* Single byte -- assumes channel selection already done */
static int DS2482_send_and_get( int fd, const BYTE wr, BYTE * rd ) {
    int read_back ;
    BYTE c ;

    /* Write data byte */
    if (i2c_smbus_write_byte_data(fd, DS2482_CMD_1WIRE_WRITE_BYTE, wr) < 0)
       return 1;

    /* read status for done */
    if ( DS2482_readstatus( &c, fd, 530, 585 ) ) return -1 ;

    /* Select the data register */
    if (i2c_smbus_write_byte_data(fd,DS2482_CMD_SET_READ_PTR,DS2482_PTR_CODE_DATA)<0) return 1 ;

    /* Read the data byte */
    read_back = i2c_smbus_read_byte(fd);

    if ( read_back < 0 ) return 1 ;
    rd[0] = (BYTE) read_back ;

    return 0 ;
}

static int HeadChannel( struct connection_in * in ) {
    struct parsedname pn ;

    /* Intentionally put the wrong index */
    in->connin.i2c.index = 1 ;
    pn.in = in ;
    if ( DS2482_channel_select(&pn) ) { /* Couldn't switch */
        in->connin.i2c.index = 0 ; /* restore correct value */
        LEVEL_CONNECT("DS2482-100 (Single channel).");
        return 0 ; /* happy as DS2482-100 */
    }
    LEVEL_CONNECT("DS2482-800 (Eight channels).");
    /* Must be a DS2482-800 */
    in->connin.i2c.channels = 8 ;
    in->Adapter = adapter_DS2482_800 ;
    return CreateChannels(in) ;
}
        
/* create more channels,
   inserts in connection_in chain
   "in" points to  first (head) channel
   called only for DS12482-800
   NOTE: coded assuming num = 1 or 8 only
 */
static int CreateChannels( struct connection_in * in ) {
    int i ;
    char * name[] = { "DS2482-800(0)", "DS2482-800(1)", "DS2482-800(2)", "DS2482-800(3)", "DS2482-800(4)", "DS2482-800(5)", "DS2482-800(6)", "DS2482-800(7)", } ;
    in->connin.i2c.index = 0 ;
    in->adapter_name = name[0] ;
    for ( i=1 ; i<8 ; ++i ) {
        struct connection_in * added = NewIn(in) ;
        if ( added==NULL ) return -ENOMEM ;
        added->connin.i2c.index = i ;
        added->adapter_name = name[i] ;
    }
    return 0 ;
}

static int DS2482_triple( BYTE * bits, int direction, int fd ) {
    /* 3 bits in bits */
    BYTE c ;

    LEVEL_DEBUG("-> TRIPLET attempt direction %d\n",direction);
    /* Write TRIPLE command */
    if (i2c_smbus_write_byte_data(fd, DS2482_CMD_1WIRE_TRIPLET, direction ? 0xFF : 0) < 0)
       return 1;

    /* read status */
    if ( DS2482_readstatus( &c, fd, 198, 219 ) ) return -1 ; // 250usec = 3*Tslot

    bits[0] = (c & DS2482_REG_STS_SBR ) != 0 ;
    bits[1] = (c & DS2482_REG_STS_TSB ) != 0 ;
    bits[2] = (c & DS2482_REG_STS_DIR ) != 0 ;
    LEVEL_DEBUG("<- TRIPLET %d %d %d\n",bits[0],bits[1],bits[2]);
    return 0 ;
}

static int DS2482_channel_select( const struct parsedname * pn ) {
    struct connection_in * head = pn->in->connin.i2c.head ;
    int chan = pn->in->connin.i2c.index ;
    int fd = head->connin.i2c.fd ;
    BYTE config = pn->in->connin.i2c.configreg ;
    int read_back ;
    /**
     * Write and verify codes for the CHANNEL_SELECT command (DS2482-800 only).
     * To set the channel, write the value at the index of the channel.
     * Read and compare against the corresponding value to verify the change.
     */
    static const BYTE W_chan[8] =
    { 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87 };
    static const BYTE R_chan[8] =
    { 0xB8, 0xB1, 0xAA, 0xA3, 0x9C, 0x95, 0x8E, 0x87 };

    if ( fd < 0 ) {
        LEVEL_CONNECT("Calling a closed i2c channel (%d)\n",chan) ;
        return -EINVAL ;
    }

    /* Already properly selected? */
    /* All `100 (1 channel) will be caught here */
    if ( chan == head->connin.i2c.current ) return 0 ;

    /* Select command */
    if (i2c_smbus_write_byte_data(fd, DS2482_CMD_CHANNEL_SELECT, W_chan[chan] ) < 0 ) return -EIO ; 
    
    /* Read back and confirm */
    read_back = i2c_smbus_read_byte(fd) ;
    if ( read_back < 0 ) return -ENODEV ;
    if ( ((BYTE)read_back) != R_chan[chan]) return -ENODEV ;

    /* Set the channel in head */
    head->connin.i2c.current = pn->in->connin.i2c.index ;

    /* Now check the configuration register */
    /* This is since configuration is per chip, not just channel */
    if ( config != head->connin.i2c.configchip ) return SetConfiguration( config, pn->in ) ;

    return 0 ;
}

/* Set the configuration register, both for this channel, and for head global data */
/* Note, config is stored as only the lower nibble */
static int SetConfiguration( BYTE c, struct connection_in * in ) {
    struct connection_in * head = in->connin.i2c.head ;
    int fd = head->connin.i2c.fd ;
    int read_back ;

    /* Write, readback, and compare configuration register */
    if ( i2c_smbus_write_byte_data( fd, DS2482_CMD_WRITE_CONFIG, c|(~(c<<4)) )
        || (read_back=i2c_smbus_read_byte(fd))<0
        || ((BYTE)read_back != c )
    ) {
        head->connin.i2c.configchip = 0xFF ; // bad value to trigger retry
        LEVEL_CONNECT("Trouble changing DS2482 configuration register\n") ;
        return -EINVAL ;
    }
    /* Clear the strong pull-up power bit(register is automatically cleared by reset) */
    in->connin.i2c.configreg = head->connin.i2c.configchip = c & ~DS2482_REG_CFG_SPU ;
    return 0 ;
}

static int DS2482_PowerByte(const BYTE byte, BYTE * resp, const UINT delay, const struct parsedname * pn) {
    /* Make sure we're using the correct channel */
    if ( DS2482_channel_select(pn) ) return -1 ;

    /* Set the power (bit is automatically cleared by reset) */
    if ( SetConfiguration( pn->in->connin.i2c.configreg | DS2482_REG_CFG_SPU, pn->in ) ) return -1 ; 

    /* send and get byte (and trigger strong pull-up */
    if ( DS2482_send_and_get( pn->in->connin.i2c.head->connin.i2c.fd, byte, resp ) ) return -1 ;

    UT_delay( delay ) ;

    return 0 ;
}

static void DS2482_close( struct connection_in * in ) {
    struct connection_in * head ;
    if ( in == NULL ) return ;
    head = in->connin.i2c.head ;
    if ( head->connin.i2c.fd < 0 ) return ;
    close( head->connin.i2c.fd ) ;
    head->connin.i2c.fd = -1 ;
}
#endif /* OW_I2C */
