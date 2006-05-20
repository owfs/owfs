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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

// Special for i2c work
#include <sys/ioctl.h>
// Header taken from lm-sensors code
// specifically lm-sensors-2.10.0
#include "i2c-dev.h"

static int DS2482_next_both(struct device_search * ds, const struct parsedname * pn) ;
static int DS2482_triple( BYTE * bits, int direction, const struct parsedname * pn ) ;
static int DS2482_send_and_get( int fd, const BYTE * wr, BYTE * rd ) ;
static int DS2482_reset( const struct parsedname * pn ) ;
static int DS2482_sendback_data( const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn ) ;
static void DS2482_setroutines( struct interface_routines * f ) ;
static int HeadChannel( struct connection_in * in ) ;
static int CreateChannels( struct connection_in * in ) ;
static int DS2482_channel_select( const struct parsedname * pn ) ;
static int DS2482_readstatus( BYTE * c, int fd, unsigned long int min_usec, unsigned long int max_usec ) ;

/**
 * The DS2482 registers - there are 3 registers that are addressed by a read
 * pointer. The read pointer is set by the last command executed.
 *
 * To read the data, issue a register read for any address
 */
#define DS2482_CMD_RESET       0xF0    /* No param */
#define DS2482_CMD_SET_READ_PTR        0xE1    /* Param: DS2482_PTR_CODE_xxx */
#define DS2482_CMD_CHANNEL_SELECT  0xC3    /* Param: Channel byte - DS2482-800 only */
#define DS2482_CMD_WRITE_CONFIG        0xD2    /* Param: Config byte */
#define DS2482_CMD_1WIRE_RESET     0xB4    /* Param: None */
#define DS2482_CMD_1WIRE_SINGLE_BIT    0x87    /* Param: Bit byte (bit7) */
#define DS2482_CMD_1WIRE_WRITE_BYTE    0xA5    /* Param: Data byte */
#define DS2482_CMD_1WIRE_READ_BYTE 0x96    /* Param: None */
/* Note to read the byte, Set the ReadPtr to Data then read (any addr) */
#define DS2482_CMD_1WIRE_TRIPLET   0x78    /* Param: Dir byte (bit7) */

/* Values for DS2482_CMD_SET_READ_PTR */
#define DS2482_PTR_CODE_STATUS     0xF0
#define DS2482_PTR_CODE_DATA       0xE1
#define DS2482_PTR_CODE_CHANNEL        0xD2    /* DS2482-800 only */
#define DS2482_PTR_CODE_CONFIG     0xC3

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
    f->PowerByte     = BUS_PowerByte_low     ;
//    f->ProgramPulse = ;
    f->sendback_data = DS2482_sendback_data  ;
    f->sendback_bits = NULL                  ;
    f->select        = BUS_select_low        ;
    f->reconnect     = NULL                  ;
    f->close         = COM_close             ;
}

