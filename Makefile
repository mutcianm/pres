CC=clang
CFLAGS=-O2 -Wall -g
LDFLAGS=-c
ALIB=util.a
SOLIB=util.so
PUTIL=prestool

all: test util

static:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ALIB) util.c

test: static
	$(CC) $(CFLAGS) -o tester tester.c $(ALIB)
	
util: static
	$(CC) $(CFLAGS) -o $(PUTIL) tool.c $(ALIB)

clean:
	rm $(ALIB) tester
