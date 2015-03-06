CC = gcc
OPTS = -Wall -pedantic -g -std=c99

GeneralHashFunctions.o: GeneralHashFunctions.c
	$(CC) $(OPTS) -c -o GeneralHashFunctions.o GeneralHashFunctions.c

hashtab.o: hashtab.c
	$(CC) $(OPTS) -c -o hashtab.o hashtab.c

test: test.c hashtab.o GeneralHashFunctions.o
	$(CC) $(OPTS) -o test test.c GeneralHashFunctions.o hashtab.o

clean:
	rm -f *.o
	rm -f *.exe
	rm -f *~
	rm -f test
