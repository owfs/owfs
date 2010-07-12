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

/* ow_opt -- owlib specific command line options processing */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

void ow_help_general(void)
{
	switch (Globals.opt) {
	case opt_owfs:
		printf("Syntax: %s [options] device mountpoint\n", SAFESTRING(Globals.progname));
		break;
	case opt_httpd:
	case opt_server:
		printf("Syntax: %s [options] device clientport\n", SAFESTRING(Globals.progname));
		break;
	case opt_ftpd:
	default:
		printf("Syntax: %s [options] device\n", SAFESTRING(Globals.progname));
		break;
	}
	printf("\n"
	"Help resources:\n"
	" %s --help              This page\n"
	" %s --help=device       Bus master device options\n"
	" %s --help=program      Program services (mountpoint, port)\n"
	" %s --help=cache        Cache and communication timing\n"
	" %s --help=job          Job control and debugging\n"
	" %s --help=temperature  Temperature scale and device format options\n"
	"\n"
	" man %s                 man page for this program\n"
	"  and man pages for individual 1-wire devices e.g. 'man DS2409'\n",
	SAFESTRING(Globals.progname), SAFESTRING(Globals.progname),
	SAFESTRING(Globals.progname), SAFESTRING(Globals.progname),
	SAFESTRING(Globals.progname), SAFESTRING(Globals.progname), SAFESTRING(Globals.progname)
	);
}

void ow_help_job(void)
{
	printf("Job Control and Information Help\n"
	"\n"
	" Information\n"
	"  --error_level n  Choose verbosity of error/debugging reports 0=low 9=high\n"
	"  --error_print n  Where debug info is placed 0-mixed 1-syslog 2-console\n"
	"  -V --version     Program and library versions\n"
	"\n"
	" Control\n"
	"  --foreground\n"
	"  --background\n"
	"  --pid_file name  file to store pid number (for control scripts)\n"
	"\n"
	" Configuration\n"
	"  -c --configuration filename\n"
	"                   file to use as additional source of configuration\n"
	"  -a --alias filename\n"
	"                   file containing device to friendly_name pairs\n"
	"\n"
	" Permission\n"
	" -r --readonly     Don't allow writing to 1-wire devices\n"
	" -w --write        Allow writing to 1-wire\n"
	" --safemode        Even more restrictive: readonly, no uncached, ...\n"
	"\n"
	" Development tests (owserver only)\n"
	"  --pingcrazy      Add lots of keep-alive messages to the owserver protocol\n"
	"  --no_dirall      DIRALL fails, drops back to older DIR (individual entries)\n"
	"  --no_get         GET fails, drops back to DIRALL and READ\n"
	"  --no_persistence persistent connections refused, drops back to non-persistent\n"
	);
}

void ow_help_temperature(void)
{
	printf("Temperature and Display Help\n"
	"\n"
	" 1-wire address format (for display, all will be accepted for input)\n"
	"                       f (family code) = 2 hex digits\n"
	"                       i (ID)          = 12 hex digits\n"
	"                       c (crc8)        = 2 hex digits\n"
	"  -f --format f[.]i[[.]c]\n"
	"\n"
	" Temperature scale\n"
	"  -C --Celsius        Celsius(default) temperature scale\n"
	"  -F --Fahrenheit     Fahrenheit temperature scale\n"
	"  -K --Kelvin         Kelvin temperature scale\n" "  -R --Rankine        Rankine temperature scale\n"
	"\n"
	" Pressure scale\n"
	"  --atm --mbar --inHg --mmHg --Pa --psi (mbar is the default)\n"
	);
}

void ow_help_cache(void)
{
	printf("Cache and Timing Help\n"
	"\n"
	" Cache\n"
	"  --cache_size n   Size in bytes of max cache memory. 0 for no limit.\n"
	"  --max_connections   [%3d] Nr of allowed concurrent connections\n"
	"\n"
	" Cache timing         [default] (in seconds)\n"
	"  --timeout_volatile  [%3d] Expiration time for changing data (e.g. temperature)\n"
	"  --timeout_stable    [%3d] Expiration time for stable data (e.g. temperature limit)\n"
	"  --timeout_directory [%3d] Expiration of directory lists\n"
	"  --timeout_presence  [%3d] Expiration of known 1-wire device location\n"
	" \n"
	" Communication timing [default] (in seconds)\n"
	"  --timeout_serial    [%3d] Timeout for serial port\n"
	"  --timeout_usb       [%3d] Timeout for USB transaction\n"
	"  --timeout_network   [%3d] Timeout for each network transaction\n"
	"  --timeout_server    [%3d] Timeout for first server connection\n"
	"  --timeout_ftp       [%3d] Timeout for FTP session\n"
	"  --timeout_ha7       [%3d] Timeout for HA7Net bus master\n"
	"  --timeout_w1        [%3d] Timeout for w1 kernel netlink\n"
	, Globals.concurrent_connections
	, Globals.timeout_volatile
	, Globals.timeout_stable
	, Globals.timeout_directory
	, Globals.timeout_presence
	, Globals.timeout_serial
	, Globals.timeout_usb
	, Globals.timeout_network
	, Globals.timeout_server
	, Globals.timeout_ftp
	, Globals.timeout_ha7
	, Globals.timeout_w1
		   );
}

