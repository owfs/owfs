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

#if OW_W1

#include "ow_w1.h"
#include "ow_connection.h"
#include "ow_codes.h"
#include "ow_counters.h"

struct toW1 {
	ASCII *command;
	ASCII lock[10];
	ASCII conditional[1];
	ASCII address[16];
	const BYTE *data;
	size_t length;
};

//static void byteprint( const BYTE * b, int size ) ;
static int W1_write(int file_descriptor, const ASCII * msg, size_t size, struct connection_in *in);
static void toW1init(struct toW1 *ha7);
static void setW1address(struct toW1 *ha7, const BYTE * sn);
static int W1_toW1(int file_descriptor, const struct toW1 *ha7, struct connection_in *in);
static int W1_read(int file_descriptor, struct memblob *mb);
static int W1_reset(const struct parsedname *pn);
static int W1_next_both(struct device_search *ds, const struct parsedname *pn);
static int W1_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int W1_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int W1_sendback_block(const BYTE * data, BYTE * resp, const size_t size, int also_address, const struct parsedname *pn);
static int W1_select(const struct parsedname *pn);
static void W1_setroutines(struct connection_in *in);
static void W1_close(struct connection_in *in);
static int W1_directory(BYTE search, struct dirblob *db, const struct parsedname *pn);

static void W1_setroutines(struct connection_in *in)
{
	in->iroutines.detect = W1_detect;
	in->iroutines.reset = W1_reset;
	in->iroutines.next_both = W1_next_both;
	in->iroutines.PowerByte = NULL;
	//    in->iroutines.ProgramPulse = ;
	in->iroutines.select_and_sendback = W1_select_and_sendback;
	in->iroutines.sendback_data = W1_sendback_data;
	//    in->iroutines.sendback_bits = ;
	in->iroutines.select = W1_select;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = W1_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset;
	in->bundling_length = W1_FIFO_SIZE;	// arbitrary number
}

int W1_detect(struct connection_in *in)
{
	struct parsedname pn;
	int pipe_fd[2] ;

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.selected_connection = in;
	LEVEL_CONNECT("W1 detect\n");

	/* Set up low-level routines */
	W1_setroutines(in);

	/* Initialize dir-at-once structures */
	DirblobInit(&(in->connin.w1.main));
	DirblobInit(&(in->connin.w1.alarm));

	if ( pipe( pipe_fd ) == 0 ) {
		in->connin.w1.read_file_descriptor = pipe_fd[0] ;
		in->connin.w1.write_file_descriptor = pipe_fd[1] ;
	} else {
		ERROR_CONNECT("W1 pipe creation error\n");
		in->connin.w1.read_file_descriptor = -1 ;
		in->connin.w1.write_file_descriptor = -1 ;
		return -1 ;
	}

	#if OW_MT
	pthread_mutex_init(&(Inbound_Control.w1_mutex), Mutex.pmattr);
	#endif							/* OW_MT */

	if (in->name == NULL) {
		return -1;
	}

	in->Adapter = adapter_w1;
	in->adapter_name = "w1";
	in->busmode = bus_w1;
	in->AnyDevices = 1;
	return 0;
}

static int W1_reset(const struct parsedname *pn)
{
	// w1 doesn't use an explicit reset
	(void) pn ;
	return 0 ;
}

static int w1_send_search( BYTE search, const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_MASTER_CMD;
	w1m.id.mst.id = pn->selected_connection->connin.w1.id ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = (search==_1W_CONDITIONAL_SEARCH_ROM) ? W1_CMD_ALARM_SEARCH : W1_CMD_SEARCH ;
	w1c.len = 0 ;

	return W1_send_msg( pn->selected_connection, &w1m, &w1c );
}

