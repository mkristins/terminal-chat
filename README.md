# terminal-chat

Terminal-chat is a project that allows multiple people to communicate inside a terminal.

## Feature list

Server can receive client messages and handle lobby-based stuff.

Clients can send/receive messages and join lobbies.

## Command list

- `/help`: show command list
- ­`/list`: show lobby id, name, member count, capacity
- `/users`: show user id, username, and lobby status
- `/create <name>`: create lobby, join creator to it, return lobby id
- `/join <id>`: move user into lobby if it exists and is not full
- `/leave`: leave current lobby
- `/pm <userid> <message>` : send private message
- Plain text: broadcast to current lobby only
- `/quit`: client disconnects cleanly

## Compile instructions

`g++ -std=c++23 -Wall -Wextra -pedantic -pthread server.cpp message.cpp -o server` 

`g++ -std=c++23 -Wall -Wextra -pedantic -pthread client.cpp -o client`

`./server ./client localhost 4040`