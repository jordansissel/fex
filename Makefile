CFLAGS=-g -Wall

fex: fex.o
	gcc $(CFLAGS) fex.o -o fex

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o || true
