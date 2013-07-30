CC=clang
CFLAGS=-O2 -Wall
LDFLAGS=-c
ALIB=util.a
SOLIB=util.so

all: static test

static:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ALIB) util.c

test:
	$(CC) $(CFLAGS) -o tester tester.c $(ALIB)

clean:
	rm $(ALIB) tester
