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

#include <config.h>
#include "owfs_config.h"
#include "ow_1923.h"

/* ------- Prototypes ----------- */

/* DS1923 Battery */
READ_FUNCTION(FS_r_temperature);
READ_FUNCTION(FS_r_humid);
READ_FUNCTION(FS_r_date);
WRITE_FUNCTION(FS_w_date);
READ_FUNCTION(FS_r_counter);
WRITE_FUNCTION(FS_w_counter);
READ_FUNCTION(FS_bitread);
WRITE_FUNCTION(FS_bitwrite);
READ_FUNCTION(FS_rbitread);
WRITE_FUNCTION(FS_rbitwrite);
READ_FUNCTION(FS_clearmem);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_run);
WRITE_FUNCTION(FS_w_run);
WRITE_FUNCTION(FS_w_mip);
READ_FUNCTION(FS_r_delay);
WRITE_FUNCTION(FS_w_delay);
READ_FUNCTION(FS_enable_osc);


/* ------- Structures ----------- */
struct BitRead {
    size_t location;
    int bit;
};
static struct BitRead BitReads[] = {
    {0x0215, 1,},               // Mission in progress
    {0x0213, 4,},               // rollover
    {0x0212, 0,},               // clock running
    {0x0213, 0,},               //sample temp in progress
    {0x0213, 1,},               //sample humidity in progress
    //{ 0x0214, 7, } , //temperature in progress
    //{ 0x0214, 4, } , //sample in progress
};

/*
  Device configuration byte (Address 0x0226)
  Add detection and support for all of them...
  00000000 DS2422
  00100000 DS1923
  01000000 DS1922L
  01100000 DS1922T
*/
struct Mission {
    _DATE start;
    int rollover;
    int interval;
    int samples;
};

struct aggregate A1923p = { 18, ag_numbers, ag_separate, };
struct aggregate A1923l = { 2048, ag_numbers, ag_mixed, };
struct aggregate A1923h = { 63, ag_numbers, ag_mixed, };
struct aggregate A1923m = { 12, ag_numbers, ag_aggregate, };
struct filetype DS1923[] = {
    F_STANDARD,
    {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
    {"pages/page", 32, &A1923p, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},    {"pages/page", 32, &A1923p, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},    {"pages/page", 32, &A1923p, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},    {"pages/page", 32, &A1923p, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
    {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_r_temperature}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_r_temperature}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile,  {o:FS_r_temperature}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temperature}, {o: NULL}, {v:NULL},},
    {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_r_humid}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_r_humid}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile,  {o:FS_r_humid}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"humidity",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_humid}, {o: NULL}, {v:NULL},},
    {"clock",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"clock",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"clock",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},    {"clock",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
    {"clock/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {v:NULL},},    {"clock/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {v:NULL},},    {"clock/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second,  {o:FS_r_date}, {o:FS_w_date}, {v:NULL},},    {"clock/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second, {o: FS_r_date}, {o: FS_w_date}, {v:NULL},},
    {"clock/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {v:NULL},},    {"clock/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {v:NULL},},    {"clock/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second,  {o:FS_r_counter}, {o:FS_w_counter}, {v:NULL},},    {"clock/udate",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_second, {o: FS_r_counter}, {o: FS_w_counter}, {v:NULL},},
    {"clock/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_rbitread}, {o:FS_rbitwrite}, {v:&BitReads[2]},},    {"clock/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_rbitread}, {o:FS_rbitwrite}, {v:&BitReads[2]},},    {"clock/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_rbitread}, {o:FS_rbitwrite}, {v:&BitReads[2]},},    {"clock/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_rbitread}, {o: FS_rbitwrite}, {v:&BitReads[2]},},
