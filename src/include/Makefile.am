owfsincludedir = ${prefix}/include
nodist_owfsinclude_HEADERS = owfs_config.h

COMMON_GCC_OPTS:= $(COMMON_FLAGS) $(CPPFLAGS) -Wno-long-long
# We want to enable 'long long' for the purpose of extracting the value of
# 'sizeof(long long)'; see the 'size-options.lnt' target below.

C_OPTS:=  $(CFLAGS) $(COMMON_GCC_OPTS)
CXX_OPTS:=$(CXXFLAGS) $(COMMON_GCC_OPTS)
# Note, we're not *yet* able to handle some of the header contents when
# -std=c++0x is given.

GCC_BIN:=gcc
GXX_BIN:=g++

GCC:=$(GCC_BIN) $(C_OPTS)
#GXX:=$(GXX_BIN) $(CXX_OPTS)
GXX:=$(GCC_BIN) $(C_OPTS)
# We don't use any C++ code right now.

TEMP_FILE_PREFIX:=co-gcc.mak.temp
E:=$(TEMP_FILE_PREFIX)-empty
SIZE_GEN:=$(TEMP_FILE_PREFIX)-generate-size-options

ECHO:=echo
TOUCH:=touch
AWK:=awk

lint_cmac.h:
	set -e ; $(TOUCH) $(E)$$$$.c ; $(GCC) -E -dM $(E)$$$$.c -o $@ ; $(RM) $(E)$$$$.c

lint_cppmac.h:
#	set -e ; $(TOUCH) $(E)$$$$.c ; $(GXX) -E -dM $(E)$$$$.c -o $@ ; $(RM) $(E)$$$$.c
	set -e ; $(TOUCH) $(E)$$$$.cpp ; $(GXX) -E -dM $(E)$$$$.cpp -o $@ ; $(RM) $(E)$$$$.cpp

gcc-include-path.lnt:
	@# Here we make options for the #include search path.
	@# Note, frameworks (a feature of Apple's GCC) are not supported
	@# yet so for now we filter them out.  Each remaining search
	@# directory 'foo' is transformed into '--i"foo"' after
	@# superfluous directory separators  are removed (as well as each
	@# CR character appearing immediately before a newline):
	$(TOUCH) $(E)$$$$.cpp ; \
	$(GXX) -v -c $(E)$$$$.cpp >$(E)$$$$.tmp 2>&1 ; \
	<$(E)$$$$.tmp $(AWK) '					\
	    BEGIN  {S=0}				\
	    /search starts here:/  {S=1;next;}		\
	    S && /Library\/Frameworks/ {next;}		\
	    S && /^ /  {				\
		sub("^ ","");				\
		gsub("//*","/");			\
		sub("\xd$$","");			\
		sub("/$$","");				\
		printf("--i\"%s\"\n", $$0);		\
		next;					\
	    }						\
	    S  {exit;}					\
	    ' >gcc-include-path.lnt ; \
	$(RM) $(E)$$$$.cpp $(E)$$$$.tmp $(E)$$$$.o
	@# Note, we deliberately use '--i' instead of '-i' here; the effect
	@# is that the directories named with the double-dash form are
	@# searched after directories named with the single-dash form.
	@# (See also the entry for '--i' in section 5.7 of the Lint
	@# manual.)
	@#
	@# We typically use '--i' when we want to name a system include
	@# directory, which GCC searches only after it searches all
	@# directories named in a '-I' option.  The upshot is that the
	@# correct search order (i.e., project includes before system
	@# includes) is preserved even when double-dash-i options are given
	@# before single-dash-i options.
	@#
	@# Also note, no harm is done if '-I' options are passed to GCC
	@# here:  directories named with '-I' will appear before the
	@# sys-include-dirs in GCC's output, so even though Lint might then
	@# see a project-include directory named with a '--i' option, that
	@# directory will still be searched before the sys-includes because
	@# of the ordering of '--i' options.  (Just make sure you don't use
	@# the double-dash form with project include dirs outside of this
	@# limited & generated sub-sequence of options because this is the
	@# only place where we are certain that project directories always
	@# come before system directories.)
	@#
	@# XXX:  We need to do something for people without an AWK
	@# implementation---hopefully not here, but perhaps we could
	@# provide an 'awk' with the Lint distro.


size-options.lnt:
	@# 'echo' seems to vary in behavior with respect to its handling
	@# of '\n'.  (Is it a newline, or a literal backslash followed by
	@# a literal 'n'?  It seems to depend on your platform.)  So we
	@# deliberately avoid the use of explicit newline characters here.
	@$(ECHO) '\
extern  "C" int printf(const char*, ...);\
int main() {\
printf( "-ss%zu  ", sizeof(short) );\
printf( "-si%zu  ", sizeof(int) );\
printf( "-sl%zu  ", sizeof(long) );\
printf( "-sll%zu  ", sizeof(long long) );\
printf( "-sf%zu  ", sizeof(float) );\
printf( "-sd%zu  ", sizeof(double) );\
printf( "-sld%zu  ", sizeof(long double) );\
printf( "-sp%zu  ", sizeof(void*) );\
printf( "-sw%zu  ", sizeof(wchar_t) );\
}' >$(SIZE_GEN).cc
	$(GCC) $(SIZE_GEN).cc -o $(SIZE_GEN)
	./$(SIZE_GEN) >size-options.lnt
	@# ... and make it newline-terminated:
	@$(ECHO) ""  >>size-options.lnt
	$(RM) $(SIZE_GEN)*


html-am: lint_cmac.h lint_cppmac.h gcc-include-path.lnt size-options.lnt