/* uses the "Triple" primative for faster search */
static int DS2482_next_both(struct device_search * ds, const struct parsedname * pn) {
    int search_direction = 0 ; /* initialization just to forestall incorrect compiler warning */
    int bit_number ;
    int last_zero = -1 ;
    BYTE bits[3] ;
    int ret ;

    // initialize for search
    // if the last call was not the last one
    if ( !pn->in->AnyDevices ) ds->LastDevice = 1 ;
    if ( ds->LastDevice ) return -ENODEV ;

    /* Make sure we're using the correct channel */
    if ( DS2482_channel_select(pn) ) return -EIO ;

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
        if ( (ret=DS2482_triple(bits, search_direction, pn)) ) return ret ;
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

/* i2c support for the DS2482-100 and DS2482-800 1-wire host adapters */
/* Stolen shamelessly from Ben Gardners kernel module */

#if 0
// Header taken from lm-sensors code
#include "i2c-dev.h"
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

#include <linux/i2c.h>

/**
 * Address is selected using 2 pins, resulting in 4 possible addresses.
 *  0x18, 0x19, 0x1a, 0x1b
 * However, the chip cannot be detected without doing an i2c write,
 * so use the force module parameter.
 */
static unsigned short normal_i2c[] = {I2C_CLIENT_END};

/**
 * Insmod parameters
 */
I2C_CLIENT_INSMOD_1(ds2482);


static int ds2482_attach_adapter(struct i2c_adapter *adapter);
static int ds2482_detect(struct i2c_adapter *adapter, int address, int kind);
static int ds2482_detach_client(struct i2c_client *client);


/**
 * Driver data (common to all clients)
 */
static struct i2c_driver ds2482_driver = {
   .driver = {
       .owner  = THIS_MODULE,
       .name   = "ds2482",
   },
   .attach_adapter = ds2482_attach_adapter,
   .detach_client  = ds2482_detach_client,
};

/*
 * Client data (each client gets its own)
 */

struct ds2482_data;

struct ds2482_w1_chan {
   struct ds2482_data  *pdev;
   u8          channel;
   struct w1_bus_master    w1_bm;
};

struct ds2482_data {
   struct i2c_client   client;
   struct semaphore    access_lock;

   /* 1-wire interface(s) */
   int         w1_count;   /* 1 or 8 */
   struct ds2482_w1_chan   w1_ch[8];

   /* per-device values */
   u8          channel;
   u8          read_prt;   /* see DS2482_PTR_CODE_xxx */
   u8          reg_config;
};


/**
 * Sets the read pointer.
 * @param pdev     The ds2482 client pointer
 * @param read_ptr see DS2482_PTR_CODE_xxx above
 * @return -1 on failure, 0 on success
 */
static inline int ds2482_select_register(struct ds2482_data *pdev, u8 read_ptr)
{
   if (pdev->read_prt != read_ptr) {
       if (i2c_smbus_write_byte_data(&pdev->client,
                         DS2482_CMD_SET_READ_PTR,
                         read_ptr) < 0)
           return -1;

       pdev->read_prt = read_ptr;
   }
   return 0;
}

/**
 * Sends a command without a parameter
 * @param pdev The ds2482 client pointer
 * @param cmd  DS2482_CMD_RESET,
 *     DS2482_CMD_1WIRE_RESET,
 *     DS2482_CMD_1WIRE_READ_BYTE
 * @return -1 on failure, 0 on success
 */
static inline int ds2482_send_cmd(struct ds2482_data *pdev, u8 cmd)
{
   if (i2c_smbus_write_byte(&pdev->client, cmd) < 0)
       return -1;

   pdev->read_prt = DS2482_PTR_CODE_STATUS;
   return 0;
}

/**
 * Sends a command with a parameter
 * @param pdev The ds2482 client pointer
 * @param cmd  DS2482_CMD_WRITE_CONFIG,
 *     DS2482_CMD_1WIRE_SINGLE_BIT,
 *     DS2482_CMD_1WIRE_WRITE_BYTE,
 *     DS2482_CMD_1WIRE_TRIPLET
 * @param byte The data to send
 * @return -1 on failure, 0 on success
 */
static inline int ds2482_send_cmd_data(struct ds2482_data *pdev,
                      u8 cmd, u8 byte)
{
   if (i2c_smbus_write_byte_data(&pdev->client, cmd, byte) < 0)
       return -1;

   /* all cmds leave in STATUS, except CONFIG */
   pdev->read_prt = (cmd != DS2482_CMD_WRITE_CONFIG) ?
            DS2482_PTR_CODE_STATUS : DS2482_PTR_CODE_CONFIG;
   return 0;
}


/*
 * 1-Wire interface code
 */

#define DS2482_WAIT_IDLE_TIMEOUT   100

/**
 * Waits until the 1-wire interface is idle (not busy)
 *
 * @param pdev Pointer to the device structure
 * @return the last value read from status or -1 (failure)
 */
static int ds2482_wait_1wire_idle(struct ds2482_data *pdev)
{
   int temp = -1;
   int retries = 0;

   if (!ds2482_select_register(pdev, DS2482_PTR_CODE_STATUS)) {
       do {
           temp = i2c_smbus_read_byte(&pdev->client);
       } while ((temp >= 0) && (temp & DS2482_REG_STS_1WB) &&
            (++retries > DS2482_WAIT_IDLE_TIMEOUT));
   }

   if (retries > DS2482_WAIT_IDLE_TIMEOUT)
       printk(KERN_ERR "%s: timeout on channel %d\n",
              __func__, pdev->channel);

   return temp;
}

/**
 * Selects a w1 channel.
 * The 1-wire interface must be idle before calling this function.
 *
 * @param pdev     The ds2482 client pointer
 * @param channel  0-7
 * @return     -1 (failure) or 0 (success)
 */
static int ds2482_set_channel(struct ds2482_data *pdev, u8 channel)
{
   if (i2c_smbus_write_byte_data(&pdev->client, DS2482_CMD_CHANNEL_SELECT,
                     ds2482_chan_wr[channel]) < 0)
       return -1;

   pdev->read_prt = DS2482_PTR_CODE_CHANNEL;
   pdev->channel = -1;
   if (i2c_smbus_read_byte(&pdev->client) == ds2482_chan_rd[channel]) {
       pdev->channel = channel;
       return 0;
   }
   return -1;
}


/**
 * Performs the triplet function, which reads two bits and writes a bit.
 * The bit written is determined by the two reads:
 *   00 => dbit, 01 => 0, 10 => 1
 *
 * @param data The ds2482 channel pointer
 * @param dbit The direction to choose if both branches are valid
 * @return b0=read1 b1=read2 b3=bit written
 */
static u8 ds2482_w1_triplet(void *data, u8 dbit)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int status = (3 << 5);

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the triplet command, wait until 1WB == 0, return the status */
   if (!ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_TRIPLET,
                 dbit ? 0xFF : 0))
       status = ds2482_wait_1wire_idle(pdev);

   up(&pdev->access_lock);

   /* Decode the status */
   return (status >> 5);
}