#if 0
    /* Just test functions */
{"running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_r_run}, {o:FS_w_run}, {v:NULL},},{"running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_r_run}, {o:FS_w_run}, {v:NULL},},{"running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_r_run}, {o:FS_w_run}, {v:NULL},},{"running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_run}, {o: FS_w_run}, {v:NULL},},
{"memory", 512, NULL, ft_binary, fc_stable,  {o:FS_r_mem}, {o:FS_w_mem}, {v:NULL},},{"memory", 512, NULL, ft_binary, fc_stable,  {o:FS_r_mem}, {o:FS_w_mem}, {v:NULL},},{"memory", 512, NULL, ft_binary, fc_stable,  {o:FS_r_mem}, {o:FS_w_mem}, {v:NULL},},{"memory", 512, NULL, ft_binary, fc_stable, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
{"clearmem", 1, NULL, ft_binary, fc_stable,  {o:FS_clearmem}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"clearmem", 1, NULL, ft_binary, fc_stable,  {o:FS_clearmem}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"clearmem", 1, NULL, ft_binary, fc_stable,  {o:FS_clearmem}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"clearmem", 1, NULL, ft_binary, fc_stable, {o: FS_clearmem}, {o: NULL}, {v:NULL},},
{"enableosc", 1, NULL, ft_binary, fc_stable,  {o:FS_enable_osc}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"enableosc", 1, NULL, ft_binary, fc_stable,  {o:FS_enable_osc}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"enableosc", 1, NULL, ft_binary, fc_stable,  {o:FS_enable_osc}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"enableosc", 1, NULL, ft_binary, fc_stable, {o: FS_enable_osc}, {o: NULL}, {v:NULL},},
#endif
{"mission",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"mission",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"mission",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},{"mission",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {o: NULL}, {o: NULL}, {v:NULL},},
{"mission/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:FS_w_mip}, {v:&BitReads[0]},},{"mission/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:FS_w_mip}, {v:&BitReads[0]},},{"mission/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:FS_w_mip}, {v:&BitReads[0]},},{"mission/running",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_bitread}, {o: FS_w_mip}, {v:&BitReads[0]},},
{"mission/rollover",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_bitread}, {o:FS_bitwrite}, {v:&BitReads[1]},},{"mission/rollover",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_bitread}, {o:FS_bitwrite}, {v:&BitReads[1]},},{"mission/rollover",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_bitread}, {o:FS_bitwrite}, {v:&BitReads[1]},},{"mission/rollover",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_bitread}, {o: FS_bitwrite}, {v:&BitReads[1]},},
{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_delay}, {o:FS_w_delay}, {v:NULL},},{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_delay}, {o:FS_w_delay}, {v:NULL},},{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_delay}, {o:FS_w_delay}, {v:NULL},},{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_delay}, {o: FS_w_delay}, {v:NULL},},
{"mission/samplingtemp",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:NO_WRITE_FUNCTION}, {v:&BitReads[3]},},{"mission/samplingtemp",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:NO_WRITE_FUNCTION}, {v:&BitReads[3]},},{"mission/samplingtemp",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:NO_WRITE_FUNCTION}, {v:&BitReads[3]},},{"mission/samplingtemp",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_bitread}, {o: NULL}, {v:&BitReads[3]},},
    {"mission/samplinghumidity",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:NO_WRITE_FUNCTION}, {v:&BitReads[4]},},    {"mission/samplinghumidity",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:NO_WRITE_FUNCTION}, {v:&BitReads[4]},},    {"mission/samplinghumidity",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile,  {o:FS_bitread}, {o:NO_WRITE_FUNCTION}, {v:&BitReads[4]},},    {"mission/samplinghumidity",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_bitread}, {o: NULL}, {v:&BitReads[4]},},
#if 0
{"mission/frequency",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {u: FS_r_samplerate}, {u: FS_w_samplerate}, {v:NULL},},
{"mission/samples",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {u: FS_r_3byte}, {o: NULL}, {s:0x021A},},
{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_delay}, {o:FS_w_delay}, {v:NULL},},{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_delay}, {o:FS_w_delay}, {v:NULL},},{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,  {o:FS_r_delay}, {o:FS_w_delay}, {v:NULL},},{"mission/delay",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile, {o: FS_r_delay}, {o: FS_w_delay}, {v:NULL},},
{"mission/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {d: FS_mdate},  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION},},{"mission/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {d: FS_mdate},  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION},},{"mission/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {d: FS_mdate},  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION},},{"mission/date",PROPERTY_LENGTH_DATE, NULL, ft_date, fc_volatile, {d: FS_mdate}, {o: NULL}, {o:NULL},},
{"mission/easystart",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: NULL}, {u: FS_easystart}, {v:NULL},},
#endif
};

DeviceEntryExtended(41, DS1923,
                    DEV_temp | DEV_alarm | DEV_ovdr | DEV_resume);

/* ------- Functions ------------ */

/* DS1923 */
static int OW_r_mem(BYTE * data,  size_t size,  off_t offset,
                     struct parsedname *pn);
