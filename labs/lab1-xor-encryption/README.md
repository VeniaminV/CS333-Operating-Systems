# Lab 1: XOR Encryption

Four small programs that encrypt and decrypt files with XOR, using two different ways to pick the key.

- **`encrypt-w-inode` / `decrypt-w-inode`**: the key is the low byte of the file's inode number (`st_ino & 0xFF` from `stat()`).
- **`encrypt-w-symlink` / `decrypt-w-symlink`**: the key is the name of a symbolic link, cycled byte by byte over the data. Decrypt uses `readlink()` to find the target file.

All four use raw `open()`/`read()`/`write()` with a buffer instead of stdio, and parse options with `getopt()`.

## Build and run

```
make
ln -s input.txt mykey
./encrypt-w-symlink -i input.txt -o out.enc -s mykey -v
./decrypt-w-symlink -s mykey -o decrypted.txt
```

Options: `-i` input, `-o` output, `-s` symlink, `-v` verbose, `-h` help.
