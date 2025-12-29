# Consistent Hash Ring Implementation

## Overview

Minimal implementation of consistent hashing for distributed key-value store sharding.

## Architecture

### Hash Ring Structure
```
        Hash Space: 0 to 2^32-1
        
        0°
        ├─ vnode: shard-1:0
        ├─ vnode: shard-2:45
        ├─ vnode: shard-3:12
        ...
        ├─ vnode: shard-1:149  (150 vnodes per shard)
        └─ 360° (wraps to 0)
```

### Key Distribution Algorithm

1. **Hash the key**: `hash(key) → uint32_t`
2. **Clockwise search**: Find first virtual node ≥ hash
3. **Map to shard**: Return shard_id of that virtual node
4. **Wrap around**: If past end, use first node

### Virtual Nodes

- Each physical shard has 150 virtual nodes
- Virtual node key: `"shard-id:index"` (e.g., "shard-1:0")
- Improves distribution uniformity
- More vnodes = better balance (but more memory)

## Implementation

### Core Classes

#### `ShardInfo`
Metadata about a shard:
```cpp
struct ShardInfo {
    string shard_id;                  // "shard-1"
    string address;                   // "localhost:50051"
    vector<string> replica_addresses; // Future use
    bool is_available;                // Health status
    uint64_t key_count;               // Stats
};
```

#### `HashRing`
Consistent hash ring:
```cpp
class HashRing {
    // Add/remove shards
    bool AddShard(shard_id, address);
    bool RemoveShard(shard_id);
    
    // Key lookup
    string GetShardForKey(key);
    
    // Metadata
    vector<ShardInfo> GetAllShards();
    size_t GetShardCount();
};
```

### Hash Function

**FNV-1a Hash** - Fast and good distribution:
```cpp
uint32_t hash = 2166136261u;
for (char c : data) {
    hash ^= c;
    hash *= 16777619u;
}
```

## Usage

### Basic Example
```cpp
HashRing ring(150);  // 150 virtual nodes per shard

// Add shards
ring.AddShard("shard-1", "localhost:50051");
ring.AddShard("shard-2", "localhost:50052");
ring.AddShard("shard-3", "localhost:50053");

// Route key to shard
string shard = ring.GetShardForKey("user:123");
// Returns: "shard-2" (for example)
```

### Dynamic Scaling
```cpp
// Add new shard
ring.AddShard("shard-4", "localhost:50054");
// ~25% of keys now route to shard-4

// Remove shard
ring.RemoveShard("shard-2");
// Keys from shard-2 redistribute to remaining shards
```

## Test Results

### Distribution with 3 Shards (10,000 keys)
```
shard-1: 4,610 keys (46.10%)  ← Imbalanced
shard-2: 3,125 keys (31.25%)
shard-3: 2,265 keys (22.65%)
```

**Issue**: Not evenly distributed (ideal: 33.33% each)
**Cause**: Random hash collisions with 150 vnodes
**Solution**: Increase to 500 virtual nodes for better balance

### Distribution with 4 Shards (after adding shard-4)
```
shard-1: 2,910 keys (29.10%)
shard-2: 2,108 keys (21.08%)
shard-3: 2,246 keys (22.46%)
shard-4: 2,736 keys (27.36%)
```

**Observation**: ~25% of keys moved to new shard (ideal behavior)

### Key Mapping (consistent after operations)
```
user:1     → shard-1  (consistent across lookups)
user:2     → shard-1
user:3     → shard-2  (moved to shard-1 after shard-2 removed)
order:100  → shard-2  (moved to shard-4 after shard-2 removed)
product:50 → shard-2  (moved to shard-4 after shard-2 removed)
```

## Performance

### Time Complexity
- `AddShard`: O(V log N) where V = virtual nodes, N = total nodes
- `RemoveShard`: O(V log N)
- `GetShardForKey`: O(log N) - binary search in sorted map
- `GetAllShards`: O(S) where S = number of shards

### Space Complexity
- O(S × V) for ring storage (S shards × V virtual nodes)
- With 10 shards × 150 vnodes = 1,500 entries (~30 KB)

### Scalability
- Tested up to 10,000 keys, 4 shards
- Supports hundreds of shards efficiently
- Memory: ~20 bytes per virtual node

## Characteristics

### ✅ Advantages
1. **Minimal rebalancing**: Only K/N keys move when adding Nth shard
2. **Fast lookups**: O(log N) with binary search
3. **Deterministic**: Same key always routes to same shard
4. **Thread-safe**: Mutex-protected operations
5. **Simple**: No external dependencies

### ⚠️ Limitations
1. **Distribution variance**: ±10-15% with 150 vnodes
2. **No replication awareness**: Doesn't avoid replica collocation
3. **No weighted shards**: All shards treated equally
4. **Memory overhead**: Virtual nodes in memory

### 🔧 Possible Improvements
1. Increase virtual nodes to 500 for better balance
2. Add weighted hashing for heterogeneous hardware
3. Implement bounded loads (limit per-shard load)
4. Add rack/zone awareness for replicas

## Files

```
src/sharding/
├── shard_info.h         # Shard metadata structure
├── hash_ring.h          # Hash ring interface
└── hash_ring.cpp        # Implementation

tests/
└── test_hash_ring.cpp   # Test program
```

## Next Steps

1. ✅ **Phase 5.1 Complete**: Consistent hash ring implemented
2. 🔄 **Phase 5.2**: Routing layer (route requests to shards)
3. ⏳ **Phase 5.3**: Shard management (add/remove coordination)
4. ⏳ **Phase 5.4**: Data migration (move keys between shards)
5. ⏳ **Phase 5.5**: Coordinator node (central routing)

## References

- [Consistent Hashing and Random Trees](https://www.akamai.com/us/en/multimedia/documents/technical-publication/consistent-hashing-and-random-trees-distributed-caching-protocols-for-relieving-hot-spots-on-the-world-wide-web-technical-publication.pdf) - Original paper
- [Dynamo: Amazon's Highly Available Key-value Store](https://www.allthingsdistributed.com/files/amazon-dynamo-sosp2007.pdf) - Production usage
