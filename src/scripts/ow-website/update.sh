#! /bin/sh
#
# $Id$
#
# This script updates the owfs web site from the contents of the
# ow-website CVS module. Its run on a regular basis from a crontab
# entry on shell.sf.net.
#
# Crontab entry
# 3 * * * *      /home/groups/o/ow/owfs/owfs/src/scripts/ow-website/update.sh >> /home/groups/o/ow/owfs/web_update.log 2>&1


date
cd /home/groups/o/ow/owfs/htdocs
cvs update
cd /home/groups/o/ow/owfs/owfs
cvs update
