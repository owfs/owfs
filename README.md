[![build](https://github.com/owfs/owfs/workflows/build/badge.svg)](https://github.com/owfs/owfs/actions?query=workflow%3Abuild)

# This is OWFS — the one-wire filesystem.

1-Wire is a data protocol stat allows simple connections to
clever chips. The chips are uniquely addressed, and have a
variety of types, including temperature and voltage sensors,
switches, memory and clocks.

The base functionality is in the owlib library. It includes adapter
interface, chip interface, caching, statistics, numeration and
command line processing.

## owfs
owfs is the filesystem portion of the package. It depends on fuse:
basically, fuse (<http://fuse.sourceforge.net>) exposes filesystem calls
in the appropriate directory to this program. This program then
calls owlib to query and modify the 1-wire bus.

Despite the project name, the _owfs_ package itself is **NOT** recommended
for any real use, it has well known issues with races etc.

## owserver
owserver is a generic backend. It can be remote, and shared by several
front ends.

This is the **recommended** way of accessing your 1-Wire bus.

## owhttpd
owhttpd is a simple webserver exposing owlib. It does not need a kernel
module and will probably run on a greater platform variety.

## Language bindings
owtcl, owphp, owperl, owpython are language bindings using the same
backend and naming scheme as owfs


# Contribution
This is an old, but stable and well used, project with few people working actively on it.
That said, there are a few people contributing and trying to maintain it.

If you have any bugfixes, new features or change requests, your contribution is welcome!

## Hosting
From mid April 2018 the source is available at <https://github.com/owfs/owfs>.
Any interaction with developers should preferably take place via PRs and Issues here,
alternatively for longer discussions the mailing list is still a good medium:

<https://sourceforge.net/p/owfs/mailman/owfs-developers/>

The old SourceForge GIT mirror and releases are no longer to be used.

The <https://owfs.org> page is still not updated to reflect the project move. This is work in progress.

# Building
If you checkout out the source
```
./bootstrap
./configure
make
make install
```

If you downloaded the source package:
```
./configure
make
make install
```

---

For more information: <http://www.owfs.org>
and <https://github.com/owfs/owfs-doc/wiki>
