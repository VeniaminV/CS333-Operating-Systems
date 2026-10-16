# Signals

The parent forks a few child processes and handles three signals:

- `SIGCHLD`: reaps a child when it exits and prints its exit value
- `SIGALRM`: timer-driven events
- `SIGINT`: custom Ctrl-C handling

The child counter is a `volatile sig_atomic_t` so it's safe to change inside the handlers.

```
gcc -Wall -o signals signals.c
```
