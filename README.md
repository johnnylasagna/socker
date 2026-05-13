# socker

The name is a play on sockets + soccer. My original goal (no pun intended) was to build a soccer game over tcp. But in the process learning through the wonderful [Beej's Guide To Networking](https://beej.us/guide/bgnet/), I built a chat application and decide to integrate my soccer idea into it. The soccer implementation is still incomplete. Messages are unencrypted because I hope you trust your friends.

Type `/help` after running the client application to find commands you can run inside the application

## Dependencies
- **gcc** (or compatible C compiler)
- **ncurses** library

## Building

You can build both the client and server using the provided Makefile:
```
make
```

This will produce the binaries in the `bin/` directory:
- `bin/client`
- `bin/server`

Alternatively, you can use the VS Code build task (C/C++: gcc build active file) if you prefer.

## Running

Start the server:
```
./bin/server <port>
```

In another terminal, start the client:
```
./bin/client <ip> <port>
```

## References

[Beej's Guide To Networking](https://beej.us/guide/bgnet/)

## Comments

There are some existing issues listed in [todo.md](todo.md)<br>
Feel free to open issues or contribute improvements!
