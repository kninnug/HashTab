CC = gcc
WARNS = -Wall -Wextra -Wwrite-strings -Wfloat-equal -Wformat -Wcast-qual -Wlogical-op -Wstrict-aliasing=3 -fstrict-aliasing -pedantic
XWARNS = -Wmissing-prototypes -Wdeclaration-after-statement
UNWARNS = -Wno-unused-parameter
OPTS = $(WARNS) $(UNWARNS) -Wall -pedantic -g -std=c99

all: test

GeneralHashFunctions.o: GeneralHashFunctions.c GeneralHashFunctions.h
	$(CC) $(OPTS) $(CFLAGS) -c -o GeneralHashFunctions.o GeneralHashFunctions.c

hashtab.o: hashtab.c hashtab.h
	$(CC) $(OPTS) $(CFLAGS) -c -o hashtab.o hashtab.c

test: test.c hashtab.o GeneralHashFunctions.o
	$(CC) $(OPTS) $(CFLAGS) -o test test.c GeneralHashFunctions.o hashtab.o

test-sm: test-sm.c hashtab.o GeneralHashFunctions.o stringmap.h
	$(CC) $(OPTS) $(CFLAGS) -o test-sm test-sm.c hashtab.o GeneralHashFunctions.o

clean:
	rm -f *.o
	rm -f *.exe
	rm -f *~
	rm -f test
