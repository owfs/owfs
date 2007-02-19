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

static struct timeval tvnet = { 0, 200000, };

//static void byteprint( const BYTE * b, int size ) ;
static int LINK_write(const BYTE * buf, const size_t size,
					  const struct parsedname *pn);
static int LINK_read(BYTE * buf, const size_t size,
					 const struct parsedname *pn, int ExtraEbyte);
static int LINK_read_low(BYTE * buf, const size_t size,
						 const struct parsedname *pn);
static int LINK_reset(const struct parsedname *pn);
static int LINK_next_both(struct device_search *ds,
						  const struct parsedname *pn);
static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay,
						  const struct parsedname *pn);
static int LINK_sendback_data(const BYTE * data, BYTE * resp,
							  const size_t len,
							  const struct parsedname *pn);
static int LINK_byte_bounce(const BYTE * out, BYTE * in,
							const struct parsedname *pn);
static int LINK_CR(const struct parsedname *pn);
static void LINK_setroutines(struct interface_routines *f);
static void LINKE_setroutines(struct interface_routines *f);
static int LINKE_preamble(const struct parsedname *pn);
static void LINKE_close(struct connection_in *in);

static void LINKE_setroutines(struct interface_routines *f)
{
	f->detect = LINKE_detect;
	f->reset = LINK_reset;
	f->next_both = LINK_next_both;
//    f->overdrive = ;
//    f->testoverdrive = ;
	f->PowerByte = LINK_PowerByte;
//    f->ProgramPulse = ;
	f->sendback_data = LINK_sendback_data;
//    f->sendback_bits = ;
	f->select = NULL;
	f->reconnect = NULL;
	f->close = LINKE_close;
	f->transaction = NULL;
	f->flags = ADAP_FLAG_2409path;
}

#define LINK_string(x)  ((BYTE *)(x))


int LINKE_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.in = in;
	LEVEL_CONNECT("LinkE detect\n");
	/* Set up low-level routines */
	LINKE_setroutines(&(in->iroutines));

    if (in->name ==
        NULL)
		return -1;
	if (ClientAddr(in->name, in))
		return -1;
	if ((pn.in->fd = ClientConnect(in)) < 0)
		return -EIO;

	in->Adapter = adapter_LINK_E;
	if (LINK_write(LINK_string(" "), 1, &pn) == 0) {
		char buf[18];
		if (LINKE_preamble(&pn) || LINK_read((BYTE *) buf, 17, &pn, 1)
			|| strncmp(buf, "Link", 4))
			return -ENODEV;
//        printf("LINKE\n");
		in->adapter_name = "Link-Hub-E";
		return 0;
	}
	return -EIO;
}

static int LINK_reset(const struct parsedname *pn)
{
	BYTE resp[5];
	int ret = 0;

	if (pn->in->Adapter != adapter_LINK_E)
		COM_flush(pn);
    //if (LINK_write(LINK_string("\rr"), 2, pn) || LINK_read(resp, 4, pn, 1)) {
    if (LINK_write(LINK_string("r"), 1, pn) || LINK_read(resp, 4, pn, 1)) {
        STAT_ADD1_BUS(BUS_reset_errors, pn->in);
		return -EIO;
	}
	switch (resp[1]) {
	case 'P':
		pn->in->AnyDevices = 1;
		break;
	case 'N':
		pn->in->AnyDevices = 0;
		break;
	default:
		ret = 1;				// marker for shorted bus
		pn->in->AnyDevices = 0;
		STAT_ADD1_BUS(BUS_short_errors, pn->in);
		LEVEL_CONNECT("1-wire bus short circuit.\n")
	}
	return 0;
}

