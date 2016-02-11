/*
     OW -- One-Wire filesystem

	  USB probe helper program

	  Probes USB bus for potential 1-wire adapters and helps
	  user to configure them

	  Written 2016 by Johan Ström

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/errno.h>

#include "config.h"
#include "owfs_config.h"
#include "../../../owlib/src/include/libusb.h"

#define error_return(code, msg, args...) do { \
	fprintf(stderr, msg"\n", ##args); \
	return (code);\
}while(0)

#define libusb_error_return(code, usb_ret, msg, args...) do { \
	fprintf(stderr, msg": %s\n", ##args, libusb_strerror(usb_ret)); \
	return (code);\
}while(0)

#define log(msg, args...) fprintf(stdout, msg"\n", ##args)

#if 1 || OW_USB

#define DS2490_USB_VENDOR  0x04FA
#define DS2490_USB_PRODUCT 0x2490

#if defined(__FreeBSD__)
#define TTY_EXAMPLE "/dev/cuaUx"
#define ACM_EXAMPLE "/dev/cuaUx" // TODO: Verify
#elif defined(__APPLE__)
#define TTY_EXAMPLE "/dev/cu.xxx"
#define ACM_EXAMPLE "/dev/cu.xxx"
#else
#define TTY_EXAMPLE "/dev/ttyUSBx"
#define ACM_EXAMPLE "/dev/ttyACMx"
#endif


#include <libusb.h>


// Struct which we gather and store device info into.
// This can house either USB or TTY names. It's not very memory efficient but
// in our very specific case we don't care.
typedef struct dev_info_s {

	int idVendor, idProduct;
	int devAddr, busNo;
	unsigned char manufacturer[255], description[255];

	union {
		unsigned char serial[255];
		unsigned char tty_name[255];
	};

	struct dev_info_s *next;
} dev_info;

static dev_info* dev_info_create(dev_info **head, dev_info **tail) {
	dev_info *di;
	// Create new result node
	if(!(di = malloc(sizeof(dev_info))))
		error_return(NULL, "malloc() failed");

	memset(di, 0, sizeof(dev_info));

	if(!(*tail)) {
		*head = di;
		*tail = di;
	} else {
		(*tail)->next = di;
		*tail = di;
	}

	return di;
}

static void dev_info_free_all(dev_info **head) {
	dev_info *next = *head;
	while(next) {
		dev_info *curr = next;
		next = curr->next;
		free(curr);
	}

	*head = NULL;
}

// Check if two device nodes are describing the same device
static int dev_match(const dev_info *list_head, const dev_info *dev) {
	const dev_info *curr = list_head;
	while(curr) {
		// For tty devices, idVendor & idProduct are 0. serial is also union with tty_name
		if(curr->idVendor == dev->idVendor &&
			curr->idProduct == dev->idProduct && 
			!strcmp((const char*)curr->serial, (const char*)dev->serial)) {
			return 1;
		}

		curr = curr->next;
	}
	return 0;
}

static int list_usb_devices_iter(libusb_device **devs, dev_info **out_list_head) {
	int i=0;
	int r;
	libusb_device *dev;
	dev_info *tail=NULL, *curr;

	*out_list_head = NULL;

	while((dev = devs[i++]) != NULL) {
		libusb_device_handle *dev_handle;
		struct libusb_device_descriptor desc;
		
		if (libusb_get_device_descriptor(dev, &desc) < 0)
			error_return(-1, "libusb_get_device_descriptor() failed");

		if(libusb_open(dev, &dev_handle) < 0)
			error_return(-1, "libusb_open() failed");

		// Create new result node
		if(!(curr = dev_info_create(out_list_head, &tail)))
			return -1;

		curr->idVendor = desc.idVendor;
		curr->idProduct = desc.idProduct;

		r = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, curr->manufacturer, sizeof(curr->manufacturer));
		if(r < 0) {
			snprintf((char*)curr->manufacturer, sizeof(curr->manufacturer), "(failed to read: %s)", libusb_strerror(r));
		}

		r = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, curr->description, sizeof(curr->description));
		if(r < 0) {
			snprintf((char*)curr->description, sizeof(curr->description), "(failed to read: %s)", libusb_strerror(r));
		}

		r = libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber, curr->serial, sizeof(curr->serial));
		if(r < 0) {
			snprintf((char*)curr->serial, sizeof(curr->serial), "(failed to read: %s)", libusb_strerror(r));
		}

		curr->devAddr = libusb_get_device_address(dev);
		curr->busNo = libusb_get_bus_number(dev);

		libusb_close(dev_handle);
	}

	return i;
}

static int list_usb_devices(dev_info **out_list_head) {
	int r;
	libusb_device **devs;
	libusb_context *usb_ctx = NULL;
	r = libusb_init(&usb_ctx) != 0;
	if(r != 0)
		error_return(1, "libusb_init() failed: %d", r);

	log(" Scanning USB devices...");
	r = libusb_get_device_list(usb_ctx, &devs);
	if(r < 0) {
		libusb_exit(usb_ctx);
		error_return(-1, "libusb_get_device_list() failed: %d", r);
	}

	log(" Found %d devices", r);

	r = list_usb_devices_iter(devs, out_list_head);

	libusb_free_device_list(devs, 1);
	libusb_exit(usb_ctx);
	return r;
}

/* Iterate all files in /dev/ and match OS-specific TTY names. */
static int list_tty_devices(dev_info **out_list_head) {
	DIR* d;
	struct dirent* e;
	int i=0;
	dev_info *tail=NULL, *curr;

	if(!(d = opendir("/dev")))
		error_return(-1, "opendir(/dev) failed: %d", errno);

	while((e = readdir(d)) != NULL) {
#if defined(__FreeBSD__)
		if(!strncmp(e->d_name, "cua", 3))
#elif defined(__APPLE__)
        if(!strncmp(e->d_name, "cu.", 3))
#else
		if(!strncmp(e->d_name, "ttyUSB", 6) || strncmp(e->d_name, "ttyACM", 6))
#endif
		{
			if(!(curr = dev_info_create(out_list_head, &tail)))
				return -1;

			snprintf((char*)curr->tty_name, sizeof(curr->tty_name), "/dev/%s", e->d_name);
		}
	}

	closedir(d);

	return i;
}

