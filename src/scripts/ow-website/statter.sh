#!/bin/sh
# Create a StatCVS report
cd ~/owfs
cvs update -R
cvs log > ~/owfs/log
cd ..
java -jar statcvs.jar -output-dir /home/groups/o/ow/owfs/htdocs/statcvs/ -notes /home/groups/o/ow/owfs/htdocs/top.ssi ~/owfs.log owfs

