#!/usr/bin/python

# OWFS test program
# See owfs.sourceforge.net for details
# {copyright} 2004 Paul H. Alfille
# GPL license

import OW
import sys

def usage():
    print "OWFS 1-wire tree perl program"
    print "  by Paul Alfille 2004 see http://owfs.sourceforge.net"
    print "Syntax:"
    print "\t$0 1wire-port"
    print "  1wire-port (required):"
    print "\t'u' for USB -or-"
    print "\t/dev/ttyS0 (or whatever) serial port"

if len(sys.argv) < 2:
    usage()
    sys.exit(10)
else:
    if OW.init(sys.argv[1]): # 1=failure.
        print "Cannot open 1wire port %s" % sys.argv[1]
        sys.exit(10)
        
def treelevel(level, path):
    print "lev=%d path=%s" % (level, path)
    res = OW.get(path)
    if not res:
        return
    for x in res.split(","):
        print "\t" * level
        print x
        if x and x[-1] == "/":
            print "\n"
            treelevel(level+1, path+x)
        else:
            r = OW.get(path+x)
            if r:
                print ": %s" % r
            print ""


treelevel(0,"/") ;
OW.finish() ;

