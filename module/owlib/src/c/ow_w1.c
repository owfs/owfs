/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_connection.h"

#if OW_W1

#include "ow_w1.h"
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
static RESET_TYPE W1_reset(const struct parsedname *pn);
static enum search_status W1_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD W1_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static GOOD_OR_BAD W1_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
static void W1_setroutines(struct connection_in *in);
static void W1_close(struct connection_in *in);

static SEQ_OR_ERROR w1_send_touch( const BYTE * data, size_t size, const struct parsedname *pn );
static SEQ_OR_ERROR w1_send_selecttouch( const BYTE * data, size_t size, const struct parsedname *pn );
static SEQ_OR_ERROR w1_send_search( struct device_search *ds, const struct parsedname *pn );
static SEQ_OR_ERROR w1_send_reset( const struct parsedname *pn );

static void W1_setroutines(struct connection_in *in)
{
	in->iroutines.detect = W1_detect;
	in->iroutines.reset = W1_reset;
	in->iroutines.next_both = W1_next_both;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.select_and_sendback = W1_select_and_sendback;
	in->iroutines.sendback_data = W1_sendback_data;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = W1_close;
	in->iroutines.verify = NO_VERIFY_ROUTINE ;
	// Directory obtained in a single gulp (W1_LIST_SLAVES)
	// Bundle transactions
	//
	in->iroutines.flags = ADAP_FLAG_dirgulp | ADAP_FLAG_bundle | ADAP_FLAG_dir_auto_reset | ADAP_FLAG_no2404delay ;
	in->bundling_length = W1_FIFO_SIZE;	// arbitrary number
}

GOOD_OR_BAD W1_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;

	/* Set up low-level routines */
	pin->type = ct_none ;
	W1_setroutines(in);
	Init_Pipe( in->master.w1.netlink_pipe ) ;

	if ( pipe( in->master.w1.netlink_pipe ) != 0 ) {
		ERROR_CONNECT("W1 pipe creation error");
		Init_Pipe( in->master.w1.netlink_pipe ) ;
		return gbBAD ;
	}

	in->Adapter = adapter_w1;
	in->adapter_name = "w1";
	pin->busmode = bus_w1;
	in->master.w1.seq = SEQ_INIT;
	return gbGOOD;
}

/* Send blindly, no response expected */
static SEQ_OR_ERROR w1_send_reset( const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_MASTER_CMD;
	w1m.id.mst.id = pn->selected_connection->master.w1.id ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = W1_CMD_RESET ;
	w1c.len = 0 ;

	LEVEL_DEBUG("Sending w1 reset message");
	return W1_send_msg( pn->selected_connection, &w1m, &w1c, NULL );
}

static RESET_TYPE W1_reset(const struct parsedname *pn)
{
	return W1_Process_Response( NULL, w1_send_reset(pn), NULL, pn ) == nrs_complete ? BUS_RESET_OK : BUS_RESET_ERROR ;
}

static SEQ_OR_ERROR w1_send_search( struct device_search *ds, const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_MASTER_CMD;
	w1m.id.mst.id = pn->selected_connection->master.w1.id ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = (ds->search==_1W_CONDITIONAL_SEARCH_ROM) ? W1_CMD_ALARM_SEARCH : W1_CMD_SEARCH ;
	w1c.len = 0 ;

	LEVEL_DEBUG("Sending w1 search (list devices) message");
	return W1_send_msg( pn->selected_connection, &w1m, &w1c, NULL );
}