static int W1_directory(BYTE search, struct dirblob *db, const struct parsedname *pn)
{
	int seq ;

	DirblobClear(db);
	printf("w1_directory\n");
	seq = w1_send_search( search, pn ) ;
	if ( seq < 0 ) {
		return -EIO ;
	}
	do {
		int i ;
		struct netlink_parse nlp ;
		if ( Get_and_Parse_Pipe( pn->selected_connection->connin.w1.read_file_descriptor, &nlp ) != 0 ) {
			return -EIO ;
		}
		if ( NL_SEQ(nlp.nlm->nlmsg_seq) != (unsigned) seq ) {
			LEVEL_DEBUG("Netlink sequence number out of order: expected %d\n",seq);
			free(nlp.nlm) ;
			continue ;
		}
		if ( nlp.w1m->type != W1_MASTER_CMD ) {
			LEVEL_DEBUG("Not W1_MASTER_CMD\n");
			free(nlp.nlm) ;
			return -EIO ;
		}
		for ( i = 0 ; i < nlp.w1c->len ; i += 8 ) {
			DirblobAdd(&nlp.data[i], db);
		}
		free(nlp.nlm) ;
		if ( nlp.cn->ack == 0 ) {
			break ;
		}
	} while (1) ;
	return 0;
}

static int W1_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->connin.w1.alarm) : &(pn->selected_connection->connin.w1.main);
	int ret = 0;

	if (!pn->selected_connection->AnyDevices) {
		ds->LastDevice = 1;
	}
	if (ds->LastDevice) {
		return -ENODEV;
	}
	if (++(ds->index) == 0) {
		if (W1_directory(ds->search, db, pn)) {
			return -EIO;
		}
	}
	ret = DirblobGet(ds->index, ds->sn, db);
	switch (ret) {
	case 0:
		if ((ds->sn[0] & 0x7F) == 0x04) {
			/* We found a DS1994/DS2404 which require longer delays */
			pn->selected_connection->ds2404_compliance = 1;
		}
		break;
	case -ENODEV:
		ds->LastDevice = 1;
		break;
	}
	return ret;
}

#define W1_READ_BUFFER_LENGTH 2000

static int W1_read(int file_descriptor, struct memblob *mb)
{
	ASCII readin_area[W1_READ_BUFFER_LENGTH + 1];
	ASCII *start;
	ssize_t read_size;
	struct timeval tvnet = { Globals.timeout_ha7, 0, };

	MemblobInit(mb, W1_READ_BUFFER_LENGTH);

	// Read first block of data from W1
	if ((read_size = tcp_read(file_descriptor, readin_area, W1_READ_BUFFER_LENGTH, &tvnet)) < 0) {
		LEVEL_CONNECT("W1_read (ethernet) error = %d\n", read_size);
		//write(1, readin_area, read_size);
		return -EIO;
	}
	// make sure null terminated (allocated extra byte in readin_area to always have room)
	readin_area[read_size] = '\0';

	// Look for happy response
	if (strncmp("HTTP/1.1 200 OK", readin_area, 15)) {	//Bad HTTP return code
		ASCII *p = strchr(&readin_area[15], '\n');
		if (p == NULL) {
			p = &readin_area[15 + 32];
		}
		LEVEL_DATA("W1 response problem:%.*s\n", p - readin_area - 15, &readin_area[15]);
		return -EINVAL;
	}
	// Look for "<body>"
	if ((start = strstr(readin_area, "<body>")) == NULL) {
		LEVEL_DATA("W1 response: No HTTP body to parse\n");
		MemblobClear(mb);
		return -EINVAL;
	}
	// HTML body found, dump header
	if (MemblobAdd((BYTE *) start, read_size - (start - readin_area), mb)) {
		MemblobClear(mb);
		return -ENOMEM;
	}
	// loop through reading in W1_READ_BUFFER_LENGTH blocks
	while (read_size == W1_READ_BUFFER_LENGTH) {	// full read, so presume more waiting
		if ((read_size = tcp_read(file_descriptor, readin_area, W1_READ_BUFFER_LENGTH, &tvnet)) < 0) {
			LEVEL_DATA("Couldn't get rest of W1 data (err=%d)\n", read_size);
			MemblobClear(mb);
			return -EIO;
		} else if (MemblobAdd((BYTE *) readin_area, read_size, mb)) {
			MemblobClear(mb);
			return -ENOMEM;
		}
	}

	// Add trailing null
	if (MemblobAdd((BYTE *) "", 1, mb)) {
		MemblobClear(mb);
		return -ENOMEM;
	}
	//printf("READ FROM W1:\n%s\n",mb->memory_storage);
	return 0;
}