static int OW_w_mem( BYTE * data,  size_t size,
                     off_t offset,  struct parsedname *pn);
static int OW_r_temperature(_FLOAT * T, const UINT delay,
                             struct parsedname *pn);
static int OW_r_humid(_FLOAT * H, const UINT delay,
                       struct parsedname *pn);
static int OW_startmission(unsigned long mdelay,
                            struct parsedname *pn);
static int OW_stopmission( struct parsedname *pn);
static int OW_MIP(struct parsedname *pn);
static int OW_clearmemory(struct parsedname *pn);
static int OW_force_conversion(const UINT delay,
                               struct parsedname *pn);
static int OW_2date(_DATE * d, const BYTE * data);
static int OW_oscillator(const int on,  struct parsedname *pn);
static void OW_date(const _DATE * d, BYTE * data);



static int FS_r_page(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	if (OW_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_r_mem))
        return -EINVAL;
    return 0;
}

static int FS_w_page(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
    if (OW_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem))
        return -EINVAL;
    return 0;
}

static int FS_r_mem(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
	if (OW_readwrite_paged(owq, 0, pagesize, OW_r_mem))
        return -EINVAL;
    return 0;
}

static int FS_w_mem(struct one_wire_query * owq)
{
	size_t pagesize = 32 ;
    if (OW_readwrite_paged(owq, 0, pagesize, OW_w_mem))
        return -EINVAL;
    return 0;
}


/* mission delay */
static int FS_r_delay(struct one_wire_query * owq)
{
    BYTE data[3];
    if (OW_r_mem(data, 3, 0x0216, PN(owq)))
        return -EINVAL;
    // should be 3 bytes!
    //u[0] = (((UINT)data[2])<<16) | (((UINT)data[1])<<8) | data[0] ;
    OWQ_U(owq) = (((UINT) data[1]) << 8) | data[0];
    return 0;
}

/* mission delay */
static int FS_w_delay(struct one_wire_query * owq)
{
    UINT U = OWQ_U(owq) ;
    BYTE data[3] =
    { U & 0xFF, (U >> 8) & 0xFF, (U >> 16) & 0xFF };
    if (OW_MIP(PN(owq)))
        return -EBUSY;
    if (OW_w_mem(data, 3, 0x0216, PN(owq)))
        return -EINVAL;
    return 0;
}

/* Just a test-function */
static int FS_enable_osc(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
#if 0
    /* Just write to address without any check */
    BYTE d = 0x01;
    if (OW_w_mem(&d, 1, 0x0212, pn)) {
        printf("OW_oscillator: error4\n");
        return -EINVAL;
}
#else
    if (OW_oscillator(1, pn)) {
    printf("FS_enable_osc: error 1\n");
    return -EINVAL;
    }
#endif
    memset(OWQ_buffer(owq), 0, 1);
    return OWQ_size(owq);
}

static int FS_clearmem(struct one_wire_query * owq)
{
    memset(OWQ_buffer(owq), 0, 1); // ??? why is this needed
    if (OW_clearmemory(PN(owq)))
        return -EINVAL;
    return OWQ_size(owq) ;
}

/* clock running? */
static int FS_r_run(struct one_wire_query * owq)
{
    BYTE cr;
    if (OW_r_mem(&cr, 1, 0x0212, PN(owq)))
        return -EINVAL;
    // only bit 0 and 1 should be used!
    if (cr & 0xFC)
        return -EINVAL;
    OWQ_Y(owq) = (cr & 0x01);
    return 0;
}

/* stop/start clock running */
static int FS_w_run(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE cr;
    BYTE check;

    if (OW_r_mem(&cr, 1, 0x0212, pn))
        return -EINVAL;
    // only bit 0 and 1 should be used!
    if (cr & 0xFC)
        return -EINVAL;
    cr = OWQ_Y(owq) ? (cr | 0x01) : (cr & 0xFE);
    if (OW_w_mem(&cr, 1, 0x0212, pn))
        return -EINVAL;
    /* Double check written value */
    if (OW_r_mem(&check, 1, 0x0212, pn))
        return -EINVAL;
    if (check != cr)
        return -EINVAL;
    return 0;
}

