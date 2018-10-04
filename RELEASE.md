How to create a release
----

* Ensure all wanted PRs are merged and CI status for `master` is green.
* Ensure you have pulled `origin/master` locally.
* Create release commit:
  * Edit & stage `configure.ac` and bump `VERSION_MAJOR/MINOR/PATCHLEVEL` as appropriate.
  * Create a git commit with text such as `Release v3.2p3`
  *  Tag commit with `vMAJOR.MINORpPATHCLEVEL` such as `git tag -v v3.2p3`
* Build a release from a clean checkout:
```
mkdir -p tmp/ && cd tmp/ && \
    git clone ${PATH_TO_YOUR_GIT_REPO}/.git owfs && \
    cd owfs && \
    git show -q && \
    ./bootstrap && \
    ./configure && \
    make clean && \
    make dist && \
    make distclean && \
    cp -v owfs-v3.2p3.tar.gz ../
```
* Now verify that the owfs-v3.2p3.tar.gz file seems sane, perhaps diff it against the previous release.
*  Push your git commit, including tag, to `origin`:
```
    git push && git push --tags
```
* Go to https://github.com/owfs/owfs/releases and click `Draft a new release`
* Select your new tag, write some release notes (look at previous ones)
* Attach the .tar.gz file created above
* Click *Publish Release*

Done!

Alternative build via docker
--
If you are not using a Linux machine with recent autoconf / automake etc, you can use the following commands
instead of the ones above, to build in a temporary docker container.

This should create very repeatable releases, and can be executed on both Linux and Mac (with Docker installed):

```
  docker build -f Dockerfile.release  -t owfs-release-builder . && \
  docker run --rm -it -v $(pwd):/extsrc:ro -v $(pwd)/tmp:/out owfs-release-builder
```

The resulting file will be placed in the `tmp` dir on the host.
Note that you still have to create the release commit & push everything manually.