# Distributed Key-Value Store

A high-performance, distributed key-value store implementation in C++ using gRPC.

## Features

- ✅ **GET** - Retrieve a value by key
- ✅ **SET** - Store a key-value pair
- ✅ **CONTAINS** - Check if a key exists
- ✅ **DELETE** - Remove a key-value pair
- ✅ **Thread-Safe** - Concurrent access support
- ✅ **gRPC** - Fast RPC-based communication

## Building

```bash
mkdir build && cd build
cmake ..
make -j8
```

## Running

**Start the server:**
```bash
./build/kvstore_server
```

**Run the test client (in another terminal):**
```bash
./build/kvstore_client
```

## Project Structure

```
distributed-key-value-store/
├── proto/
│   └── kvstore.proto        # gRPC service definitions
├── src/
│   ├── storage/             # Thread-safe storage layer
│   ├── service/             # gRPC service implementation
│   ├── server/              # Server wrapper
│   ├── client/              # Test client
│   └── main.cpp             # Server entry point
└── CMakeLists.txt           # Build configuration
```

## Clean Build

```bash
rm -rf build && mkdir build && cd build && cmake .. && make -j8
```