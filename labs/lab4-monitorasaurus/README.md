# Lab 4: monitorasaurus

Terminal system monitor, kind of like `watch`. Every interval it clears the screen, runs a system command with `popen()`, and prints the output, cutting off lines that are too wide.

## The trick

The refresh timer is a mutex. The main thread locks a mutex and then calls `pthread_mutex_timedlock()` on that same mutex with a deadline. It can never get the lock, so the call just waits until the deadline and returns `ETIMEDOUT`, which is the signal to refresh. Ctrl-C interrupts the wait (`EINTR`), and the SIGINT handler clears a `sig_atomic_t` flag so the loop exits cleanly.

## Options

`-i` refresh interval, `-c` number of refreshes before exiting, `-w` max line width, `-v` verbose, `-h` help.

## Notes

`monitorasaurus.h` (defaults and the command it runs) came from the instructor and isn't included.
