CC = gcc
CFLAGS = -g -Wall
DEBUG = -D DEBUG

default: xgb_recv

all: xgb_recv tools

tools: pkt_gen ring_buffer_test

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

xgb_recv: Makefile xgb_recv.o ring_buffer.o
	$(CC) $(CFLAGS) xgb_recv.o ring_buffer.o -o $@ -lpthread

pkt_gen: Makefile pkt_gen.c
	$(CC) $(CFLAGS) pkt_gen.c -o $@

ring_buffer_test: Makefile ring_buffer_test.c ring_buffer.o
	$(CC) $(CFLAGS) $(DEBUG) ring_buffer_test.c ring_buffer.o -o ring_buffer_test -lpthread

clean:
	@rm -vf *.o
	@rm -vf *~
	@rm -vf xgb_recv
	@rm -vf pkt_gen
	@rm -vf *_test
