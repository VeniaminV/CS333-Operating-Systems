# Veniamin Velikoretskikh
# CS 333, Makefile for Lab 2: psups program

CC = gcc

# flags from lab pdf
CFLAGS = -Wall -Wshadow -Wunreachable-code -Wredundant-decls \
-Wmissing-declarations -Wold-style-definition \
-Wmissing-prototypes -Wdeclaration-after-statement \
-Wno-return-local-addr -Wunsafe-loop-optimizations \
-Wuninitialized -Werror -Wextra

# default target
all: psups


# build the psups program
psups: psups.o psups_stat.o
	$(CC) $(CFLAGS) -o psups psups.o psups_stat.o


# build psups.o
psups.o: psups.c psups.h
	$(CC) $(CFLAGS) -c psups.c


# build psups_stat.o
psups_stat.o: psups_stat.c psups.h
	$(CC) $(CFLAGS) -c psups_stat.c


# clean everything
clean:
	rm -f *.o psups *~ \#*
