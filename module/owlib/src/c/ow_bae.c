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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

/* Pascal Baerten's BAE 910 device -- functional */
/* Pascal Baerten's BAE 911 device -- preliminary */

#include <config.h>
#include "owfs_config.h"
#include "ow_bae.h"

/* ------- Prototypes ----------- */

/* BAE */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
//READ_FUNCTION(FS_r_flash);
WRITE_FUNCTION(FS_w_flash);
WRITE_FUNCTION(FS_w_extended);
//WRITE_FUNCTION(FS_writebyte);
WRITE_FUNCTION(FS_w_lcd_text);
WRITE_FUNCTION(FS_w_sector_nr);
READ_FUNCTION(FS_r_sector_nr);

READ_FUNCTION(FS_version_state) ;
READ_FUNCTION(FS_version) ;
READ_FUNCTION(FS_version_device) ;
READ_FUNCTION(FS_version_bootstrap) ;

READ_FUNCTION(FS_type_state) ;
READ_FUNCTION(FS_localtype) ;
READ_FUNCTION(FS_type_device) ;
READ_FUNCTION(FS_type_chip) ;

WRITE_FUNCTION(FS_eeprom_erase);
READ_FUNCTION(FS_eeprom_r_page);
WRITE_FUNCTION(FS_eeprom_w_page);
READ_FUNCTION(FS_eeprom_r_mem);
WRITE_FUNCTION(FS_eeprom_w_mem);

READ_FUNCTION(FS_r_8) ;
WRITE_FUNCTION(FS_w_8) ;
READ_FUNCTION(FS_r_16) ;
WRITE_FUNCTION(FS_w_16) ;
READ_FUNCTION(FS_r_32) ;
WRITE_FUNCTION(FS_w_32) ;

READ_FUNCTION(FS_r_911_pio) ;
WRITE_FUNCTION(FS_w_911_pio) ;
READ_FUNCTION(FS_r_pio_bit) ;
WRITE_FUNCTION(FS_w_pio_bit) ;

/* ------- Structures ----------- */

/* common values to all FC chips */


#define _FC_EEPROM_PAGE_SIZE   512
#define _FC_MAX_EEPROM_PAGES    32
#define _FC_EEPROM_OFFSET   0x0200
#define _FC_MAX_FLASH_SIZE  0x4000

#define _FC02_MEMORY_SIZE      128
#define _FC02_EEPROM_OFFSET 0xE000
#define _FC02_EEPROM_PAGES       2
#define _FC03_EEPROM_PAGES       8

#define _FC02_MAX_WRITE_GULP	32
#define _FC02_MAX_READ_GULP	32
#define _FC02_MAX_COMMAND_GULP	255

/* BAE registers -- memory mapped */
#define _FC02_ADC        50   /* u8 */
#define _FC02_ADCAN      24   /* u16 */
#define _FC02_ADCAP      22   /* u16 */
#define _FC02_ADCC       2    /* u8 */
#define _FC02_ADCTOTN    36   /* u32 */
#define _FC02_ADCTOTP    32   /* u32 */
#define _FC02_ALAN       66   /* u16 */
#define _FC02_ALAP       64   /* u16 */
#define _FC02_ALARM      52   /* u8 */
#define _FC02_ALARMC     6    /* u8 */
#define _FC02_ALCPS      68   /* u16 */
#define _FC02_ALCT       70   /* u32 */
#define _FC02_ALRT       74   /* u32 */
#define _FC02_CNT        51   /* u8 */
#define _FC02_CNTC       3    /* u8 */
#define _FC02_COUNT      44   /* u32 */
#define _FC02_CPS        30   /* u16 */
#define _FC02_DUTY1      14   /* u16 */
#define _FC02_DUTY2      16   /* u16 */
#define _FC02_DUTY3      18   /* u16 */
#define _FC02_DUTY4      20   /* u16 */
#define _FC02_MAXAN      28   /* u16 */
#define _FC02_MAXAP      26   /* u16 */
#define _FC02_MAXCPS     94   /* u16 */
#define _FC02_OUT        48   /* u8 */
#define _FC02_OUTC       4    /* u8 */
#define _FC02_OVRUNCNT   92   /* u16 */
#define _FC02_PC0        54   /* u16 */
#define _FC02_PC1        56   /* u16 */
#define _FC02_PC2        58   /* u16 */
#define _FC02_PC3        60   /* u16 */
#define _FC02_PERIOD1    10   /* u16 */
#define _FC02_PERIOD2    12   /* u16 */
#define _FC02_PIO        49   /* u8 */
#define _FC02_PIOC       5    /* u8 */
#define _FC02_RESETCNT   86   /* u16 */
#define _FC02_RTC        40   /* u32 */
#define _FC02_RTCC       7    /* u8 */
#define _FC02_SELECTCNT  88   /* u16 */
#define _FC02_STALLEDCNT 90   /* u16 */
#define _FC02_TPM1C      8    /* u8 */
#define _FC02_TPM2C      9    /* u8 */
#define _FC02_USERA      96   /* u8 */
#define _FC02_USERB      97   /* u8 */
#define _FC02_USERC      98   /* u8 */
#define _FC02_USERD      99   /* u8 */
#define _FC02_USERE      100  /* u8 */
#define _FC02_USERF      101  /* u8 */
#define _FC02_USERG      102  /* u8 */
#define _FC02_USERH      103  /* u8 */
#define _FC02_USERI      104  /* u16 */
#define _FC02_USERJ      106  /* u16 */
#define _FC02_USERK      108  /* u16 */
#define _FC02_USERL      110  /* u16 */
#define _FC02_USERM      112  /* u32 */
#define _FC02_USERN      116  /* u32 */
#define _FC02_USERO      120  /* u32 */
#define _FC02_USERP      124  /* u32 */

