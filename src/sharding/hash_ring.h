#pragma once

#include "shard_info.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>
#include <cstdint>

namespace kvstore {

/**
 * Consistent hash ring for distributing keys across shards
 * Uses virtual nodes to ensure uniform distribution
 */
class HashRing {
public:
    /**
     * Create a hash ring with specified number of virtual nodes per physical node
     * @param virtual_nodes_per_shard Number of virtual nodes (default: 150)
     */
    explicit HashRing(int virtual_nodes_per_shard = 150);
    
    /**
     * Add a shard to the hash ring
     * @param shard_id Unique identifier for the shard
     * @param address Network address of the shard (host:port)
     * @return true if added successfully
     */
    bool AddShard(const std::string& shard_id, const std::string& address);
    
    /**
     * Remove a shard from the hash ring
     * @param shard_id Identifier of the shard to remove
     * @return true if removed successfully
     */
    bool RemoveShard(const std::string& shard_id);
    
    /**
     * Find which shard owns a given key
     * @param key The key to look up
     * @return Shard ID that owns this key, or empty string if no shards
     */
    std::string GetShardForKey(const std::string& key) const;
    
    /**
     * Get information about a specific shard
     * @param shard_id The shard to query
     * @return ShardInfo if found, nullptr otherwise
     */
    const ShardInfo* GetShard(const std::string& shard_id) const;
    
    /**
     * Get all shards in the ring
     * @return Vector of all shard information
     */
    std::vector<ShardInfo> GetAllShards() const;
    
    /**
     * Get number of shards in the ring
     */
    size_t GetShardCount() const;
    
    /**
     * Check if ring is empty
     */
    bool IsEmpty() const;
    
private:
    /**
     * Compute hash value for a string
     * Uses simple but effective hash function
     */
    uint32_t ComputeHash(const std::string& data) const;
    
    /**
     * Generate virtual node key for hash computation
     */
    std::string GetVirtualNodeKey(const std::string& shard_id, int virtual_index) const;
    
    // Number of virtual nodes per physical shard
    int virtual_nodes_per_shard_;
    
    // Hash ring: hash value -> shard_id
    // Ordered map for efficient range queries
    std::map<uint32_t, std::string> ring_;
    
    // Shard metadata: shard_id -> ShardInfo
    std::unordered_map<std::string, ShardInfo> shards_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace kvstore