void ow_help_program(void)
{
	printf("Program-Specific Help\n"
	"\n"
	" owfs (FUSE-based filesystem)\n"
	"  -m --mountpoint path  Where to mount virtual 1-wire file system\n"
	"  --fuse_open_opt args  Special arguments to pass to FUSE (Quoted and escaped)\n"
	"  --allow_other         Allow other users to see owfs file system\n"
	"                         needs /etc/fuse.conf setting\n"
	"\n"
	" owhttpd (web server)\n"
	"  -p --port [ip:]port   TCP address and port number for access\n"
	"  --zero                Announce service via zeroconf\n"
	"  --announce name       Name for service given in zeroconf broadcast\n"
	"  --nozero              Don't announce service via zeroconf\n"
	"\n"
	" owserver (OWFS server)\n"
	"  -p --port [ip:]port   TCP address and port number for access\n"
	"\n"
	" owftpd (ftp server)\n"
	"  -p --port [ip:]port   TCP address and port number for access\n"
	"                           has default port 22 (root only)\n"
	"  --zero                Announce service via zeroconf\n"
	"  --announce name       Name for service given in zeroconf broadcast\n"
	"  --nozero              Don't announce service via zeroconf\n" "\n"
	);
}

void ow_help_device(void)
{
	printf("Device Help\n"
	"  More than one device (1-wire bus master) can be specified. All will be used.\n"
	"\n"
	" Serial devices (dev is port name, e.g. /dev/ttyS0)\n"
	"  -d dev          DS9097U or DS9097 bus master (or LINK in emulation mode)\n"
	"  --8bit          Open 8bit (instead of 6) serial-port with DS9097\n"
	"  --baud=1200|9600|19200|38400|57600|115200    Speed of communication to bus master\n"
	"  --serial_flextime | --serial_regulartime (timing adjustments for DS2480B)\n"
	"  --straight_polarity | --reverse_polarity (for DS2480B based bus master)\n"
	"  --LINK=dev      Serial LINK bus master (non-emulation)\n"
	"  --HA3=dev       Serial HA3 bus master\n"
	"  --HA4B=dev      Serial HA4B bus master\n"
	"  --HA5=dev:ah     Serial HA5 bus master Channel a and h ( of a-z))\n"
	"  --HA7E=dev      Serial HA7E bus master\n"
	"  --HA7S=dev      Serial HA7S bus master\n"
	"  --checksum | --no_checksum for HA5\n"
	"\n"
	" i2c devices\n"
	"  -d dev          DS2482-x00 bus master (dev is /dev/i2c-0)\n"
	"  --i2c=dev       DS2482-x00 bus master (dev is /dev/i2c-0)\n"
	" --i2c=dev:0     DS2482-x00 bus master address 0 of 0-7\n"
	" --i2c=ALL:ALL   DS2482-x00 bus master ALL is wildcard\n"
	" --ActivePullUp         | --no_ActivePullUp          See datasheet on APU\n"
	" --PresencePulseMasking | --no_PresencePulseMasking  See datasheet on PPM\n"
	"\n"
	"USB\n"
	"  -u    --USB     DS9490R or PuceBaboon bus master\n"
	"  -uall --USB=all Find and use all DS9490-type bus masters\n"
	"  -u3:4 --USB=3:4 Specific USB location (bus 3, device 4)\n"
	"  -uscan --USB_scan=n Keep looking for new USB adapters (every n seconds; default 10)\n"
	"  -d /dev/ttyUSB0 ECLO USB bus master\n"
	"  --altUSB        Change some settings for DS9490 bus master (especially for AAG and DS2423)\n"
	"  --usb_flextime | --usb_regulartime     Needed for Louis Swart's LCD module\n"
	"\n"
	" Network (address is form [ip:]port, ip DNS name or n.n.n.n, port is port number)\n"
	"  -s address      owserver\n"
	"  --LINK=address  LINK-HUB-E network LINK\n"
	"  --HA7NET=address HA7NET bus master\n"
	"  --HA7NET        HA7NET bus master address auto-discovered\n"
	"  --ENET=address  OWServer-Enet bus master\n"
	"  --etherweather=address EtherWeather\n"
	"  --autoserver    Find owserver using zeroconf\n"
	"\n"
	" Simulated\n"
	"  --fake=list     List of devices to simulate (random ID, random data)\n"
	"                   use family codes in hex\n"
	"                   e.g. 1F,10,21 for DS2409,DS18S20,DS1921\n"
	"  --tester=list   List of devices to simulate (non-random ID, non-random data)\n"
	"  --temperature_low=0.0   --temperature_high=100.0 temperature range for fake readings\n"
	"\n"
	" Linux Kernel Device\n"
	"  --w1            Scan for kernel-managed bus masters\n"
	"\n" " 1-wire device selection\n" "  --one-device     Only single device on bus, use ROM SKIP command\n");
}

void FS_help(const char *arg)
{
	printf("1-WIRE access programs         by Paul H Alfille and others.\n" "\n");
	if (arg) {
		switch (arg[0]) {
		case 'd':
		case 'D':
			ow_help_device();
			break;
		case 'p':
		case 'P':
			ow_help_program();
			break;
		case 'j':
		case 'J':
			ow_help_job();
			break;
		case 'c':
		case 'C':
			ow_help_cache();
			break;
		case 't':
		case 'T':
			ow_help_temperature();
			break;
		default:
			ow_help_general();
		}
	} else {
		ow_help_general();
	}

	printf("\n" "Copyright 2003-8 GPLv2 by Paul Alfille. See http://www.owfs.org for support, downloads\n");
}
