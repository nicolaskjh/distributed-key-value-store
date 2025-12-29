#include "hash_ring.h"
#include <iostream>
#include <sstream>

namespace kvstore {

HashRing::HashRing(int virtual_nodes_per_shard)
    : virtual_nodes_per_shard_(virtual_nodes_per_shard) {
    if (virtual_nodes_per_shard_ <= 0) {
        virtual_nodes_per_shard_ = 150; // Default
    }
}

bool HashRing::AddShard(const std::string& shard_id, const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if shard already exists
    if (shards_.find(shard_id) != shards_.end()) {
        std::cerr << "Shard already exists: " << shard_id << std::endl;
        return false;
    }
    
    // Create shard info
    ShardInfo shard_info(shard_id, address);
    shards_[shard_id] = shard_info;
    
    // Add virtual nodes to the ring
    for (int i = 0; i < virtual_nodes_per_shard_; ++i) {
        std::string vnode_key = GetVirtualNodeKey(shard_id, i);
        uint32_t hash = ComputeHash(vnode_key);
        ring_[hash] = shard_id;
    }
    
    std::cout << "Added shard '" << shard_id << "' to hash ring with " 
              << virtual_nodes_per_shard_ << " virtual nodes" << std::endl;
    
    return true;
}

bool HashRing::RemoveShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if shard exists
    if (shards_.find(shard_id) == shards_.end()) {
        std::cerr << "Shard not found: " << shard_id << std::endl;
        return false;
    }
    
    // Remove virtual nodes from the ring
    for (int i = 0; i < virtual_nodes_per_shard_; ++i) {
        std::string vnode_key = GetVirtualNodeKey(shard_id, i);
        uint32_t hash = ComputeHash(vnode_key);
        ring_.erase(hash);
    }
    
    // Remove shard info
    shards_.erase(shard_id);
    
    std::cout << "Removed shard '" << shard_id << "' from hash ring" << std::endl;
    
    return true;
}

std::string HashRing::GetShardForKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (ring_.empty()) {
        return "";
    }
    
    // Compute hash of the key
    uint32_t hash = ComputeHash(key);
    
    // Find the first node on the ring >= hash (clockwise search)
    auto it = ring_.lower_bound(hash);
    
    // If we're past the end, wrap around to the beginning
    if (it == ring_.end()) {
        it = ring_.begin();
    }
    
    return it->second;
}

const ShardInfo* HashRing::GetShard(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shards_.find(shard_id);
    if (it != shards_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ShardInfo> HashRing::GetAllShards() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ShardInfo> result;
    result.reserve(shards_.size());
    
    for (const auto& [shard_id, shard_info] : shards_) {
        result.push_back(shard_info);
    }
    
    return result;
}

size_t HashRing::GetShardCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shards_.size();
}

bool HashRing::IsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shards_.empty();
}

uint32_t HashRing::ComputeHash(const std::string& data) const {
    // Simple but effective hash function (FNV-1a)
    // Good distribution for consistent hashing
    uint32_t hash = 2166136261u;
    
    for (char c : data) {
        hash ^= static_cast<uint32_t>(c);
        hash *= 16777619u;
    }
    
    return hash;
}

std::string HashRing::GetVirtualNodeKey(const std::string& shard_id, int virtual_index) const {
    std::ostringstream oss;
    oss << shard_id << ":" << virtual_index;
    return oss.str();
}

} // namespace kvstore
