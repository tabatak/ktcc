CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

ktcc: $(OBJS)
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): ktcc.h

test: ktcc
		./test.sh

clean: 
	rm -f ktcc *.o *~ tmp*

.PHONY: test clean
