CC = gcc
CFLAGS = -Wall -Wextra -Wshadow -Wunreachable-code \
	-Wredundant-decls -Wmissing-declarations \
	-Wold-style-definition -Wmissing-prototypes \
	-Wdeclaration-after-statement -Wno-return-local-addr \
	-Wunsafe-loop-optimizations -Wuninitialized -Werror \
	-Wno-unused-parameter
LDFLAGS = -lpthread -lrt

all: pyrofactor

pyrofactor: pyrofactor.o
	$(CC) $(CFLAGS) -o pyrofactor pyrofactor.o $(LDFLAGS)

pyrofactor.o: pyrofactor.c pyrofactor.h
	$(CC) $(CFLAGS) -c pyrofactor.c

clean:
	rm -f *.o pyrofactor *~ \#*
