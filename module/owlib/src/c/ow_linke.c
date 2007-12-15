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
					 const struct parsedname *pn);
static int LINK_reset(const struct parsedname *pn);
static int LINK_next_both(struct device_search *ds,
						  const struct parsedname *pn);
static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay,
						  const struct parsedname *pn);
static int LINK_sendback_data(const BYTE * data, BYTE * resp,
							  const size_t len,
							  const struct parsedname *pn);
static int LINKE_setroutines(struct connection_in *in);
static int LINKE_preamble(const struct parsedname *pn);
static void LINKE_close(struct connection_in *in);

static int LINKE_setroutines(struct connection_in * in)
{
	in->iroutines.detect = LINKE_detect;
    in->iroutines.reset = LINK_reset;
    in->iroutines.next_both = LINK_next_both;
    in->iroutines.PowerByte = LINK_PowerByte;
//    in->iroutines.ProgramPulse = ;
    in->iroutines.sendback_data = LINK_sendback_data;
//    in->iroutines.sendback_bits = ;
    in->iroutines.select = NULL;
    in->iroutines.reconnect = NULL;
    in->iroutines.close = LINKE_close;
    in->iroutines.transaction = NULL;
    in->iroutines.flags = ADAP_FLAG_2409path;
    in->combuffer_length = 0 ; // no buffer needed
    return 0 ;
}

#define LINK_string(x)  ((BYTE *)(x))


int LINKE_detect(struct connection_in *in)
{
	struct parsedname pn;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;
	LEVEL_CONNECT("LinkE detect\n");
	/* Set up low-level routines */
    if (LINKE_setroutines(in)) {
        return -ENOMEM ;
    }

    if (in->name ==
        NULL)
		return -1;
	if (ClientAddr(in->name, in))
		return -1;
	if ((pn.selected_connection->file_descriptor = ClientConnect(in)) < 0)
		return -EIO;

	in->Adapter = adapter_LINK_E;
	if (LINK_write(LINK_string(" "), 1, &pn) == 0) {
		char buf[18];
		if (LINKE_preamble(&pn) || LINK_read((BYTE *) buf, 18, &pn)
			|| strncmp(buf, "Link", 4))
			return -ENODEV;
		in->adapter_name = "Link-Hub-E";
		return 0;
	}
	return -EIO;
}

static int LINK_reset(const struct parsedname *pn)
{
	BYTE resp[8];
	int ret ;
	
	tcp_read_flush( pn->selected_connection->file_descriptor ) ;
	
	// Send 'r' reset
	if (LINK_write(LINK_string("r"), 1, pn) || LINK_read(resp, 4, pn)) {
		return -EIO;
	}

	switch (resp[0]) {

	case 'P':
		pn->selected_connection->AnyDevices = 1;
		ret = BUS_RESET_OK ;
		break;

	case 'N':
		pn->selected_connection->AnyDevices = 0;
		ret = BUS_RESET_OK ;
		break;

	case 'S':
		ret = BUS_RESET_SHORT ;
		break ;

	default:
		ret = -EIO;
		break ;

	}

	return ret ;
}

static int LINK_next_both(struct device_search *ds,
						  const struct parsedname *pn)
{
	char resp[21];
	int ret;

	if (!pn->selected_connection->AnyDevices)
		ds->LastDevice = 1;
	if (ds->LastDevice)
		return -ENODEV;

    ++ds->index ;
    if (ds->index == 0) {
		if ((ret = LINK_write(LINK_string("f"), 1, pn)))
			return ret;
	} else {
		if ((ret = LINK_write(LINK_string("n"), 1, pn)))
			return ret;
	}

	if ((ret = LINK_read(LINK_string(resp), 21, pn))) {
		return ret;
	}

	switch (resp[0]) {
	case '-':
		ds->LastDevice = 1;
	case '+':
		break;
	case 'N':
		pn->selected_connection->AnyDevices = 0;
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
		pn->selected_connection->ds2404_compliance = 1;
	}

	LEVEL_DEBUG("LINK_next_both SN found: " SNformat "\n", SNvar(ds->sn));
	return 0;
}

/* Read from Link or Link-E
   0=good else bad
   Note that buffer length should 1 exta char long for ethernet reads
*/
static int LINK_read(BYTE * buf, const size_t size,
					 const struct parsedname *pn)
{
    if ( tcp_read(pn->selected_connection->file_descriptor, buf, size, &tvnet) != (ssize_t) size ) {
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
    //Debug_Bytes( "LINK write", buf, size) ;
    r = write(pn->selected_connection->file_descriptor, buf, size);

    if (r < 0) {
        ERROR_CONNECT("Trouble writing data to LINK: %s\n",
                        SAFESTRING(pn->selected_connection->name));
        return r ;
    }

    tcdrain(pn->selected_connection->file_descriptor);
    gettimeofday(&(pn->selected_connection->bus_write_time), NULL);
	
    if (r < (ssize_t) size) {
        LEVEL_CONNECT("Short write to LINK -- intended %d, sent %d\n",(int)size,(int)r) ;
		STAT_ADD1_BUS(e_bus_write_errors, pn->selected_connection);
		return -EIO;
	}
	//printf("Link wrote <%*s>\n",(int)size,buf);
	return 0;
}

static int LINK_PowerByte(const BYTE data, BYTE * resp, const UINT delay,
						  const struct parsedname *pn)
{
	ASCII buf[3] = "pxx" ;
	
	num2string( &buf[1], data ) ;
	
	if (LINK_write(LINK_string(buf), 3, pn) || LINK_read(LINK_string(buf),2,pn) ) {
		return -EIO;			// send just the <CR>
	}
	
	resp[0] = string2num(buf) ;
	
	// delay
	UT_delay(delay);
	
	if (LINK_write(LINK_string("\r"), 1, pn) || LINK_read(LINK_string(buf),3,pn) ) {
	return -EIO;            // send just the <CR>
	}
	
	return 0 ;
}

// _sendback_data
//  Send data and return response block
/* return 0=good
   sendout_data, readin
 */
// Assume buffer length (combuffer) is 1 + 32*2 + 1
static int LINK_sendback_data(const BYTE * data, BYTE * resp,
							  const size_t size,
							  const struct parsedname *pn)
{
	size_t left = size ;
	BYTE *buf = pn->selected_connection->combuffer;

    if (size == 0)
		return 0;

    while ( left > 0 ) {
        buf[0] = 'b' ; //put in byte mode
        size_t this_length = (left > 32) ? 32 : left;
        size_t total_length = 2 * this_length + 2 ;
//        printf(">> size=%d, left=%d, i=%d\n",size,left,i);
		bytes2string((char *) &buf[1], &data[size - left], this_length);
        buf[total_length-1] = '\r' ; // take out of byte mode
		if (LINK_write(buf, total_length, pn) || LINK_read(buf, total_length+2, pn))
			return -EIO;
		string2bytes((char *) buf, &resp[size - left], this_length);
		left -= this_length;
	}
	return 0;
}

/* read the telnet-formatted start of a response line from the Link-Hub-E */
static int LINKE_preamble(const struct parsedname *pn)
{
	BYTE data[6];
	struct timeval tvnetfirst = { Global.timeout_network, 0, };
	if (tcp_read(pn->selected_connection->file_descriptor, data, 6, &tvnetfirst) != 6)
		return -EIO;
	LEVEL_CONNECT("Good preamble\n");
	return 0;
}

static void LINKE_close(struct connection_in *in)
{
	if (in->file_descriptor >= 0) {
		close(in->file_descriptor);
		in->file_descriptor = -1;
	}
	FreeClientAddr(in);
}
