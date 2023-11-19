CFLAGS=-std=c11 -g -static

ktcc: ktcc.c

test: ktcc
		./test.sh

clean: 
	rm -f ktcc *.o *~ tmp*

.PHONY: test clean
