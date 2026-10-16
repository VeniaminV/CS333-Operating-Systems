# Vector

A growable array of `point_t` structs. It grows with `realloc()` as points get added, then gets sorted with `qsort()` and a custom compare function. Options come in through `getopt()`.

Building with `-DNOISY_DEBUG` turns on extra debug prints.

```
gcc -g -Wall -o vector vector.c
gcc -g -Wall -DNOISY_DEBUG -o vector vector.c
```
