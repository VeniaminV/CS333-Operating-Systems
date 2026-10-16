# Lab 3: velocicryptor

Multi-threaded password cracker. You give it a file of password hashes and a dictionary file, and it tries every dictionary word against every hash using `crypt_rn()`.

## How it works

- **Dynamic load balancing:** threads don't get a fixed slice of the hashes. Each thread grabs the next unclaimed hash from a shared counter behind a mutex, so fast threads just do more work.
- **Thread-safe crypt:** uses `crypt_rn()` with a per-thread `struct crypt_data` instead of plain `crypt()`, which isn't safe across threads.
- **Hash type detection:** recognizes DES, NT, MD5, SHA-256, SHA-512, yescrypt, gost-yescrypt, and bcrypt from the hash prefix and counts each type.
- **Locks:** one mutex keeps output lines from interleaving, another protects the totals that each thread adds into at the end.
- Prints per-thread and total stats plus elapsed time to stderr.

## Build and run

Needs `libcrypt`.

```
make
./velocicryptor -i hashes.txt -d dictionary.txt -t 8
```

Options: `-i` hash file, `-d` dictionary, `-o` output file, `-t` threads, `-n` nice value, `-v` verbose.

Example output:

```
cracked secret $6$abc$IdWK...
cracked banana $5$xyz$DGJv...
*** failed to crack $1$q$EH/0...
```