static int LINK_next_both(struct device_search *ds,
						  const struct parsedname *pn)
{
	char resp[21];
	int ret;

	if (!pn->in->AnyDevices)
		ds->LastDevice = 1;
	if (ds->LastDevice)
		return -ENODEV;

	if (pn->in->Adapter != adapter_LINK_E)
		COM_flush(pn);
	if (ds->LastDiscrepancy == -1) {
		if ((ret = LINK_write(LINK_string("f"), 1, pn)))
			return ret;
		ds->LastDiscrepancy = 0;
	} else {
		if ((ret = LINK_write(LINK_string("n"), 1, pn)))
			return ret;
	}

	if ((ret = LINK_read(LINK_string(resp), 20, pn, 1))) {
		return ret;
	}

	switch (resp[0]) {
	case '-':
		ds->LastDevice = 1;
	case '+':
		break;
	case 'N':
		pn->in->AnyDevices = 0;
		return -ENODEV;
	case 'X':
	default:
		return -EIO;
	}

	ds->sn[7] = string2num(&resp[2]);
	ds->sn[6] = string2num(&resp[4]);
	ds->sn[5] = string2num(&resp[6]);
	ds->sn[4] = string2num(&resp[8]);
	ds->sn[3] = string2num(&resp[10]);
	ds->sn[2] = string2num(&resp[12]);
	ds->sn[1] = string2num(&resp[14]);
	ds->sn[0] = string2num(&resp[16]);

	// CRC check
	if (CRC8(ds->sn, 8) || (ds->sn[0] == 0)) {
		/* A minor "error" and should perhaps only return -1 to avoid reconnect */
		return -EIO;
	}

	if ((ds->sn[0] & 0x7F) == 0x04) {
		/* We found a DS1994/DS2404 which require longer delays */
		pn->in->ds2404_compliance = 1;
	}

	LEVEL_DEBUG("LINK_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return 0;
}

/* Read from Link or Link-E
   0=good else bad
   Note that buffer length should 1 exta char long for ethernet reads
*/
static int LINK_read(BYTE * buf, const size_t size,
					 const struct parsedname *pn, int ExtraEbyte)
{
    if ( tcp_read(pn->in->fd, buf, size+ExtraEbyte, &tvnet) != size+ExtraEbyte ) {
        LEVEL_CONNECT("LINK_read (ethernet) error\n");
        return -EIO ;
	}
	return 0;
}

//
// Write a string to the serial port
/* return 0=good,
          -EIO = error
   Special processing for the remote hub (add 0x0A)
 */
static int LINK_write(const BYTE * buf, const size_t size,
					  const struct parsedname *pn)
{
	ssize_t r ;
    struct iovec write_vectors[2] = {
        { buf, size } ,
        { "\r",  1  } ,
    } ;
    int vector_count = (pn->in->Adapter == adapter_LINK_E) ? 2 : 1 ;
    Debug_Bytes( "LINK write", buf, size) ;
    if ( vector_count>1) Debug_Bytes("LF",write_vectors[1].iov_base,write_vectors[1].iov_len) ;
//    COM_flush(pn) ;
    r = writev(pn->in->fd, write_vectors, vector_count);

    if (r < 0) {
        ERROR_CONNECT("Trouble writing data to LINK: %s\n",
                        SAFESTRING(pn->in->name));
        return r ;
    }

    tcdrain(pn->in->fd);
    gettimeofday(&(pn->in->bus_write_time), NULL);
	
    if (r+1 < size+vector_count) {
        LEVEL_CONNECT("Short write to LINK -- intended %d, sent %d\n",(int)size+vector_count-1,(int)r) ;
		STAT_ADD1_BUS(BUS_write_errors, pn->in);
		return -EIO;
	}
	//printf("Link wrote <%*s>\n",(int)size,buf);
	return 0;
}

static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay,
						  const struct parsedname *pn)
{

	if (LINK_write(LINK_string("p"), 1, pn)
		|| LINK_byte_bounce(&data, resp, pn)) {
		STAT_ADD1_BUS(BUS_PowerByte_errors, pn->in);
		return -EIO;			// send just the <CR>
	}
	// delay
	UT_delay(delay);

	// flush the buffers
	return LINK_CR(pn);			// send just the <CR>
}

// DS2480_sendback_data
//  Send data and return response block
//  puts into data mode if needed.
/* return 0=good
   sendout_data, readin
 */
static int LINK_sendback_data(const BYTE * data, BYTE * resp,
							  const size_t size,
							  const struct parsedname *pn)
{
	size_t i;
	size_t left;
	BYTE *buf = pn->in->combuffer;

	if (size == 0)
		return 0;
	if (LINK_write(LINK_string("b"), 1, pn))
		return -EIO;
//    for ( i=0; ret==0 && i<size ; ++i ) ret = LINK_byte_bounce( &data[i], &resp[i], pn ) ;
	for (left = size; left;) {
		i = (left > 16) ? 16 : left;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) buf, &data[size - left], i);
		if (LINK_write(buf, i << 1, pn) || LINK_read(buf, i << 1, pn, 0))
			return -EIO;
		string2bytes((char *) buf, &resp[size - left], i);
		left -= i;
	}
	return LINK_CR(pn);
}

/*
static void byteprint( const BYTE * b, int size ) {
    int i ;
    for ( i=0; i<size; ++i ) printf( "%.2X ",b[i] ) ;
    if ( size ) printf("\n") ;
}
*/

static int LINK_byte_bounce(const BYTE * out, BYTE * in,
							const struct parsedname *pn)
{
	BYTE data[2];

	num2string((char *) data, out[0]);
	if (LINK_write(data, 2, pn) || LINK_read(data, 2, pn, 0))
		return -EIO;
	in[0] = string2num((char *) data);
	return 0;
}

static int LINK_CR(const struct parsedname *pn)
{
	BYTE data[3];
	if (LINK_write(LINK_string("\r"), 1, pn) || LINK_read(data, 2, pn, 1))
		return -EIO;
	return 0;
}

/* read the telnet-formatted start of a response line from the Link-Hub-E */
static int LINKE_preamble(const struct parsedname *pn)
{
	BYTE data[6];
	struct timeval tvnetfirst = { Global.timeout_network, 0, };
	if (tcp_read(pn->in->fd, data, 6, &tvnetfirst) != 6)
		return -EIO;
	LEVEL_CONNECT("Good preamble\n");
	return 0;
}

static void LINKE_close(struct connection_in *in)
{
	if (in->fd >= 0) {
		close(in->fd);
		in->fd = -1;
	}
	FreeClientAddr(in);
}
