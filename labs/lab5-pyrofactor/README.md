# Lab 5: pyrofactor

Process manager that factors large integers by racing worker processes against each other. For each number it reads, it forks and execs a mix of workers using three algorithms (naive trial division, Pollard's rho, and Fermat's method), and whichever finds a factor first wins.

## How it works

- **POSIX shared memory:** creates a segment with `shm_open()` + `ftruncate()` + `mmap()` that holds the current number, the result, and who won.
- **Process-shared mutex:** the mutex lives inside the shared segment and is set to `PTHREAD_PROCESS_SHARED`, so separate processes can lock it.
- **Splitting the work:** naive workers each start at a different divisor and skip by a stride, so they cover different ranges.
- **Process groups:** pyrofactor puts itself in its own process group so it can `killpg()` all workers at once with SIGTERM when a factor is found, or forward SIGINT on Ctrl-C.
- **Timeout:** `alarm()` stops the search after the timeout, and it reaps every child so there are no zombies. The segment gets `shm_unlink()`ed on exit.

## Options

`-n` shared memory name, `-i` input file, `-t` timeout in minutes, `-N`/`-P`/`-F` number of naive/Pollard/Fermat workers, `-v` verbose.

## Notes

`pyrofactor.h` and the three worker programs were given by the instructor and aren't included. `pyrofactor.c` is the manager I wrote.
