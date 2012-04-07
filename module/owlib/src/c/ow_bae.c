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
WRITE_FUNCTION(FS_w_iic);
WRITE_FUNCTION(FS_r_iic);
WRITE_FUNCTION(FS_w_seq_text);
WRITE_FUNCTION(FS_w_sector_nr);
READ_FUNCTION(FS_r_sector_nr);
WRITE_FUNCTION(FS_w_sector_data);
READ_FUNCTION(FS_r_sector_data);

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

READ_FUNCTION(FS_r_cnt1_bit) ;
READ_FUNCTION(FS_r_cnt2_bit) ;
WRITE_FUNCTION(FS_w_cnt1_bit);
WRITE_FUNCTION(FS_w_cnt2_bit);

READ_FUNCTION(FS_r_cmp_bit);
WRITE_FUNCTION(FS_w_cmp_bit);

/* ------- Structures ----------- */

/* common values to all FC chips */


#define _FC_EEPROM_PAGE_SIZE   512
#define _FC_SDCARD_SECTOR_SIZE 512
#define _FC_MAX_EEPROM_PAGES    32
#define _FC_EEPROM_OFFSET   0x0000 //changed adressing scheme, valid from 0911 models 
#define _FC_MAX_FLASH_SIZE  0x4000
#define _FC_MAX_INTERLEAVE     255
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

#define _FC03_IICC       0x7E00    /* u8 */
#define _FC03_LCDC       0x7E01    /* u8 */
#define _FC03_SCIC       0x7E02    /* u16 */
#define _FC03_SDC        0x7E04    /* u8 */
#define _FC03_SPIC       0x7E05    /* u16 */
#define _FC03_PC0        0x7E07    /* u16 */
#define _FC03_CLATCH     0x7E09    /* u8 */

#define _FC03_SEQW_LCDD 0x00
#define _FC03_SEQW_SD 0x02
#define _FC03_SEQW_IIC 0x03
#define _FC03_SEQW_SCI 0x04
#define _FC03_SEQR_SD 0x00
#define _FC03_SEQR_IIC 0x01
#define _FC03_SEQR_SCI 0x02

#define _FC03_WR_BEGIN_IIC 0x00
#define _FC03_WR_END_IIC 0x01
#define _FC03_WR_BEGIN_SD_W 0x02
#define _FC03_WR_END_SD_W 0x03
#define _FC03_WR_BEGIN_SD_R 0x04
#define _FC03_WR_END_SD_R 0x05

#define _FC03_ADC0       0x7F00    /* u16 */
#define _FC03_ADC1       0x7F02    /* u16 */
#define _FC03_ADC2       0x7F04    /* u16 */
#define _FC03_ADC3       0x7F06    /* u16 */
#define _FC03_ADC4       0x7F08    /* u16 */
#define _FC03_ADC5       0x7F0A    /* u16 */
#define _FC03_ADC6       0x7F0C    /* u16 */
#define _FC03_ADC7       0x7F0E    /* u16 */
#define _FC03_ADC8       0x7F10    /* u16 */
#define _FC03_ADC9       0x7F12    /* u16 */
#define _FC03_ADC10      0x7F14    /* u16 */
#define _FC03_ADC11      0x7F16    /* u16 */
#define _FC03_ADC12      0x7F18    /* u16 */
#define _FC03_ADC13      0x7F1A    /* u16 */
#define _FC03_ADC14      0x7F1C    /* u16 */
#define _FC03_ADC15      0x7F1E    /* u16 */
#define _FC03_COUNT0     0x7F20    /* u32 */
#define _FC03_COUNT1     0x7F24    /* u32 */
#define _FC03_COUNT2     0x7F28    /* u32 */
#define _FC03_COUNT3     0x7F2C    /* u32 */
#define _FC03_COUNT4     0x7F30    /* u32 */
#define _FC03_COUNT5     0x7F34    /* u32 */
#define _FC03_COUNT6     0x7F38    /* u32 */
#define _FC03_COUNT7     0x7F3C    /* u32 */
#define _FC03_COUNT8     0x7F40    /* u32 */
#define _FC03_COUNT9     0x7F44    /* u32 */
#define _FC03_COUNT10    0x7F48    /* u32 */
#define _FC03_COUNT11    0x7F4C    /* u32 */
#define _FC03_COUNT12    0x7F50    /* u32 */
#define _FC03_COUNT13    0x7F54    /* u32 */
#define _FC03_COUNT14    0x7F58    /* u32 */
#define _FC03_COUNT15    0x7F5C    /* u32 */
#define _FC03_COUNT16    0x7F60    /* u32 */
#define _FC03_COUNT17    0x7F64    /* u32 */
#define _FC03_COUNT18    0x7F68    /* u32 */
#define _FC03_COUNT19    0x7F6C    /* u32 */
#define _FC03_COUNT20    0x7F70    /* u32 */
#define _FC03_COUNT21    0x7F74    /* u32 */
#define _FC03_EALARMC    0x7F78    /* u8 */
#define _FC03_OWIDLETIME 0x7F79    /* u8 */
#define _FC03_RTCH       0x7F7A    /* u32 */
#define _FC03_RTCL       0x7F7E    /* u8 */
#define _FC03_TIMER1     0x7F7F    /* u16 */
#define _FC03_TIMER2     0x7F81    /* u16 */
#define _FC03_UALARM     0x7F83    /* u32 */
#define _FC03_UALARM_EN  0x7F87    /* u32 */
#define _FC03_USER_ARRAY 0x7F8B    /* u256 */
#define _FC03_USER_BYTES 0x7FAB    /* u128 */
#define _FC03_USER_WORDS 0x7FBB    /* u256 */
#define _FC03_USER_WW    0x7FDB    /* u256 */




