CC=gcc
CFLAGS=-O2 -Wall -g
LDFLAGS=-lz

all: prestool

prestool: pres.o tool.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ tool.c pres.o
	
pres.o: util.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ util.c
	
pres.so: util.c
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ util.c
	
tester: prestool tester.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ tester.c pres.o
	

clean:
	rm -f *.o *.so prestool tester
