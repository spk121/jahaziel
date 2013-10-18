#!/usr/bin/make -f

# Copyright (C)

# LICENSE
SHELL = /bin/sh

################################################################
# EXECUTABLES

AR = ar
BISON = bison
CC = clang
FLEX = flex
INSTALL = install
LD = ld
LDCONFIG = ldconfig
LEX = lex
MAKE = make
MAKEINFO = makeinfo
RANLIB = ranlib
TEXI2DVI = texi2dvi
TEXI2HTML = `makeinfo --no-split --html`
YACC = bison -y

INSTALL = install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = ${INSTALL} -m 644

################################################################
# DIRECTORIES

# directory for the sources being compiled
srcdir = .

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
# sbindir = $(exec_prefix)/sbin
# libexecdir = $(exec_prefix)/libexec
datarootdir = $(prefix)/share
datadir = $(datarootdir)
# sysconfdir = $(prefix)/etc
# sharedstatedir = $(prefix)/com
# localstatedir = $(prefix)/var
# runstatedir = $(localstatedir)/run
includedir = $(prefix)/include
# oldincludedir = /usr/include
docdir = $(datarootdir)/doc/$(package)
infodir = $(datarootdir)/info
htmldir = $(docdir)
dvidir = $(docdir)
pdfdir = $(docdir)
psdir = $(docdir)
libdir = $(exec_prefix)/lib
# lispdir = $(datarootdir)/emacs/site-lisp
# localedir = $(datarootdir)/locale
# mandir = $(datarootdir)/man

# Prepended to installed targets for staged installs
DESTDIR =

################################################################
# FLAGS

CPPFLAGS = $(shell pkg-config --cflags-only-I \
	gtk+-3.0 \
	glib-2.0 \
	gmodule-export-2.0 \
	libczmq)

# User-overrideable flags
CFLAGS = -g -O0
CFLAGS += $(shell pkg-config --cflags-only-other \
	gtk+-3.0 \
	glib-2.0 \
	gmodule-export-2.0 \
	libczmq)
LIBS   = -Wl,--export-dynamic \
	$(shell pkg-config --libs \
	gtk+-3.0 \
	glib-2.0 \
	gmodule-export-2.0 \
	libczmq)

# Non-overrideable flags
ALL_CFLAGS = $(CFLAGS) -std=c11

################################################################
# SOURCES

sources = $(wildcard *.c)
headers = $(wildcard *.h)
objects = $(sources:.c=.o)
depends = $(sources:.c=.d)
aux    = README COPYING ChangeLog texinfo.tex

%.o: %.c
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<

# Rule to have the compiler generate the dependencies sub-makefiles.
%.d: %.c
	set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

################################################################
# PRIMARY TARGETS

.PHONY: all install
all: jahaziel jahaziel.info

# Install files
# Don't forget $(DESTDIR)
# Don't forget $(NORMAL_INSTALL) $(POST_INSTALL) $(PRE_INSTALL)
install:
	$(PRE_INSTALL)
	$(NORMAL_INSTALL)
	$(INSTALL) jahaziel
	$(POST_INSTALL)

install-html:
	$(PRE_INSTALL)
	$(NORMAL_INSTALL)
	$(POST_INSTALL)

install-pdf:
	$(PRE_INSTALL)
	$(NORMAL_INSTALL)
	$(POST_INSTALL)

install-strip:
	$(MAKE) INSTALL_PROGRAM=`$(INSTALL_PROGRAM) -s` install

uninstall:
	$(PRE_UNINSTALL)
	$(NORMAL_UNINSTALL)
	$(POST_UNINSTALL)

# delete files created by this makefile that are normally created by
# building the program, and that aren't in revision control.  Delete
# .dvi.
clean:
	rm -f *.o jahaziel core

# Like `clean' but also delete any other files that are created by
# "make all".
.PHONY: distclean
distclean:
	-rm -f jahaziel.aux jahaziel.cp \
	jahaziel.cps jahaziel.dvi \
	jahaziel.fn jahaziel.ky \
	jahaziel.log jahaziel.pg \
	jahaziel.toc jahaziel.tp \
	jahaziel.vr \
	*~

# Like `distclean' but also generated C souce files, tags tables, and
# info files.
maintainer-clean:
	@echo 'This command is intended for maintainers to use; it'
	@echo 'delete files that may need special tools to rebuild'
	rm -f jahaziel.info* TAGS joza_msg.c joza_msg.h *.d *.d.*

# Update a tags table for this program
TAGS:  $(sources) $(headers)
	etags $^

# Generate any info files needed
info: $(project).info

dvi: jahaziel.dvi

# Create a distribution tar file for this program
dist: $(SRCS) $(AUX) # Depends on all non-sources files in the distro.
	echo jahaziel-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q version.c` > .fname
	-rm -rf `cat .fname`
	mkdir `cat .fname`
	ln $(sources) `cat .fname`
	tar chzf `cat .fname`.tar.gz `cat .fname`
	-rm -rf `cat .fname` .fname

# Perform self-tests
check:

################################################################
# SECONDARY TARGETS

# Create the directories where files are installed, and their parent
# directories.
installdirs: mkinstalldirs
	$(srcdir)/mkinstalldirs $(DESTDIR)$(bindir) \
				$(DESTDIR)$(datadir) \
				$(DESTDIR)$(infodir)
#				$(DESTDIR)$(libdir)
#				$(DESTDIR)$(mandir)

# This copies the info file into the right directory, and, if the
# "install-info" program is available, it runs that.
do-install-info: jahaziel.info installdirs
	$(NORMAL_INSTALL)
	if test -f jahaziel.info; then d=.; \
	  else d="$(srcdir)"; \
	fi; \
	$(INSTALL_DATA) $$d/jahaziel.info "$(DESTDIR)$(infodir)/jahaziel.info"
	$(POST_INSTALL)
	if $(SHELL) -c `install-info --version` > /dev/null 2>&1; then \
	  install-info --dir-file="$(DESTDIR)$(infodir)/dir" "$(DESTDIR)$(infodir)/foo.info"; \
	else true; fi

jahaziel.info: jahaziel.texi 
	$(MAKEINFO) $(srcdir)/jahaziel.texi

jahaziel.dvi: jahaziel.texi 
	$(TEXI2DVI) $(srcdir)/jahaziel.texi

jahaziel.html: jahaziel.texi
	$(TEXI2HTML) $(srcdir)/jahaziel.texi

jahaziel: $(objects)
	$(CC) $(LDFLAGS) -o $@ $(objects) $(LIBS)

-include $(depends)