static int W1_write(int file_descriptor, const ASCII * msg, size_t length, struct connection_in *in)
{
	ssize_t r, sl = length;
	ssize_t size = sl;
	while (sl > 0) {
		r = write(file_descriptor, &msg[size - sl], sl);
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			ERROR_CONNECT("Trouble writing data to W1: %s\n", SAFESTRING(in->name));
			break;
		}
		sl -= r;
	}
	gettimeofday(&(in->bus_write_time), NULL);
	if (sl > 0) {
		STAT_ADD1_BUS(e_bus_write_errors, in);
		return -EIO;
	}
	return 0;
}

static int W1_toW1(int file_descriptor, const struct toW1 *ha7, struct connection_in *in)
{
	int first = 1;
	int probable_length;
	int ret = 0;
	char *full_command;
	LEVEL_DEBUG
		("To W1 command=%s address=%.16s conditional=%.1s lock=%.10s\n",
		 SAFESTRING(ha7->command), SAFESTRING(ha7->address), SAFESTRING(ha7->conditional), SAFESTRING(ha7->lock));

	if (ha7->command == NULL) {
		return -EINVAL;
	}

	probable_length = 11 + strlen(ha7->command) + 5 + ((ha7->address[0]) ? 1 + 8 + 16 : 0)
		+ ((ha7->conditional[0]) ? 1 + 12 + 1 : 0)
		+ ((ha7->data) ? 1 + 5 + ha7->length * 2 : 0)
		+ ((ha7->lock[0]) ? 1 + 7 + 10 : 0)
		+ 11 + 1;

	full_command = malloc(probable_length);
	if (full_command == NULL) {
		return -ENOMEM;
	}
	memset(full_command, 0, probable_length);

	strcpy(full_command, "GET /1Wire/");
	strcat(full_command, ha7->command);
	strcat(full_command, ".html");

	if (ha7->address[0]) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "Address=");
		strcat(full_command, ha7->address);
		first = 0;
	}

	if (ha7->conditional[0]) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "Conditional=");
		strcat(full_command, ha7->conditional);
		first = 0;
	}

	if (ha7->data) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "Data=");
		bytes2string(&full_command[strlen(full_command)], ha7->data, ha7->length);
	}

	if (ha7->lock[0]) {
		strcat(full_command, first ? "?" : "&");
		strcat(full_command, "LockID=");
		strcat(full_command, ha7->lock);
		first = 0;
	}

	strcat(full_command, " HTTP/1.0\n\n");

	LEVEL_DEBUG("To W1 %s", full_command);

	ret = W1_write(file_descriptor, full_command, probable_length, in);
	free(full_command);
	return ret;
}

static int w1_send_touch( const BYTE * data, size_t size, const struct parsedname *pn )
{
    struct w1_netlink_msg w1m;
    struct w1_netlink_cmd w1c;

    memset(&w1m, 0, W1_W1M_LENGTH);
    w1m.type = W1_SLAVE_CMD;
    memcpy( w1m.id.id, pn->sn, 8) ;

    memset(&w1c, 0, W1_W1C_LENGTH);
    w1c.cmd = W1_CMD_TOUCH_BLOCK ;
    w1c.len = size ;

    memcpy( w1c.data, data, size ) ;

    return W1_send_msg( pn->selected_connection, &w1m, &w1c );
}

// Reset, select, and read/write data
/* return 0=good
   sendout_data, readin
 */
static int W1_select_and_sendback(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
    int seq = w1_send_touch(data,size,pn) ;

    if ( seq < 0 ) {
        return -EIO ;
    }

    do {
        int i ;
        struct netlink_parse nlp ;
        if ( Get_and_Parse_Pipe( pn->selected_connection->connin.w1.read_file_descriptor, &nlp ) != 0 ) {
            return -EIO ;
        }
        if ( NL_SEQ(nlp.nlm->nlmsg_seq) != (unsigned) seq ) {
            LEVEL_DEBUG("Netlink sequence number out of order: expected %d\n",seq);
            free(nlp.nlm) ;
            continue ;
        }
        if ( nlp.w1c.len == size ) {}
        for ( i = 0 ; i < nlp.w1c->len ; i += 8 ) {
            DirblobAdd(&nlp.data[i], db);
        }
        free(nlp.nlm) ;
        if ( nlp.cn->ack == 0 ) {
            break ;
        }
    } while (1) ;
    return 0;
}

