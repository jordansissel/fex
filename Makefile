CFLAGS=-g -Wall
CC=gcc
STDIN_CC=-x c
SNPRINTF_DEF=`sh need_snprintf.sh > /dev/null && echo "-DNEED_SNPRINTF_2_2"`
SNPRINTF_CC=`sh need_snprintf.sh`
PACKAGE_FILES=*.c *.h t snprintf_2.2 need_snprintf.sh \
  CHANGELIST README Makefile*

fex: fex.o
	$(CC) $(CFLAGS) fex.o $(SNPRINTF_CC) -o $@

%.o: %.c
	$(CC) $(SNPRINTF_DEF) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o */*.o || true

package: test-package-build create-package

create-package:
	NAME=fex-`date +%Y%m%d`; \
	mkdir $${NAME}; \
	rsync --exclude .svn -av `ls -d $(PACKAGE_FILES)` $${NAME}/; \
	tar -zcf $${NAME}.tar.gz $${NAME}/; \
	rm -rf $${NAME}/

# Make sure the package we're building compiles.
test-package-build: create-package
	NAME=fex-`date +%Y%m%d`; \
	tar -zxf $${NAME}.tar.gz; \
	make -C $${NAME} fex; \
	rm -rf $${NAME}/
	rm -f $${NAME}.tar.gz

