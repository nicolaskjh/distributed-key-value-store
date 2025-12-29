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
- **Async Master-Replica Replication** - Distribute reads across multiple nodes
  - Master node handles all writes
  - Replica nodes receive updates asynchronously
  - Eventually consistent reads from replicas
- **Sharding** - Horizontal partitioning for scalability
  - **Consistent Hashing** - Hash ring with virtual nodes for uniform distribution
  - **Routing Layer** - Client-side routing with connection pooling
  - Dynamic shard addition/removal with minimal rebalancing
  - O(log N) key lookup performance
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

### Single Node Mode

Start the server:
```bash
./build/kvstore_server
```

Run the test client in another terminal:
```bash
./build/kvstore_client
```

### Distributed Mode (Master-Replica)

Start a master node:
```bash
./build/kvstore_server --master --address 0.0.0.0:50051 --replicas localhost:50052,localhost:50053
```

Start replica nodes in separate terminals:
```bash
# Replica 1
mkdir -p replica1 && cd replica1
../build/kvstore_server --replica --address 0.0.0.0:50052 --master-address localhost:50051

# Replica 2 (in another terminal)
mkdir -p replica2 && cd replica2
../build/kvstore_server --replica --address 0.0.0.0:50053 --master-address localhost:50051
```

Write to master, read from any node:
```bash
./build/kvstore_client localhost:50051  # Write to master
./build/read_test localhost:50052       # Read from replica
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
│   ├── persistence/            # Persistence layer
│   │   ├── aof_persistence.*   # Append-only file handler
│   │   └── rdb_persistence.*   # Snapshot handler
│   ├── storage/                # Storage layer
│   │   └── storage.cpp/h       # Thread-safe storage with TTL
│   ├── replication/            # Replication layer
│   │   └── replication_manager.* # Master-replica replication
│   ├── sharding/               # Sharding layer
│   │   ├── shard_info.h        # Shard metadata
│   │   ├── hash_ring.*         # Consistent hashing
│   │   └── shard_router.*      # Client-side routing
│   ├── service/                # gRPC service implementation
│   ├── server/                 # Server wrapper
│   └── main.cpp                # Server entry point
├── tests/                      # Test suite
│   ├── test_all.sh             # Run all tests
│   ├── client_test.cpp         # Test client implementation
│   ├── snapshot_test.cpp       # Snapshot test client
│   ├── verify_persistence.cpp  # Persistence verification client
│   ├── read_test.cpp           # Read-only test client
│   ├── test_hash_ring.cpp      # Hash ring unit test
│   ├── test_shard_router.cpp   # Shard router unit test
│   └── README.md               # Test documentation
├── docs/                       # Documentation
│   ├── HASH_RING.md            # Consistent hashing details
│   ├── SHARD_ROUTER.md         # Routing layer details
│   └── REPLICATION_ARCHITECTURE.md # Replication details
└── CMakeLists.txt              # Build configuration
```

## Persistence

The server uses a hybrid persistence strategy:

1. **AOF (Append-Only File)**: Every write operation (SET, DELETE, EXPIRE) is immediately appended to `kvstore.aof`
2. **RDB (Snapshots)**: A background thread saves the entire dataset to `kvstore.rdb` every 60 seconds
3. **Recovery**: On startup, the server loads the RDB snapshot first, then replays the AOF to ensure no data loss

This provides both fast recovery (from RDB) and durability (from AOF).

## Replication Architecture

Asynchronous master-replica replication for horizontal read scaling.

### Architecture

```
                    ┌─────────────┐
                    │   CLIENT    │
                    └──────┬──────┘
                           │
                    WRITES │ READS
                           │
                    ┌──────▼──────┐
                    │   MASTER    │ ◄─── Accepts ALL writes
                    │  (Port 50051)│
                    └──────┬──────┘
                           │
            Async Replication (Fire & Forget)
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
    ┌────▼────┐       ┌────▼────┐      ┌────▼────┐
    │ REPLICA │       │ REPLICA │      │ REPLICA │
    │   #1    │       │   #2    │      │   #3    │
    │(50052)  │       │(50053)  │      │(50054)  │
    └─────────┘       └─────────┘      └─────────┘
         ▲                 ▲                 ▲
         │                 │                 │
         └─────────────────┴─────────────────┘
                    READ requests
```

