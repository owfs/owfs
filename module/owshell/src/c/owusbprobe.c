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

#include "config.h"
#include "owfs_config.h"

#define error_return(code, msg, args...) do { \
	fprintf(stderr, msg"\n", ##args); \
	return (code);\
}while(0)


#define log(msg, args...) fprintf(stdout, msg"\n", ##args)

#if OW_USB

#define DS2490_USB_VENDOR  0x04FA
#define DS2490_USB_PRODUCT 0x2490


#include <libusb.h>

// Struct which we gather and store device info into
typedef struct dev_info_s {
	int idVendor, idProduct;
	int devAddr, busNo;
	unsigned char manufacturer[255], description[255], serial[255];

	struct dev_info_s *next;
} dev_info;

static void free_dev_info(dev_info ** head) {
	dev_info *next = *head;
	while(next) {
		dev_info *curr = next;
		next = curr->next;
		free(curr);
	}

	*head = NULL;
}

static int dev_found(const dev_info *list_head, const dev_info *dev) {
	const dev_info *curr = list_head;
	while(curr) {
		if(curr->idVendor == dev->idVendor &&
			curr->idProduct == dev->idProduct && 
			!strcmp((const char*)curr->serial, (const char*)dev->serial)) {
			return 1;
		}

		curr = curr->next;
	}
	return 0;
}

static int list_devices_iter(libusb_device **devs, dev_info **out_list_head) {
	int i=0;
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
		if(!(curr = malloc(sizeof(dev_info))))
			error_return(-1, "malloc() failed");

		memset(curr, 0, sizeof(dev_info));
		curr->idVendor = desc.idVendor;
		curr->idProduct = desc.idProduct;

		if(!tail) {
			*out_list_head = curr;
			tail = curr;
		} else {
			tail->next = curr;
			tail = curr;
		}

		if(libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, curr->manufacturer, sizeof(curr->manufacturer)) < 0) {
			libusb_close(dev_handle);
			error_return(-1, "libusb_get_string_descriptor_ascii() for iManufacturer failed");
		}

		if(libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, curr->description, sizeof(curr->description)) < 0) {
			libusb_close(dev_handle);
			error_return(-1, "libusb_get_string_descriptor_ascii() for iProduct failed");
		}

		if(libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber, curr->serial, sizeof(curr->serial)) < 0) {
			libusb_close(dev_handle);
			error_return(-1, "libusb_get_string_descriptor_ascii() for iSerial failed");
		}

		curr->devAddr = libusb_get_device_address(dev);
		curr->busNo = libusb_get_bus_number(dev);

		libusb_close(dev_handle);
	}

	return i;
}

static int list_devices(dev_info **out_list_head) {
	int r;
	libusb_device **devs;
	libusb_context *usb_ctx = NULL;
	r = libusb_init(&usb_ctx) != 0;
	if(r != 0)
		error_return(1, "libusb_init() failed: %d", r);

	log("Scanning USB devices...");
	r = libusb_get_device_list(usb_ctx, &devs);
	if(r < 0) {
		libusb_exit(usb_ctx);
		error_return(-1, "libusb_get_device_list() failed: %d", r);
	}

	log("Found %d devices", r);

	r = list_devices_iter(devs, out_list_head);

	libusb_free_device_list(devs, 1);
	libusb_exit(usb_ctx);
	return r;
}

static void describe_device(const dev_info *dev) {
	int vid = dev->idVendor, pid = dev->idProduct;

	log("> Vendor ID    : 0x%04x               Product ID   : 0x%04x", vid, pid);
	log("  Manufacturer : %- 20s Description  : %-30s", dev->manufacturer, dev->description);
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
		log("This device is identified as a DS2490 USB adapter.\n");

		// XXX: This is pretty undeterministic between reboots etc...
		log("You may put the following in your owfs.conf:\n");
		log("  usb = %d:%d\n", dev->devAddr, dev->busNo);

		log("Or, if running from command line:\n");
		log("  --usb %d:%d", dev->devAddr, dev->busNo);
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

int main(int argc, char *argv[])
{
	int cnt_baseline = 0, cnt_updated = 0;
	int i;
	int interactive = 1;
	dev_info *baseline = NULL, *updated = NULL;

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

		if((cnt_baseline = list_devices(&baseline)) < 0) {
			free_dev_info(&baseline);
			return 1;
		}

		log("Please reconnect one of the device you which to identify.\n\n"
			 "When you are ready to proceed, wait a few seconds.\n"
			 "Then press Enter");

		getc(stdin);
	}

	for(i=0; i < 5; i++){
		if((cnt_updated = list_devices(&updated)) < 0) {
			free_dev_info(&baseline);
			free_dev_info(&updated);
			return 1;
		}

		if(interactive && cnt_updated <= cnt_baseline) {
			// No change; sleep a bit and try agian
			log("No new devices detect.. Re-scanning in 2s...");
			sleep(2);
		}else
			break;
	}

	if(interactive && cnt_baseline == cnt_updated) {
		log("Sorry, no new USB devices was found.");
		if(geteuid() != 0)
			log("This is likely due to running as non-root. ");

		free_dev_info(&baseline);
		free_dev_info(&updated);
		return 0;
	}

	if(updated) {
		const dev_info *curr = updated;
		// Iterate all newly found and compare with baseline
		while(curr) {
			if(!interactive || !dev_found(baseline, curr))
				describe_device(curr);

			curr = curr->next;
		}

		free_dev_info(&updated);

		if(geteuid() == 0) {
			log("\nNOTE: You are running as root. You may have to configure your OS to allow\n"
				 "your user to access the device. Please see owfs documentation for details.");
		}
	}

	return 0;
}


#else
int main(int arc, char *argv[])
{
	error_return(1, "Sorry, OWFS was not built with USB support."
			"Please rebuild with USB support to use this tool.");
}
#endif
