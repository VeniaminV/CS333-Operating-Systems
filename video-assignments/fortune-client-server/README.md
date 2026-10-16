# Fortune Client/Server

A small TCP client and server.

- **Server:** listens on an IPv6 TCP socket. For each connection it runs `fortune` with `popen()` and writes the output back to the client.
- **Client:** connects to the server by IP and port and prints whatever it gets.

```
gcc -Wall -Werror -g -o fortune_server fortune_server.c
gcc -Wall -Werror -g -o fortune_client fortune_client.c

./fortune_server -p 50001
./fortune_client -a <server ip> -p 50001
```

The server needs `fortune` installed.
