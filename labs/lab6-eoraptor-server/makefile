# Veniamin Velikoretskikh veniamin@pdx.edu
# CS 333 Lab 6 - Spring 2026

CC     = gcc
CFLAGS = -Wall -Wextra -Wshadow -Wunreachable-code \
         -Wredundant-decls -Wmissing-declarations \
         -Wold-style-definition -Wmissing-prototypes \
         -Wdeclaration-after-statement -Wno-return-local-addr \
         -Wunsafe-loop-optimizations -Wuninitialized -Werror \
         -Wno-unused-parameter

all: eoraptor_server eoraptor_client

eoraptor_server: eoraptor_server.o
	$(CC) $(CFLAGS) -o eoraptor_server eoraptor_server.o -lm

eoraptor_server.o: eoraptor_server.c eoraptor_ui.h
	$(CC) $(CFLAGS) -c eoraptor_server.c

eoraptor_client: eoraptor_client.o eoraptor_ui.o
	$(CC) $(CFLAGS) -o eoraptor_client eoraptor_client.o eoraptor_ui.o -lncurses

eoraptor_client.o: eoraptor_client.c eoraptor_ui.h
	$(CC) $(CFLAGS) -c eoraptor_client.c

eoraptor_ui.o: eoraptor_ui.c eoraptor_ui.h
	$(CC) $(CFLAGS) -c eoraptor_ui.c

clean:
	rm -f eoraptor_server eoraptor_client \
	      eoraptor_server.o eoraptor_client.o eoraptor_ui.o \
	      *~ \#*
