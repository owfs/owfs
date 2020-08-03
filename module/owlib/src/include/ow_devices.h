/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifndef OW_DEVICES_H
#define OW_DEVICES_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
//  specific read/write code for each device

#include "ow_standard.h"			// Needed for all devices -- should be before other devices
#include "ow_none.h"			// Non-existent devices
#include "ow_1820.h"			// thermometer
#include "ow_1821.h"			// thermostat
#include "ow_1921.h"			// thermometer
#include "ow_1923.h"			// thermometer / humidity
#include "ow_1963.h"			// ibutton SHA memory
#include "ow_1977.h"			// password memory
#include "ow_1991.h"			// ibutton Multikey memory
#include "ow_1993.h"			// ibutton memory
#include "ow_2401.h"			// Simple ID and base defines
#include "ow_2404.h"			// dual port  mem and clock
#include "ow_2405.h"			// switch
#include "ow_2406.h"			// switch
#include "ow_2408.h"			// switch
#include "ow_2409.h"			// hub
#include "ow_2413.h"			// switch
#include "ow_2415.h"			// Clock
#include "ow_2423.h"			// Counter
#include "ow_2430.h"			// eeprom
#include "ow_2433.h"			// eeprom
#include "ow_2436.h"			// Battery
#include "ow_2438.h"			// Battery
#include "ow_2450.h"			// A/D convertor
#include "ow_2502.h"			// write-only memory
#include "ow_2505.h"			// write-only memory
#include "ow_2760.h"			// battery
#include "ow_2804.h"			// switch
#include "ow_2890.h"			// potentiometer
#include "ow_bae.h"			    // Pascal Baerten's PIC-based device
#include "ow_cmciel.h"          // CMCIEL like infraread temperature
#include "ow_eds.h"             // Embedded Data Systems
#include "ow_eeef.h"			// Hobby Boards UVI etc
#include "ow_example_slave.h"   // Example of slave programming
#include "ow_iblss.h"			// iButtonLink SmartSlave
#include "ow_infernoembedded.h" // Inferno Embedded meta driver
#include "ow_interface.h"		// interface pseudo-device
#include "ow_lcd.h"				// LCD driver
#include "ow_simultaneous.h"	// fake entry to address entire directory
#include "ow_stats.h"			// statistic reporting pseudo-device
#include "ow_seahu_CD.h"		// Seahu_CD - open source arduino 1-Wire multi sensor/actor slave library
#include "ow_settings.h"		// settings pseudo-device
#include "ow_system.h"			// system pseudo-device

#endif
