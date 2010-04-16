CFLAGS=-g -Wall
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
INSTALLMAN?=$(PREFIX)/man
INSTALLINCLUDE?=$(PREFIX)/include

DPREFIX=$(DESTDIR)$(PREFIX)
DINSTALLBIN=$(DESTDIR)$(INSTALLBIN)
DINSTALLLIB=$(DESTDIR)$(INSTALLLIB)
DINSTALLMAN=$(DESTDIR)$(INSTALLMAN)
DINSTALLINCLUDE=$(DESTDIR)$(INSTALLINCLUDE)

VERSION=$(shell sh version.sh)
PACKAGE=fex-$(VERSION)

all: fex

install: fex
	install -d $(DINSTALLBIN)
	install -m 755 fex $(DINSTALLBIN)/

fex: fex.o
	$(CC) $(CFLAGS) fex.o $(SNPRINTF_CC) -o $@

fex.o: fex_version.h

fex_version.h:
	sh version.sh --header > $@

VERSION:
	sh version.sh --shell > $@

%.o: %.c
	$(CC) $(SNPRINTF_DEF) $(CFLAGS) $< -c -o $@

fex.spec: fex.spec.in
	sed -e 's/%{version}/$(VERSION)/' $< > $@

clean:
	rm -f fex *.o */*.o VERSION fex_version.h fex.spec || true

package: test-package-build create-package

pre-package:
	rm -f VERSION fex_version.h fex.spec

rpm: package
	tmp=$$(mktemp -d); \
	arch=$$(uname -m); \
	rel=$$(awk '/^Release:/ { print $$2 }' fex.spec); \
	rpmbuild -tb --define "arch $${arch}" $(PACKAGE).tar.gz \
	         --define "_topdir $$tmp" --buildroot "$$tmp/BUILD.fex"; \
	mv $$tmp/RPMS/$${arch}/$(PACKAGE)-$${rel}.$${arch}.rpm .; \
	rm -r $$tmp

create-package: pre-package fex_version.h VERSION fex.spec
	mkdir $(PACKAGE)
	rsync --exclude .svn -a `ls -d $(PACKAGE_FILES) 2> /dev/null` $(PACKAGE)/
	tar -zcf $(PACKAGE).tar.gz $(PACKAGE)/
	rm -rf $(PACKAGE)

# Make sure the package we're building compiles.
test-package-build: create-package
	echo "Testing to build of $(PACKAGE)"
	tar -zxf $(PACKAGE).tar.gz
	make -C $(PACKAGE) fex
	rm -rf $(PACKAGE)

