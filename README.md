# CS 333 Operating Systems

All my CS 333 (Intro to Operating Systems) work from Portland State, Spring 2026, in one repo. Everything is in C on Linux, built with strict gcc warnings (`-Wall -Wextra -Werror` and friends).

## Labs

| Lab | What it is | Main topics |
|-----|-----------|-------------|
| [Lab 1: XOR encryption](labs/lab1-xor-encryption) | Encrypt/decrypt files with XOR, keyed off an inode number or a symlink name | file descriptors, `stat()`, `readlink()`, `getopt()` |
| [Lab 2: psups](labs/lab2-psups) | My own `ps` clone that reads `/proc` and can filter and signal processes | `/proc`, `opendir()`, `getpwuid()`, `kill()` |
| [Lab 3: velocicryptor](labs/lab3-velocicryptor) | Multi-threaded dictionary password cracker | pthreads, mutexes, dynamic load balancing, `crypt_rn()` |
| [Lab 4: monitorasaurus](labs/lab4-monitorasaurus) | Terminal system monitor that refreshes on a timer | `pthread_mutex_timedlock()` as a timer, `popen()`, signals |
| [Lab 5: pyrofactor](labs/lab5-pyrofactor) | Manager that races worker processes to factor huge integers | POSIX shared memory, process-shared mutex, `fork`/`exec`, process groups |
| [Lab 6: eoraptor server](labs/lab6-eoraptor-server) | UDP game server for a multiplayer lunar lander game | UDP sockets, `poll()`, `timerfd`, 60 Hz game loop |

## Video assignments

Smaller follow-along assignments from the lecture videos.

| Assignment | What it is |
|-----------|-----------|
| [Fibonacci](video-assignments/fibonacci) | Recursive, iterative, and GMP big-number versions |
| [Vector](video-assignments/vector) | Dynamic array of structs with `realloc()` and `qsort()` |
| [Signals](video-assignments/signals) | Forking children and handling `SIGCHLD`, `SIGALRM`, `SIGINT` |
| [POSIX threads](video-assignments/posix-threads) | Prime finder that goes from 1 thread to a mutex-balanced thread pool in 5 steps |
| [Fortune client/server](video-assignments/fortune-client-server) | TCP server that sends `fortune` output to a client |

## Building

Each folder has its own Makefile or a compile line at the top of the `.c` file. Some labs depend on headers or starter code the instructor provided, which aren't included here, so those won't build on their own. Each lab's README says which ones.
