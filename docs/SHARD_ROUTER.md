# Shard Router

## Overview

The `ShardRouter` is the client-facing component that routes key-value operations to the appropriate shards based on consistent hashing. It acts as a smart proxy, maintaining a connection pool to all shards and automatically directing each operation to the correct shard.

## Architecture

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │
       │ Set("user:123", "data")
       ▼
┌─────────────────────────────────────┐
│        ShardRouter                   │
│  ┌───────────────────────────────┐  │
│  │  HashRing                      │  │
│  │  GetShardForKey("user:123")   │  │
│  │  → "shard-2"                   │  │
│  └───────────────────────────────┘  │
│                                      │
│  ┌───────────────────────────────┐  │
│  │  Connection Pool               │  │
│  │  shard-1: gRPC Stub            │  │
│  │  shard-2: gRPC Stub ◄─────────┼──┼─── Reuse existing
│  │  shard-3: gRPC Stub            │  │     connection
│  └───────────────────────────────┘  │
│                                      │
│  ┌───────────────────────────────┐  │
│  │  Statistics                    │  │
│  │  Per-shard request counts      │  │
│  │  Success/failure tracking      │  │
│  └───────────────────────────────┘  │
└──────┬───────────────────────────────┘
       │
       │ RPC: Set("user:123", "data")
       ▼
┌─────────────┐
│   Shard-2   │  ← Owns "user:123"
└─────────────┘
```

## Key Components

### 1. Hash Ring Integration

The router uses a `HashRing` to determine which shard owns each key:

```cpp
// Determine target shard
std::string shard_id = hash_ring_->GetShardForKey(key);
```

**Why this matters:**
- Same key always routes to same shard (deterministic)
- Distribution handled by consistent hashing algorithm
- Adding/removing shards automatically updates routing

### 2. Connection Pooling

Instead of creating a new gRPC connection for every request, the router maintains a persistent connection pool:

```cpp
std::unordered_map<std::string, std::shared_ptr<KeyValueStore::Stub>> shard_stubs_;
```

**Connection lifecycle:**
1. First request to a shard → create connection
2. Subsequent requests → reuse existing connection
3. Connection stored in map by shard_id

**Benefits:**
- **Performance**: Avoid connection overhead (TCP handshake, TLS negotiation)
- **Resource efficiency**: Limited number of connections
- **Automatic**: Transparent to caller

**Example:**
```cpp
// First call to shard-1: creates connection
router.Set("key1", "value1");  // shard-1

// Second call to shard-1: reuses connection
router.Get("key2");  // shard-1 (no new connection!)

// First call to shard-2: creates new connection
router.Set("key3", "value3");  // shard-2
```

### 3. Thread Safety

The router is thread-safe for concurrent client requests:

```cpp
std::mutex connection_mutex_;  // Protects connection pool
std::mutex stats_mutex_;       // Protects statistics
```

**Two separate mutexes:**
- `connection_mutex_`: Guards `shard_stubs_` map (only during connection creation)
- `stats_mutex_`: Guards statistics counters (updated frequently)

**Why separate mutexes?**
- Statistics updates shouldn't block connection pool
- Most requests reuse connections (no contention on connection_mutex_)
- Fine-grained locking improves concurrency

### 4. Statistics Tracking

The router tracks operational metrics:

```cpp
struct RoutingStats {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    std::unordered_map<std::string, uint64_t> per_shard_requests;
};
```

**Use cases:**
- Monitor load distribution across shards
- Detect hot shards (receiving disproportionate traffic)
- Track error rates per shard
- Capacity planning

## Supported Operations

All standard KV operations are supported:

```cpp
// Write operations
bool Set(const std::string& key, const std::string& value);
bool Delete(const std::string& key);
bool Expire(const std::string& key, int64_t ttl_seconds);

// Read operations
std::optional<std::string> Get(const std::string& key);
bool Contains(const std::string& key);
std::optional<int64_t> TTL(const std::string& key);

// Monitoring
RoutingStats GetStats() const;
void ResetStats();
```

## Request Flow

Every operation follows the same pattern:

```
┌──────────────────────────────────────┐
│ 1. Determine Shard                   │
│    hash_ring_->GetShardForKey(key)   │
└──────────┬───────────────────────────┘
           │
           ▼
┌──────────────────────────────────────┐
│ 2. Get/Create Connection             │
│    GetShardStub(shard_id)            │
│    - Check pool                      │
│    - Create if missing               │
└──────────┬───────────────────────────┘
           │
           ▼
┌──────────────────────────────────────┐
│ 3. Build Request                     │
│    SetRequest request;               │
│    request.set_key(key);             │
│    request.set_value(value);         │
└──────────┬───────────────────────────┘
           │
           ▼
┌──────────────────────────────────────┐
│ 4. Make RPC Call                     │
│    stub->Set(&context, request, &reply)│
└──────────┬───────────────────────────┘
           │
           ▼