/* start/stop mission */
static int FS_w_mip(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    unsigned long mdelay;
    BYTE data[3];
    printf("FS_w_mip:\n");
    if (OWQ_Y(owq)) {                   /* start a mission! */
        printf("FS_w_mip: start\n");
        if (OW_r_mem(data, 3, 0x0216, pn))
            return -EINVAL;
        mdelay = data[0] | data[1] << 8 | data[2] << 16;
        return OW_startmission(mdelay, pn);
    } else {
        printf("FS_w_mip: stop\n");
        return OW_stopmission(pn);
    }
}

static int FS_bitread(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE d;
    struct BitRead *br;
    if (pn->ft->data.v == NULL)
        return -EINVAL;
    br = ((struct BitRead *) (pn->ft->data.v));
    if (OW_r_mem(&d, 1, br->location, pn))
        return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&d, br->bit);
    return 0;
}

static int FS_bitwrite(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE d;
    struct BitRead *br;
    if (pn->ft->data.v == NULL)
        return -EINVAL;
    br = ((struct BitRead *) (pn->ft->data.v));
    if (OW_r_mem(&d, 1, br->location, pn))
        return -EINVAL;
    UT_setbit(&d, br->bit, OWQ_Y(owq));
    if (OW_w_mem(&d, 1, br->location, pn))
        return -EINVAL;
    return 0;
}

static int FS_rbitread(struct one_wire_query * owq)
{
    int ret = FS_bitread(owq);
    OWQ_Y(owq) = !OWQ_Y(owq);
    return ret;
}

static int FS_rbitwrite(struct one_wire_query * owq)
{
    OWQ_Y(owq) = !OWQ_Y(owq);
    return FS_bitwrite(owq);
}

/* Temperature -- force if not in progress */
static int FS_r_temperature(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    UINT delay = 666;
    int ret;
    if ((ret = OW_MIP(pn))) {
        printf("FS_r_temperature: mip error\n");
        return ret;             /* Mission in progress */
    }

    if (OW_force_conversion(delay, pn)) {
        printf("FS_r_temperature: conv\n");
        return -EINVAL;
    }

    if (OW_r_temperature(&OWQ_F(owq), delay, pn))
        return -EINVAL;
    return 0;
}

static int FS_r_humid(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    UINT delay = 666;
    int ret;
    if ((ret = OW_MIP(pn))) {
        printf("FS_r_humid: mip error\n");
        return ret;             /* Mission in progress */
    }

    if (OW_force_conversion(delay, pn)) {
        printf("FS_r_humid: conv\n");
        return -EINVAL;
    }
    if (OW_r_humid(&OWQ_F(owq), delay, pn))
        return -EINVAL;
    return 0;
}

/* translate 7 byte field to a Unix-style date (number) */
static int OW_2date(_DATE * d, const BYTE * data)
{
    struct tm t;

    /* Prefill entries */
    d[0] = time(NULL);
    if (gmtime_r(d, &t) == NULL) {
        printf("OW_2date: error1\n");
        return -EINVAL;
    }

    printf
            ("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",
             t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon, t.tm_year,
             t.tm_wday, t.tm_isdst);

#define bcd2dec(x) (((x)&0x70)>>4)*10 + ((x)&0x0F)
    t.tm_sec = bcd2dec(data[0]);
    t.tm_min = bcd2dec(data[1]);
    if (data[2] & 0x40) {
        // am/pm mode
        t.tm_hour = ((data[2] & 0x20) ? 12 : 0) + bcd2dec(data[2] & 0x1F);
    } else {
        t.tm_hour = bcd2dec(data[2] & 0x2F);
    }
    t.tm_mday = bcd2dec(data[3] & 0x2F);
    t.tm_mon = bcd2dec(data[4] & 0x1F); // should be range 0-11
    //The number of years since 1900.
    t.tm_year = (data[4] & 0x80 ? 100 : 0) + bcd2dec(data[5] & 0xFF);

    printf("_DATE_READ data=%2X, %2X, %2X, %2X, %2X, %2X\n", data[0],
           data[1], data[2], data[3], data[4], data[5]);
    printf
            ("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",
             t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon, t.tm_year,
             t.tm_wday, t.tm_isdst);

    /* Pass through time_t again to validate */
    if ((*d = mktime(&t)) == -1) {
        printf("2date: error2\n");
        return -EINVAL;
    }

    return 0;
}


