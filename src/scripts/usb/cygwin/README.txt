
Some instructions how to install and use OWFS on Windows with Cygwin.


Install Cygwin
Install cygwin package libusb-win32-0.1.10.1-3)
	Easiest way is to use cygwin's setup.exe to download and install it.
	Source could be found at: http://brl.thefreecat.org/libusb-win32/
	  if you want to compile it by yourself. (haven't tried it myself)

# /usr/sbin/libusb-install.exe
This will install the Windows service called "Libusb-win32 daemon"

# testlibusb
This will check if libusb-win32 works.


Download the OWFS Windows binary package.
# tar -C/ -xvzf owfs-2.5p4_cygwin.tar.gz
# ls -l /opt/owfs/
total 0
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:27 bin
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:55 drivers
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:28 include
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:28 lib
drwxr-xr-x+ 6 Mag Ingen 0 Oct 15 09:54 man


Plugin the DS9490 USB-adapter.
Manually install the driver by choosing C:\cygwin\opt\owfs\drivers\ds9490.inf
You will be asked to point out the location of the files:
    C:\cygwin\lib\libusb\libusb0.sys
    C:\cygwin\usr\bin\cygusb0.dll

Note: The reason to the different dll-prefix could be found here:
      http://comments.gmane.org/gmane.os.cygwin.applications/13209


You will now see your "DS9490 1-Wire USB Adapter" in your device list.

# /opt/owfs/bin/owserver.exe -u -p 3000
# /opt/owfs/bin/owhttpd.exe -s 3000 -p 3001
# /opt/owfs/bin/owdir.exe -s 3000 /
bus.0
settings
system
statistics
/10.94C7ED000800
/81.465723000000
/37.11A401000000

Visit the web-page http://127.0.0.1:3001/  to see the 1-wire devices in your web-browser.

