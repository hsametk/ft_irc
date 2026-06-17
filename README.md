*This project has been created as part of the 42 curriculum by zuonen and hakotu.*

# ft_irc

## Description

ft_irc is a minimal IRC server written in C++98. It accepts multiple simultaneous client connections through a single non-blocking `poll()` loop and speaks a useful subset of the IRC protocol (RFC 1459 / 2812), enough for a real client like irssi or HexChat to connect, register, join channels, send messages, and use operator commands.

The goal of the project is to get hands-on experience with low-level network programming: TCP sockets, non-blocking I/O, multiplexing with `poll()`, partial reads, line-based protocol parsing, and per-client state management. No external networking library is used; everything is done with the C standard socket API.

Once a client is connected, the server expects the standard registration flow (`PASS`, `NICK`, `USER`). After registration, channels can be created and joined with `JOIN`, messages can be sent with `PRIVMSG`, and the channel founder receives operator status, which gives them access to channel management commands such as `TOPIC`, `INVITE`, `KICK`, and `MODE`.

## Instructions

### Requirements

- A C++ compiler that supports C++98 (`clang++` or `g++`)
- `make`
- A POSIX system (Linux or macOS)

### Build

```
make
```
This produces an executable named `ircserv` at the project root.

Helper targets:
```
make clean    # remove object files
make fclean   # remove object files and the binary
make re       # fclean followed by make
```

### Run

```
./ircserv <port> <password>
```

- `<port>` — TCP port the server will listen on (e.g. 6667)
- `<password>` — the password every client must send via `PASS` before registering

Example:

```
./ircserv 6667 secret
```
### Connecting with a real client

Any standard IRC client works. With `irssi`:

```
irssi -c 127.0.0.1 -p 6667 -w secret -n alice
```
### Connecting with netcat (manual testing)

```
nc 127.0.0.1 6667
PASS secret
NICK alice
USER alice 0 * :alice
JOIN #room
PRIVMSG #room :hello there
TOPIC #room :welcome
INVITE bob #room
PART #room :bye
QUIT
```

## Features

- Non-blocking server based on a single `poll()` loop, no threads
- Per-client receive buffer that handles partial reads and multiple commands in one packet
- Both `\r\n` and `\n` line terminators are accepted
- Registration flow: `PASS`, `NICK`, `USER`, with proper error replies
- Channel commands: `JOIN`, `PART`, `TOPIC`, `INVITE`, `PRIVMSG`
- The first user to join a channel automatically becomes its operator
- Operator-only commands return `482 ERR_CHANOPRIVSNEEDED` for non-operators
- Invite list mechanism so invited users can join `+i` (invite-only) channels
- Standard numeric replies: `001`–`004`, `331`, `332`, `341`, `375`, `372`, `376`, plus error codes `401`, `403`, `421`, `431`, `433`, `442`, `443`, `461`, `464`, `471`, `473`, `475`, `482`
- Graceful shutdown on `SIGINT` / `SIGTERM`

## Project structure

```
ft_irc/
├── include/
│   ├── Auth.hpp
│   ├── Channel.hpp
│   ├── Client.hpp
│   └── Server.hpp
├── src/
│   ├── Auth.cpp
│   ├── Channel.cpp
│   ├── Client.cpp
│   ├── Server.cpp
│   ├── ServerInit.cpp
│   ├── ServerLoop.cpp
│   └── ServerClient.cpp
├── Makefile
└── README.md
```
## Team and division of work

- **zuonen** — client buffering, command parser, `PASS` / `NICK` / `USER` validation, `Channel` class, `TOPIC`, `INVITE`, error replies, tests, README
- **hakotu** — socket setup, `poll()` loop, client lifecycle, registration state, `JOIN`, `PRIVMSG`, channel broadcast, `KICK`, `MODE`, final integration

## Resources

Classic references that we used while working on the project:

- **RFC 1459 — Internet Relay Chat Protocol.** The original IRC specification. Defines the message format, registration flow and core commands. <https://datatracker.ietf.org/doc/html/rfc1459>
- **RFC 2812 — Internet Relay Chat: Client Protocol.** A later, clearer revision focused on the client side; the numeric reply codes used in this project come from here. <https://datatracker.ietf.org/doc/html/rfc2812>
- **Modern IRC documentation by ircdocs.horse.** A practical, well-organised reference for command behavior and numeric replies. <https://modern.ircdocs.horse/>
- **Beej's Guide to Network Programming.** Used to refresh socket API details, `poll()`, non-blocking I/O, and `recv` edge cases. <https://beej.us/guide/bgnet/>
- **`man` pages** for `socket`, `bind`, `listen`, `accept`, `poll`, `fcntl`, `recv`, `send`, `setsockopt`, `signal`.
- **irssi** and **HexChat** as real-world test clients to validate compatibility.

### Use of AI

We used an AI assistant (Anthropic's Claude) as a support tool during the project. It was helpful, but every line of code that ended up in the repository was read, understood and adjusted by us before being committed. Concretely, AI was used for the following tasks:

- Sanity-checking our parser against IRC quirks such as multiple commands arriving in a single `recv`, mixed `\r\n` / `\n` line endings, and trailing parameters introduced with `:`.
- Suggesting the order of validation checks for `TOPIC` and `INVITE` (`461` → `403` → `442` → `482` → `401` → `443`) so that error replies match what real IRC servers return.
- Reviewing the design of the `Channel` class and pointing out that an invite list (`_invited` set) is needed to make `INVITE` meaningful for `+i` channels later, when `MODE` is implemented.
- Drafting the structure of this README so it covers everything required by the subject.
- Brainstorming manual test scenarios with `nc` (non-operator changing topic, inviting a user already on the channel, inviting a non-existent nick, etc.) which we then ran ourselves against the running server.

AI was **not** used to write code that we did not understand, to bypass the project's constraints (no external libraries, C++98, non-blocking I/O), or to replace any of the actual learning involved in socket programming, protocol design and team work. The architecture decisions, the debugging, and the integration between the two of us were done by us.
