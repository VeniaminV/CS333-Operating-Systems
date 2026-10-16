C = gcc

CFLAGS = -Wall -Wextra -Wshadow -Wunreachable-code \
-Wredundant-decls -Wmissing-declarations \
-Wold-style-definition -Wmissing-prototypes \
-Wdeclaration-after-statement -Wno-return-local-addr \
-Wunsafe-loop-optimizations -Wuninitialized -Werror \
-Wno-unused-parameter \
-Wno-string-compare -Wno-stringop-overflow \
-Wno-stringop-overread -Wno-stringop-truncation \
-g -pthread

LDFLAGS = -lcrypt -pthread

all: velocicryptor

velocicryptor: velocicryptor.o
	$(CC) $(CFLAGS) -o velocicryptor velocicryptor.o $(LDFLAGS)

velocicryptor.o: velocicryptor.c velocicryptor.h
	$(CC) $(CFLAGS) -c velocicryptor.c

clean:
	rm -f *.o velocicryptor *~ \#*
