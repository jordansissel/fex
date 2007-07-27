CFLAGS=-g -Wall

fex: fex.o
	gcc $(CFLAGS) fex.o -o fex

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o || true

package: test-package-build create-package

create-package:
	NAME=fex-`date +%Y%m%d`; \
	mkdir $${NAME}; \
	rsync --exclude .svn -av `ls *.c *.h README Makefile* 2> /dev/null` $${NAME}/; \
	tar -zcf $${NAME}.tar.gz $${NAME}/; \
	rm -rf $${NAME}/

# Make sure the package we're building compiles.
test-package-build: create-package
	NAME=fex-`date +%Y%m%d`; \
	tar -zxf $${NAME}.tar.gz; \
	make -C $${NAME} fex; \
	rm -rf $${NAME}/
	rm -f $${NAME}.tar.gz