### Node Roles

**Master:**
- Accepts all write operations (SET, DELETE, EXPIRE)
- Applies operations locally and responds immediately
- Replicates to all replicas asynchronously
- Can serve read requests

**Replicas:**
- Read-only from client perspective
- Only accept ReplicationCommand RPCs from master
- Apply operations in sequence ID order
- Serve read requests to distribute load

### Replication Protocol

```protobuf
message ReplicationCommand {
  CommandType type = 1;      // SET, DELETE, or EXPIRE
  string key = 2;
  string value = 3;
  int32 seconds = 4;
  int64 sequence_id = 5;     // Monotonically increasing
}
```

Operations are assigned sequence IDs by master to ensure consistent ordering across all replicas.

**Consistency Model:** Eventual consistency - replicas may lag behind master by network latency + processing time.

## Sharding Architecture

Horizontal partitioning of data across multiple shards for scalability.

### Architecture

```
                    ┌─────────────┐
                    │   CLIENT    │
                    └──────┬──────┘
                           │
                    ┌──────▼──────────────┐
                    │   ShardRouter       │
                    │  ┌──────────────┐   │
                    │  │  Hash Ring   │   │  ← Determines shard
                    │  │  (150 vnodes)│   │     for each key
                    │  └──────────────┘   │
                    │  ┌──────────────┐   │
                    │  │ Connection   │   │  ← Pools gRPC
                    │  │    Pool      │   │     connections
                    │  └──────────────┘   │
                    └──┬──────┬──────┬────┘
                       │      │      │
            ┌──────────┘      │      └──────────┐
            │                 │                 │
       ┌────▼────┐       ┌────▼────┐      ┌────▼────┐
       │ SHARD-1 │       │ SHARD-2 │      │ SHARD-3 │
       │(50051)  │       │(50052)  │      │(50053)  │
       │ Keys:   │       │ Keys:   │      │ Keys:   │
       │ user:*  │       │ order:* │      │ prod:*  │
       └─────────┘       └─────────┘      └─────────┘
```

### Components

**Hash Ring:**
- Consistent hashing with virtual nodes (150 per shard)
- FNV-1a hash function for uniform distribution
- O(log N) lookup using binary search
- Dynamic shard addition/removal
- See [docs/HASH_RING.md](docs/HASH_RING.md)

**Shard Router:**
- Client-side routing based on hash ring
- Connection pooling for performance (reuse gRPC stubs)
- Statistics tracking (per-shard request counts, success/failure rates)
- Thread-safe for concurrent clients
- Transparent API matching single-node interface
- See [docs/SHARD_ROUTER.md](docs/SHARD_ROUTER.md)

### Example Usage

```cpp
// Create hash ring with 3 shards
auto hash_ring = std::make_shared<HashRing>(150);
hash_ring->AddShard("shard-1", "localhost:50051");
hash_ring->AddShard("shard-2", "localhost:50052");
hash_ring->AddShard("shard-3", "localhost:50053");

// Create router
ShardRouter router(hash_ring);

// Use like single-node store
router.Set("user:123", "John Doe");     // → shard-1
router.Set("order:456", "Order data");  // → shard-2
auto value = router.Get("user:123");    // → shard-1 (consistent!)

// Monitor distribution
auto stats = router.GetStats();
// Shows requests per shard, success/failure rates
```

### Testing

```bash
# Test hash ring distribution
./build/test_hash_ring

# Test routing logic (shards don't need to be running)
./build/test_shard_router
```

## Testing

The project includes a comprehensive test suite with unit and integration tests:

```bash
cd tests
./test_all.sh
```

This runs all tests including:
- **Unit tests**: Hash ring, shard router (no server required)
- **Integration tests**: Basic operations, TTL, persistence, replication, concurrency

Individual tests can be run directly:

```bash
cd build

# Unit tests
./test_hash_ring       # Test consistent hashing
./test_shard_router    # Test routing logic

# Integration tests (start server first)
./kvstore_server --master --address 0.0.0.0:50051  # Terminal 1
./kvstore_client localhost:50051                    # Terminal 2
```

See [tests/README.md](tests/README.md) for detailed test documentation.

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