// Work around for omap hdq driver bug which reverses the slave address
// We check the CRC8 and use reversed if forward is incorrect.
static void search_callback( struct netlink_parse * nlp, void * v, const struct parsedname * pn )
{
	int i ;
	struct connection_in * in = pn->selected_connection ;
	struct device_search *ds = v ;
	BYTE sn[SERIAL_NUMBER_SIZE] ;
	BYTE * sn_pointer ;

	for ( i = 0 ; i < nlp->w1c->len ; i += SERIAL_NUMBER_SIZE ) {
		switch( in->master.w1.w1_slave_order ) {
			case w1_slave_order_forward:
				sn_pointer = &nlp->data[i] ;
				break ;
			case w1_slave_order_reversed:
				// reverse bytes
				sn[0] = nlp->data[i+7] ;
				sn[1] = nlp->data[i+6] ;
				sn[2] = nlp->data[i+5] ;
				sn[3] = nlp->data[i+4] ;
				sn[4] = nlp->data[i+3] ;
				sn[5] = nlp->data[i+2] ;
				sn[6] = nlp->data[i+1] ;
				sn[7] = nlp->data[i+0] ;
				sn_pointer = sn ;
				break ;
			case w1_slave_order_unknown:
			default:
				sn_pointer = &nlp->data[i] ;
				if ( CRC8(sn_pointer, SERIAL_NUMBER_SIZE) == 0 ) {
					in->master.w1.w1_slave_order = w1_slave_order_forward ;
					break ;
				}
				// reverse bytes
				sn[0] = nlp->data[i+7] ;
				sn[1] = nlp->data[i+6] ;
				sn[2] = nlp->data[i+5] ;
				sn[3] = nlp->data[i+4] ;
				sn[4] = nlp->data[i+3] ;
				sn[5] = nlp->data[i+2] ;
				sn[6] = nlp->data[i+1] ;
				sn[7] = nlp->data[i+0] ;
				sn_pointer = sn ;
				in->master.w1.w1_slave_order = w1_slave_order_reversed ;
				LEVEL_DEBUG( "w1 bus master%d uses reversed slave order", in->master.w1.id ) ;
				break ;
		}
		DirblobAdd(sn_pointer, &(ds->gulp) );
	}
}

static enum search_status W1_next_both(struct device_search *ds, const struct parsedname *pn)
{
	if (ds->LastDevice) {
		return search_done;
	}
	if (++(ds->index) == 0) {
		// first pass, load the directory
		DirblobClear( &(ds->gulp) );
		if ( W1_Process_Response( search_callback, w1_send_search(ds,pn), ds, pn ) != nrs_complete) {
			return search_error;
		}
	}

	switch ( DirblobGet(ds->index, ds->sn, &(ds->gulp) ) ) {
		case 0:
			LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));
			return search_good;
		case -ENODEV:
		default:
			ds->LastDevice = 1;
			LEVEL_DEBUG("SN finished");
			return search_done;
	}
}

static SEQ_OR_ERROR w1_send_selecttouch( const BYTE * data, size_t size, const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_SLAVE_CMD;
	memcpy( w1m.id.id, pn->sn, 8) ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = W1_CMD_TOUCH ;
	w1c.len = size ;

	LEVEL_DEBUG("Sending w1 select message for "SNformat,SNvar(pn->sn));
	return W1_send_msg( pn->selected_connection, &w1m, &w1c, data );
}

struct touch_struct {
	BYTE * resp ;
	size_t size ;
} ;

static void touch( struct netlink_parse * nlp, void * v, const struct parsedname * pn )
{
	struct touch_struct * ts = v ;
	(void) pn ;
	if ( nlp->data == NULL ) {
		return ;
	}
	if ( ts->size == (size_t)nlp->data_size ) {
		memcpy( ts->resp, nlp->data, nlp->data_size ) ;
	}
}

// Reset, select, and read/write data
static GOOD_OR_BAD W1_select_and_sendback(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct touch_struct ts = { resp, size, } ;
	return W1_Process_Response( touch, w1_send_selecttouch(data,size,pn), &ts, pn)==nrs_complete ? gbGOOD : gbBAD ;
}

static SEQ_OR_ERROR w1_send_touch( const BYTE * data, size_t size, const struct parsedname *pn )
{
	struct w1_netlink_msg w1m;
	struct w1_netlink_cmd w1c;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_MASTER_CMD;
	w1m.id.mst.id = pn->selected_connection->master.w1.id ;

	memset(&w1c, 0, W1_W1C_LENGTH);
	w1c.cmd = W1_CMD_TOUCH ;
	w1c.len = size ;

	LEVEL_DEBUG("Sending w1 send/receive data message for "SNformat,SNvar(pn->sn));
	return W1_send_msg( pn->selected_connection, &w1m, &w1c, data );
}

//  Send data and return response block
static GOOD_OR_BAD W1_sendback_data(const BYTE * data, BYTE * resp, const size_t size, const struct parsedname *pn)
{
	struct touch_struct ts = { resp, size, } ;
	return W1_Process_Response( touch, w1_send_touch(data,size,pn), &ts, pn)==nrs_complete ? gbGOOD : gbBAD ;
}

static void W1_close(struct connection_in *in)
{
	Test_and_Close_Pipe( in->master.w1.netlink_pipe );
}

#else							/* OW_W1 */

GOOD_OR_BAD W1_detect(struct port_in *pin)
{
	(void) pin ;
	LEVEL_CONNECT("Kernel 1-wire support was not configured in") ;
	return gbBAD ;
}

#endif							/* OW_W1 */
