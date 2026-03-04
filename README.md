# VoidStore

VoidStore is a Redis-compatible in-memory key-value database server built from scratch in C++.

The project implements core database infrastructure including an event-driven networking layer using `epoll`, Redis RESP protocol parsing, command dispatching, TTL expiration, and append-only persistence.

This project was built as a systems programming exercise to understand how high-performance backend infrastructure like Redis works internally.

---

## Features

- Redis RESP protocol support
- Multi-client TCP server
- Event-driven architecture using `epoll`
- In-memory key-value storage
- TTL key expiration
- Append-Only File (AOF) persistence
- Redis compatible commands via `redis-cli`

Supported commands:

```

SET key value
SET key value ttl
GET key
DEL key

```

---

## Architecture

The project is organized in modular layers similar to real database systems.

```

Client (redis-cli)
│
▼
TCP Socket Server
│
▼
epoll Event Loop
│
▼
RESP Protocol Parser
│
▼
Command Dispatcher
│
▼
In-Memory Key Value Store
│
▼
TTL Expiration + AOF Persistence

```

---

## Project Structure

```

voidstore/
│
├── src/
│   main.cpp
│   server.cpp
│   resp_parser.cpp
│   commands.cpp
│   storage.cpp
│   persistence.cpp
│
├── include/
│   server.h
│   resp_parser.h
│   commands.h
│   storage.h
│   persistence.h
│
├── db.aof
└── README.md

````

---

## Build

Compile the project:

```bash
g++ src/*.cpp -Iinclude -o server
````

Run the server:

```bash
./server
```

---

## Testing with redis-cli

Install redis-cli if needed.

```
redis-cli -p 8080
```

Example commands:

```
SET name akshay
GET name
DEL name
```

---

## Persistence

VoidStore uses an **Append-Only File (AOF)** to store commands.

Every write operation is logged:

```
SET name akshay
DEL name
```

When the server starts, the database state is rebuilt by replaying the AOF log.

---

## TTL Expiration

Keys can expire automatically:

```
SET session abc123 10
```

This sets a key that expires in **10 seconds**.

---

## Technologies Used

* C++
* POSIX sockets
* epoll (Linux event-driven I/O)
* Redis RESP protocol
* STL containers

---

## Future Improvements

Planned features:

* Redis LIST data structure
* Redis SET data structure
* LRANGE and LPUSH commands
* Background expiration cleaner
* RDB snapshot persistence
* Non-blocking sockets
* Better RESP streaming parser
* Replication support
