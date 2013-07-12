/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_opt -- owlib specific command line options processing */

#include "owshell.h"

void ow_help(void)
{
	printf("\n1-wire access programs\n"
		   "By Paul H Alfille and others. See http://www.owfs.org\n"
		   "\n"
		   "Command line access to owserver process\n"
		   "\n"
		   "Syntax: owdir     [Options] -s Server DirPath\n"
		   "Syntax: owread    [Options] -s Server ReadPath\n"
		   "Syntax: owwrite   [Options] -s Server WritePath WriteVal\n"
		   "Syntax: owpresent [Options] -s Server Path\n"
		   "\n"
		   "Server is an owserver net address (port number or ipaddress:port)\n"
		   "\n"
		   "Path is in OWFS format e.g. 10.2301A3008000/temperature\n"
		   "  more than one path (or path/value pair for owwrite) can be given\n"
		   "  each is processed in turn\n"
		   "\n"
		   "Options include:\n"
		   "     --autoserver                |Use Bonjour (zeroconf) to find owserver\n"
		   "  -C --Celsius                   |Celsius(default) temperature scale\n"
		   "  -F --Fahrenheit                |Fahrenheit temperature scale\n"
		   "  -K --Kelvin                    |Kelvin temperature scale\n"
		   "  -R --Rankine                   |Rankine temperature scale\n"
		   "  -f --format                    |format for 1-wire unique serial IDs display\n"
		   "                                 |  f[.]i[[.]c] f-amily i-d c-rc (all in hex)\n"
           "     --hex                       |data in hexidecimal format\n"
           "     --size                      |size of data in bytes\n"
           "     --offset                    |start of read/write in field\n"
           "     --dir                       |add a trailing '/' for directories\n"
		   "  -V --version                   |Program version\n" 
		   "  -q --quiet                     |suppress error messages\n"
		   "  -h --help                      |Basic help page\n"
		   );
}