#define _FC03_PIO        0x00 /* u8 x 22 */
#define _FC03_PIO_CONF   0x16 /* u8 x 22 */
#define _FC03_TPM1C      0x2C /* u8 */
#define _FC03_TPM2C      0x2D /* u8 */
#define _FC03_PERIOD1    0x2E /* u16 */
#define _FC03_PERIOD2    0x30 /* u16 */
#define _FC03_DUTY       0x32 /* u16 x 4 */
#define _FC03_SPIC       0x3A /* u8 */
#define _FC03_SPIBR      0x3B /* u8 */
#define _FC03_SPID       0x3C /* u8 */
#define _FC03_IICD       0x3D /* u8 */
#define _FC03_IICC       0x3E /* u8 */
#define _FC03_IICFR      0x3F /* u8 */
#define _FC03_LCDINIT    64   /* u8 */
#define _FC03_LCDCTRL    65   /* u8 */
#define _FC03_LCDDATA    66   /* u8 */
#define _FC03_ELARAM     67   /* u8 */
#define _FC03_CLARAM     68   /* u16 */
#define _FC03_ELARAM     67   /* u8 */
#define _FC03_CLARAM     68   /* u8 */
#define _FC03_SDINIT     69   /* u8 */


#define _FC03_ADC        0x100 /* u16 x 16 */
#define _FC03_EALARMC    288  /* u8 */
#define _FC03_CALARMC    289  /* u8 */


