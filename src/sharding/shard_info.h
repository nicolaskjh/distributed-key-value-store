#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace kvstore {

/**
 * Information about a shard in the cluster
 */
struct ShardInfo {
    std::string shard_id;                       // Unique identifier for the shard
    std::string address;                        // Primary address (host:port)
    std::vector<std::string> replica_addresses; // Replica addresses
    bool is_available;                          // Health status
    uint64_t key_count;                         // Approximate number of keys
    
    ShardInfo() 
        : is_available(true), key_count(0) {}
    
    ShardInfo(const std::string& id, const std::string& addr)
        : shard_id(id), address(addr), is_available(true), key_count(0) {}
};

} // namespace kvstore
