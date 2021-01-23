cc = gcc
CFLAGS = -Wall -g
all: argus.c argusd.c argus.h
	gcc argusd.c -o argusd
	gcc argus.c -o argus

clean:
	rm -f argus
	rm -f argusd
	rm -f program *.o