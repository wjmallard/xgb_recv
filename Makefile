CC = gcc
CFLAGS = -g -Wall
DEBUG = -D DEBUG

SRC = src
INC = include
LIB = lib
BIN = bin
CFLAGS += -I $(INC)

default: $(BIN)/xgb_recv

all: $(BIN)/xgb_recv tools

tools: $(BIN)/pkt_gen $(BIN)/ring_buffer_test

$(LIB):
	mkdir -p $(LIB)

$(BIN):
	mkdir -p $(BIN)

$(LIB)/%.o: $(SRC)/%.c $(INC)/%.h | $(LIB)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/xgb_recv: Makefile $(LIB)/xgb_recv.o $(LIB)/ring_buffer.o | $(BIN)
	$(CC) $(CFLAGS) $(LIB)/xgb_recv.o $(LIB)/ring_buffer.o -o $@ -lpthread

$(BIN)/pkt_gen: Makefile $(SRC)/pkt_gen.c | $(BIN)
	$(CC) $(CFLAGS) $(SRC)/pkt_gen.c -o $@

$(BIN)/ring_buffer_test: Makefile $(SRC)/ring_buffer_test.c $(LIB)/ring_buffer.o | $(BIN)
	$(CC) $(CFLAGS) $(DEBUG) $(SRC)/ring_buffer_test.c $(LIB)/ring_buffer.o -o $@ -lpthread

clean:
	@rm -rvf $(LIB)
	@rm -rvf $(BIN)
	@rm -vf *~
