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
#include "ow_lcd.h"

/* ------- Prototypes ----------- */

/* LCD display */
READ_FUNCTION(FS_r_version);
READ_FUNCTION(FS_r_counters);
WRITE_FUNCTION(FS_w_on);
WRITE_FUNCTION(FS_w_backlight);
READ_FUNCTION(FS_r_gpio);
WRITE_FUNCTION(FS_w_gpio);
READ_FUNCTION(FS_r_data);
WRITE_FUNCTION(FS_w_data);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_register);
WRITE_FUNCTION(FS_w_register);
#if OW_CACHE
READ_FUNCTION(FS_r_cum);
WRITE_FUNCTION(FS_w_cum);
#endif							/* OW_CACHE */
WRITE_FUNCTION(FS_w_screenX);
WRITE_FUNCTION(FS_w_lineX);

/* ------- Structures ----------- */

struct aggregate ALCD = { 4, ag_numbers, ag_aggregate, };
struct aggregate ALCD_L16 = { 4, ag_numbers, ag_separate, };
struct aggregate ALCD_L20 = { 4, ag_numbers, ag_separate, };
struct aggregate ALCD_L40 = { 2, ag_numbers, ag_separate, };
struct filetype LCD[] = {
	F_STANDARD,
  {"LCDon",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   NO_READ_FUNCTION, FS_w_on, {v:NULL},} ,
  {"backlight",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   NO_READ_FUNCTION, FS_w_backlight, {v:NULL},} ,
  {"version", 16, NULL, ft_ascii, fc_stable,   FS_r_version, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"gpio",PROPERTY_LENGTH_BITFIELD, &ALCD, ft_bitfield, fc_volatile,   FS_r_gpio, FS_w_gpio, {v:NULL},} ,
  {"register",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,   FS_r_register, FS_w_register, {v:NULL},} ,
  {"data",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_volatile,   FS_r_data, FS_w_data, {v:NULL},} ,
  {"counters",PROPERTY_LENGTH_UNSIGNED, &ALCD, ft_unsigned, fc_volatile,   FS_r_counters, NO_WRITE_FUNCTION, {v:NULL},} ,
#if OW_CACHE
  {"cumulative",PROPERTY_LENGTH_UNSIGNED, &ALCD, ft_unsigned, fc_volatile,   FS_r_cum, FS_w_cum, {v:NULL},} ,
#endif							/*OW_CACHE */
  {"memory", 112, NULL, ft_binary, fc_stable,   FS_r_memory, FS_w_memory, {v:NULL},} ,
  {"screen16", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_w_screenX, {i:16},} ,
  {"screen20", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_w_screenX, {i:20},} ,
  {"screen40", 128, NULL, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_w_screenX, {i:40},} ,
  {"line16", 16, &ALCD_L16, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_w_lineX, {i:16},} ,
  {"line20", 20, &ALCD_L20, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_w_lineX, {i:20},} ,
  {"line40", 40, &ALCD_L40, ft_ascii, fc_stable,   NO_READ_FUNCTION, FS_w_lineX, {i:40},} ,
};

DeviceEntryExtended(FF, LCD, DEV_alarm);

/* ------- Functions ------------ */

/* LCD by L. Swart */
static int OW_r_scratch(BYTE * data, const int length,
						const struct parsedname *pn);
static int OW_w_scratch(const BYTE * data, const int length,
						const struct parsedname *pn);
static int OW_w_on(const int state, const struct parsedname *pn);
static int OW_w_backlight(const int state, const struct parsedname *pn);
static int OW_w_register(const BYTE data, const struct parsedname *pn);
static int OW_r_register(BYTE * data, const struct parsedname *pn);
static int OW_w_data(const BYTE data, const struct parsedname *pn);
static int OW_r_data(BYTE * data, const struct parsedname *pn);
static int OW_w_gpio(const BYTE data, const struct parsedname *pn);
static int OW_r_gpio(BYTE * data, const struct parsedname *pn);
static int OW_r_version(BYTE * data, const struct parsedname *pn);
static int OW_r_counters(UINT * data, const struct parsedname *pn);
static int OW_r_memory(BYTE * data,  size_t size,  off_t offset,
					    struct parsedname *pn);