static int OW_oscillator(const int on,  struct parsedname *pn)
{
    BYTE d;
    BYTE check;
    /* Since the DS1923 has a bug and permanently hangs if oscillator is
    * turned off, I make this real paranoid read/write/read of the
    * oscillator bit until I know the code really works.
    */
    if (OW_r_mem(&d, 1, 0x0212, pn)) {
        printf("OW_oscillator: error1\n");
        return -EINVAL;
    }
    /* Only bit 0 and 1 are used... All other bits should be 0 */
    if (d & 0xFC)
        return -EINVAL;
    if (on) {
        if (d & 0x01)
            return 0;           // already on
        d |= 0x01;
    } else {
        if (!(d & 0x01))
            return 0;           // already off
        d &= 0xFE;
    }
    if (OW_w_mem(&d, 1, 0x0212, pn)) {
        printf("OW_oscillator: error4\n");
        return -EINVAL;
    }
    if (OW_r_mem(&check, 1, 0x0212, pn)) {
        printf("OW_oscillator: error5\n");
        return -EINVAL;
    }
    if (check != d)
        return -EINVAL;         // failed to change value

    UT_delay(1000);             // I just want to wait a second and let clock update

    return 0;
}


/* read clock */
int FS_r_date(struct one_wire_query * owq)
{
    BYTE data[6];
    int ret;

    if (OW_r_mem(data, 6, 0x0200, PN(owq))) {
        printf("FS_r_date: error 2\n");
        return -EINVAL;
    }
    ret = OW_2date(&OWQ_D(owq), data);
    return ret;
}

/* read clock */
static int FS_r_counter(struct one_wire_query * owq)
{
    BYTE data[6];
    _DATE d;
    int ret;

    /* Get date from chip */
    if (OW_r_mem(data, 6, 0x0200, PN(owq)))
        return -EINVAL;
    if ((ret = OW_2date(&d, data)))
        return ret;
    OWQ_U(owq) = (UINT) d;
    return 0;
}

/* set clock */
static int FS_w_date(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[6];
    int ret;

    /* Busy if in mission */
    if ((ret = OW_MIP(pn))) {
        printf("FS_w_date: mip error\n");
        return ret;             /* Mission in progress */
    }

    if (OW_oscillator(1, pn)) {
        printf("FS_w_date: error 1\n");
        return -EINVAL;
    }

    OW_date(&OWQ_D(owq), data);
    if (OW_w_mem(data, 6, 0x0200, pn))
        return -EINVAL;
    OWQ_Y(owq) = 1 ; // for turning on chip
    return FS_w_run(owq);
}

static int FS_w_counter(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    BYTE data[6];
    _DATE d = (_DATE) OWQ_U(owq);
    int ret;

    /* Busy if in mission */
    if ((ret = OW_MIP(pn))) {
        printf("FS_w_counter: mip error\n");
        return ret;             /* Mission in progress */
    }

    OW_date(&d, data);
    return OW_w_mem(data, 6, 0x0200, pn) ? -EINVAL : 0;
}