static char *gather_ttys(dev_info *baseline, dev_info *updated, int cnt_baseline, int cnt_updated) {
	int sz = (cnt_baseline > cnt_updated ? cnt_baseline : cnt_updated) * 255;
	int of=0;
	const dev_info *curr = updated;
	char *result = malloc(sz);
	memset(result, 0, sz);

	while(curr) {
		if(!dev_match(baseline, curr)){
			of+= snprintf(result+of, sz-of-1, "%s%s", (of > 0 ? ", ":""), curr->tty_name);
		}

		curr = curr->next;
	}

	return result;
}



static void describe_usb_device(const dev_info *dev, const char *ttys_found) {
	int vid = dev->idVendor, pid = dev->idProduct;

	log("> Vendor ID    : 0x%04x               Product ID   : 0x%04x", vid, pid);
	log("  Manufacturer : %-20s Description  : %-30s", dev->manufacturer, dev->description);
	log("  Serial       : %-20s Bus: %d, Addr: %d\n", dev->serial, dev->busNo, dev->devAddr);

	// Compare vid/pid with known device list
	if(vid == 0x0403 &&
			(pid == 0x6001 || pid == 0x6010 || pid == 0x6011 ||
			 pid == 0x6014 || pid == 0x6015)) {
		char addr[255];
		snprintf(addr, sizeof(addr), "ftd:s:0x%04x:0x%04x:%s", vid, pid, dev->serial);
		log("This device is identified as a generic FTDI adapter. To address it, use the following adressing:\n");
		log("  %s\n", addr);

		if(pid == 0x6001) {
			log("This MAY be a LinkUSB device. If so, you may put the following in your owfs.conf:\n");
			log("  link = %s\n", addr);

			log("Or, if running from command line:\n");
			log("  --link %s\n", addr);

			printf("If NOT a LinkUSB, you ");
		} else {
			printf("You ");
		}
		log("have to check the documentation on how to target your particular device type.");
		log("For any serial type device, you should be able to use the above addressing.");

	}else if(vid == DS2490_USB_VENDOR && pid == DS2490_USB_PRODUCT) {
		log("This device is identified as a DS2490 / DS9490 USB adapter.\n");

		// XXX: This is pretty undeterministic between reboots etc...
		log("You may put the following in your owfs.conf:\n");
		log("  usb = %d:%d\n", dev->busNo, dev->devAddr);

		log("Or, if running from command line:\n");
		log("  --usb %d:%d", dev->busNo, dev->devAddr);
	}else if(vid == 0x067B && pid == 0x2303) {
		log("This device is identified as a generic Prolific USB adapter.\n"
				"It MAY be a DS9481 adapter. If it is, you need to use the DS2480B device, \n"
				"and point it to the appropriate %s device.\n", TTY_EXAMPLE);
		if(ttys_found) {
			log("The following devices where found: %s", ttys_found);
		}

		log("You may put the following in your owfs.conf:\n");
		log("  device = %s\n", TTY_EXAMPLE);

		log("Or, if running from command line:\n");
		log("  --device %s", TTY_EXAMPLE);

	}else if(vid == 0x0B6A && pid == 0x5A03) {
		log("This device is identified as a DS18E17 / DS9481P-300 USB adapter.");
		log("To use this, you need to use the DS2480B device, and point it to\n"
				"the appropriate %s device\n", ACM_EXAMPLE);
		if(ttys_found) {
			log("The following devices where found: %s", ttys_found);
		}

		log("You may put the following in your owfs.conf:\n");
		log("  device = %s\n", ACM_EXAMPLE);

		log("Or, if running from command line:\n");
		log("  --device %s", ACM_EXAMPLE);
	}else{
		log("This device is not a known compatible device.");
	}
	log("------------------------------------------------------------------");
}

