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

char * return_code_strings[] = {
	"Good result", // 0
	"Startup - command line parameters invalid",
	"legacy - No such entity",
	"Startup - a device could not be opened",
	"legacy - Interrupted",
	"legacy - IO error", // 5
	"Startup - network could not be opened",
	"Startup - Avahi library could not be loaded",
	"Startup - Bonjour library could not be loaded",
	"legacy - Bad filesystem",
	"Startup - zeroconf communication problem", // 10
	"legacy - Temporary interruption",
	"legacy - memory exhausted",
	"legacy - Access error",
	"legacy - Communication fault",
	"Startup - memory could not be allocated", // 15
	"legacy - Busy",
	"Program - not initialized",
	"Program - not yet ready",
	"legacy - No such device",
	"legacy - Not a directory", // 20
	"legacy - Is a directory",
	"legacy - Invalid transaction",
	"Program - closing",
	"Program - closed",
	"Program - cannot close", // 25
	"Path - input path too long",
	"Path - bad path syntax",
	"Device - Device name bad CRC8",
	"Device - Unrecognized device name or alias",
	"legacy - Read-only file system", // 30
	"Device - alias name too long",
	"Device - unrecognized device property",
	"Device - device property not an array",
	"legacy - Out of range",
	"Device - device property should be an array", // 35
	"legacy - Name too long",
	"Device - device not a bit field",
	"Device - array index out of range",
	"Device - device property not a subpath",
	"legacy - Loop discovered", // 40
	"Device - device not found",
	"legacy - No message",
	"Device - other device error",
	"Bus - bus short",
	"Bus - no such bus", // 45
	"Bus - bus not appropriate",
	"Bus - bus not responding",
	"Bus - bus being reset",
	"Bus - bus closed",
	"Bus - bus could not be opened", // 50
	"Bus - communication error on bus",
	"Bus - communication timeout",
	"Bus - telnet error",
	"Bus - tcp error",
	"Bus - bus is local", // 55
	"Bus - bus is remote",
	"Read - data too large",
	"Read - data communication error",
	"Read - not a property",
	"Read - not a readable property", // 60
	"Write - data too large",
	"Write - data too small",
	"Write - data wrong format",
	"Write - not a property",
	"Write - not a writable property", // 65
	"Write - read-only mode",
	"Write - data communication error",
	"Directory - output path too long",
	"Directory - not a directory",
	"Presence - not a device", // 70
	"Unknown query type",
	"Owserver protocol - socket problem",
	"Owserver protocol - timeout",
	"legacy - Bad message",
	"Owserver protocol - version mismatch", // 75
	"Owserver protocol - packet size error",
	"Unassigned error 77",
	"Unassigned error 78",
	"Unassigned error 79",
	"Unassigned error 80", // 80
	"Unassigned error 81",
	"Unassigned error 82",
	"Unassigned error 83",
	"Unassigned error 84",
	"Unassigned error 85", // 85
	"Unassigned error 86",
	"Unassigned error 87",
	"Unassigned error 88",
	"Unassigned error 89",
	"legacy - Message size problem", // 90
	"Unassigned error 91",
	"Unassigned error 92",
	"Unassigned error 93",
	"Unassigned error 94",
	"legacy - Not supported", // 95
	"Unassigned error 96",
	"Unassigned error 97",
	"legacy - Address in use",
	"legacy - Address not available",
	"Unassigned error 100", // 100
	"Unassigned error 101",
	"Unassigned error 102",
	"legacy - Connection aborted",
	"Unassigned error 104",
	"legacy - No buffers", // 105
} ;