/* Placeholder for special vsibility code for firmware types "personalities" */
static enum e_visibility VISIBLE_eeprom_pages( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_eeprom_bytes( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_generic( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_910( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_911( const struct parsedname * pn ) ;


struct aggregate ABAEeeprom = { _FC_MAX_EEPROM_PAGES, ag_numbers, ag_separate, };
struct aggregate A911pio = { 22, ag_numbers, ag_separate, };
struct aggregate A911pwm = { 4, ag_numbers, ag_separate, };
struct aggregate A911adc = { 16, ag_numbers, ag_separate, };
struct filetype BAE[] = {
	F_STANDARD,
	{"command", _FC02_MAX_COMMAND_GULP, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_extended, VISIBLE, NO_FILETYPE_DATA,},
	//{"writebyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, NO_READ_FUNCTION, FS_writebyte, VISIBLE, NO_FILETYPE_DATA, },
	{"versionstate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_version_state, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"version", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"device_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_device, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"bootstrap_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_bootstrap, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"typestate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_type_state, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"localtype", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"device_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_device, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"chip_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_chip, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},

	{"firmware", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"firmware/function", _FC_MAX_FLASH_SIZE, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_flash, VISIBLE, NO_FILETYPE_DATA,},

	{"eeprom", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
//	{"eeprom/memory",_FC_EEPROM_PAGE_SIZE*_FC_MAX_EEPROM_PAGES, NON_AGGREGATE, ft_binary, fc_link, FS_eeprom_r_mem, FS_eeprom_w_mem, VISIBLE_eeprom_bytes, NO_FILETYPE_DATA,},
	{"eeprom/page",_FC_EEPROM_PAGE_SIZE, &ABAEeeprom, ft_binary, fc_page, FS_eeprom_r_page, FS_eeprom_w_page, VISIBLE_eeprom_pages, NO_FILETYPE_DATA,},
	{"eeprom/erase", PROPERTY_LENGTH_YESNO, &ABAEeeprom, ft_yesno, fc_link, NO_READ_FUNCTION, FS_eeprom_erase, VISIBLE_eeprom_pages, NO_FILETYPE_DATA,},

	{"generic", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_generic, NO_FILETYPE_DATA,},
	{"generic/memory", _FC02_MEMORY_SIZE, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE_generic, NO_FILETYPE_DATA,},
	
	{"910", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_910, NO_FILETYPE_DATA,},
	{"910/adcc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_ADCC,}, },
	{"910/cntc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_CNTC,}, },
	{"910/outc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_OUTC,}, },
	{"910/pioc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_PIOC,}, },
	{"910/alarmc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_ALARMC,}, },
	{"910/rtcc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_RTCC,}, },
	{"910/tpm1c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_TPM1C,}, },
	{"910/tpm2c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_TPM2C,}, },
	{"910/period1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_PERIOD1,}, },
	{"910/period2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_PERIOD2,}, },
	{"910/duty1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_DUTY1,}, },
	{"910/duty2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_DUTY2,}, },
	{"910/duty3", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_DUTY3,}, },
	{"910/duty4", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_DUTY4,}, },
	{"910/alap", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_ALAP,}, },
	{"910/alan", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_ALAN,}, },
	{"910/cps", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, NO_WRITE_FUNCTION, VISIBLE_910, {u:_FC02_CPS,}, },
	{"910/alcps", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_ALCPS,}, },
	{"910/alct", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_ALCT,}, },
	{"910/adcap", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, VISIBLE_910, {u:_FC02_ADCAP,}, },
	{"910/adcan", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, VISIBLE_910, {u:_FC02_ADCAN,}, },
	{"910/maxap", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_MAXAP,}, },
	{"910/maxan", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_MAXAN,}, },
	{"910/maxcps", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_MAXCPS,}, },
	{"910/adctotp", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_ADCTOTP,}, },
	{"910/adctotn", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_ADCTOTN,}, },
	{"910/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_RTC,}, },
	{"910/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE_910, {a:"910/udate"},},
	{"910/count", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_COUNT,}, },
	{"910/out", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_OUT,}, },
	{"910/pio", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_PIO,}, },
	{"910/adc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, VISIBLE_910, {u:_FC02_ADC,}, },
	{"910/cnt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, VISIBLE_910, {u:_FC02_CNT,}, },
	{"910/alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_ALARM,}, },
	{"910/ovruncnt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_OVRUNCNT,}, },
	{"910/resetcnt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_RESETCNT,}, },
	{"910/selectcnt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_SELECTCNT,}, },
	{"910/stalledcnt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_STALLEDCNT,}, },

	{"910/usera", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERA,}, },
	{"910/userb", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERB,}, },
	{"910/userc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERC,}, },
	{"910/userd", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERD,}, },
	{"910/usere", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERE,}, },
	{"910/userf", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERF,}, },
	{"910/userg", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERG,}, },
	{"910/userh", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_910, {u:_FC02_USERH,}, },
	{"910/useri", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_USERI,}, },
	{"910/userj", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_USERJ,}, },
	{"910/userk", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_USERK,}, },
	{"910/userl", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_USERL,}, },
	{"910/userm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_USERM,}, },
	{"910/usern", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_USERN,}, },
	{"910/usero", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_USERO,}, },
	{"910/userp", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_910, {u:_FC02_USERP,}, },

	{"910/pc0", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_PC0,}, },
	{"910/pc1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_PC1,}, },
	{"910/pc2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_PC2,}, },
	{"910/pc3", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_910, {u:_FC02_PC3,}, },

	{"911", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/pio", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/pio/piostate", PROPERTY_LENGTH_UNSIGNED, &A911pio, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u:_FC03_PIO,}, },
	{"911/pio/pio", PROPERTY_LENGTH_YESNO, &A911pio, ft_yesno, fc_link, FS_r_911_pio, FS_w_911_pio, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/pio/pio_config", PROPERTY_LENGTH_UNSIGNED, &A911pio, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_PIO_CONF,}, },
	{"911/pio/pio_ds", PROPERTY_LENGTH_YESNO, &A911pio, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {i: 3}, },
	{"911/pio/pio_pd", PROPERTY_LENGTH_YESNO, &A911pio, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {i: 2}, },
	{"911/pio/pio_pe", PROPERTY_LENGTH_YESNO, &A911pio, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {i: 1}, },
	{"911/pio/pio_dd", PROPERTY_LENGTH_YESNO, &A911pio, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {i: 0}, },
	{"911/pwm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/pwm/tpm1c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_TPM1C,}, },
	{"911/pwm/tpm2c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_TPM2C,}, },
	{"911/pwm/period1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PERIOD1,}, },
	{"911/pwm/period2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PERIOD2,}, },
	{"911/pwm/duty", PROPERTY_LENGTH_UNSIGNED, &A911pwm, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_DUTY,}, },
	{"911/lcd", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/lcd/init", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_LCDINIT,}, },
	{"911/lcd/char", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, NO_READ_FUNCTION, FS_w_8, VISIBLE_911, {u:_FC03_LCDDATA,}, },
	{"911/lcd/control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_LCDCTRL,}, },
	{"911/lcd/text", 255, NON_AGGREGATE, ft_ascii, fc_volatile, NO_READ_FUNCTION, FS_w_lcd_text, VISIBLE_911, NO_FILETYPE_DATA, },
	
	{"911/sdcard", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/sdcard/init", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SDINIT,}, },
	{"911/sdcard/sector_nr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_sector_nr, FS_w_sector_nr, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/spi", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/spi/spic", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SPIC,}, },
	{"911/spi/spibr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SPIBR,}, },
	{"911/spi/spid", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SPID,}, },
	{"911/iic", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/iic/iicd", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_IICD,}, },
	{"911/iic/iicc", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_IICC,}, },
	{"911/iic/iicfr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_IICFR,}, },
	{"911/adc", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA,},
	{"911/adc/adc", PROPERTY_LENGTH_UNSIGNED, &A911adc, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_ADC,}, },
};

DeviceEntryExtended(FC, BAE, DEV_resume | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE );

/* <AE command codes */
#define _1W_ECMD_ERASE_FIRMWARE 0xBB
#define _1W_ECMD_FLASH_FIRMWARE 0xBA
#define _1W_WF_WRITE_LCD_DATA 0x01

#define _1W_READ_VERSION 0x11
#define _1W_READ_TYPE 0x12
#define _1W_EXTENDED_COMMAND 0x13
#define _1W_READ_BLOCK_WITH_LEN 0x14
#define _1W_WRITE_BLOCK_WITH_LEN 0x15
#define _1W_ERASE_EEPROM_PAGE 0x16
#define _1W_WRITE_FLOW      0x17
#define _1W_CONFIRM_WRITE 0xBC

/* Persistent storage */
MakeInternalProp(SNR, fc_persistent);	// sector number

/* ------- Functions ------------ */

static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_mem_eeprom(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_extended(BYTE * data, size_t size, struct parsedname *pn);
static GOOD_OR_BAD OW_version( UINT * version, struct parsedname * pn ) ;
static GOOD_OR_BAD OW_type( UINT * localtype, struct parsedname * pn ) ;
static GOOD_OR_BAD OW_r_mem(BYTE *bytes, size_t size, off_t offset, struct parsedname * pn);
static GOOD_OR_BAD OW_r_mem_small(BYTE *bytes, size_t size, off_t offset, struct parsedname * pn);
static GOOD_OR_BAD OW_eeprom_erase( off_t offset, struct parsedname * pn ) ;
static GOOD_OR_BAD OW_w_flow_nocrc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size, struct parsedname *pn);

//static void OW_siumulate_eeprom(BYTE * eeprom, const BYTE * data, size_t size) ;

static GOOD_OR_BAD OW_initiate_flash(BYTE * data, struct parsedname *pn, int duration);
static GOOD_OR_BAD OW_write_flash(BYTE * data, struct parsedname *pn);

static uint16_t BAE_uint16(BYTE * p);
static uint32_t BAE_uint32(BYTE * p);
static void BAE_uint16_to_bytes( uint16_t num, unsigned char * p );
static void BAE_uint32_to_bytes( uint32_t num, unsigned char * p );
static size_t eeprom_offset( const struct parsedname * pn );

static int VISIBLE_BAE( const struct parsedname * pn ) ;

/* finds the visibility value (910, 911, ...) either cached, or computed via the device_type (then cached) */
static int VISIBLE_BAE( const struct parsedname * pn )
{
	int visibility_parameter = -1 ;
	
	LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
	if ( BAD( GetVisibilityCache( &visibility_parameter, pn ) ) ) {
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		if ( owq != NULL) {
			UINT device_type ;
			if ( FS_r_sibling_U( &device_type, "device_type", owq ) == 0 ) {
				switch (device_type) {
					case 2:
						visibility_parameter = 910 ;
						SetVisibilityCache( visibility_parameter, pn ) ;
						break ;
					case 3:
						visibility_parameter = 911 ;
						SetVisibilityCache( visibility_parameter, pn ) ;
						break ;
					default:
						visibility_parameter = -1 ;
				}
			}
			OWQ_destroy(owq) ;
		}
	}
	return visibility_parameter ;
}

static enum e_visibility VISIBLE_eeprom_pages( const struct parsedname * pn )
{
	switch ( VISIBLE_BAE(pn) ) {
		case -1:
			return visible_not_now ;
		case 910: //modify ABAEeeprom.elements to number of pages;
			pn->selected_filetype->ag->elements=_FC02_EEPROM_PAGES;
			return visible_now ;
		case 911:
			pn->selected_filetype->ag->elements=_FC03_EEPROM_PAGES; 
			return visible_now ;
		default:
			return visible_not_now ;
	}
}
static enum e_visibility VISIBLE_eeprom_bytes( const struct parsedname * pn )
{
	switch ( VISIBLE_BAE(pn) ) {
		case -1:
			return visible_not_now ;
		case 910: 
			pn->selected_filetype->suglen=_FC02_EEPROM_PAGES * _FC_EEPROM_PAGE_SIZE;
			return visible_now ;
		case 911:
			pn->selected_filetype->suglen=_FC03_EEPROM_PAGES * _FC_EEPROM_PAGE_SIZE; 
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_generic( const struct parsedname * pn )
{
	switch ( VISIBLE_BAE(pn) ) {
		case -1:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_910( const struct parsedname * pn )
{
	switch ( VISIBLE_BAE(pn) ) {
		case 910:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}

static enum e_visibility VISIBLE_911( const struct parsedname * pn )
{
	switch ( VISIBLE_BAE(pn) ) {
		case 911:
			return visible_now ;
		default:
			return visible_not_now ;
	}
}
static size_t eeprom_offset( const struct parsedname * pn )
{
	switch ( VISIBLE_BAE(pn) ) {
		case 910: //future vesion of 910 will work with uniform eeprom offset
			return _FC02_EEPROM_OFFSET ;
		default:
			return _FC_EEPROM_OFFSET ;
	}
}

static ZERO_OR_ERROR FS_r_sector_nr(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT sector_nr;					

	if ( BAD( Cache_Get_Internal_Strict((void *) &sector_nr, sizeof(UINT), InternalProp(SNR), pn)) ) { // record doesn't (yet) exist
		sector_nr=0;
	} 
	OWQ_U(owq) = sector_nr;
	if (Cache_Add_Internal((void *) &sector_nr, sizeof(UINT), InternalProp(SNR), pn)) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_sector_nr(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT sector_nr;					

	sector_nr = OWQ_U(owq);
	if (Cache_Add_Internal((void *) &sector_nr, sizeof(UINT), InternalProp(SNR), pn)) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_lcd_text(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE *start_of_text = (BYTE*)OWQ_buffer(owq);
	BYTE param; //optional cursor position *not implemented* 
	LEVEL_DEBUG("lcd text size is %d", (int)OWQ_size(owq) ) ;
	if (OWQ_offset(owq)) {
		return -ERANGE;
	}
	RETURN_ERROR_IF_BAD(OW_w_flow_nocrc(_1W_WF_WRITE_LCD_DATA, &param, 0, start_of_text, OWQ_size(owq), pn));
	return 0;
}

/* BAE memory functions */
static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	RETURN_ERROR_IF_BAD(OW_r_mem( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq))) ;
	OWQ_length(owq) = OWQ_size(owq) ;
	return 0;
}

static ZERO_OR_ERROR FS_eeprom_r_mem(struct one_wire_query *owq)
{
	return COMMON_offset_process(FS_r_mem,owq, eeprom_offset(PN(owq))) ;
}

static ZERO_OR_ERROR FS_eeprom_r_page(struct one_wire_query *owq)
{
	return COMMON_offset_process(FS_eeprom_r_mem,owq, _FC_EEPROM_PAGE_SIZE * OWQ_pn(owq).extension) ;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_w_mem( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_eeprom_w_mem(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	size_t size = OWQ_size(owq) ;
/*
	BYTE * eeprom = owmalloc( size ) ;
	if ( eeprom==NULL ) {
		return -ENOMEM ;
	}
	
	// read existing eeprom pattern
	if ( BAD( OW_r_mem(eeprom, size, OWQ_offset(owq)+eeprom_offset(pn), pn)) ) {
		LEVEL_DEBUG("Cannot read eeprom prior to writing") ;
		owfree(eeprom) ;
		return -EINVAL ;
	}
	
	// modify eeprom with new data
	OW_siumulate_eeprom(eeprom, (const BYTE *) OWQ_buffer(owq), size) ;
*/

	// write back
	if ( BAD( OW_w_mem_eeprom((BYTE *) OWQ_buffer(owq), size, OWQ_offset(owq)+eeprom_offset(pn), pn)) ) {
		LEVEL_DEBUG("Cannot write to eeprom") ;
		//owfree(eeprom) ;
		return -EINVAL ;
	}
	//owfree(eeprom) ;
	return 0 ;
}

static ZERO_OR_ERROR FS_eeprom_w_page(struct one_wire_query *owq)
{
	return COMMON_offset_process(FS_eeprom_w_mem,owq, _FC_EEPROM_PAGE_SIZE * OWQ_pn(owq).extension) ;
}

static ZERO_OR_ERROR FS_eeprom_erase(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	if (OWQ_Y(owq)) {
		off_t offset = _FC_EEPROM_PAGE_SIZE * pn->extension + eeprom_offset(pn) ;
		return GB_to_Z_OR_E( OW_eeprom_erase(offset,pn) ) ;
	} else{
		return 0 ;
	}
}

/* BAE flash functions */
static ZERO_OR_ERROR FS_w_flash(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE * rom_image = (BYTE *) OWQ_buffer(owq) ;

	size_t rom_offset,expected_size ;
	

	if ( OWQ_size(owq) % 0x200 ) {
		LEVEL_DEBUG("Flash size of %d is not a multiple of 512.", (int)OWQ_size(owq) ) ;
		return -EBADMSG ;
	}
	if ( OWQ_offset(owq) % 0x200 ) {
		LEVEL_DEBUG("Flash offset %d is not a multiple of 512.", (int)OWQ_offset(owq) ) ;
		return -EBADMSG ;
	}
	//flash firmware may exceed 8192bytes, thus FS_w_flash is called more than once with successive buffers
	if ( OWQ_offset(owq) == 0 ) {
		LEVEL_DEBUG("Beginning of flash (from start of buffer).") ;
		// test size
		expected_size=1+(rom_image[7]+rom_image[6]*256)-(rom_image[5]+rom_image[4]*256);
		LEVEL_DEBUG("Flash size=%d, (size calculated from header=%d) .", (int)OWQ_size(owq), (int)expected_size ) ;
		if ( OWQ_size(owq) > expected_size ) {
			LEVEL_DEBUG("Flash size of %d is greater than expected %d bytes .", (int)OWQ_size(owq), (int)expected_size ) ;
			return -ERANGE ;
		}
		// start flash process, added duration param calculated as 1ms for 16 bytes
		if ( BAD( OW_initiate_flash( rom_image, pn, expected_size>>4 ) ) ) {
			LEVEL_DEBUG("Unsuccessful flash initialization");
			return -EFAULT ;
		}
	}

	// loop though pages, up to 5 attempts for each page
	for ( rom_offset=0 ; rom_offset<OWQ_size(owq) ; rom_offset += _FC02_MAX_WRITE_GULP ) {
		int tries = 0 ;
		LEVEL_DEBUG("Flash up to %d bytes.",rom_offset);
		while ( BAD( OW_write_flash( &rom_image[rom_offset], pn ) ) ) {
			++tries ;
			if ( tries > 4 ) {
				LEVEL_DEBUG( "Too many failures writing flash at offset %d.", rom_offset ) ;
				return -EIO ;
			}
			LEVEL_DEBUG( "retry %d of 5 when writing flash.", tries ) ;
			
		}
	}
	
	LEVEL_DEBUG("Successfully flashed full rom.") ;
	return 0;
}

/*
static ZERO_OR_ERROR FS_r_flash( struct one_wire_query *owq)
{
	RETURN_ERROR_IF_BAD( OW_r_mem( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq)+_FC02_FUNCTION_FLASH_OFFSET, PN(owq) ) );
	OWQ_length(owq) = OWQ_size(owq) ;
	return 0 ;
}
*/

/* BAE extended command */
static ZERO_OR_ERROR FS_w_extended(struct one_wire_query *owq)
{
	// Write data 255 bytes maximum (1byte minimum)
	if ( OWQ_size(owq) == 0 ) {
		return -EINVAL ;
	}
	return GB_to_Z_OR_E( OW_w_extended( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), PN(owq)  ) ) ;
}

/*static ZERO_OR_ERROR FS_writebyte(struct one_wire_query *owq)
{
	off_t location = OWQ_U(owq)>>8 ;
	BYTE data = OWQ_U(owq) & 0xFF ;
	
	// Write 1 byte ,
	return GB_to_Z_OR_E( OW_w_mem( &data, 1, location, PN(owq)  ) ) ;
}*/

/* BAE version */
static ZERO_OR_ERROR FS_version_state(struct one_wire_query *owq)
{
	UINT v ;
	RETURN_ERROR_IF_BAD( OW_version( &v, PN(owq) ) ) ;
	OWQ_U(owq) = v ;
	return 0 ;
}

// there may need to be a 0<->1 sense interchange
static ZERO_OR_ERROR FS_r_911_pio( struct one_wire_query * owq ) {
	UINT piostate ;
	if ( FS_r_sibling_U( &piostate, "911/pio/piostate", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = piostate & 0x01 ;
	return 0 ;
}

// there may need to be a 0<->1 sense interchange
static ZERO_OR_ERROR FS_w_911_pio( struct one_wire_query * owq ) {
	return FS_w_sibling_U( OWQ_Y(owq), "911/pio/piostate", owq ) ;
}

static ZERO_OR_ERROR FS_r_pio_bit( struct one_wire_query * owq ) {
	UINT pioconf ;
	if ( FS_r_sibling_U( &pioconf, "911/pio/pio_config", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = UT_getbit( (void *) &pioconf, PN(owq)->selected_filetype->data.u ) ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_pio_bit( struct one_wire_query * owq ) {
	return FS_w_sibling_bitwork( OWQ_Y(owq), (1<<(PN(owq)->selected_filetype->data.u)), "911/pio_config", owq ) ;
}

static ZERO_OR_ERROR FS_version(struct one_wire_query *owq)
{
	char v[6];
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) != 0 ) {
		return -EINVAL ;
	}
	
	UCLIBCLOCK;
	snprintf(v,6,"%.2X.%.2X",version&0xFF, (version>>8)&0xFF);
	UCLIBCUNLOCK;
	
	return OWQ_format_output_offset_and_size(v, 5, owq);
}

static ZERO_OR_ERROR FS_version_device(struct one_wire_query *owq)
{
	UINT version = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &version, "versionstate", owq ) ;
	
	OWQ_U(owq) = (version>>8) & 0xFF ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_version_bootstrap(struct one_wire_query *owq)
{
	UINT version = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &version, "versionstate", owq ) ;
	
	OWQ_U(owq) = version & 0xFF ;
	return z_or_e ;
}

/* BAE type */
static ZERO_OR_ERROR FS_type_state(struct one_wire_query *owq)
{
	UINT t ;
	RETURN_ERROR_IF_BAD( OW_type( &t, PN(owq) ) );
	OWQ_U(owq) = t ;
	return 0 ;
}

static ZERO_OR_ERROR FS_localtype(struct one_wire_query *owq)
{
	char t[6];
	UINT localtype ;
	
	if ( FS_r_sibling_U( &localtype, "typestate", owq ) != 0 ) {
		return -EINVAL ;
	}
	
	UCLIBCLOCK;
	snprintf(t,6,"%.2X.%.2X",localtype&0xFF, (localtype>>8)&0xFF);
	UCLIBCUNLOCK;
	
	return OWQ_format_output_offset_and_size(t, 5, owq);
}

static ZERO_OR_ERROR FS_type_device(struct one_wire_query *owq)
{
	UINT t = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &t, "typestate", owq ) ;
	
	OWQ_U(owq) = (t>>8) & 0xFF ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_type_chip(struct one_wire_query *owq)
{
	UINT t = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &t, "typestate", owq ) ;
	
	OWQ_U(owq) = t & 0xFF ;
	return z_or_e ;
}

/* read an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_r_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[1] ; // 8/8 = 1
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, 1, pn->selected_filetype->data.u + pn->extension, pn ) );
	OWQ_U(owq) = data[0] ;
	return 0 ;
}

/* write an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_w_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[1] ; // 8/8 = 1
	
	data[0] = BYTE_MASK( OWQ_U(owq) ) ;
	return GB_to_Z_OR_E(OW_w_mem(data, 1, pn->selected_filetype->data.u + pn->extension, pn ) ) ;
}

/* read a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[2] ; // 16/8 = 2
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, 2, pn->selected_filetype->data.u + 2 * pn->extension, pn ) );
	OWQ_U(owq) = BAE_uint16(data) ;
	return 0 ;
}

/* write a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[2] ; // 16/8 = 2

	BAE_uint16_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, 2, pn->selected_filetype->data.u + 2 * pn->extension, pn ) ) ;
}

/* read a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[4] ; // 32/8 = 4
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, 4, pn->selected_filetype->data.u, pn ) );
	OWQ_U(owq) = BAE_uint32(data) ;
	return 0 ;
}

/* write a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	BYTE data[4] ; // 32/8 = 2
	
	BAE_uint32_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, 4, pn->selected_filetype->data.u, pn ) ) ;
}

/* Lower level functions */

// Extended command -- insert length, The first byte of the payload is a subcommand but that is invisible to us.
static GOOD_OR_BAD OW_w_extended(BYTE * data, size_t size, struct parsedname *pn)
{
	BYTE p[1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_EXTENDED_COMMAND, BYTE_MASK(size-1), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+1+size, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(2),
		TRXN_END,
	};
	
	/* Copy to write buffer */
	memcpy(&p[2], data, size);
	
	return BUS_transaction(t, pn) ;
}

// Flow write command without crc 
static GOOD_OR_BAD OW_w_flow_nocrc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size, struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_WRITE_FLOW, wf, 0, BYTE_MASK(plen), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4+plen, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1),
		TRXN_WRITE(data, size),
		TRXN_END,
	};
	
	/* Copy to write buffer */
	memcpy(&p[4], param, plen);
	return BUS_transaction(t, pn) ;
}

//read bytes[size] from position
static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	size_t remain = size ;
	off_t local_offset = 0 ;
	
	while ( remain > 0 ) {
		size_t gulp = remain ;
		if ( gulp > _FC02_MAX_READ_GULP ) {
			gulp = _FC02_MAX_READ_GULP ;
		}
		RETURN_BAD_IF_BAD( OW_r_mem_small( &data[local_offset], gulp, offset+local_offset, pn ));
		remain -= gulp ;
		local_offset += gulp ;
	}
	return gbGOOD ;	
}

//write bytes[size] to position
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	size_t remain = size ;
	off_t local_offset = 0 ;
	
	while ( remain > 0 ) {
		size_t gulp = remain ;
		if ( gulp > _FC02_MAX_READ_GULP ) {
			gulp = _FC02_MAX_READ_GULP ;
		}
		RETURN_BAD_IF_BAD( OW_w_mem_small( &data[local_offset], gulp, offset+local_offset, pn ));
		remain -= gulp ;
		local_offset += gulp ;
	}
	return gbGOOD ;	
}

//write bytes[size] to eeprom position ( only difference is eeprom delay)
static GOOD_OR_BAD OW_w_mem_eeprom(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	size_t remain = size ;
	off_t local_offset = 0 ;
	
	while ( remain > 0 ) {
		size_t gulp = remain ;
		if ( gulp > _FC02_MAX_READ_GULP ) {
			gulp = _FC02_MAX_READ_GULP ;
		}
		RETURN_BAD_IF_BAD( OW_w_mem_small( &data[local_offset], gulp, offset+local_offset, pn ));
		UT_delay(2) ; //1.5 msec
		remain -= gulp ;
		local_offset += gulp ;
	}
	return gbGOOD ;	
}

/* the crc returned is calculated on buffer transmitted and not on resulting memory contents
// Simulate eeprom (for correct CRC)
static void OW_siumulate_eeprom(BYTE * eeprom, const BYTE * data, size_t size)
{
	size_t i ;
	for ( i=0 ; i<size ; ++i) {
		eeprom[i] &= data[i] ;
	}
}
*/

/* Already constrained to _FC02_MAX_READ_GULP aliquots */
static GOOD_OR_BAD OW_r_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	BYTE p[1+2+1 + _FC02_MAX_READ_GULP + 2] = { _1W_READ_BLOCK_WITH_LEN, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, size),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	memcpy(data, &p[4], size);
	return gbGOOD;
}

/* size in already constrained to 32 bytes (_FC02_MAX_WRITE_GULP) */
static GOOD_OR_BAD OW_w_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1+2+1 + _FC02_MAX_WRITE_GULP + 2] = { _1W_WRITE_BLOCK_WITH_LEN, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+ 2 + 1 + size, 0),
		TRXN_WRITE1(q),
		TRXN_END,
	};
	
	/* Copy to scratchpad */
	memcpy(&p[4], data, size);

	LEVEL_DEBUG("Write to BAE size=%d offset=%x\n",(int)size,(unsigned int)offset) ;
	Debug_Bytes("BAE write",p,1+2+1+size) ;
	
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_version( UINT * version, struct parsedname * pn )
{
	BYTE p[5] = { _1W_READ_VERSION, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 2),
		TRXN_END,
	} ;
		
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	version[0] = BAE_uint16(&p[1]) ;
	return gbGOOD ;
};

static GOOD_OR_BAD OW_type( UINT * localtype, struct parsedname * pn )
{
	BYTE p[5] = { _1W_READ_TYPE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 2),
		TRXN_END,
	} ;
	
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
	localtype[0] = BAE_uint16(&p[1]) ;
	return gbGOOD ;
};

/* Routines to play with byte <-> integer */
static uint16_t BAE_uint16(BYTE * p)
{
	return (((uint16_t) p[0]) << 8) | ((uint16_t) p[1]);
}

static uint32_t BAE_uint32(BYTE * p)
{
	return (((uint32_t) p[0]) << 24) | (((uint32_t) p[1]) << 16) | (((uint32_t) p[2]) << 8) | ((uint32_t) p[3]);
}

static void BAE_uint16_to_bytes( uint16_t num, unsigned char * p )
{
	p[1] = num&0xFF ;
	p[0] = (num>>8)&0xFF ;
}

static void BAE_uint32_to_bytes( uint32_t num, unsigned char * p )
{
	p[3] = num&0xFF ;
	p[2] = (num>>8)&0xFF ;
	p[1] = (num>>16)&0xFF ;
	p[0] = (num>>24)&0xFF ;
}

static GOOD_OR_BAD OW_initiate_flash( BYTE * data, struct parsedname * pn , int duration)
{
	BYTE p[1+1+1+32+2] = { _1W_EXTENDED_COMMAND, 32,_1W_ECMD_ERASE_FIRMWARE, } ;
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+1+1+32, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(duration),
		TRXN_END,
	} ;

	memcpy(&p[3], data, 32 ) ;
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_write_flash( BYTE * data, struct parsedname * pn )
{
	BYTE p[1+1+1+32+2] = { _1W_EXTENDED_COMMAND, 32,_1W_ECMD_FLASH_FIRMWARE,  } ;
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+1+1+32, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(2),
		TRXN_END,
	} ;
	
	memcpy(&p[3], data, 32 ) ;
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_eeprom_erase( off_t offset, struct parsedname * pn )
{
	BYTE p[1+2+2] = { _1W_ERASE_EEPROM_PAGE, LOW_HIGH_ADDRESS(offset), } ;
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+2, 0),
		TRXN_WRITE1(q),
		TRXN_END,
	} ;
	
	return BUS_transaction(t, pn) ;
}