static int usage(char *progname) {
	log("OWFS USB probe utility");
	log("usage: %s [-a|--all]\n", progname);
	log("By default, this will run an interactive guide to identify your USB based 1-wire adapters.");
	log("If you run with -a/--all, we will print all found devices and exit");
	return 1;
}

#define DEV_INFO_CLEANUP() do {\
		dev_info_free_all(&usb_baseline);\
		dev_info_free_all(&usb_updated);\
		dev_info_free_all(&tty_baseline);\
		dev_info_free_all(&tty_updated);\
		if(tty_result){ free(tty_result); tty_result = NULL;} \
	}while(0)

int main(int argc, char *argv[])
{
	int cnt_usb_baseline = 0, cnt_usb_updated = 0;
	int cnt_tty_baseline = 0, cnt_tty_updated = 0;
	int i;
	int interactive = 1;
	char *tty_result = NULL;

	dev_info *usb_baseline = NULL, *usb_updated = NULL;
	dev_info *tty_baseline = NULL, *tty_updated = NULL;

	if(argc > 2) {
		usage(argv[0]);
		return 1;
	}else if(argc == 2) {
		if(!strcmp(argv[1], "-a") || !strcmp(argv[1], "--all"))
			interactive = 0;
		else
			usage(argv[0]);
	}

	if(geteuid() != 0) {
		log("NOTE: You are running as a non-root user. Unless permissions are already configured\n"
			 "you will probably not find any devices.\n");
	}


	if(interactive) {
		log("Please disconnect any USB devices you which to identify.\n"
			 "A bus scan will then be made to create a base-line of ignored devices.\n\n"
			 "Press Enter when you are ready to proceed");

		getc(stdin);

		if((cnt_usb_baseline = list_usb_devices(&usb_baseline)) < 0) {
			DEV_INFO_CLEANUP();
			return 1;
		}

		if((cnt_tty_baseline = list_tty_devices(&tty_baseline)) < 0) {
			DEV_INFO_CLEANUP();
			return 1;
		}

		log("Please reconnect one of the device you which to identify.\n\n"
			 "When you are ready to proceed, wait a few seconds.\n"
			 "Then press Enter");

		getc(stdin);
	}

	for(i=0; i < 5; i++){
		if((cnt_usb_updated = list_usb_devices(&usb_updated)) < 0) {
			DEV_INFO_CLEANUP();
			return 1;
		}

		if(interactive && cnt_usb_updated <= cnt_usb_baseline) {
			// No change; sleep a bit and try agian
			log("No new devices detect.. Re-scanning in 2s...");
			sleep(2);
		}else
			break;
	}

	if(interactive) {
		if((cnt_tty_updated = list_tty_devices(&tty_updated)) < 0) {
			DEV_INFO_CLEANUP();
			return 1;
		}

		// Find any new in tty_updated
		tty_result = gather_ttys(tty_baseline, tty_updated, cnt_tty_baseline, cnt_tty_updated);
	}

	if(interactive && cnt_usb_baseline == cnt_usb_updated) {
		log("Sorry, no new USB devices was found.");
		if(geteuid() != 0)
			log("This is likely due to running as non-root. ");

		DEV_INFO_CLEANUP();
		return 0;
	}

	if(usb_updated) {
		const dev_info *curr = usb_updated;
		// Iterate all newly found and compare with baseline
		while(curr) {
			if(!interactive || !dev_match(usb_baseline, curr))
				describe_usb_device(curr, tty_result);

			curr = curr->next;
		}

		if(geteuid() == 0) {
			log("\nNOTE: You are running as root. You may have to configure your OS to allow\n"
				 "your user to access the device. Please see owfs documentation for details.");
		}
	}

	DEV_INFO_CLEANUP();
	return 0;
}


#else
int main(int arc, char *argv[])
{
	error_return(1, "Sorry, OWFS was not built with USB support."
			"Please rebuild with USB support to use this tool.");
}
#endif
