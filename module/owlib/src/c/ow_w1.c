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
static int W1_reset(const struct parsedname *pn);
static int W1_next_both(struct device_search *ds, const struct parsedname *pn);
static int W1_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int W1_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static int W1_sendback_block(const BYTE * data, BYTE * resp, const size_t size, int also_address, const struct parsedname *pn);
static int W1_select(const struct parsedname *pn);
static void W1_setroutines(struct connection_in *in);
static void W1_close(struct connection_in *in);
static int W1_directory(BYTE search, struct dirblob *db, const struct parsedname *pn);
static int sendback(int seq, BYTE * resp, const size_t size, const struct parsedname *pn) ;

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
	// Directory obtained in a single gulp (W1_LIST_SLAVES)
	// Bundle transactions
	//
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

	in->connin.w1.awaiting_response = 0 ; // start out with no expectations.

	if ( pipe( pipe_fd ) == 0 ) {
		in->connin.w1.read_file_descriptor = pipe_fd[0] ;
		in->connin.w1.write_file_descriptor = pipe_fd[1] ;
	} else {
		ERROR_CONNECT("W1 pipe creation error\n");
		in->connin.w1.read_file_descriptor = -1 ;
		in->connin.w1.write_file_descriptor = -1 ;
		return -1 ;
	}

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
	return -ENOTSUP ;
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

	return W1_send_msg( pn->selected_connection, &w1m, &w1c, NULL );
}

static int W1_directory(BYTE search, struct dirblob *db, const struct parsedname *pn)
{
	int seq ;
	int ret ;

	DirblobClear(db);
	seq = w1_send_search( search, pn ) ;
	if ( seq < 0 ) {
		return -EIO ;
	}

	while ( W1PipeSelect_timeout(pn->selected_connection->connin.w1.read_file_descriptor)  ==0 ) {
		struct netlink_parse nlp ;
		int i ;
		if ( Get_and_Parse_Pipe( pn->selected_connection->connin.w1.read_file_descriptor, &nlp ) != 0 ) {
			LEVEL_DEBUG("Error reading pipe for w1_bus_master%d\n",pn->selected_connection->connin.w1.id);
			ret = -EIO ;
			break ;
		}
		if ( NL_SEQ(nlp.nlm->nlmsg_seq) != (unsigned int) seq ) {
			LEVEL_DEBUG("Netlink sequence number out of order\n");
			free(nlp.nlm) ;
			continue ;
		}
		for ( i = 0 ; i < nlp.w1c->len ; i += 8 ) {
			DirblobAdd(&nlp.data[i], db);
		}
		free(nlp.nlm) ;
		if ( nlp.cn->ack == 0 ) {
			break ;
		}
	}
	pn->selected_connection->connin.w1.awaiting_response = 0 ;
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

static int sendback(int seq, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	int ret = -EINVAL ;

	if ( seq < 0 ) {
		return -EIO ;
	}

	while ( W1PipeSelect_timeout(pn->selected_connection->connin.w1.read_file_descriptor)  ==0 ) {
		struct netlink_parse nlp ;
		if ( Get_and_Parse_Pipe( pn->selected_connection->connin.w1.read_file_descriptor, &nlp ) != 0 ) {
			LEVEL_DEBUG("Error reading pipe for w1_bus_master%d\n",pn->selected_connection->connin.w1.id);
			ret  =-EIO ;
			break ;
		}
		if ( NL_SEQ(nlp.nlm->nlmsg_seq) != (unsigned int) seq ) {
			LEVEL_DEBUG("Netlink sequence number out of order\n");
		} else if ( nlp.w1c->len == size ) {
			memcpy(resp, nlp.w1c->data, size ) ;
			ret = 0 ;
		}
		free(nlp.nlm) ;
	}
	pn->selected_connection->connin.w1.awaiting_response = 0 ;
	return ret ;
}

static int w1_send_selecttouch( const BYTE * data, size_t size, const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_SLAVE_CMD;
	memcpy( w1m.id.id, pn->sn, 8) ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = W1_CMD_TOUCH ;
	w1c.len = size ;

	return W1_send_msg( pn->selected_connection, &w1m, &w1c, data );
}

// Reset, select, and read/write data
/* return 0=good
sendout_data, readin
*/
static int W1_select_and_sendback(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	return sendback( w1_send_selecttouch(data,size,pn), resp, size, pn) ;
}

static int w1_send_touch( const BYTE * data, size_t size, const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_MASTER_CMD;
	w1m.id.mst.id = pn->selected_connection->connin.w1.id ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = W1_CMD_TOUCH ;
	w1c.len = size ;

	return W1_send_msg( pn->selected_connection, &w1m, &w1c, data );
}

// DS2480_sendback_data
//  Send data and return response block
/* return 0=good
sendout_data, readin
*/
static int W1_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	return sendback( w1_send_touch(data,size,pn), resp, size, pn) ;
}

static int W1_select(const struct parsedname *pn)
{
	(void) pn ;
	return -ENOTSUP ;
}

static void W1_close(struct connection_in *in)
{
	DirblobClear(&(in->connin.w1.main));
	DirblobClear(&(in->connin.w1.alarm));
	Test_and_Close( &(in->connin.w1.read_file_descriptor) );
	Test_and_Close( &(in->connin.w1.write_file_descriptor) );
	FreeClientAddr(in);
}

#endif							/* OW_W1 */
