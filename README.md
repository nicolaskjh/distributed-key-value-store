# Distributed Key-Value Store

A high-performance, distributed key-value store implementation in C++ using gRPC.

## Features

### Core Operations
- **GET** - Retrieve a value by key
- **SET** - Store a key-value pair
- **CONTAINS** - Check if a key exists
- **DELETE** - Remove a key-value pair

### Advanced Features
- **TTL/Expiration** - Set time-to-live for keys with EXPIRE and TTL operations
- **Hybrid Persistence** - Combines RDB snapshots and AOF for durability
  - RDB: Periodic snapshots (every 60 seconds)
  - AOF: Append-only file for write operations
  - Recovery: Loads RDB snapshot then replays AOF
- **Thread-Safe** - Read-write locks for concurrent access
- **gRPC** - Fast RPC-based communication

## Requirements

- C++17 compiler
- CMake 3.15+
- gRPC
- Protocol Buffers

## Building

```bash
mkdir build && cd build
cmake ..
make -j8
```

## Running

Start the server:
```bash
./build/kvstore_server
```

Run the test client in another terminal:
```bash
./build/kvstore_client
```

The server creates two persistence files in the working directory:
- `kvstore.rdb` - Snapshot file
- `kvstore.aof` - Append-only log file

## Project Structure

```
distributed-key-value-store/
├── proto/
│   └── kvstore.proto           # gRPC service definitions
├── src/
│   ├── storage/                # Storage layer with persistence
│   │   ├── storage.cpp/h       # Thread-safe storage with TTL
│   │   ├── aof_persistence.*   # Append-only file handler
│   │   └── rdb_persistence.*   # Snapshot handler
│   ├── service/                # gRPC service implementation
│   ├── server/                 # Server wrapper
│   └── main.cpp                # Server entry point
├── tests/                      # Test clients
│   ├── client_test.cpp         # Main test suite
│   ├── snapshot_test.cpp       # Snapshot testing
│   └── verify_persistence.cpp  # Persistence verification
└── CMakeLists.txt              # Build configuration
```

## Persistence

The server uses a hybrid persistence strategy:

1. **AOF (Append-Only File)**: Every write operation (SET, DELETE, EXPIRE) is immediately appended to `kvstore.aof`
2. **RDB (Snapshots)**: A background thread saves the entire dataset to `kvstore.rdb` every 60 seconds
3. **Recovery**: On startup, the server loads the RDB snapshot first, then replays the AOF to ensure no data loss

This provides both fast recovery (from RDB) and durability (from AOF).

## Operations

### Basic Operations
```cpp
SET key value       // Store a key-value pair
GET key            // Retrieve a value
CONTAINS key       // Check if key exists
DELETE key         // Remove a key
```

### TTL Operations
```cpp
EXPIRE key seconds  // Set expiration time
TTL key            // Get remaining time (-1 if no expiration, -2 if not found)
```

Keys with expired TTL are automatically removed when accessed.

## Clean Build

```bash
rm -rf build && mkdir build && cd build && cmake .. && make -j8
```