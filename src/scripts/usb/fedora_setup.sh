#!/bin/sh
#
# $Id$
# OWFS setup routines for SUSE systems
# Written by Paul Alfille and others.
# udev routines by Peter Kropf
# GPL v2 license (like all of OWFS)
# copyrite 12/2006 Paul H Alfille
#
### ------------------
### -- Constants -----
### ------------------
OWFS_GROUP=ow
#
#
###  -----------------
###  -- Group --------
###  -----------------
groupadd $OWFS_GROUP
#
###  -----------------
###  -- Links --------
###  -----------------
# Put all the ninaries in /usr/bin
#  make them part of the "ow" group
#  and let only their owner and group read or execute them
OWFS_bin="owfs owhttpd owftpd owserver owread owwrite owpresent owdir"
for x in $OWFS_bin
  do
  ln -sfv /opt/owfs/bin/$x /usr/bin/$x
done
#
###  -----------------
###  -- Rules --------
###  -----------------
cat >/etc/udev/rules.d/46_ds2490.rules << RULES
BUS=="usb", ACTION=="add", SYSFS{idVendor}=="04fa", SYSFS{idProduct}=="2490", \
        PROGRAM="/bin/sh -c 'K=%k; K=\$\${K#usbdev}; printf bus/usb/%%03i/%%03i \$\${K%%%%.*} \$\${K#*.}'", \
        NAME="%c", MODE="0664", RUN+="/etc/udev/ds2490 '%c'"
RULES
#
###  -----------------
###  -- Shell --------
###  -----------------
cat >/etc/udev/ds2490 << SHELL
#! /bin/sh -x
    /sbin/rmmod ds9490r
    MATCH="no"
    if [ "\$1" != "" ]; then
        if [ -f /proc/\$1 ]; then
            chgrp $OWFS_GROUP /proc/\$1 && \
            chmod g+rw /proc/\$1 && \
            logger ow udev: group set to $OWFS_GROUP and permission g+rw on /proc/\$1
            MATCH="yes"
        fi

        if [ -e /dev/\$1 ]; then
            chgrp $OWFS_GROUP /dev/\$1 && \
            chmod g+rw /dev/\$1 && \
            logger ow udev: group set to $OWFS_GROUP and permission g+rw on /dev/\$1
            MATCH="yes"
        fi
    fi

    if [ "\$MATCH" = "no" ]; then
        echo ow udev: no device file found for "\$1"
        logger ow udev: no device file found for "\$1"
    fi
SHELL
chmod 755 /etc/udev/ds2490
