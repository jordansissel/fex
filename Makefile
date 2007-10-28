CFLAGS=-g -Wall
CC=gcc

fex: fex.o snprintf_2.2/snprintf.o
	$(CC) $(CFLAGS) $^ -o $@

snprintf_2.2/snprintf.o:
	# Test for asprintf
	echo "#include <stdio.h>\nint main() { asprintf; }" \
	| $(CC) -D_GNU_SOURCE -x c /dev/stdin; \
	if [ $$? -ne 0 ] ; then \
		make -C snprintf_2.2; \
	else \
		touch $@; \
	fi

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o */*.o || true


package: test-package-build create-package

create-package:
	NAME=fex-`date +%Y%m%d`; \
	mkdir $${NAME}; \
	rsync --exclude .svn -av `ls -d *.c *.h t snprintf_2.2 CHANGELIST README Makefile* 2> /dev/null` $${NAME}/; \
	tar -zcf $${NAME}.tar.gz $${NAME}/; \
	rm -rf $${NAME}/

# Make sure the package we're building compiles.
test-package-build: create-package
	NAME=fex-`date +%Y%m%d`; \
	tar -zxf $${NAME}.tar.gz; \
	make -C $${NAME} fex; \
	rm -rf $${NAME}/
	rm -f $${NAME}.tar.gz