/**
 * Performs the write byte function.
 *
 * @param data The ds2482 channel pointer
 * @param byte The value to write
 */
static void ds2482_w1_write_byte(void *data, u8 byte)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the write byte command */
   ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_WRITE_BYTE, byte);

   up(&pdev->access_lock);
}

/**
 * Performs the read byte function.
 *
 * @param data The ds2482 channel pointer
 * @return The value read
 */
static u8 ds2482_w1_read_byte(void *data)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int result;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the read byte command */
   ds2482_send_cmd(pdev, DS2482_CMD_1WIRE_READ_BYTE);

   /* Wait until 1WB == 0 */
   ds2482_wait_1wire_idle(pdev);

   /* Select the data register */
   ds2482_select_register(pdev, DS2482_PTR_CODE_DATA);

   /* Read the data byte */
   result = i2c_smbus_read_byte(&pdev->client);

   up(&pdev->access_lock);

   return result;
}


/**
 * Sends a reset on the 1-wire interface
 *
 * @param data The ds2482 channel pointer
 * @return 0=Device present, 1=No device present or error
 */
static u8 ds2482_w1_reset_bus(void *data)
{
   struct ds2482_w1_chan *pchan = data;
   struct ds2482_data    *pdev = pchan->pdev;
   int err;
   u8 retval = 1;

   down(&pdev->access_lock);

   /* Select the channel */
   ds2482_wait_1wire_idle(pdev);
   if (pdev->w1_count > 1)
       ds2482_set_channel(pdev, pchan->channel);

   /* Send the reset command */
   err = ds2482_send_cmd(pdev, DS2482_CMD_1WIRE_RESET);
   if (err >= 0) {
       /* Wait until the reset is complete */
       err = ds2482_wait_1wire_idle(pdev);
       retval = !(err & DS2482_REG_STS_PPD);

       /* If the chip did reset since detect, re-config it */
       if (err & DS2482_REG_STS_RST)
           ds2482_send_cmd_data(pdev, DS2482_CMD_WRITE_CONFIG,
                        0xF0);
   }

   up(&pdev->access_lock);

   return retval;
}


/*
 * The following function does more than just detection. If detection
 * succeeds, it also registers the new chip.
 */