static int OW_w_mem( BYTE * data,  size_t size,
                     off_t offset,  struct parsedname *pn)
{
    BYTE p[3 + 1 + 32 + 2] =
    { 0x0F, offset & 0xFF, (offset >> 8) & 0xFF, };
    BYTE passwd[8];
    int rest = 32 - (offset & 0x1F);
    int ret;
    int i;

    LEVEL_DEBUG("OW_w_mem: size=%ld offset=%X rest=%ld\n", size, offset,
                rest);

    memset(passwd, 0xFF, 8);    // dummy password

    memset(&p[3], 0xFF, 32);
    memcpy(&p[3], data, size);

    BUSLOCK(pn);
    ret = BUS_select(pn) || BUS_send_data(p, 3 + rest, pn);
    if (ret) {
        printf("OW_w_mem: err1\n");
    }
    if (ret) {
        BUSUNLOCK(pn);
        return 1;
    }
    /* Re-read scratchpad and compare */
    /* Note: location of data has now shifted down a byte for E/S register */
    //ret = BUS_select(pn) || BUS_send_data( p,3,pn) || BUS_readin_data( &p[3],1+rest+2,pn) || CRC16(p,4+rest+2) || memcmp(&p[4], data, size) ;

    ret = BUS_select(pn);
    if (ret) {
        printf("OW_w_mem: err3\n");
        BUSUNLOCK(pn);
        return 1;
    }
    p[0] = 0xAA;
    ret = BUS_send_data(p, 1, pn);
    if (ret) {
        printf("OW_w_mem: err4\n");
        BUSUNLOCK(pn);
        return 1;
    }
    // read TAL TAH E/S + rest bytes
    ret = BUS_readin_data(&p[1], 3 + rest, pn);
    if (ret) {
        printf("OW_w_mem: err5\n");
        BUSUNLOCK(pn);
        return 1;
    }
    ret = CRC16(p, 4 + rest);
    if (ret) {
        printf("OW_w_mem: crc err1\n");
    }
#if 1
{
    printf("OW_w_mem: sz=%d ", 4 + rest);
    for (i = 0; i < 4 + rest; i++) {
        printf("%02X", p[i]);
    }
    printf("\n");
}
    if (ret) {
        printf("OW_w_mem: err6\n");
        BUSUNLOCK(pn);
        return 1;
    }
    ret = memcmp(&p[4], data, size);
    if (ret) {
        printf("OW_w_mem: err7\n");
        BUSUNLOCK(pn);
        return 1;
    }
#endif

    /* Copy Scratchpad to SRAM */
    //ret = BUS_select(pn) || BUS_send_data( p,4,pn) || BUS_send_data( passwd,8,pn) ;
    ret = BUS_select(pn);
    if (ret) {
        printf("OW_w_mem: err8\n");
        BUSUNLOCK(pn);
        return 1;
    }
    // send 0x99 TAL TAH E/S
    p[0] = 0x99;
    ret = BUS_send_data(p, 4, pn);
    if (ret) {
        printf("OW_w_mem: err9\n");
        BUSUNLOCK(pn);
        return 1;
    }
    ret = BUS_send_data(passwd, 8, pn);
    if (ret) {
        printf("OW_w_mem: err10\n");
        BUSUNLOCK(pn);
        return 1;
    }
    BUSUNLOCK(pn);
    if (ret)
        return 1;

#if 0
    // Loop waiting for completion
    for (i = 0; i < 10; ++i) {
        UT_delay(1);
        BUSLOCK(pn);
        //           ret = BUS_read(p,1,pn) ;
        BUSUNLOCK(pn);
        printf("OW_w_mem: get ret=%d and %02X as result\n", ret, p[0]);
        if (ret)
            return 1;
        if (p[0] == 0xAA) {
            return 0;
}
        if (p[0] == 0xFF) {
            return 1;
}
}
    return 1;                   // timeout
#else
    UT_delay(1);
    return 0;
#endif
}

static int OW_clearmemory(struct parsedname *pn)
{
    BYTE p[3 + 8 + 32 + 2] = { 0x96, };
    BYTE r;
    int ret;

    memset(&p[1], 0xFF, 8);     // password
    p[9] = 0xFF;                // dummy byte

    BUSLOCK(pn);
    ret = BUS_select(pn);
    if (ret) {
        BUSUNLOCK(pn);
        printf("error1\n");
        return ret;
    }
    ret = BUS_send_data(p, 10, pn);
    if (ret) {
        BUSUNLOCK(pn);
        printf("error2\n");
        return ret;
    }
    UT_delay(1);
    BUSUNLOCK(pn);


    ret = OW_r_mem(&r, 1, 0x0215, pn);
    if (ret) {
        //printf("error2\n");
        return ret;
    }
    LEVEL_DEBUG("Read 0x0215: MEMCLR=%d %02X\n", (r & 0x08 ? 1 : 0), r);

    return 0;
}

/* Just a test function to read all available bytes */
static int OW_flush(struct parsedname *pn, int lock)
{
    int i = 0;
    int ret;

    if (lock)
        BUSLOCK(pn);
    do {
//    ret = BUS_read(&p,1,pn ) ;
        if (ret) {
            break;
        }
        i++;
        if ((i % 5) == 0) {
            printf(".");
            fflush(stdout);
        }
    } while (ret == 0);
    if (lock)
        BUSUNLOCK(pn);
    //printf("flush: read %d bytes ret=%d\n", i, ret);
    return 0;
}


static int OW_r_mem(BYTE * data,  size_t size,  off_t offset,
                     struct parsedname *pn)
{
    BYTE p[3 + 8 + 32 + 2] =
    { 0x69, offset & 0xFF, (offset >> 8) & 0xFF, };
    //BYTE r[3+8+32+2] = { 0xAA, } ;
    int rest = 32 - (offset & 0x1F);
    BYTE passwd[8];
    int ret;
    int i;

