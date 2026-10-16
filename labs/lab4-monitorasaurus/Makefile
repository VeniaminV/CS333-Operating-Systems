CC = gcc

CFLAGS = -Wall -Wextra -Wshadow -Wunreachable-code \
-Wredundant-decls -Wmissing-declarations \
-Wold-style-definition -Wmissing-prototypes \
-Wdeclaration-after-statement -Wno-return-local-addr \
-Wunsafe-loop-optimizations -Wuninitialized -Werror \
-Wno-unused-parameter \
-g -pthread

LDFLAGS = -lpthread

all: monitorasaurus

monitorasaurus: monitorasaurus.o
	$(CC) $(CFLAGS) -o monitorasaurus monitorasaurus.o $(LDFLAGS)

monitorasaurus.o: monitorasaurus.c monitorasaurus.h
	$(CC) $(CFLAGS) -c monitorasaurus.c

clean:
	rm -f *.o monitorasaurus *~ \#*
