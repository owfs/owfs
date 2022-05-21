
Some instructions how to install and use OWFS on Windows (without Cygwin).

(This file is NOT complete yet...)

Download and install libusb-win32-filter-bin-0.1.10.1.exe

# testlibusb-win.exe
This will check if libusb-win32 works.


Download the OWFS Windows binary package.
# unpack owfs-2.5p4_windows.tar.gz
# C:
# cd \owfs
total 0
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:27 bin
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:55 drivers
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:28 include
drwxr-xr-x+ 2 Mag Ingen 0 Oct 19 11:28 lib
drwxr-xr-x+ 6 Mag Ingen 0 Oct 15 09:54 man


Plugin the DS9490 USB-adapter.
Manually install the driver by choosing C:\owfs\drivers\ds9490.inf
You will be asked to point out the location of the files:
    C:\windows\system32\drivers\libusb0.sys
    C:\windows\system32\libusb0.dll


You will now see your "DS9490 1-Wire USB Adapter" in your device list.

# owserver.exe -u -p 3000
# owhttpd.exe -s 3000 -p 3001
# owdir.exe -s 3000 /
bus.0
settings
system
statistics
/10.94C7ED000800
/81.465723000000
/37.11A401000000

Visit the web-page http://127.0.0.1:3001/  to see the 1-Wire devices in your web-browser.