    memset(passwd, 0xFF, 8);    // dummy password
    memset(data, 0, size);      // clear output

    //printf("OW_r_mem: size=%lX offset=%lX  %02X %02X %02X\n", size, offset, p[0], p[1], p[2]);

    BUSLOCK(pn);
#if 0
    ret = BUS_select(pn) || BUS_send_data(p, 3, pn)
        || BUS_send_data(passwd, 8, pn)
        || BUS_readin_data(&p[3], rest + 2, pn) || CRC16(p, 3 + rest + 2);
#else

    ret = BUS_select(pn);
    if (ret) {
        BUSUNLOCK(pn);
        //printf("error1\n");
        return ret;
    }

    ret = BUS_send_data(p, 3, pn);
    if (ret) {
        BUSUNLOCK(pn);
        //printf("error2\n");
        return ret;
    }
    ret = BUS_send_data(passwd, 8, pn);
    if (ret) {
        BUSUNLOCK(pn);
        //printf("error2\n");
        return ret;
    }
    ret = BUS_readin_data(&p[3], rest + 2, pn);
    if (ret) {
        BUSUNLOCK(pn);
        printf("error4\n");
        return ret;
    }
    {
        printf("Read: sz=%d ", 3 + rest + 2);
        for (i = 0; i < 3 + rest + 2; i++) {
            printf("%02X", p[i]);
        }
        printf("\n");
    }
    ret = CRC16(p, 3 + rest + 2);
    if (ret) {
        printf("crc error\n");
    }
#endif

    BUSUNLOCK(pn);

    if (!ret)
        memcpy(data, &p[3], size);
    return ret;
}

/* many things are disallowed if mission in progress */
/* returns 1 if MIP, 0 if not, <0 if error */
static int OW_MIP(struct parsedname *pn)
{
    BYTE data;
    int ret = OW_r_mem(&data, 1, 0x0215, pn);   /* read status register */

    if (ret) {
        printf("OW_MIP: err1\n");
        return -EINVAL;
    }
    if (UT_getbit(&data, 1))
        return -EBUSY;
    return 0;
}

/* set clock */
static void OW_date(const _DATE * d, BYTE * data)
{
    struct tm tm;
    int year;

    /* Convert time format */
    gmtime_r(d, &tm);

    data[0] = tm.tm_sec + 6 * (tm.tm_sec / 10); /* dec->bcd */
    data[1] = tm.tm_min + 6 * (tm.tm_min / 10); /* dec->bcd */
    data[2] = tm.tm_hour + 6 * (tm.tm_hour / 10);   /* dec->bcd */
    data[3] = tm.tm_mday + 6 * (tm.tm_mday / 10);   /* dec->bcd */
    data[4] = tm.tm_mon + 6 * (tm.tm_mon / 10); /* dec->bcd */
    year = tm.tm_year % 100;
    data[5] = year + 6 * (year / 10);   /* dec->bcd */
    if (tm.tm_year > 99 && tm.tm_year < 200)
        data[4] |= 0x80;
//printf("_DATE_WRITE data=%2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5]);
//printf("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;
}

static int OW_force_conversion(const UINT delay,
                                struct parsedname *pn)
{
    BYTE t[2] = { 0x55, 0xFF };
    int ret = 0;

    if (OW_oscillator(1, pn)) {
        printf("OW_force_conversion: error 1\n");
        return -EINVAL;
    }

    /* Mission not progress, force conversion */
    BUSLOCK(pn);
    ret = BUS_select(pn) || BUS_send_data(t, 2, pn);
    if (ret) {
        printf("conv: err3\n");
        return 1;
    }
    if (!ret) {
        UT_delay(delay);
        ret = BUS_reset(pn);
    }
    BUSUNLOCK(pn);
    if (ret) {
        printf("conv: err4\n");
        return 1;
    }
    return 0;
}

static int OW_r_temperature(_FLOAT * T, const UINT delay,
                            struct parsedname *pn)
{
    BYTE data[32];
    (void) delay;

    if (OW_r_mem(data, 8, 0x020C, pn)) {    /* read temp register */
        printf("OW_r_temperature: error1\n");
        return -EINVAL;
    }
    *T = ((_FLOAT) ((BYTE) data[1])) / 2 - 41;

    if (data[7] & 0x04) {
        *T += ((_FLOAT) ((BYTE) data[0])) / 512;
    }
    return 0;
}


