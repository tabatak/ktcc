ktcc: ktcc.c

test: ktcc
	./test.sh

clean:
	rm -f ktcc *.o *~ tmp*