#define _FC03_PIO0       0x7900    /* u8 */
#define _FC03_PIO1       0x7901    /* u8 */
#define _FC03_PIO2       0x7902    /* u8 */
#define _FC03_PIO3       0x7903    /* u8 */
#define _FC03_PIO4       0x7904    /* u8 */
#define _FC03_PIO5       0x7905    /* u8 */
#define _FC03_PIO6       0x7906    /* u8 */
#define _FC03_PIO7       0x7907    /* u8 */
#define _FC03_PIO8       0x7908    /* u8 */
#define _FC03_PIO9       0x7909    /* u8 */
#define _FC03_PIO10      0x790A    /* u8 */
#define _FC03_PIO11      0x790B    /* u8 */
#define _FC03_PIO12      0x790C    /* u8 */
#define _FC03_PIO13      0x790D    /* u8 */
#define _FC03_PIO14      0x790E    /* u8 */
#define _FC03_PIO15      0x790F    /* u8 */
#define _FC03_PIO16      0x7910    /* u8 */
#define _FC03_PIO17      0x7911    /* u8 */
#define _FC03_PIO18      0x7912    /* u8 */
#define _FC03_PIO19      0x7913    /* u8 */
#define _FC03_PIO20      0x7914    /* u8 */
#define _FC03_PIO21      0x7915    /* u8 */
#define _FC03_PIODIR0    0x7916    /* u8 */
#define _FC03_PIODIR1    0x7917    /* u8 */
#define _FC03_PIODIR2    0x7918    /* u8 */
#define _FC03_PIODIR3    0x7919    /* u8 */
#define _FC03_PIODIR4    0x791A    /* u8 */
#define _FC03_PIODIR5    0x791B    /* u8 */
#define _FC03_PIODIR6    0x791C    /* u8 */
#define _FC03_PIODIR7    0x791D    /* u8 */
#define _FC03_PIODIR8    0x791E    /* u8 */
#define _FC03_PIODIR9    0x791F    /* u8 */
#define _FC03_PIODIR10   0x7920    /* u8 */
#define _FC03_PIODIR11   0x7921    /* u8 */
#define _FC03_PIODIR12   0x7922    /* u8 */
#define _FC03_PIODIR13   0x7923    /* u8 */
#define _FC03_PIODIR14   0x7924    /* u8 */
#define _FC03_PIODIR15   0x7925    /* u8 */
#define _FC03_PIODIR16   0x7926    /* u8 */
#define _FC03_PIODIR17   0x7927    /* u8 */
#define _FC03_PIODIR18   0x7928    /* u8 */
#define _FC03_PIODIR19   0x7929    /* u8 */
#define _FC03_PIODIR20   0x792A    /* u8 */
#define _FC03_PIODIR21   0x792B    /* u8 */
#define _FC03_PIODS0     0x792C    /* u8 */
#define _FC03_PIODS1     0x792D    /* u8 */
#define _FC03_PIODS2     0x792E    /* u8 */
#define _FC03_PIODS3     0x792F    /* u8 */
#define _FC03_PIODS4     0x7930    /* u8 */
#define _FC03_PIODS5     0x7931    /* u8 */
#define _FC03_PIODS6     0x7932    /* u8 */
#define _FC03_PIODS7     0x7933    /* u8 */
#define _FC03_PIODS8     0x7934    /* u8 */
#define _FC03_PIODS9     0x7935    /* u8 */
#define _FC03_PIODS10    0x7936    /* u8 */
#define _FC03_PIODS11    0x7937    /* u8 */
#define _FC03_PIODS12    0x7938    /* u8 */
#define _FC03_PIODS13    0x7939    /* u8 */
#define _FC03_PIODS14    0x793A    /* u8 */
#define _FC03_PIODS15    0x793B    /* u8 */
#define _FC03_PIODS16    0x793C    /* u8 */
#define _FC03_PIODS17    0x793D    /* u8 */
#define _FC03_PIODS18    0x793E    /* u8 */
#define _FC03_PIODS19    0x793F    /* u8 */
#define _FC03_PIODS20    0x7940    /* u8 */
#define _FC03_PIODS21    0x7941    /* u8 */
#define _FC03_PIOPE0     0x7942    /* u8 */
#define _FC03_PIOPE1     0x7943    /* u8 */
#define _FC03_PIOPE2     0x7944    /* u8 */
#define _FC03_PIOPE3     0x7945    /* u8 */
#define _FC03_PIOPE4     0x7946    /* u8 */
#define _FC03_PIOPE5     0x7947    /* u8 */
#define _FC03_PIOPE6     0x7948    /* u8 */
#define _FC03_PIOPE7     0x7949    /* u8 */
#define _FC03_PIOPE8     0x794A    /* u8 */
#define _FC03_PIOPE9     0x794B    /* u8 */
#define _FC03_PIOPE10    0x794C    /* u8 */
#define _FC03_PIOPE11    0x794D    /* u8 */
#define _FC03_PIOPE12    0x794E    /* u8 */
#define _FC03_PIOPE13    0x794F    /* u8 */
#define _FC03_PIOPE14    0x7950    /* u8 */
#define _FC03_PIOPE15    0x7951    /* u8 */
#define _FC03_PIOPE16    0x7952    /* u8 */
#define _FC03_PIOPE17    0x7953    /* u8 */
#define _FC03_PIOPE18    0x7954    /* u8 */
#define _FC03_PIOPE19    0x7955    /* u8 */
#define _FC03_PIOPE20    0x7956    /* u8 */
#define _FC03_PIOPE21    0x7957    /* u8 */
#define _FC03_LATCH0     0x7958    /* u8 */
#define _FC03_LATCH1     0x7959    /* u8 */
#define _FC03_LATCH2     0x795A    /* u8 */
#define _FC03_LATCH3     0x795B    /* u8 */
#define _FC03_LATCH4     0x795C    /* u8 */
#define _FC03_LATCH5     0x795D    /* u8 */
#define _FC03_LATCH6     0x795E    /* u8 */
#define _FC03_LATCH7     0x795F    /* u8 */
#define _FC03_LATCH8     0x7960    /* u8 */
#define _FC03_LATCH9     0x7961    /* u8 */
#define _FC03_LATCH10    0x7962    /* u8 */
#define _FC03_LATCH11    0x7963    /* u8 */
#define _FC03_LATCH12    0x7964    /* u8 */
#define _FC03_LATCH13    0x7965    /* u8 */
#define _FC03_LATCH14    0x7966    /* u8 */
#define _FC03_LATCH15    0x7967    /* u8 */
#define _FC03_LATCH16    0x7968    /* u8 */
#define _FC03_LATCH17    0x7969    /* u8 */
#define _FC03_LATCH18    0x796A    /* u8 */
#define _FC03_LATCH19    0x796B    /* u8 */
#define _FC03_LATCH20    0x796C    /* u8 */
#define _FC03_LATCH21    0x796D    /* u8 */
#define _FC03_ECOUNT0    0x796E    /* u8 */
#define _FC03_ECOUNT1    0x796F    /* u8 */
#define _FC03_ECOUNT2    0x7970    /* u8 */
#define _FC03_ECOUNT3    0x7971    /* u8 */
#define _FC03_ECOUNT4    0x7972    /* u8 */
#define _FC03_ECOUNT5    0x7973    /* u8 */
#define _FC03_ECOUNT6    0x7974    /* u8 */
#define _FC03_ECOUNT7    0x7975    /* u8 */
#define _FC03_ECOUNT8    0x7976    /* u8 */
#define _FC03_ECOUNT9    0x7977    /* u8 */
#define _FC03_ECOUNT10   0x7978    /* u8 */
#define _FC03_ECOUNT11   0x7979    /* u8 */
#define _FC03_ECOUNT12   0x797A    /* u8 */
#define _FC03_ECOUNT13   0x797B    /* u8 */
#define _FC03_ECOUNT14   0x797C    /* u8 */
#define _FC03_ECOUNT15   0x797D    /* u8 */
#define _FC03_ECOUNT16   0x797E    /* u8 */
#define _FC03_ECOUNT17   0x797F    /* u8 */
#define _FC03_ECOUNT18   0x7980    /* u8 */
#define _FC03_ECOUNT19   0x7981    /* u8 */
#define _FC03_ECOUNT20   0x7982    /* u8 */
#define _FC03_ECOUNT21   0x7983    /* u8 */
#define _FC03_ALARM0     0x7984    /* u8 */
#define _FC03_ALARM1     0x7985    /* u8 */
#define _FC03_ALARM2     0x7986    /* u8 */
#define _FC03_ALARM3     0x7987    /* u8 */
#define _FC03_ALARM4     0x7988    /* u8 */
#define _FC03_ALARM5     0x7989    /* u8 */
#define _FC03_ALARM6     0x798A    /* u8 */
#define _FC03_ALARM7     0x798B    /* u8 */
#define _FC03_ALARM8     0x798C    /* u8 */
#define _FC03_ALARM9     0x798D    /* u8 */
#define _FC03_ALARM10    0x798E    /* u8 */
#define _FC03_ALARM11    0x798F    /* u8 */
#define _FC03_ALARM12    0x7990    /* u8 */
#define _FC03_ALARM13    0x7991    /* u8 */
#define _FC03_ALARM14    0x7992    /* u8 */
#define _FC03_ALARM15    0x7993    /* u8 */
#define _FC03_ALARM16    0x7994    /* u8 */
#define _FC03_ALARM17    0x7995    /* u8 */
#define _FC03_ALARM18    0x7996    /* u8 */
#define _FC03_ALARM19    0x7997    /* u8 */
#define _FC03_ALARM20    0x7998    /* u8 */
#define _FC03_ALARM21    0x7999    /* u8 */
#define _FC03_EENCODER0  0x799A    /* u8 */
#define _FC03_EENCODER5  0x799B    /* u8 */
#define _FC03_EENCODER8  0x799C    /* u8 */
#define _FC03_EENCODER9  0x799D    /* u8 */
#define _FC03_EENCODER16 0x799E    /* u8 */
#define _FC03_EENCODER17 0x799F    /* u8 */
#define _FC03_PULLDOWN0  0x79A0    /* u8 */
#define _FC03_PULLDOWN1  0x79A1    /* u8 */
#define _FC03_PULLDOWN2  0x79A2    /* u8 */
#define _FC03_PULLDOWN3  0x79A3    /* u8 */
#define _FC03_PULLDOWN4  0x79A4    /* u8 */
#define _FC03_PULLDOWN5  0x79A5    /* u8 */
#define _FC03_PULLDOWN6  0x79A6    /* u8 */
#define _FC03_PULLDOWN7  0x79A7    /* u8 */
#define _FC03_CAPFLAG1   0x79A8    /* u8 */
#define _FC03_CAPFLAG2   0x79A9    /* u8 */
#define _FC03_CAPFLAG3   0x79AA    /* u8 */
#define _FC03_CAPFLAG4   0x79AB    /* u8 */
#define _FC03_ACMPE      0x79AC    /* u8 */
#define _FC03_ACMPIS     0x79AD    /* u8 */
#define _FC03_ACMPLATCH  0x79AE    /* u8 */
#define _FC03_ACMPO      0x79AF    /* u8 */
#define _FC03_ACMPOE     0x79B0    /* u8 */
#define _FC03_TPM1C      0x7A00    /* u8 */
#define _FC03_TPM2C      0x7A01    /* u8 */

