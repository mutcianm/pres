CC=gcc
CFLAGS=-O2 -Wall -g

all: prestool

prestool: pres.o tool.c
	$(CC) $(CFLAGS) -o $@ tool.c pres.o
	
pres.o: util.c
	$(CC) $(CFLAGS) -c -o $@ util.c
	
pres.so: util.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ util.c
	
tester: prestool tester.c 
	$(CC) $(CFLAGS) -o $@ tester.c pres.o
	

clean:
	rm -f *.o *.so prestool tester
