CC = gcc
CFLAGS = -g -Wall
DEBUG = -D DEBUG

default: all

all: xgb_recv

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

xgb_recv: Makefile xgb_recv.o ring_buffer.o
	$(CC) $(CFLAGS) xgb_recv.o ring_buffer.o -o $@ -lpthread

ring_buffer_test: Makefile ring_buffer_test.c ring_buffer.o
	$(CC) $(CFLAGS) $(DEBUG) ring_buffer_test.c ring_buffer.o -o ring_buffer_test -lpthread

clean:
	@rm -vf *.o
	@rm -vf *~
	@rm -vf xgb_recv
	@rm -vf *_test
