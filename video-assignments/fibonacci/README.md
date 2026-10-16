# Fibonacci

Three versions of Fibonacci to compare approaches:

- `fibr.c`: recursive (slow, but it's the point)
- `fibi.c`: iterative with `unsigned long`
- `fibg.c`: uses the GMP library so it can go way past what fits in 64 bits

```
gcc -Wall fibr.c -o fibr
gcc -Wall fibi.c -o fibi
gcc -Wall fibg.c -o fibg -lgmp
```
