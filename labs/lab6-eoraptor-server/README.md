# Lab 6: eoraptor server

UDP server for Eoraptor, a multiplayer lunar lander game. The server owns all the game state (terrain, every player's lander, fuel, thrust) and the clients just send key presses and draw what they get back.

## How it works

- **One event loop with `poll()`** over three file descriptors: stdin for operator commands, the UDP socket for client messages, and a `timerfd` that ticks at 60 Hz.
- **Physics tick:** each timer tick updates every lander, checks for landings and crashes, and sends the whole moon state to all clients.
- **Players:** a client gets a player slot the first time it sends anything, matched by its address.
- **Terrain:** generates random lunar terrain with mountains and flat landing pads.
- **Server console:** `L` launches all landers, `R` resets, `l`/`k` list players and key bindings, `q` quits. Ctrl-C also shuts down cleanly.

## Run

```
./eoraptor_server -P <port> -v
```

## Notes

The client, the ncurses UI (`eoraptor_client.c`, `eoraptor_ui.c`), and `eoraptor_ui.h` came from the instructor and aren't included. The server is my part.
