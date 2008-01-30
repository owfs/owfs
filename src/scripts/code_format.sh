#!/bin/bash

#program to "pretty-up" OWFS source code
#needs a special fix for raw family codes that are altered
# i.e. DeviceHeader(1D -> DeviceHeader(1 D
#which screws up evertthing

find . -name "*.[c|h]" | while read file; do

echo "Indent file=$file"
grep DeviceEntry $file
/usr/bin/indent -kr -l150 -ts4 $file
/usr/bin/perl -i -ple 's/\(\s*([0-9]) ([A-F])/\(\1\2/ if /DeviceEntry/' $file
grep DeviceEntry $file

done

exit 0

grep DeviceEntry *.[ch]
/usr/bin/indent -kr -ts4 *.[ch]
/usr/bin/perl -i -ple 's/\(\s*([0-9]) ([A-F])/\(\1\2/ if /DeviceEntry/' *.[ch]
grep DeviceEntry *.[ch]