static int OW_r_humid(_FLOAT * H, const UINT delay,
                      struct parsedname *pn)
{
    _FLOAT ADVAL, IVAL;
    BYTE data[32];
    (void) delay;

    if (OW_r_mem(data, 6, 0x020E, pn)) {
        printf("OW_r_humid: error1\n");
        return -EINVAL;
    }
    if (data[5] & 0x08) {
        // high resolution
        IVAL = (((BYTE) data[1]) * 256 + (BYTE) data[0]) / 16;
        ADVAL = (IVAL * 5.02) / 4096;
    } else {
        // low resolution
        ADVAL = ((_FLOAT) ((BYTE) data[1])) * 5.02 / 256;
    }
    *H = (ADVAL - 0.958) / 0.0307;
    return 0;
}

static int OW_stopmission(struct parsedname *pn)
{
    BYTE data[10] = { 0x33, };
    int ret;
    memset(&data[1], 0xFF, 8);  // dummy password
    data[9] = 0xFF;

    BUSLOCK(pn);
    ret = BUS_select(pn) || BUS_send_data(data, 10, pn);
    BUSUNLOCK(pn);
    if (ret) {
        printf("stopmission err1\n");
    }
    return ret;
}

static int OW_w_delay(unsigned long mdelay, struct parsedname *pn)
{
    BYTE p[3];
    // mission start delay
    p[0] = mdelay & 0xFF;
    p[1] = (mdelay >> 8) & 0xFF;
    p[2] = (mdelay >> 16) & 0xFF;
    if (OW_w_mem(p, 3, 0x0216, pn))
        return -EINVAL;
    return 0;
}

static int OW_startmission(unsigned long mdelay,
                           struct parsedname *pn)
{
    BYTE cc, data;
    BYTE p[10] = { 0xCC, };
    int ret;

    /* stop the mission */
    if (OW_stopmission(pn))
        return -EINVAL;         /* stop */

    if (mdelay == 0)
        return 0;               /* stay stopped */

    if (mdelay & 0xFF000000)
        return -ERANGE;         /* Bad interval */

    if (OW_r_mem(&cc, 1, 0x0212, pn))
        return -EINVAL;
    if (cc & 0xFC)
        return -EINVAL;
    if (!(cc & 0x01)) {         /* clock stopped */
		OWQ_make( owq_dateset ) ;
		OWQ_create_temporary( owq_dateset, NULL, 0, 0, pn ) ;
        OWQ_D(owq_dateset) = time(NULL);
        /* start clock */
        if (FS_w_date(owq_dateset))
            return -EINVAL;     /* set the clock to current time */
        UT_delay(1000);         /* wait for the clock to count a second */
    }
#if 1
    if (mdelay >= 15 * 60) {
    cc |= 0x02;             // Enable high speed sample (minute)
    mdelay = mdelay / 60;
    } else {
        cc &= 0xFD;             // Enable high speed sample (second)
    }
    if (OW_w_mem(&cc, 1, 0x0212, pn))
        return -EINVAL;
#endif

#if 1
    // mission start delay
    p[0] = (mdelay & 0xFF);
    p[1] = (mdelay & 0xFF00) >> 8;
    p[2] = (mdelay & 0xFF0000) >> 16;
    if (OW_w_mem(p, 3, 0x0216, pn))
        return -EINVAL;
#endif

    /* clear memory */
    if (OW_clearmemory(pn))
        return -EINVAL;

    data = 0xA0;                // Bit 6&7 always set
    data |= 0x01;               // start Temp logging
    data |= 0x02;               // start Humidity logging
    data |= 0x04;               // store Temp in high resolution
    data |= 0x08;               // store Humidity in high resolution
    data |= 0x10;               // Rollover and overwrite
    //data |= 0x20 ; // start mission upon temperature alarm
    ret = OW_w_mem(&data, 1, 0x0213, pn);
    if (ret) {
        printf("startmission err1\n");
        return ret;
    }

    p[0] = 0xCC;
    memset(&p[1], 0xFF, 8);     // dummy password
    p[9] = 0xFF;                // dummy byte
    BUSLOCK(pn);
    ret = BUS_select(pn) || BUS_send_data(p, 10, pn);
    BUSUNLOCK(pn);
    if (ret) {
        printf("startmission err1\n");
        return ret;
    }

    return ret;
}
