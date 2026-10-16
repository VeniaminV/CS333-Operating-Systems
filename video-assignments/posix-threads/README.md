# POSIX Threads

Finding primes up to an upper bound, rebuilt five times to go from single-threaded to a properly balanced multi-threaded version.

| File | Change |
|------|--------|
| `tprimes1.c` | single-threaded baseline |
| `tprimes2.c` | adds timing with `clock_gettime()` |
| `tprimes3.c` | multiple threads, static split: thread *i* checks *i*, *i*+N, *i*+2N... |
| `tprimes4.c` | dynamic split: threads grab the next candidate from a shared counter behind a mutex |
| `tprimes5.c` | same idea, with the locking moved into a `get_next_candidate()` function |

Going from 3 to 4 shows why static splitting is uneven (bigger numbers take longer to check) and why handing out work dynamically balances it. This is the same pattern velocicryptor uses.

```
gcc -g3 -O0 -Wall -Werror -pthread -o tprimes5 tprimes5.c
```