#define _FC03_TPM1C      0x7A00    /* u8 */
#define _FC03_TPM2C      0x7A01    /* u8 */
#define _FC03_PERIOD1    0x7A02    /* u16 */
#define _FC03_PERIOD2    0x7A04    /* u16 */
#define _FC03_DUTY1C0    0x7A06    /* u16 */
#define _FC03_DUTY1C1    0x7A08    /* u16 */
#define _FC03_DUTY2C0    0x7A0A    /* u16 */
#define _FC03_DUTY2C1    0x7A0C    /* u16 */
#define _FC03_C1CH0      0x7A0E    /* u8 */
#define _FC03_C1CH1      0x7A0F    /* u8 */
#define _FC03_C2CH0      0x7A10    /* u8 */
#define _FC03_C2CH1      0x7A11    /* u8 */
#define _FC03_NBYTES     0x7A12    /* u8 */
 




/* Placeholder for special vsibility code for firmware types "personalities" */
static enum e_visibility VISIBLE_eeprom_pages( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_eeprom_bytes( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_generic( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_910( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_911( const struct parsedname * pn ) ;


static struct aggregate ABAEeeprom = { _FC_MAX_EEPROM_PAGES, ag_numbers, ag_separate, };
static struct aggregate A911pio = { 22, ag_numbers, ag_separate, };
static struct aggregate A911piodd = { 22, ag_numbers, ag_separate, };
static struct aggregate A911piods = { 22, ag_numbers, ag_separate, };
static struct aggregate A911piope = { 22, ag_numbers, ag_separate, };
static struct aggregate A911piopd = { 8, ag_numbers, ag_separate, };
static struct aggregate A911latch = { 22, ag_numbers, ag_separate, };
static struct aggregate A911elatch = { 22, ag_numbers, ag_separate, };
static struct aggregate A911counter = { 18, ag_numbers, ag_separate, };
static struct aggregate A911ecount = { 18, ag_numbers, ag_separate, };
static struct aggregate A911pwm = { 4, ag_numbers, ag_separate, };
static struct aggregate A911adc = { 16, ag_numbers, ag_separate, };
static struct aggregate A911userb = { 16, ag_numbers, ag_separate, };
static struct aggregate A911userw = { 16, ag_numbers, ag_separate, };
static struct aggregate A911userdw = { 8, ag_numbers, ag_separate, };
static struct aggregate A911userarray = { 32, ag_numbers, ag_separate, };

static struct filetype BAE[] = {
	F_STANDARD,
	{"command", _FC02_MAX_COMMAND_GULP, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_extended, VISIBLE, NO_FILETYPE_DATA, },
	//{"writebyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_read_stable, NO_READ_FUNCTION, FS_writebyte, VISIBLE, NO_FILETYPE_DATA, },
	{"versionstate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_version_state, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"version", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"device_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_device, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"bootstrap_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_bootstrap, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"typestate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_type_state, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"localtype", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"device_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_device, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"chip_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_chip, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"firmware", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"firmware/function", _FC_MAX_FLASH_SIZE, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_flash, VISIBLE, NO_FILETYPE_DATA, },

	{"eeprom", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
//	{"eeprom/memory",_FC_EEPROM_PAGE_SIZE*_FC_MAX_EEPROM_PAGES, NON_AGGREGATE, ft_binary, fc_link, FS_eeprom_r_mem, FS_eeprom_w_mem, VISIBLE_eeprom_bytes, NO_FILETYPE_DATA, },
	{"eeprom/page",_FC_EEPROM_PAGE_SIZE, &ABAEeeprom, ft_binary, fc_page, FS_eeprom_r_page, FS_eeprom_w_page, VISIBLE_eeprom_pages, NO_FILETYPE_DATA, },
	{"eeprom/erase", PROPERTY_LENGTH_YESNO, &ABAEeeprom, ft_yesno, fc_link, NO_READ_FUNCTION, FS_eeprom_erase, VISIBLE_eeprom_pages, NO_FILETYPE_DATA, },

	{"generic", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_generic, NO_FILETYPE_DATA, },
	{"generic/memory", _FC02_MEMORY_SIZE, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE_generic, NO_FILETYPE_DATA, },
	
	{"910", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_910, NO_FILETYPE_DATA, },
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
	{"910/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE_910, {a:"910/udate"}, },
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

	{"911", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/automation_engine", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/automation_engine/pc0", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PC0,}, },
	{"911/pio", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/pio/pio",   PROPERTY_LENGTH_YESNO, &A911pio , ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_PIO0,}, },
	{"911/pio/direction", PROPERTY_LENGTH_YESNO, &A911piodd, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_PIODIR0,}, },
	{"911/pio/strenght",  PROPERTY_LENGTH_YESNO, &A911piods, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_PIODS0,}, },
	{"911/pio/pull_enable", PROPERTY_LENGTH_YESNO, &A911piope, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_PIOPE0,}, },
	{"911/pio/pull_down", PROPERTY_LENGTH_YESNO, &A911piopd, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_PULLDOWN0,}, },
	{"911/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/alarm/latch", PROPERTY_LENGTH_YESNO, &A911latch, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_LATCH0,}, },
	{"911/alarm/enable_latch", PROPERTY_LENGTH_YESNO, &A911elatch, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_ALARM0,}, },
	{"911/alarm/clear_latchs",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_CLATCH,}, },
	{"911/alarm/enable_global",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EALARMC,}, },
	{"911/alarm/userbits",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_unsigned, fc_volatile,  FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_UALARM,}, },
	{"911/alarm/userbits_enable",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_unsigned, fc_volatile,  FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_UALARM_EN,}, },
	{"911/counters", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/counters/count", PROPERTY_LENGTH_UNSIGNED, &A911counter, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_COUNT0,}, },
	{"911/counters/enable_counter", PROPERTY_LENGTH_YESNO, &A911ecount, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_ECOUNT0,}, },
	{"911/counters/enable_encoder0",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EENCODER0,}, },
	{"911/counters/enable_encoder5",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EENCODER5,}, },
	{"911/counters/enable_encoder8",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EENCODER8,}, },
	{"911/counters/enable_encoder9",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EENCODER9,}, },
	{"911/counters/enable_encoder16",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EENCODER16,}, },
	{"911/counters/enable_encoder17",  PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_EENCODER17,}, },
  {"911/timers", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/timers/countdown", PROPERTY_LENGTH_UNSIGNED, &A911userarray, ft_unsigned, fc_volatile, FS_r_8, FS_w_8,  VISIBLE_911, {u:_FC03_USER_ARRAY,}, },
	{"911/timers/countdown_number",  PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_NBYTES,}, },
	{"911/timers/clock", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile,  FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_RTCH,}, },
	{"911/timers/random", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile,  FS_r_8, NO_WRITE_FUNCTION, VISIBLE_911, {u:_FC03_RTCL,}, },
	{"911/user_registers", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/user_registers/byte", PROPERTY_LENGTH_UNSIGNED, &A911userb, ft_unsigned, fc_volatile, FS_r_8, FS_w_8,  VISIBLE_911, {u:_FC03_USER_BYTES,}, },
	{"911/user_registers/word", PROPERTY_LENGTH_UNSIGNED, &A911userw, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_USER_WORDS,}, },
	{"911/user_registers/dword", PROPERTY_LENGTH_UNSIGNED, &A911userdw, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_USER_WW,}, },
	{"911/analog", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/analog/adc", PROPERTY_LENGTH_UNSIGNED, &A911adc, ft_unsigned, fc_volatile, FS_r_16, NO_WRITE_FUNCTION, VISIBLE_911, {u:_FC03_ADC0,}, },
	{"911/analog/comparator", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/analog/comparator/acmp_enable", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_ACMPE,}, },
	{"911/analog/comparator/out_enable", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_ACMPOE,}, },
	{"911/analog/comparator/out_state", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, VISIBLE_911, {u:_FC03_ACMPO,}, },
	{"911/analog/comparator/internal_ref", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_ACMPIS,}, },
	{"911/analog/comparator/changed", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_ACMPLATCH,}, },
	{"911/pwm1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/pwm1/ref_clock", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_TPM1C,}, },
	{"911/pwm1/period", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PERIOD1,}, },
	{"911/pwm1/mode0", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_C1CH0,}, },
	{"911/pwm1/mode1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_C1CH1,}, },
	{"911/pwm1/duty0", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_DUTY1C0,}, },
	{"911/pwm1/duty1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_DUTY1C1,}, },
	{"911/pwm2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/pwm2/ref_clock", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_TPM2C,}, },
	{"911/pwm2/period", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PERIOD2,}, },
	{"911/pwm2/mode0", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_C2CH0,}, },
	{"911/pwm2/mode1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_C2CH1,}, },
	{"911/pwm2/duty0", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_DUTY2C0,}, },
	{"911/pwm2/duty1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_DUTY2C1,}, },
	{"911/iic", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/iic/init", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_IICC,}, },
	{"911/iic/cat9554", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/iic/cat9554/direction", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_iic, FS_w_iic, VISIBLE_911, {a:"x40x03$0-x40x03=$0"}, },
	{"911/iic/cat9554/pio", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_iic, FS_w_iic, VISIBLE_911, {a:"x40x01$0-x40x01=$0"}, },
	{"911/sdcard", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/sdcard/init", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SDC,}, },
	{"911/sdcard/sector_nr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_sector_nr, FS_w_sector_nr, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/sdcard/sector_data", _FC_SDCARD_SECTOR_SIZE, NON_AGGREGATE, ft_binary, fc_stable, FS_r_sector_data, FS_w_sector_data, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/lcd", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/lcd/init", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_LCDC,}, },
	{"911/lcd/text", 255, NON_AGGREGATE, ft_ascii, fc_volatile, NO_READ_FUNCTION, FS_w_seq_text, VISIBLE_911, {u:_FC03_SEQW_LCDD,}, },
	{"911/serial", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/serial/scic", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_SCIC,}, },
	{"911/serial/dataout", 255, NON_AGGREGATE, ft_ascii, fc_volatile, NO_READ_FUNCTION, FS_w_seq_text, VISIBLE_911, {u:_FC03_SEQW_SCI,}, },

	
/*{"911/pio/piostate", PROPERTY_LENGTH_UNSIGNED, &A911pio, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u:_FC03_PIO0,}, },
	{"911/pio/pio",    PROPERTY_LENGTH_YESNO, &A911pio, ft_yesno, fc_link, FS_r_911_pio, FS_w_911_pio, VISIBLE_911, NO_FILETYPE_DATA, },
  {"911/pio/pio_config", PROPERTY_LENGTH_UNSIGNED, &A911pioc, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u:_FC03_PIO_CONF,}, },
	{"911/pio/pio_ds", PROPERTY_LENGTH_YESNO, &A911pioc, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {u:3,}, },
	{"911/pio/pio_pe", PROPERTY_LENGTH_YESNO, &A911pioc, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {u:1,}, },
	{"911/pio/pio_dd", PROPERTY_LENGTH_YESNO, &A911pioc, ft_yesno, fc_link, FS_r_pio_bit, FS_w_pio_bit, VISIBLE_911, {u:0,}, },


	{"911/counter1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/counter1/count", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_CNT1,}, },
	{"911/counter1/config", PROPERTY_LENGTH_UNSIGNED, &A911cnt1, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u:_FC03_PIO_CONF+10,}, },
	{"911/counter1/pulldown", PROPERTY_LENGTH_YESNO, &A911cnt1, ft_yesno, fc_link, FS_r_cnt1_bit, FS_w_cnt1_bit, VISIBLE_911, {u:2,}, },
	{"911/counter1/enable", PROPERTY_LENGTH_YESNO, &A911cnt1, ft_yesno, fc_link, FS_r_cnt1_bit, FS_w_cnt1_bit, VISIBLE_911, {u:4,}, },
	
	{"911/counter2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/counter2/count", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_32, FS_w_32, VISIBLE_911, {u:_FC03_CNT2,}, },
	{"911/counter2/config", PROPERTY_LENGTH_UNSIGNED, &A911cnt2, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u:_FC03_PIO_CONF+16,}, },
	{"911/counter2/pulldown", PROPERTY_LENGTH_YESNO, &A911cnt2, ft_yesno, fc_link, FS_r_cnt2_bit, FS_w_cnt2_bit, VISIBLE_911, {u:2,}, },
	{"911/counter2/enable", PROPERTY_LENGTH_YESNO, &A911cnt2, ft_yesno, fc_link, FS_r_cnt2_bit, FS_w_cnt2_bit, VISIBLE_911, {u:4,}, },


	{"911/pwm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/pwm/tpm1c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_TPM1C,}, },
	{"911/pwm/tpm2c", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_TPM2C,}, },
	{"911/pwm/period1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PERIOD1,}, },
	{"911/pwm/period2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_PERIOD2,}, },
	{"911/pwm/duty", PROPERTY_LENGTH_UNSIGNED, &A911pwm, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_DUTY,}, },

	
	
	{"911/spi", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/spi/spic", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SPIC,}, },
	{"911/spi/spibr", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SPIBR,}, },
	{"911/spi/spid", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_911, {u:_FC03_SPID,}, },

	{"911/serial", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_911, NO_FILETYPE_DATA, },
	{"911/serial/scic", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_16, FS_w_16, VISIBLE_911, {u:_FC03_SCIC,}, },
*/
};

DeviceEntryExtended(FC, BAE, DEV_resume | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE );

/* <AE command codes */
#define _1W_ECMD_ERASE_FIRMWARE 0xBB
#define _1W_ECMD_FLASH_FIRMWARE 0xBA

#define _1W_BEGIN_IIC 0x00
#define _1W_END_IIC 0x01
#define _1W_BEGIN_LCD 0x02
#define _1W_BEGIN_SD_W 0x03
#define _1W_END_SD_W 0x04
#define _1W_BEGIN_SD_R 0x05
#define _1W_END_SD_R 0x06

#define _1W_SEQW_LCD 0
#define _1W_SEQW_SD 1
#define _1W_SEQW_IIC 2
#define _1W_SEQW_SCI 3
#define _1W_SEQR_SD 0
#define _1W_SEQR_IIC 1
#define _1W_SEQR_SCI 2


#define _1W_READ_VERSION 0x11
#define _1W_READ_TYPE 0x12
#define _1W_EXTENDED_COMMAND 0x13
#define _1W_READ_BLOCK_WITH_LEN 0x14
#define _1W_WRITE_BLOCK_WITH_LEN 0x15
#define _1W_ERASE_EEPROM_PAGE 0x16

#define _1W_WRCMD			  	0x17
#define _1W_SEQWCMD  			0x18
#define _1W_SEQRCMD  			0x19
#define _1W_QUERY_DATA_COMMAND 	0x1A
#define _1W_CONFIRM_WRITE 0xBC

/* Persistent storage */
Make_SlaveSpecificTag(SNR, fc_persistent);	// sector number

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

static GOOD_OR_BAD OW_wr_complete_transaction( int wlen,BYTE func, int* rlen, BYTE * wparam, BYTE * rparam, int timeout, struct parsedname *pn);
static GOOD_OR_BAD OW_seqw_complete_transaction(int wlen, BYTE func, BYTE * wparam, int timeout, struct parsedname *pn);
static GOOD_OR_BAD OW_seqr_complete_transaction(int *rlen,BYTE func,BYTE * rparam, int timeout,  struct parsedname *pn);
static GOOD_OR_BAD OW_query_cmd(int *prlen, BYTE *retcode,BYTE * rparam , struct parsedname *pn);

/*static GOOD_OR_BAD OW_w_flow_nocrc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size, struct parsedname *pn);
  static GOOD_OR_BAD OW_r_flow_nocrc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size, struct parsedname *pn);
  static GOOD_OR_BAD OW_w_flow_crc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size,size_t chunksize, struct parsedname *pn);
*/

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

	if ( BAD( Cache_Get_SlaveSpecific((void *) &sector_nr, sizeof(UINT), SlaveSpecificTag(SNR), pn)) ) { // record doesn't (yet) exist
		sector_nr=0;
	} 
	OWQ_U(owq) = sector_nr;
	if (Cache_Add_SlaveSpecific((void *) &sector_nr, sizeof(UINT), SlaveSpecificTag(SNR), pn)) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_sector_nr(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	UINT sector_nr;					

	sector_nr = OWQ_U(owq);
	FS_del_sibling( "911/sdcard/sector_data", owq ) ;
	if (Cache_Add_SlaveSpecific((void *) &sector_nr, sizeof(UINT), SlaveSpecificTag(SNR), pn)) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_sector_data(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE *buffer = (BYTE*)OWQ_buffer(owq);
	BYTE rparam[_FC02_MAX_WRITE_GULP+1];
	BYTE wparam[4] ; 
	UINT sector_nr;					
	int size=(int)OWQ_size(owq);
	int wlen,rlen,i,retry;
	int timeout=100;
	if (BAD( Cache_Get_SlaveSpecific((void *) &sector_nr, sizeof(UINT), SlaveSpecificTag(SNR), pn))) sector_nr=0; // record doesn't (yet) exist

	LEVEL_DEBUG("write SD sector %d data len=%d",sector_nr,size ) ;
	if (OWQ_offset(owq)) return -ERANGE;
	if (size!=512) return -ERANGE;
	BAE_uint32_to_bytes( sector_nr*512, wparam ); // convert 32bit value to msb first
	rlen=0;
	wlen=4;
	RETURN_ERROR_IF_BAD(OW_wr_complete_transaction(wlen,_FC03_WR_BEGIN_SD_W, &rlen, wparam, rparam, timeout, pn));
	wlen= _FC02_MAX_WRITE_GULP;
	retry=0;
	for(i=size;i;)
	{
		if (i>_FC02_MAX_WRITE_GULP) wlen=_FC02_MAX_WRITE_GULP; else wlen=i;
		LEVEL_DEBUG("WRITE SD %d bytes at offset %d ",size,512-i ) ;
		if (BAD(OW_seqw_complete_transaction(wlen,_FC03_SEQW_SD, buffer, timeout, pn)))
		{
			if (retry++>3) return gbBAD;
		}
		else
		{
			buffer+=wlen;
			i-=wlen;
			retry=0;
		}
	}
	rlen=0;
	wlen=0;
	LEVEL_DEBUG("WRITE SD END, send trailer " ) ;
	return OW_wr_complete_transaction(wlen,_FC03_WR_END_SD_W, &rlen, wparam, rparam, timeout, pn);
}

static ZERO_OR_ERROR FS_r_sector_data(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE *buffer = (BYTE*)OWQ_buffer(owq);
	BYTE rparam[_FC02_MAX_READ_GULP+1];
	BYTE wparam[4] ; 
	UINT sector_nr;					
	int size=(int)OWQ_size(owq);
	int wlen,rlen,i,retry;
	int timeout=100;
	if (BAD( Cache_Get_SlaveSpecific((void *) &sector_nr, sizeof(UINT), SlaveSpecificTag(SNR), pn))) sector_nr=0; // record doesn't (yet) exist

	LEVEL_DEBUG("read SD sector %d data len=%d",sector_nr,size ) ;
	if (OWQ_offset(owq)) return -ERANGE;
	if (size!=512) return -ERANGE;
	BAE_uint32_to_bytes( sector_nr*512, wparam ); // convert 32bit value to msb first
	rlen=0;
	wlen=4;
	retry=0;
	RETURN_ERROR_IF_BAD(OW_wr_complete_transaction(wlen,_FC03_WR_BEGIN_SD_R, &rlen, wparam, rparam, timeout, pn));
	rlen= _FC02_MAX_READ_GULP;
	for(i=size;i;)
	{
		if (i>_FC02_MAX_READ_GULP) rlen=_FC02_MAX_WRITE_GULP; else rlen=i;
		LEVEL_DEBUG("READ SD %d bytes at offset %d ",size,512-i ) ;
		if (BAD(OW_seqr_complete_transaction(&rlen,_FC03_SEQR_SD, buffer, timeout, pn)))
		{
			if (retry++>3) return gbBAD;
		}
		else
		{
				buffer+=rlen;
				i-=rlen;
				retry=0;
		}
	}
	OWQ_length(owq) = OWQ_size(owq);
	rlen=0;
	wlen=0;
	LEVEL_DEBUG("READ SD END, send trailer " ) ;
	return OW_wr_complete_transaction(wlen,_FC03_WR_END_SD_R, &rlen, wparam, rparam, timeout, pn);
}


int hex_digit(BYTE c)
{
	int v;
	if ((c>='0') && (c<='9')) v=c-'0';
	else if ((c>='a') && (c<='f')) v=c-'a'+10;
			 else if ((c>='A') && (c<='F')) v=c-'A'+10;
			  		else v=-1000;
	return v;		  		
}

GOOD_OR_BAD scan_token_read(char* mask,BYTE *buffer, struct one_wire_query *owq)
{
int i,j,v;
char *pt=mask;
	LEVEL_DEBUG("begin token read loop: " ) ;
	OWQ_U(owq)=0;
	for(i=0;*pt;pt++)
	{
			v=hex_digit(*pt)*16 + hex_digit(*(pt+1));
			if (v>=0) { 
				pt+=2;
				if (buffer[i++]!=v) return gbBAD;  //if the received byte doen not match the mask, return error
			}
			switch (*pt)
			{
				case '?': i++; // '?' in mask means to ignore this received byte
				case '$': if ((*(pt+1)>='0') && (*(pt+1) <='7'))  //store received byte on position j
									{
										pt++;
										j=*pt-'0';
 									  OWQ_U(owq)|= buffer[i++]<<j; 
										LEVEL_DEBUG("after token assign: @%d(%d)  %08x", j,buffer[i], OWQ_U(owq) ) ;
									}
			}
	}
	return gbGOOD;
}


//mask example:set_part-get_part. ex.: "x40x01$0-x40x01=$0"
// set_part contains a writebloc and an optional readbloc separated by '=' :
//     ex.:"x40x01$0" write two fixed value bytes followed by lo byte of value to set, read nothing
// get_part contains a writebloc and a mandatory readbloc separated by '=' :
//     ex.:"x40x01=$0" write two fixed value bytes then read one byte. this read byte will be assigned to read value 


void scan_token_write(char* mask,BYTE *buffer,	int *plenw,int * plenr, struct one_wire_query *owq)
{
int i,j,v;
char *pt=mask;
	*plenw=-1;
	LEVEL_DEBUG("begin token write loop: " ) ;
	
	for(i=0;*pt;pt++)
	{
			v=hex_digit(*pt)*16 + hex_digit(*(pt+1));
			if (v>=0) { buffer[i++]=v;pt+=2;}
			switch (*pt)
			{
				case '=': *plenw=i; //write limit, after '=' this is the receive mask
									break;
				case '$': if ((*(pt+1)>='0') && (*(pt+1)<='7')) 
									{
										pt++;
										j=*pt-'0';
										LEVEL_DEBUG("token test: @%d= %d", j,BYTE_MASK(OWQ_U(owq)>>j) ) ;
										buffer[i++]= BYTE_MASK(OWQ_U(owq)>>j); // BYTE_MASK(OWQ_array_U(owq,j));
									}
									break;
			}
	}
	if (*plenw<0) *plenw=i; 
	*plenr=i-*plenw;
	buffer[i]='\0'; //just for level debug
}

static ZERO_OR_ERROR FS_w_iic(struct one_wire_query *owq) //generic write to iic
{
	struct parsedname *pn = PN(owq);
	BYTE wparam[(_FC02_MAX_WRITE_GULP+1)*2];
	BYTE *rparam;
	int wlen,rlen,timeout;
	char mask[255],*pt; 
	rparam=wparam+_FC02_MAX_WRITE_GULP+1;
	if (OWQ_offset(owq)) return -ERANGE;
	strcpy(mask,pn->selected_filetype->data.a);
	pt=strchr(mask,'-');
	if (pt==NULL) return -EINVAL; //invalid mask for generic iic
	*pt=0; //split mask write and mask read
	scan_token_write(mask, wparam, &wlen, &rlen, owq);
	timeout=100; 
	LEVEL_DEBUG("iic write: wlen=%d mask=%s wparam=%s", wlen,mask,wparam ) ;
	RETURN_ERROR_IF_BAD(OW_wr_complete_transaction(wlen,_FC03_WR_BEGIN_IIC, &rlen, wparam, rparam, timeout, pn));
	pt=strchr(mask,'=');
	if (pt) RETURN_ERROR_IF_BAD(scan_token_read(pt+1, rparam,  owq)); 
	wlen=rlen=0;
	return OW_wr_complete_transaction(wlen,_FC03_WR_END_IIC, &rlen, wparam, rparam, timeout, pn);
}

static ZERO_OR_ERROR FS_r_iic(struct one_wire_query *owq) //generic read from iic
{
	BYTE wparam[(_FC02_MAX_WRITE_GULP+1)*2];
	BYTE *rparam;
	int wlen, rlen, timeout;
	char mask[255],*pt;
	struct parsedname *pn = PN(owq);
	rparam=wparam+_FC02_MAX_WRITE_GULP+1;
	if (OWQ_offset(owq)) return -ERANGE;
	strcpy(mask,pn->selected_filetype->data.a);
	pt=strchr(mask,'-');
	if (pt==NULL) return -EINVAL; //invalid mask for generic iic
	scan_token_write(pt+1, wparam, &wlen, &rlen, owq); 
	pt=strchr(mask,'=');
	if (pt==NULL) return -EINVAL; //read mask has no '=' token!
	timeout=100;
	LEVEL_DEBUG("iic read: wlen=%d mask=%s wparam=%s", wlen,mask,wparam ) ;
	RETURN_ERROR_IF_BAD(OW_wr_complete_transaction(wlen,_FC03_WR_BEGIN_IIC, &rlen, wparam, rparam, timeout, pn));
	RETURN_ERROR_IF_BAD(scan_token_read(pt+1, rparam,  owq)); 
	wlen=rlen=0;
	return OW_wr_complete_transaction(wlen,_FC03_WR_END_IIC, &rlen, wparam, rparam, timeout, pn);
}

static ZERO_OR_ERROR FS_w_seq_text(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE *start_of_text = (BYTE*)OWQ_buffer(owq);
	int i,retry,wlen,timeout;
	int size=(int)OWQ_size(owq);
	BYTE func=BYTE_MASK(PN(owq)->selected_filetype->data.u);
	LEVEL_DEBUG("SEQ text func=%d len=%d, offset=%d",func, size,OWQ_offset(owq) ) ;
	if (func==_FC03_SEQW_LCDD) size--; //remove last CR for lcd
	retry=0;
	timeout=100;
  if (size<=0)	return -EINVAL;
	for(i=size;i;)
	{
		if (i>_FC02_MAX_WRITE_GULP) wlen=_FC02_MAX_WRITE_GULP; else wlen=i;
		LEVEL_DEBUG("WRITE seq %d bytes at position %d ",wlen,size-i ) ;
		if (BAD(OW_seqw_complete_transaction(wlen,func, start_of_text, timeout, pn)))
		{
			if (retry++>3) return -EINVAL;
		}
		else
		{
			start_of_text+=wlen;
			i-=wlen;
			retry=0;
		}
	}
	LEVEL_DEBUG("SEQ text end" ) ;
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
//just for diagnostic...   
        {UINT v;
            if (BAD(OW_version( &v, PN(owq) ) ))
           {      LEVEL_DEBUG("Cannot read version after begin flash initialization");}
            else LEVEL_DEBUG("version=%04X",v);
        }
        		
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

/*
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
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
	if ( FS_r_sibling_U( &pioconf, "911/pio/pio_config", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = UT_getbit( (void *) &pioconf, bitnr ) ;
    LEVEL_DEBUG("sibling bit read bitnr=%d, set=%04X  mask=%04X", bitnr ,OWQ_Y(owq)<<bitnr, 1<<bitnr) ;
	return 0 ;
} 
static ZERO_OR_ERROR FS_w_pio_bit( struct one_wire_query * owq ) {
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
    LEVEL_DEBUG("sibling bit write bitnr=%d, set=%04X  mask=%04X", bitnr ,OWQ_Y(owq)<<bitnr, 1<<bitnr) ;
	return FS_w_sibling_bitwork( OWQ_Y(owq)<<bitnr, 1<<bitnr, "911/pio/pio_config", owq ) ;
}
static ZERO_OR_ERROR FS_r_cnt1_bit( struct one_wire_query * owq ) {
	UINT pioconf ;
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
    LEVEL_DEBUG("counter1/config: read bit nr %d ", bitnr ) ;
	if ( FS_r_sibling_U( &pioconf, "911/counter1/config", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = UT_getbit( (void *) &pioconf, bitnr ) ;
	return 0 ;
}
static ZERO_OR_ERROR FS_r_cnt2_bit( struct one_wire_query * owq ) {
    UINT pioconf ;
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
	if ( FS_r_sibling_U( &pioconf, "911/counter2/config", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = UT_getbit( (void *) &pioconf, bitnr ) ;
	return 0 ;
}

static ZERO_OR_ERROR FS_w_cnt1_bit( struct one_wire_query * owq ) {
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
	return FS_w_sibling_bitwork( OWQ_Y(owq)<<bitnr, 1<<bitnr, "911/counter1/config", owq ) ;
}
static ZERO_OR_ERROR FS_w_cnt2_bit( struct one_wire_query * owq ) {
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
	return FS_w_sibling_bitwork( OWQ_Y(owq)<<bitnr, 1<<bitnr, "911/counter2/config", owq ) ;
}

static ZERO_OR_ERROR FS_r_cmp_bit( struct one_wire_query * owq ) {
    UINT pioconf ;
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
	if ( FS_r_sibling_U( &pioconf, "911/acmp/config", owq ) != 0 ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = UT_getbit( (void *) &pioconf, bitnr ) ;
	return 0 ;
}
static ZERO_OR_ERROR FS_w_cmp_bit( struct one_wire_query * owq ) {
    UINT bitnr=PN(owq)->selected_filetype->data.u ;
	return FS_w_sibling_bitwork( OWQ_Y(owq)<<bitnr, 1<<bitnr, "911/acmp/config", owq ) ;
}
*/

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
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = data[0] ;
	return 0 ;
}

/* write an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_w_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	data[0] = BYTE_MASK( OWQ_U(owq) ) ;
	return GB_to_Z_OR_E(OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = BAE_uint16(data) ;
	return 0 ;
}

/* write a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;

	BAE_uint16_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 32/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = BAE_uint32(data) ;
	return 0 ;
}

/* write a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 32/8 ;
	BYTE data[bytes] ;
	
	BAE_uint32_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
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
/*
static GOOD_OR_BAD OW_w_flow_nocrc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size, struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_FLOW, BYTE_MASK(plen),wf, 0,  };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4+plen, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1),
		TRXN_WRITE(data, size),
		TRXN_END,
	};
	LEVEL_DEBUG("Send flow command") ;
// Copy to write buffer 
	memcpy(&p[4], param, plen);
	RETURN_BAD_IF_BAD( BUS_transaction(t, pn) );
	return gbGOOD ;	
}
// Flow write command WITH crc interleave 
static GOOD_OR_BAD OW_w_flow_crc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size,size_t chunksize, struct parsedname *pn)
{
	BYTE *pt;
	size_t i;
	BYTE p[1 + 1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_FLOW, BYTE_MASK(plen),wf, chunksize,  };
	BYTE chunk[_FC_MAX_INTERLEAVE+2] ;
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log header[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4+plen, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1),
	};
	struct transaction_log content[] = {
		TRXN_WR_CRC16(chunk, chunksize,0),
	};
	struct transaction_log remainder[] = {
		TRXN_WRITE(chunk, size % chunksize), //remainder of bloc if not multiple
	};
	struct transaction_log trailer[] = {
		TRXN_END,
	};
	LEVEL_DEBUG("Begin stream write of %d bytes", (int)size ) ;
	memcpy(&p[4], param, plen);
	RETURN_BAD_IF_BAD(BUS_transaction(header, pn));
	pt=data;
	for(i=size/chunksize;i;i--){
		memcpy(chunk, pt, chunksize);
		pt+=chunksize;
		RETURN_BAD_IF_BAD(BUS_transaction(content, pn));
		LEVEL_DEBUG("chunk write ok, remaining chunks=%d", (int)i ) ;
	}
	if (size % chunksize){
		memcpy(chunk, pt, size % chunksize);
		RETURN_BAD_IF_BAD( BUS_transaction(remainder, pn) ) ;
	}
	RETURN_BAD_IF_BAD( BUS_transaction(trailer, pn) ) ;
	return gbGOOD ;	
}


// Flow read command without crc 
static GOOD_OR_BAD OW_r_flow_nocrc(BYTE wf, BYTE * param, size_t plen, BYTE * data, size_t size, struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_FLOW, BYTE_MASK(plen), wf, 0, };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4+plen, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1),
		TRXN_READ(data, size),
		TRXN_END,
	};
	// Copy to write buffer 
	memcpy(&p[4], param, plen);
	RETURN_ERROR_IF_BAD( BUS_transaction(t, pn)) ;
	Debug_Bytes("BAE flow read",data,size) ;
	return gbGOOD ;	
}
*/

static GOOD_OR_BAD OW_poll_until_timeout(int *prlen, BYTE * rparam, int timeout, struct parsedname *pn)
{
	UINT tries;
	BYTE retcode;	
	int rlen;  //temporary var to hold len received rlen when retcode==0xff
	GOOD_OR_BAD r;
	LEVEL_DEBUG("BAE Query result until timeout (%d ms), expected rlen=%d", timeout, *prlen) ;
	tries=0;
	
	while (1)
	{
			LEVEL_DEBUG( "try %d when querying result from previous function.", tries ) ;
		  r=OW_query_cmd(&rlen, &retcode,rparam,pn );
		  if (BAD(r))
		  {
			  	++tries ;
					if ( tries > 5 ) 
					{
	 			 		LEVEL_DEBUG( "Too many failures getting result from previous function." ) ;
						return -EIO ;
					}
		  }
		  else
		  {
					if (retcode==0xff) 
					{
							timeout-=10;
							if (timeout<=0) 	{
									LEVEL_DEBUG( "Timeout getting result from previous function." ) ;
									return -EINVAL ;
							}
							UT_delay(10); //10ms delay if slave still busy
					}
					else 
					{
							 *prlen=rlen; //only when retcode is not 0xff 
							 LEVEL_DEBUG( "Previous command terminated with retcode=%d, rlen=%d.", retcode,rlen ) ;
							 if (retcode==0) return gbGOOD;
		 					 return -EINVAL ;
					}
			}
	}
}

//WR command 0X17  with query until return of result
static GOOD_OR_BAD OW_wr_complete_transaction( int wlen,BYTE func, int *rlen, BYTE * wparam, BYTE * rparam, int timeout, struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_WRCMD, BYTE_MASK(wlen), BYTE_MASK(func), BYTE_MASK(*rlen), 0, };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4+wlen, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1), //any WR commands require at least 1ms before query result
		TRXN_END,
	};
	LEVEL_DEBUG("BAE WR transaction function=%d, wlen=%d, rlen=%d", func, wlen, *rlen) ;
	Debug_Bytes("WR_cmd, data:",wparam,wlen) ;
	memcpy(p+4, wparam, wlen);
	RETURN_ERROR_IF_BAD(BUS_transaction(t, pn)) ;
	return OW_poll_until_timeout(rlen, rparam, timeout, pn);
}
// SEQW command 0X18 with query until return of result
static GOOD_OR_BAD OW_seqw_complete_transaction(int wlen, BYTE func, BYTE * wparam, int timeout, struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_SEQWCMD, BYTE_MASK(wlen), BYTE_MASK(func), 0, };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3+wlen, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1), //any SEQ commands require at least 1ms before query result
		TRXN_END,
	};
	int rlen=0;
	LEVEL_DEBUG("BAE SEQWR transaction function=%d, wlen=%d", func, wlen) ;
	Debug_Bytes("SEQW_cmd, data:",wparam,wlen) ;
	memcpy(p+3, wparam, wlen);
	RETURN_ERROR_IF_BAD( BUS_transaction(t, pn)) ;
	return OW_poll_until_timeout(&rlen, p, timeout, pn);
	
}