// DS2480_sendback_data
//  Send data and return response block
/* return 0=good
   sendout_data, readin
 */
static int W1_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	size_t location = 0;

	while (location < size) {
		size_t block = size - location;
		if (block > 32) {
			block = 32;
		}
		// Don't add address (that's the "0")
		if (W1_sendback_block(&data[location], &resp[location], block, 0, pn)) {
			return -EIO;
		}
		location += block;
	}
	return 0;
}

// W1 only allows WriteBlock of 32 bytes
// This routine assumes that larger writes have already been broken up
static int W1_sendback_block(const BYTE * data, BYTE * resp, const size_t size, int also_address, const struct parsedname *pn)
{
	int file_descriptor;
	struct memblob mb;
	struct toW1 ha7;
	int ret = -EIO;

	if ((file_descriptor = ClientConnect(pn->selected_connection)) < 0) {
		return -EIO;
	}

	toW1init(&ha7);
	ha7.command = "WriteBlock";
	ha7.data = data;
	ha7.length = size;
	if (also_address) {
		setW1address(&ha7, pn->sn);
	}

	if (W1_toW1(file_descriptor, &ha7, pn->selected_connection) == 0) {
		if (W1_read(file_descriptor, &mb) == 0) {
			ASCII *p = (ASCII *) mb.memory_storage;
			if ((p = strstr(p, "<INPUT TYPE=\"TEXT\" NAME=\"ResultData_0\""))
				&& (p = strstr(p, "VALUE=\""))) {
				p += 7;
				LEVEL_DEBUG("W1_sendback_data received(%d): %.*s\n", size * 2, size * 2, p);
				if (strspn(p, "0123456789ABCDEF") >= size << 1) {
					string2bytes(p, resp, size);
					ret = 0;
				}
			}
			MemblobClear(&mb);
		} else {
			STAT_ADD1_BUS(e_bus_read_errors, pn->selected_connection);
		}
	}
	close(file_descriptor);
	return ret;
}

static void setW1address(struct toW1 *ha7, const BYTE * sn)
{
	num2string(&(ha7->address[0]), sn[7]);
	num2string(&(ha7->address[2]), sn[6]);
	num2string(&(ha7->address[4]), sn[5]);
	num2string(&(ha7->address[6]), sn[4]);
	num2string(&(ha7->address[8]), sn[3]);
	num2string(&(ha7->address[10]), sn[2]);
	num2string(&(ha7->address[12]), sn[1]);
	num2string(&(ha7->address[14]), sn[0]);
}

static int W1_select(const struct parsedname *pn)
{
	int ret = -EIO;

	if (pn->selected_device) {
		int file_descriptor = ClientConnect(pn->selected_connection);

		if (file_descriptor >= 0) {
			struct toW1 ha7;
			toW1init(&ha7);
			ha7.command = "AddressDevice";
			setW1address(&ha7, pn->sn);
			if (W1_toW1(file_descriptor, &ha7, pn->selected_connection)
				== 0) {
				struct memblob mb;
				if (W1_read(file_descriptor, &mb) == 0) {
					MemblobClear(&mb);
					ret = 0;
				}
			}
			close(file_descriptor);
		}
	} else {
		return W1_reset(pn);
	}
	return ret;
}

static void W1_close(struct connection_in *in)
{
	DirblobClear(&(in->connin.ha7.main));
	DirblobClear(&(in->connin.ha7.alarm));
	Test_and_Close( &(in->connin.w1.read_file_descriptor) );
	Test_and_Close( &(in->connin.w1.write_file_descriptor) );
	FreeClientAddr(in);
}

static void toW1init(struct toW1 *ha7)
{
	memset(ha7, 0, sizeof(struct toW1));
}

#endif							/* OW_W1 */
