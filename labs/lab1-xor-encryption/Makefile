# Veniamin Velikoretskikh veniamin@pdx.edu
# CS 333, make file for lab 1 assignment

CC = gcc

#flags that were given in the lab pdf
CFLAGS = -Wall -Wshadow -Wunreachable-code -Wredundant-decls \
-Wmissing-declarations -Wold-style-definition \
-Wmissing-prototypes -Wdeclaration-after-statement \
-Wno-return-local-addr -Wunsafe-loop-optimizations \
-Wuninitialized -Werror -Wextra

all: encrypt-w-inode decrypt-w-inode encrypt-w-symlink decrypt-w-symlink


# This is going to be for the encrypt-w-inode
encrypt-w-inode: encrypt-w-inode.o
	$(CC) $(CFLAGS) -o encrypt-w-inode encrypt-w-inode.o

encrypt-w-inode.o: encrypt-w-inode.c
	$(CC) $(CFLAGS) -c encrypt-w-inode.c


# This is going to be for the decrypt-w-inode
decrypt-w-inode: decrypt-w-inode.o
	$(CC) $(CFLAGS) -o decrypt-w-inode decrypt-w-inode.o

decrypt-w-inode.o: decrypt-w-inode.c
	$(CC) $(CFLAGS) -c decrypt-w-inode.c


# This is going to be for encrypt-w-symlink
encrypt-w-symlink: encrypt-w-symlink.o
	$(CC) $(CFLAGS) -o encrypt-w-symlink encrypt-w-symlink.o

encrypt-w-symlink.o: encrypt-w-symlink.c
	$(CC) $(CFLAGS) -c encrypt-w-symlink.c

# This is going to be for decrypt-w-symlink
decrypt-w-symlink: decrypt-w-symlink.o
	$(CC) $(CFLAGS) -o decrypt-w-symlink decrypt-w-symlink.o

decrypt-w-symlink.o: decrypt-w-symlink.c
	$(CC) $(CFLAGS) -c decrypt-w-symlink.c


clean:
	rm -f *.o encrypt-w-inode decrypt-w-inode encrypt-w-symlink decrypt-w-symlink *~ \#*




