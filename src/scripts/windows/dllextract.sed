# dllextract.sed - Copyright (C) 2001 Pat Thoyts <Pat.Thoyts@bigfoot.com>
#
# Build an exports list from a Windows DLL using 'objdump -p'
#
/^\[Ordinal\/Name Pointer\] Table/,/^$/{
  /^\[Ordinal\/Name Pointer\] Table/d
  /\[/s/[ 	]\+\[[ 	]*\([0123456789]\+\)\] \(.*\)/\2/p
}