┌──────────────────────────────────────┐
│ 5. Update Statistics                 │
│    stats.total_requests++            │
│    stats.successful_requests++       │
└──────────────────────────────────────┘
```

## Error Handling

The router handles failures gracefully:

### Connection Failures
```cpp
if (!stub) {
    std::cerr << "Failed to create connection to shard '" 
              << shard_id << "'" << std::endl;
    UpdateStats(false, shard_id);
    return false;
}
```

**Scenarios:**
- Shard server not running
- Network partition
- Invalid address

**Current behavior:**
- Log error message
- Update failure statistics
- Return failure to client

### RPC Failures
```cpp
if (!status.ok()) {
    std::cerr << "RPC failed for key '" << key 
              << "': " << status.error_message() << std::endl;
    UpdateStats(false, shard_id);
    return false;
}
```

**Scenarios:**
- Request timeout
- Shard overloaded
- Key doesn't exist (for Get)

## Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Set/Get/Delete | O(log N) | Hash ring lookup |
| Connection creation | O(1) | Amortized - only first request |
| Statistics update | O(1) | Map lookup |

**Scalability:**
- **Shards**: O(log N) lookup scales to hundreds of shards
- **Concurrent clients**: Fine-grained locking allows high concurrency
- **Connection pool**: One connection per shard (not per client)

## Testing

### Unit Test
`tests/test_shard_router.cpp` demonstrates:

1. **Routing logic**: Keys correctly distributed across shards
2. **Consistent hashing**: Same key → same shard every time
3. **Connection pooling**: Reuses connections for same shard
4. **Statistics**: Tracks per-shard distribution
5. **Dynamic sharding**: Adding shards updates routing

**Sample output:**
```
Per-Shard Distribution:
       shard-1:    12 requests (40.0%)
       shard-2:    16 requests (53.3%)
       shard-3:     2 requests (6.7%)
```

**Run test:**
```bash
cd build
./test_shard_router
```

### Integration Testing

To test with actual running shards:

```bash
# Terminal 1: Start shard-1
./kvstore_server --port=50051 --data-dir=/tmp/shard1

# Terminal 2: Start shard-2
./kvstore_server --port=50052 --data-dir=/tmp/shard2

# Terminal 3: Start shard-3
./kvstore_server --port=50053 --data-dir=/tmp/shard3

# Terminal 4: Run router test
./test_shard_router
```

## Configuration

```cpp
// Create hash ring
auto hash_ring = std::make_shared<HashRing>(150);  // 150 virtual nodes

// Add shards
hash_ring->AddShard("shard-1", "localhost:50051");
hash_ring->AddShard("shard-2", "localhost:50052");
hash_ring->AddShard("shard-3", "localhost:50053");

// Create router
ShardRouter router(hash_ring);

// Use router
router.Set("user:123", "John Doe");
auto value = router.Get("user:123");
```

## Monitoring

### Statistics API

```cpp
auto stats = router.GetStats();

std::cout << "Total requests: " << stats.total_requests << std::endl;
std::cout << "Success rate: " 
          << (100.0 * stats.successful_requests / stats.total_requests) 
          << "%" << std::endl;

for (const auto& [shard_id, count] : stats.per_shard_requests) {
    double percentage = (100.0 * count) / stats.total_requests;
    std::cout << shard_id << ": " << count 
              << " (" << percentage << "%)" << std::endl;
}
```

### Identifying Hot Shards

A "hot shard" receives disproportionate traffic:

```cpp
auto stats = router.GetStats();
double avg_per_shard = stats.total_requests / stats.per_shard_requests.size();

for (const auto& [shard_id, count] : stats.per_shard_requests) {
    if (count > avg_per_shard * 1.5) {  // 50% above average
        std::cout << "Hot shard detected: " << shard_id << std::endl;
    }
}
```

**Solutions for hot shards:**
- Split hot shard into multiple shards
- Add more virtual nodes for better distribution
- Use application-level key design (avoid sequential keys)

## Implementation Details

### Connection Creation

```cpp
std::shared_ptr<KeyValueStore::Stub> ShardRouter::CreateShardConnection(
    const std::string& address) 
{
    auto channel = grpc::CreateChannel(
        address, 
        grpc::InsecureChannelCredentials()
    );
    return KeyValueStore::NewStub(channel);
}
```

**Channel options** (potential optimization):
```cpp
grpc::ChannelArguments args;
args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);  // 10 sec keepalive
args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

auto channel = grpc::CreateCustomChannel(
    address, 
    grpc::InsecureChannelCredentials(), 
    args
);
```

### Thread-Safe Statistics

```cpp
void ShardRouter::UpdateStats(bool success, const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_requests++;
    if (success) {
        stats_.successful_requests++;
    } else {
        stats_.failed_requests++;
    }
    stats_.per_shard_requests[shard_id]++;
}
```

**Why lock_guard?**
- RAII-style lock (exception-safe)
- Automatic unlock when leaving scope
- Simple and correct

---

## Summary

The `ShardRouter` provides:
✅ Transparent routing based on consistent hashing  
✅ Connection pooling for performance  
✅ Thread-safe concurrent access  
✅ Statistics for monitoring  
✅ Clean API matching single-node interface  

**Key insight:** Clients interact with the router exactly like a single-node KV store, but operations are automatically distributed across shards for horizontal scalability.
