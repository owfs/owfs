Notes on setting up a system to allow the owfs module to be used by a
user other than root, i.e. a regular user.

There are two ways to allow a regular user to access the 1-wire bus:
running owserver and allowing direct file access. This document
describes setting up direct file access by changing the file
permissions on the USB file associated with the 1-wire controller.

Note that this process was created on a Debian Linux system running
the 2.6.9 Linux kernel. The particular details for your system may be
different.



1) Create a new group called owfs.

2) Add users who should have access to the 1-wire bus to the owfs
   group. Note that the users who are currently logged in when they're
   added to the owfs group will will have to logout before they'll see
   the new group. Use the groups command to verify that the user is in
   the owfs group.

3) Copy the owfs_access and owfs.usermap files to the
   /etc/hotplug/usb/ directory. Verify that the owfs_access file is
   executable.

4) Plug in the 1-wire controller.

5) Any users in the owfs group should now have direct access to the
   1-wire controller on the USB bus.
