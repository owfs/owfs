#!/bin/sh

# routine to make skeleton device directories to show structure
# $ID$
# Paul Alfille 2007

mountpoint=~/1wire
treedir=~/tree
for f0 in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do
  for f1 in 0 1 2 3 4 5 6 7 8 9 A B C D E F; do
    family=$f0$f1
    owfs -m $mountpoint --tester=$family
    dev=`ls $mountpoint | grep $family.`
    pushd $mountpoint > /dev/null
    tree $dev > $treedir/$family
    popd > /dev/null
    cat $treedir/$family
    killall owfs
  done
done

