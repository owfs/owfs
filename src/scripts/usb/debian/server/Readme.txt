Notes on setting up a system to start owserver when a ds9490r USB
1-wire controller is inserted.

Note that this process was created on a Debian Linux system running
the 2.6.9 Linux kernel. The particular details for your system may be
different.


1) Copy the owfs_server and owfs.usermap files to the
   /etc/hotplug/usb/ directory. Verify that the owfs_server file is
   executable.

2) Plug in the 1-wire controller.

3) The owserver process should now be running on port 3003 with the
   PID of the process written out to /var/run/owserver.pid.
