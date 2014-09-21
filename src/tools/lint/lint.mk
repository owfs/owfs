
DOS_OWFSROOT ?= C:/projects/softhub/
LINT_DIR ?= $(DOS_OWFSROOT)/src/tools/lint
LINT_CC ?= $(LINT_DIR)/lint-nt.exe

COMP_DIR ?= C:/cygwin

# The posix/ansi includes doesn't work very good...
# Lots of linux-specific things are missing. (termios, termbits etc)
# Only use some of them located in $(LINT_DIR)/include
LINT_POSIX:= -I$(LINT_DIR)/include/posix -I$(LINT_DIR)/include/posix/sys -I$(LINT_DIR)/include/ansi

LINT_INC:= \
		$(INC) \
		-D_LINT_ \
		-D_lint \
		-I$(LINT_DIR)/lnt \
		$(LINT_POSIX) \
		-I$(LINT_DIR)/environment/lintrules \
		-I$(COMP_DIR)/usr/include \
		-I$(INCPREFIX)../../../../src/include \
		-I$(INCPREFIX)../include \
		-I$(INCPREFIX)../../../owlib/src/include

LINT_DEFINES:=

LINT_LEVEL = 2

LINT_ENV ?= env-owfs.lnt
LINTFLAGS= $(LINT_INC) $(addprefix -D,$(LINT_DEFINES)) au-misra3.lnt $(LINT_ENV) co-gcc.lnt co-owfs.lnt -u -t4 -zero -w$(LINT_LEVEL)

LINT_OBJ = $(SRC:.c=.lint)

lint-local: $(LINT_OBJ)

html: $(LINT_OBJ)

%.lint: %.c
	$(LINT_CC) $(LINTFLAGS) $< | tee $(subst .c,.lint,$<)