static int ds2482_detect(struct i2c_adapter *adapter, int address, int kind) {
   struct ds2482_data *data;
   struct i2c_client  *new_client;
   int err = 0;
   int temp1;
   int idx;

   if (!i2c_check_functionality(adapter,
                    I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
                    I2C_FUNC_SMBUS_BYTE))
       goto exit;

   if (!(data = kzalloc(sizeof(struct ds2482_data), GFP_KERNEL))) {
       err = -ENOMEM;
       goto exit;
   }

   new_client = &data->client;
   i2c_set_clientdata(new_client, data);
   new_client->addr = address;
   new_client->driver = &ds2482_driver;
   new_client->adapter = adapter;

   /* Reset the device (sets the read_ptr to status) */
   if (ds2482_send_cmd(data, DS2482_CMD_RESET) < 0) {
       dev_dbg(&adapter->dev, "DS2482 reset failed at 0x%02x.\n",
           address);
       goto exit_free;
   }

   /* Sleep at least 525ns to allow the reset to complete */
   ndelay(525);

   /* Read the status byte - only reset bit and line should be set */
   temp1 = i2c_smbus_read_byte(new_client);
   if (temp1 != (DS2482_REG_STS_LL | DS2482_REG_STS_RST)) {
       dev_dbg(&adapter->dev, "DS2482 (0x%02x) reset status "
           "0x%02X - not a DS2482\n", address, temp1);
       goto exit_free;
   }

   /* Detect the 8-port version */
   data->w1_count = 1;
   if (ds2482_set_channel(data, 7) == 0)
       data->w1_count = 8;

   /* Set all config items to 0 (off) */
   ds2482_send_cmd_data(data, DS2482_CMD_WRITE_CONFIG, 0xF0);

   /* We can fill in the remaining client fields */
   snprintf(new_client->name, sizeof(new_client->name), "ds2482-%d00",
        data->w1_count);

   init_MUTEX(&data->access_lock);

   /* Tell the I2C layer a new client has arrived */
   if ((err = i2c_attach_client(new_client)))
       goto exit_free;

   /* Register 1-wire interface(s) */
   for (idx = 0; idx < data->w1_count; idx++) {
       data->w1_ch[idx].pdev = data;
       data->w1_ch[idx].channel = idx;

       /* Populate all the w1 bus master stuff */
       data->w1_ch[idx].w1_bm.data       = &data->w1_ch[idx];
       data->w1_ch[idx].w1_bm.read_byte  = ds2482_w1_read_byte;
       data->w1_ch[idx].w1_bm.write_byte = ds2482_w1_write_byte;
       data->w1_ch[idx].w1_bm.touch_bit  = ds2482_w1_touch_bit;
       data->w1_ch[idx].w1_bm.triplet    = ds2482_w1_triplet;
       data->w1_ch[idx].w1_bm.reset_bus  = ds2482_w1_reset_bus;

       err = w1_add_master_device(&data->w1_ch[idx].w1_bm);
       if (err) {
           data->w1_ch[idx].pdev = NULL;
           goto exit_w1_remove;
       }
   }

   return 0;

#endif /* 0 */

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */

/* Open a DS2482 after an unsucessful DS2480_detect attempt */
/* _detect is a bit of a misnomer, no detection is actually done */
// no bus locking here (higher up)
int DS2482_detect( struct connection_in * in ) {
    int test_address[8] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, } ; // the last 4 are -800 only
    int i ;
    int fd ;
    
    /* open the COM port */
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

            /* write the RESET code */
            if (
               i2c_smbus_write_byte( fd,  DS2482_CMD_RESET )  // reset
               || DS2482_readstatus(&c,fd,1,2) // pause .5 usec then read status
               || ( c != (DS2482_REG_STS_LL | DS2482_REG_STS_RST) ) // make sure status is properly set
              ) {
                LEVEL_CONNECT("i2c device at %s address %d cannot be reset\n",in->name, test_address[i]) ;
                continue ; ;
            }
            LEVEL_CONNECT("i2c device at %s address %d appears to be DS2482-x00\n",in->name, test_address[i]) ;

            /* Now see if DS2482-100 or DS2482-800 */
            return HeadChannel(in) ;
        }
    }
    /* fellthough, no device found */
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
        if ( ret < 0 ) return 1 ;
        if ( ( ret & DS2482_REG_STS_1WB ) == 0x00 ) {
            c[0] = (BYTE) ret ;
            LEVEL_DEBUG("DS2482 read status ok\n") ;
            return 0 ;
        }
        if ( i++ == 3 ) {
            LEVEL_DEBUG("DS2482 read status fail\n") ;
            return 1 ;
        }
       UT_delay_us( delta_usec ) ; // increment up to three times
    } while ( 1 ) ;
}

/* DS2482 Reset -- A little different from DS2480B */
// return 1 shorted, 0 ok, <0 error
static int DS2482_reset( const struct parsedname * pn ) {
    BYTE c;
    int fd = pn->in->connin.i2c.fd ;

    if( fd < 0 ) return -1;

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
    int fd = pn->in->connin.i2c.fd ;
    size_t i ;

    /* Make sure we're using the correct channel */
    if ( DS2482_channel_select(pn) ) return -1 ;

    for ( i = 0 ; i < len ; ++i ) {
        if ( DS2482_send_and_get( fd, data[i], &resp[i] ) ) return 1 ;
    }
    return 0 ;
}

/* Single byte -- assumes channel selection already done */
static int DS2482_send_and_get( int fd, const BYTE * wr, BYTE * rd ) {
    int read_back ;
    BYTE c ;

    /* Write TRIPLE command */
    if (i2c_smbus_write_byte_data(fd, DS2482_CMD_1WIRE_WRITE_BYTE, wr) < 0)
       return 1;

    /* read status */
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
#ifdef OW_MT
    pthread_mutex_init(&(in->connin.i2c.i2c_mutex), pmattr);
#endif /* OW_MT */
    in->busmode = bus_i2c ;
    in->Adapter = adapter_DS2482_100 ;

    /* Not intentionally put the wrong index */
    in->connin.i2c.index = 1 ;
    pn.in = in ;
    if ( DS2482_channel_select(&pn) ) { /* Couldn't switch */
        in->connin.i2c.index = 0 ; /* restore correct value */
        return 0 ; /* happy as DS2482-100 */
    }
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
    for ( i=0 ; i<8 ; ++i ) {
        struct connection_in * added = NewIn(in) ;
        if ( added==NULL ) return -ENOMEM ;
        added->connin.i2c.index = i ;
        added->adapter_name = name[i] ;
    }
    return 0 ;
}

static int DS2482_triple( BYTE * bits, int direction, const struct parsedname * pn ) {
    /* 3 bits in bits */
    int fd = pn->in->connin.i2c.fd ;
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
    int fd = pn->in->connin.i2c.fd ;
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
    return 0 ;
}