//SEQR command 0X19  with query until return of result
static GOOD_OR_BAD OW_seqr_complete_transaction(int *rlen,BYTE func,BYTE * rparam, int timeout,  struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + 2] = { _1W_SEQRCMD, BYTE_MASK(*rlen), BYTE_MASK(func), 0, };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;

	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(1), //any SEQ commands require at least 1ms before query result
		TRXN_END,
	};
	LEVEL_DEBUG( "BAE SEQR transaction: function=%d, rlen= %d", func, *rlen ) ;
	RETURN_ERROR_IF_BAD(BUS_transaction(t, pn));
	return OW_poll_until_timeout(rlen, rparam, timeout, pn);
}

//QUERY command 0x1A
static GOOD_OR_BAD OW_query_cmd(int *prlen, BYTE *retcode,BYTE * rparam , struct parsedname *pn)
{
	BYTE p[1 + 1 + 1 + _FC02_MAX_COMMAND_GULP + 2] = { _1W_QUERY_DATA_COMMAND };
	int rlen;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(p),  //query command
		TRXN_READ2(p+1), //retcode and len
		TRXN_END,
	};
	LEVEL_DEBUG( "query_cmd: getting header ") ;
	RETURN_BAD_IF_BAD( BUS_transaction(t, pn)) ;
	LEVEL_DEBUG( "query_cmd: header received: retcode=%d, rlen=%d ", p[1],p[2]) ;
	retcode[0]=p[1]; //retcode of previous command
	rlen=p[2];
	*prlen=rlen;
	if (p[1]==0xff)  return gbGOOD;  //no communication error, but still busy processing the previous command
	else
	{
		struct transaction_log t2[] = {
			TRXN_READ(p+3,rlen+2),
			TRXN_CRC16(p,rlen+3+2),
			TRXN_END,
		};
		RETURN_BAD_IF_BAD( BUS_transaction(t2, pn)) ;
		memcpy(rparam,p+3, rlen);
		Debug_Bytes("BAE query_cmd, received:",rparam,rlen) ;
	  return gbGOOD ;	
	}
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
	LEVEL_DEBUG("Read from BAE size=%d offset=%x\n",(int)size,(unsigned int)offset) ;
	Debug_Bytes("BAE read",p,1+2+1+size) ;
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
	GOOD_OR_BAD ret;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+1+1+32, 0),
		TRXN_WRITE1(q),
		TRXN_DELAY(2),
		TRXN_END,
	} ;
	
	memcpy(&p[3], data, 32 ) ;
	 ret=BUS_transaction(t, pn) ;
	_Debug_Bytes("write flash buffer details",p,37);
	return ret;
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
