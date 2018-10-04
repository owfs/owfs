# Please see RELEASE.md for usage information

FROM ubuntu:18.04

RUN apt update && apt -y install autoconf make libtool pkg-config && apt clean
RUN apt install -y git

ENV GITREF=master
VOLUME ['/extsrc']

WORKDIR /build
CMD git clone /extsrc/.git owfs && \
    cd owfs && \
    git checkout ${GITREF} && \
    git show -q && \
    ./bootstrap && \
    ./configure && \
    make clean && \
    make dist && \
    make distclean && \
    cp -v owfs-*tar.gz /out
