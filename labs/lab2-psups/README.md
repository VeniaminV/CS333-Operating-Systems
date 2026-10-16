# Lab 2: psups

My own version of `ps`. It walks `/proc`, picks out the PID directories, reads each process's `stat` file, and prints a table. It can also send signals to whatever processes match the filters.

## Options

| Flag | Does |
|------|------|
| `-u uid` / `-U user` / `-m` | only processes owned by that uid, username, or me |
| `-n str` | only processes whose name matches |
| `-z` | only zombies |
| `-x` | extended output |
| `-s num` / `-S name` | send a signal by number or name to every match |
| `-i` / `-K` | shortcuts for SIGINT / SIGKILL |
| `-v` | verbose |

## Notes

`psups.h` and `psups_stat.c` (the `/proc/[pid]/stat` parser and signal-name table) were given by the instructor and aren't included, so this folder won't build by itself. `psups.c` is my part.
