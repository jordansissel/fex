CFLAGS+=-g -Wall -std=c99
CC=gcc
STDIN_CC=-x c
SNPRINTF_DEF=`sh need_snprintf.sh > /dev/null && echo "-DNEED_SNPRINTF_2_2"`
SNPRINTF_CC=`sh need_snprintf.sh`
PACKAGE_FILES=*.c *.h t snprintf_2.2 need_snprintf.sh \
  CHANGELIST COPYRIGHT README Makefile* fex.spec

DESTDIR?=
PREFIX?=/usr/local
INSTALLBIN?=$(PREFIX)/bin
INSTALLLIB?=$(PREFIX)/lib
INSTALLMAN?=$(PREFIX)/share/man
INSTALLINCLUDE?=$(PREFIX)/include

DPREFIX=$(DESTDIR)$(PREFIX)
DINSTALLBIN=$(DESTDIR)$(INSTALLBIN)
DINSTALLLIB=$(DESTDIR)$(INSTALLLIB)
DINSTALLMAN=$(DESTDIR)$(INSTALLMAN)
DINSTALLINCLUDE=$(DESTDIR)$(INSTALLINCLUDE)

VERSION=$(shell sh version.sh)
PACKAGE=fex-$(VERSION)

all: fex

test:
	cd t; sh test.sh

install: fex
	install -d $(DINSTALLBIN)
	install -m 755 fex $(DINSTALLBIN)/
	install -d $(DINSTALLMAN)
	install -d $(DINSTALLMAN)/man1
	install fex.1 $(DINSTALLMAN)/man1/

fex: fex.o
	$(CC) $(CFLAGS) fex.o $(SNPRINTF_CC) -o $@

fex.o: fex_version.h

fex_version.h:
	sh version.sh --header > $@

%.o: %.c
	$(CC) $(SNPRINTF_DEF) $(CFLAGS) $< -c -o $@

fex.spec: fex.spec.in
	sed -e 's/%{version}/$(VERSION)/' $< > $@

clean:
	-rm -f fex *.o */*.o fex_version.h fex.spec fex.1 

package: test-package create-package

pre-package:
	rm -f fex_version.h fex.spec
	$(MAKE) VERSION fex_version.h fex.spec

rpm: package
	rpmbuild -tb $(PACKAGE).tar.gz

create-package: pre-package fex.1 fex.spec
	-rm -rf $(PACKAGE)
	-mkdir $(PACKAGE)
	git ls-files | cpio -p --make-directories $(PACKAGE)/ 
	(echo "fex.spec"; echo "fex.1") | cpio -p --make-directories $(PACKAGE)/ 
	tar -zcf $(PACKAGE).tar.gz $(PACKAGE)/
	rm -rf $(PACKAGE)

# Make sure the package we're building compiles.
test-package: create-package
	echo "Testing to build of $(PACKAGE)"
	tar -zxf $(PACKAGE).tar.gz
	make -C $(PACKAGE) fex test
	rm -rf $(PACKAGE)

fex.1: fex.pod
	pod2man -c "" -r "" $< > $@

.PHONY: showman
showman: fex.1
	nroff -man $<
