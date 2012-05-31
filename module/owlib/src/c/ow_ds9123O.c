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

/* DS9123O support
   PIC (Actually PICBrick) based
   PIC16C745

   Actually the adapter supports several protocols, including 1-wire, 2-wire, 3-wire, SPI...
   but we only support 1-wire

   USB 1.1 based, using HID interface

   ENDPOINT 1, 8-byte commands
   simple commands: reset, in/out 1bit, 1byte, ...
*/


/* All the rest of the program sees is the DS9123O_detect and the entry in iroutines */

#define	OneBit	0xFF
#define ZeroBit 0xC0

/*'*********************************************
'Available One Wire PICBrick Dispatch Commands
 *'*********************************************/

#define DS9123O_VendorID     0x04FA
#define DS9123O_ProductID    0x9123

#define DS9123O_owreset     0x10
#define DS9123O_owresetskip     0x11
#define DS9123O_owtx8     0x12
#define DS9123O_owrx8     0x13
#define DS9123O_owpacketread     0x14
#define DS9123O_owpacketwrite     0x15
#define DS9123O_owread8bytes     0x16
#define DS9123O_owdqhi     0x17
#define DS9123O_owdqlow     0x18
#define DS9123O_owreadbit     0x19
#define DS9123O_owwritebit     0x1A
#define DS9123O_owtx8turbo     0x1B
#define DS9123O_owrx8turbo     0x1C
#define DS9123O_owtx1turbo     0x1D
#define DS9123O_owrx1turbo     0x1E
#define DS9123O_owresetturbo     0x1F
#define DS9123O_owpacketreadturbo     0x80
#define DS9123O_owpacketwriteturbo     0x81
#define DS9123O_owread8bytesturbo     0x82
#define DS9123O_owtx1rx2     0x83
#define DS9123O_owrx2     0x84
#define DS9123O_owtx1rx2turbo     0x85
#define DS9123O_owrx2turbo     0x86

/* Calls from visual basic program:
DS9123OwithOther.frm:4730:            WriteToPIC doowtx8turbo, Command, doportblongpulselow, PinMask, 4, 1, nullcmd, nullcmd
DS9123OwithOther.frm:4732:            WriteToPIC doowtx8, Command, doportblongpulselow, PinMask, 4, 1, nullcmd, nullcmd
DS9123OwithOther.frm:4750:            WriteToPIC doowtx8turbo, data, nullcmd, nullcmd, _
DS9123OwithOther.frm:4753:            WriteToPIC doowtx8, data, nullcmd, nullcmd, _
DS9123OwithOther.frm:4783:'            OutputReportData(0) = doowpacketwriteturbo
DS9123OwithOther.frm:4785:'            OutputReportData(0) = doowpacketwrite
DS9123OwithOther.frm:4798:            WriteToPIC doowpacketwriteturbo, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4801:            WriteToPIC doowpacketwrite, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4841:'            OutputReportData(0) = doowpacketwriteturbo
DS9123OwithOther.frm:4843:'            OutputReportData(0) = doowpacketwrite
DS9123OwithOther.frm:4857:            WriteToPIC doowpacketwriteturbo, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4860:            WriteToPIC doowpacketwrite, BytesToWrite, DataToPic(0), DataToPic(1), _
DS9123OwithOther.frm:4886:            WriteToPIC doowtx1turbo, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:4889:            WriteToPIC doowwritebit, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:4908:            WriteToPIC doowdqhi, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:4911:            WriteToPIC doowdqlow, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:4965:                    OutputReportData(I) = doowread8bytesturbo
DS9123OwithOther.frm:4967:                    OutputReportData(I) = doowread8bytes
DS9123OwithOther.frm:4988:                WriteToPIC doowpacketread, CByte(bytestoread), nullcmd, nullcmd, _
DS9123OwithOther.frm:4991:                WriteToPIC doowpacketreadturbo, CByte(bytestoread), nullcmd, nullcmd, _
DS9123OwithOther.frm:5016:            WriteToPIC doowrx1turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5019:            WriteToPIC doowreadbit, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5041:            WriteToPIC doowrx2turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5044:            WriteToPIC doowrx2, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5074:            WriteToPIC doowtx1rx2turbo, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:5077:            WriteToPIC doowtx1rx2, Bit, nullcmd, nullcmd, _
DS9123OwithOther.frm:5103:            WriteToPIC doowrx8turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5106:            WriteToPIC doowrx8, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5126:            WriteToPIC doowresetturbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5129:            WriteToPIC doowreset, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5154:            WriteToPIC doowresetturbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5157:            WriteToPIC doowreset, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5174:            WriteToPIC doowresetturbo, doowtx8turbo, &HCC, nullcmd, _
DS9123OwithOther.frm:5177:            WriteToPIC doowresetskip, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:5202:            WriteToPIC doowresetturbo, doowtx8turbo, Command, nullcmd, _
DS9123OwithOther.frm:5205:            WriteToPIC doowreset, doowtx8, Command, nullcmd, _
DS9123OwithOther.frm:5230:            WriteToPIC doowresetturbo, doowtx8turbo, &HCC, doowtx8turbo, _
DS9123OwithOther.frm:5233:            WriteToPIC doowresetskip, doowtx8, Command, nullcmd, _
DS9123OwithOther.frm:7087:            WriteToPIC doowrx8turbo, nullcmd, nullcmd, nullcmd, _
DS9123OwithOther.frm:7090:            WriteToPIC doowrx8, nullcmd, nullcmd, nullcmd, _
*/


/* _detect is a bit of a misnomer, no detection is actually done */
/* Note, devfd alread allocated */
/* Note, terminal settings already saved */
ZERO_OR_ERROR DS9123O_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;

	return -ENOTSUP;
}