static int OW_w_memory( BYTE * data,  size_t size,
					    off_t offset,  struct parsedname *pn);
static int OW_clear(const struct parsedname *pn);
static int OW_w_screen(const BYTE loc, const char *text, const int size,
					   const struct parsedname *pn);
static int LCD_byte(BYTE byte, int delay, const struct parsedname *pn);
static int LCD_2byte(BYTE * byte, int delay, const struct parsedname *pn);

/* Internal files */
//static struct internal_prop ip_cum = { "CUM", fc_persistent };
MakeInternalProp(CUM,fc_persistent) ; //cumulative

/* LCD */
static int FS_r_version(struct one_wire_query * owq)
{
	/* Not sure if this is valid, but won't allow offset != 0 at first */
	/* otherwise need a buffer */
	BYTE v[16];
    if (OW_r_version(v, PN(owq)))
		return -EINVAL;
    return Fowq_output_offset_and_size( (ASCII *) v, 16, owq);
}

static int FS_w_on(struct one_wire_query * owq)
{
    if (OW_w_on(OWQ_Y(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_backlight(struct one_wire_query * owq)
{
    if (OW_w_backlight(OWQ_Y(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_gpio(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_gpio(&data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = (~data) & 0x0F ;
	return 0;
}

/* 4 value array */
static int FS_w_gpio(struct one_wire_query * owq)
{
	BYTE data;

//    /* First get current states */
    if (OW_r_gpio(&data, PN(owq)))
		return -EINVAL;
	/* Now set pins */
    data = (data & 0xF0) | (~OWQ_U(owq) & 0x0F) ;
    if (OW_w_gpio(data, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_register(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_register(&data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = data;
	return 0;
}

static int FS_w_register(struct one_wire_query * owq)
{
    if ( OW_w_register((BYTE) (BYTE_MASK(OWQ_U(owq))), PN(owq)) )
		return -EINVAL;
	return 0;
}

static int FS_r_data(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_data(&data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = data;
	return 0;
}

static int FS_w_data(struct one_wire_query * owq)
{
    if ( OW_w_data((BYTE) (BYTE_MASK(OWQ_U(owq))), PN(owq)) )
		return -EINVAL;
	return 0;
}

static int FS_r_counters(struct one_wire_query * owq)
{
    UINT u[4] ;
    if (OW_r_counters(u, PN(owq)))
		return -EINVAL;
    OWQ_array_U(owq,0) = u[0] ;
    OWQ_array_U(owq,1) = u[1] ;
    OWQ_array_U(owq,2) = u[2] ;
    OWQ_array_U(owq,3) = u[3] ;
    return 0;
}

#if OW_CACHE					/* Special code for cumulative counters -- read/write -- uses the caching system for storage */
static int FS_r_cum(struct one_wire_query * owq)
{
    UINT u[4] ;
    if (OW_r_counters(u, PN(owq)))
		return -EINVAL;			/* just to prime the "CUM" data */
	if (Cache_Get_Internal_Strict
           ((void *) u, 4 * sizeof(UINT), InternalProp(CUM), PN(owq)))
		return -EINVAL;
    OWQ_array_U(owq,0) = u[0] ;
    OWQ_array_U(owq,1) = u[1] ;
    OWQ_array_U(owq,2) = u[2] ;
    OWQ_array_U(owq,3) = u[3] ;
    return 0;
}

static int FS_w_cum(struct one_wire_query * owq)
{
    UINT u[4] = {
        OWQ_array_U(owq,0),
        OWQ_array_U(owq,1),
        OWQ_array_U(owq,2),
        OWQ_array_U(owq,3),
    } ;
    return Cache_Add_Internal((const void *) u, 4 * sizeof(UINT), InternalProp(CUM),
                               PN(owq)) ? -EINVAL : 0;
}
#endif							/*OW_CACHE */

static int FS_w_lineX(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int width = pn->ft->data.i;
	BYTE loc[] = { 0x00, 0x40, 0x00 + width, 0x40 + width };
	char line[width];
    size_t size = OWQ_size(owq) ;

    if (OWQ_offset(owq))
		return -EADDRNOTAVAIL;
    memcpy(line, OWQ_buffer(owq), size);
	memset(&line[size], ' ', width - size);
	if (OW_w_screen(loc[pn->extension], line, width, pn))
		return -EINVAL;
	return 0;
}

static int FS_w_screenX(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int width = pn->ft->data.i;
	int rows = (width == 40) ? 2 : 4;	/* max number of rows */
	char *nl;
    char *b = OWQ_buffer(owq);
    char *end = OWQ_buffer(owq) + OWQ_size(owq);
    int extension ;
	OWQ_make( owq_line ) ;

    if (OWQ_offset(owq))
		return -EADDRNOTAVAIL;

	if (OW_clear(pn))
		return -EFAULT;

    OWQ_create_shallow_single( owq_line, owq ) ; // won't bother to destroy
    
	for (extension = 0; extension < rows; ++extension) {
        OWQ_pn(owq_line).extension = extension ;
		nl = memchr(b, '\n', end - b);
		if (nl && nl < b + width) {
            OWQ_buffer(owq_line) = b ;
            OWQ_size(owq_line) = b - nl ;
			if (FS_w_lineX(owq_line))
				return -EINVAL;
			b = nl + 1;			/* skip over newline */
		} else {
			nl = b + width;
			if (nl > end)
				nl = end;
            OWQ_buffer(owq_line) = b ;
            OWQ_size(owq_line) = b - nl ;
            if (FS_w_lineX(owq_line))
                return -EINVAL;
            b = nl;
		}
		if (b >= end)
			break;
	}
	return 0;
}

static int FS_r_memory(struct one_wire_query * owq)
{
	size_t pagesize = 16 ;
//    if ( OW_r_memory(buf,size,offset,pn) ) return -EFAULT ;
    if (OW_readwrite_paged(owq, 0, pagesize, OW_r_memory))
		return -EFAULT;
    return 0;
}

static int FS_w_memory(struct one_wire_query * owq)
{
	size_t pagesize = 16 ;
//    if ( OW_w_memory(buf,size,offset,pn) ) return -EFAULT ;
    if (OW_readwrite_paged(owq, 0, pagesize, OW_w_memory))
		return -EFAULT;
	return 0;
}

static int OW_w_scratch(const BYTE * data, const int length,
						const struct parsedname *pn)
{
	BYTE w = 0x4E;
	struct transaction_log t[] = {
		TRXN_START,
		{&w, NULL, 1, trxn_match,},
		{data, NULL, length, trxn_match,},
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_r_scratch(BYTE * data, const int length,
						const struct parsedname *pn)
{
	BYTE r = 0xBE;
	struct transaction_log t[] = {
		TRXN_START,
		{&r, NULL, 1, trxn_match,},
		{NULL, data, length, trxn_read,},
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_w_on(const int state, const struct parsedname *pn)
{
	BYTE w[] = { 0x03, 0x05, };	/* on off */
	return LCD_byte(w[!state], 0, pn);
}

static int OW_w_backlight(const int state, const struct parsedname *pn)
{
	BYTE w[] = { 0x08, 0x07, };	/* on off */
	return LCD_byte(w[!state], 0, pn);
}

static int OW_w_register(const BYTE data, const struct parsedname *pn)
{
	BYTE w[] = { 0x10, data, };
	// 100uS
	return LCD_2byte(w, 1, pn);
}

static int OW_r_register(BYTE * data, const struct parsedname *pn)
{
	// 150uS
	return LCD_byte(0x11, 1, pn) || OW_r_scratch(data, 1, pn);
}

static int OW_w_data(const BYTE data, const struct parsedname *pn)
{
	BYTE w[] = { 0x12, data, };
	// 100uS
	return LCD_2byte(w, 1, pn);
}

static int OW_r_data(BYTE * data, const struct parsedname *pn)
{
	// 150uS
	return LCD_byte(0x13, 1, pn) || OW_r_scratch(data, 1, pn);
}

static int OW_w_gpio(const BYTE data, const struct parsedname *pn)
{
	/* Note, it would be nice to control separately, nut
	   we can't know the set state of the pin, i.e. sensed and set
	   are confused */
	BYTE w[] = { 0x21, data, };
	// 20uS
	return LCD_2byte(w, 1, pn);
}

static int OW_r_gpio(BYTE * data, const struct parsedname *pn)
{
	// 70uS
	return LCD_byte(0x22, 1, pn) || OW_r_scratch(data, 1, pn);
}

static int OW_r_counters(UINT * data, const struct parsedname *pn)
{
	BYTE d[8];
	UINT cum[4];

	if (LCD_byte(0x23, 1, pn) || OW_r_scratch(d, 8, pn))
		return 1;				// 80uS

	data[0] = ((UINT) d[1]) << 8 | d[0];
	data[1] = ((UINT) d[3]) << 8 | d[2];
	data[2] = ((UINT) d[5]) << 8 | d[4];
	data[3] = ((UINT) d[7]) << 8 | d[6];

//printf("OW_COUNTER key=%s\n",key);
    if (Cache_Get_Internal_Strict((void *) cum, sizeof(cum), InternalProp(CUM), pn)) {	/* First pass at cumulative */
		cum[0] = data[0];
		cum[1] = data[1];
		cum[2] = data[2];
		cum[3] = data[3];
	} else {
		cum[0] += data[0];
		cum[1] += data[1];
		cum[2] += data[2];
		cum[3] += data[3];
	}
    Cache_Add_Internal((void *) cum, sizeof(cum), InternalProp(CUM), pn);
	return 0;
}

/* memory is 112 bytes */
/* can only read 16 bytes at a time (due to scratchpad size) */
/* Will pretend pagesize = 16 */
/* minor inefficiency if start is not on "page" boundary */
/* Call will not span "page" */
static int OW_r_memory(BYTE * data,  size_t size,  off_t offset,
					    struct parsedname *pn)
{
	BYTE buf[2] = { BYTE_MASK(offset), BYTE_MASK(size), };

	if (buf[1] == 0)
		return 0;

	// 500uS
	return OW_w_scratch(buf, 2, pn) || LCD_byte(0x37, 1, pn)
		|| OW_r_scratch(data, buf[1], pn);
}

/* memory is 112 bytes */
/* can only write 16 bytes at a time (due to scratchpad size) */
/* Will pretend pagesize = 16 */
/* minor inefficiency if start is not on "page" boundary */
/* Call will not span "page" */
static int OW_w_memory( BYTE * data,  size_t size,
					    off_t offset,  struct parsedname *pn)
{
	BYTE buf[17] = { BYTE_MASK(offset), };

	if (size == 0)
		return 0;
	memcpy(&buf[1], data, size);

	return OW_w_scratch(buf, size + 1, pn) || LCD_byte(0x39, 4 * size, pn);
	// 4mS/byte
}

/* data is 16 bytes */
static int OW_r_version(BYTE * data, const struct parsedname *pn)
{
	// 500uS
	return LCD_byte(0x41, 1, pn) || OW_r_scratch(data, 16, pn);
}

static int OW_w_screen(const BYTE loc, const char *text, const int size,
					   const struct parsedname *pn)
{
	BYTE t[17] = { loc, };
	int s;
	int l;

	for (s = size; s > 0; s -= l) {
		l = s;
		if (l > 16)
			l = 16;
		memcpy(&t[1], &text[size - s], (size_t) l);
		if (OW_w_scratch(t, l + 1, pn) || LCD_byte(0x48, 0, pn))
			return 1;
		t[0] += l;
	}
	UT_delay(2);				// 120uS/byte (max 1.92mS)
	return 0;
}

static int OW_clear(const struct parsedname *pn)
{
	/* clear */
	return LCD_byte(0x49, 3, pn);
	// 2.5mS
}

static int LCD_byte(BYTE byte, int delay, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		{&byte, NULL, 1, trxn_match,},
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;
	UT_delay(delay);			// mS
	return 0;
}

static int LCD_2byte(BYTE * byte, int delay, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		{byte, NULL, 2, trxn_match,},
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;
	UT_delay(delay);			// mS
	return 0;
}
