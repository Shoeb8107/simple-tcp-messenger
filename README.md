# Simple TCP Messenger

A lightweight TCP chat application for Windows built with C++ and Winsock.

This repository contains:
- **Server** (`server/server.cpp`): accepts multiple clients, relays chat messages, and provides server-side moderation/utility commands.
- **Client** (`client/client.cpp`): connects to the server, sends messages, and receives chat updates in real time.

## Features

- Multi-client TCP chat over `127.0.0.1:55555`
- Username on connect
- Broadcast messaging
- Private whispers (`/w <user> <message>`)
- Client disconnect command (`/exit`)
- Server commands:
  - `/kick <username>`: remove a user from chat
  - `/w <username> <message>`: whisper from server to one client
  - `/list`: list currently connected users
- Maximum concurrent clients: **5**

## Project Structure

```text
simple-tcp-messenger/
├── client/
│   ├── client.cpp
│   └── client.vcxproj
├── server/
│   ├── server.cpp
│   └── server.vcxproj
├── simple-tcp-messenger.slnx
└── LICENSE.txt
```

## Requirements

- Windows (WinSock2 APIs are used directly)
- Visual Studio with C++ desktop development tools

## Build (Visual Studio)

1. Open `simple-tcp-messenger.slnx` in Visual Studio.
2. Select your desired configuration (e.g., `Debug | x64`).
3. Build the solution (`Build > Build Solution`).

## Run

1. Start the **server** executable first.
2. Start one or more **client** executables.
3. In each client, enter a username when prompted.

Default connection settings are hardcoded in source:
- Host: `127.0.0.1`
- Port: `55555`

If needed, edit these in:
- `server/server.cpp` (bind/listen endpoint)
- `client/client.cpp` (connect endpoint)

## Usage

### Client Commands

- `/w <username> <message>` — send a private message to one user.
- `/exit` — disconnect from the chat.

### Server Console Commands

- `/kick <username>` — disconnect a client.
- `/w <username> <message>` — send a private message from the server.
- `/list` — print the current connected-client list.
- Any other text — broadcast a server message to all connected clients.

## Notes

- This is a simple educational project and does not include authentication, encryption, or robust error handling.
- Messages use a fixed buffer size (`512` bytes), so very long messages may be truncated.

## License

This project is licensed under the terms in [`LICENSE.txt`](LICENSE.txt).
