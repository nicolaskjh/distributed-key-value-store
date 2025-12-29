#pragma once

#include "hash_ring.h"
#include "../storage/storage.h"
#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace kvstore {

/**
 * Routes requests to appropriate shards based on consistent hashing
 * 
 * The ShardRouter maintains:
 * 1. A hash ring to determine which shard owns each key
 * 2. gRPC client connections to all shards
 * 3. Connection pooling for efficient communication
 */
class ShardRouter {
public:
    /**
     * Create a router with an existing hash ring
     * @param hash_ring Shared pointer to the hash ring
     */
    explicit ShardRouter(std::shared_ptr<HashRing> hash_ring);
    
    ~ShardRouter();
    
    // Disable copy
    ShardRouter(const ShardRouter&) = delete;
    ShardRouter& operator=(const ShardRouter&) = delete;
    
    /**
     * Route a SET operation to the appropriate shard
     * @param key The key to set
     * @param value The value to store
     * @return true if successful
     */
    bool Set(const std::string& key, const std::string& value);
    
    /**
     * Route a GET operation to the appropriate shard
     * @param key The key to retrieve
     * @return The value if found, std::nullopt otherwise
     */
    std::optional<std::string> Get(const std::string& key);
    
    /**
     * Route a DELETE operation to the appropriate shard
     * @param key The key to delete
     * @return true if key existed and was deleted
     */
    bool Delete(const std::string& key);
    
    /**
     * Route a CONTAINS operation to the appropriate shard
     * @param key The key to check
     * @return true if key exists
     */
    bool Contains(const std::string& key);
    
    /**
     * Route an EXPIRE operation to the appropriate shard
     * @param key The key to expire
     * @param seconds TTL in seconds
     * @return true if successful
     */
    bool Expire(const std::string& key, int seconds);
    
    /**
     * Route a TTL operation to the appropriate shard
     * @param key The key to query
     * @return Seconds remaining, -1 if no expiration, -2 if not found
     */
    int TTL(const std::string& key);
    
    /**
     * Get routing statistics
     */
    struct RoutingStats {
        uint64_t total_requests;
        uint64_t successful_requests;
        uint64_t failed_requests;
        std::unordered_map<std::string, uint64_t> per_shard_requests;
    };
    
    RoutingStats GetStats() const;
    void ResetStats();
    
private:
    /**
     * Get or create a gRPC stub for a shard
     * Uses connection pooling to reuse connections
     */
    std::unique_ptr<KeyValueStore::Stub>& GetShardStub(const std::string& shard_id);
    
    /**
     * Create a new gRPC channel and stub for a shard
     */
    void CreateShardConnection(const std::string& shard_id, const std::string& address);
    
    /**
     * Remove a shard connection (when shard is removed)
     */
    void RemoveShardConnection(const std::string& shard_id);
    
    // Hash ring for determining shard ownership
    std::shared_ptr<HashRing> hash_ring_;
    
    // Connection pool: shard_id -> gRPC stub
    std::unordered_map<std::string, std::unique_ptr<KeyValueStore::Stub>> shard_stubs_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    RoutingStats stats_;
    
    // Connection mutex
    std::mutex connection_mutex_;
};

} // namespace kvstore
