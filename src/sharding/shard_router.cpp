#include "shard_router.h"
#include <iostream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace kvstore {

ShardRouter::ShardRouter(std::shared_ptr<HashRing> hash_ring)
    : hash_ring_(hash_ring) {
    stats_.total_requests = 0;
    stats_.successful_requests = 0;
    stats_.failed_requests = 0;
    
    // Create connections to all existing shards
    auto shards = hash_ring_->GetAllShards();
    for (const auto& shard : shards) {
        CreateShardConnection(shard.shard_id, shard.address);
    }
    
    std::cout << "ShardRouter initialized with " << shards.size() << " shards" << std::endl;
}

ShardRouter::~ShardRouter() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    shard_stubs_.clear();
}

bool ShardRouter::Set(const std::string& key, const std::string& value) {
    // Step 1: Determine which shard owns this key using the hash ring
    std::string shard_id = hash_ring_->GetShardForKey(key);
    
    if (shard_id.empty()) {
        std::cerr << "No shard available for key: " << key << std::endl;
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    // Step 2: Get the gRPC stub for this shard (creates connection if needed)
    auto& stub = GetShardStub(shard_id);
    if (!stub) {
        std::cerr << "Failed to get stub for shard: " << shard_id << std::endl;
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    // Step 3: Build the gRPC request
    SetRequest request;
    request.set_key(key);
    request.set_value(value);
    
    SetResponse response;
    ClientContext context;
    
    // Step 4: Make the RPC call to the shard
    Status status = stub->Set(&context, request, &response);
    
    // Step 5: Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
        stats_.per_shard_requests[shard_id]++;
        
        if (status.ok() && response.success()) {
            stats_.successful_requests++;
            return true;
        } else {
            stats_.failed_requests++;
            if (!status.ok()) {
                std::cerr << "RPC failed for key '" << key << "' on shard '" 
                          << shard_id << "': " << status.error_message() << std::endl;
            }
            return false;
        }
    }
}

std::optional<std::string> ShardRouter::Get(const std::string& key) {
    // Similar pattern: hash ring lookup → get stub → make RPC call
    std::string shard_id = hash_ring_->GetShardForKey(key);
    
    if (shard_id.empty()) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return std::nullopt;
    }
    
    auto& stub = GetShardStub(shard_id);
    if (!stub) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return std::nullopt;
    }
    
    GetRequest request;
    request.set_key(key);
    
    GetResponse response;
    ClientContext context;
    
    Status status = stub->Get(&context, request, &response);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
        stats_.per_shard_requests[shard_id]++;
        
        if (status.ok()) {
            stats_.successful_requests++;
            if (response.found()) {
                return response.value();
            }
            return std::nullopt;
        } else {
            stats_.failed_requests++;
            std::cerr << "RPC failed for key '" << key << "': " 
                      << status.error_message() << std::endl;
            return std::nullopt;
        }
    }
}

bool ShardRouter::Delete(const std::string& key) {
    std::string shard_id = hash_ring_->GetShardForKey(key);
    
    if (shard_id.empty()) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    auto& stub = GetShardStub(shard_id);
    if (!stub) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    DeleteRequest request;
    request.set_key(key);
    
    DeleteResponse response;
    ClientContext context;
    
    Status status = stub->Delete(&context, request, &response);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
        stats_.per_shard_requests[shard_id]++;
        
        if (status.ok()) {
            stats_.successful_requests++;
            return response.found();
        } else {
            stats_.failed_requests++;
            return false;
        }
    }
}

bool ShardRouter::Contains(const std::string& key) {
    std::string shard_id = hash_ring_->GetShardForKey(key);
    
    if (shard_id.empty()) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    auto& stub = GetShardStub(shard_id);
    if (!stub) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    ContainsRequest request;
    request.set_key(key);
    
    ContainsResponse response;
    ClientContext context;
    
    Status status = stub->Contains(&context, request, &response);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
        stats_.per_shard_requests[shard_id]++;
        
        if (status.ok()) {
            stats_.successful_requests++;
            return response.exists();
        } else {
            stats_.failed_requests++;
            return false;
        }
    }
}

bool ShardRouter::Expire(const std::string& key, int seconds) {
    std::string shard_id = hash_ring_->GetShardForKey(key);
    
    if (shard_id.empty()) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    auto& stub = GetShardStub(shard_id);
    if (!stub) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return false;
    }
    
    ExpireRequest request;
    request.set_key(key);
    request.set_seconds(seconds);
    
    ExpireResponse response;
    ClientContext context;
    
    Status status = stub->Expire(&context, request, &response);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
        stats_.per_shard_requests[shard_id]++;
        
        if (status.ok()) {
            stats_.successful_requests++;
            return response.success();
        } else {
            stats_.failed_requests++;
            return false;
        }
    }
}

int ShardRouter::TTL(const std::string& key) {
    std::string shard_id = hash_ring_->GetShardForKey(key);
    
    if (shard_id.empty()) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return -2;
    }
    
    auto& stub = GetShardStub(shard_id);
    if (!stub) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.total_requests++;
            stats_.failed_requests++;
        }
        return -2;
    }
    
    TTLRequest request;
    request.set_key(key);
    
    TTLResponse response;
    ClientContext context;
    
    Status status = stub->TTL(&context, request, &response);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_requests++;
        stats_.per_shard_requests[shard_id]++;
        
        if (status.ok()) {
            stats_.successful_requests++;
            return response.seconds();
        } else {
            stats_.failed_requests++;
            return -2;
        }
    }
}

std::unique_ptr<KeyValueStore::Stub>& ShardRouter::GetShardStub(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    // Check if we already have a connection to this shard
    auto it = shard_stubs_.find(shard_id);
    if (it != shard_stubs_.end()) {
        return it->second;
    }
    
    // Connection doesn't exist, create it
    const ShardInfo* shard = hash_ring_->GetShard(shard_id);
    if (!shard) {
        static std::unique_ptr<KeyValueStore::Stub> null_stub;
        return null_stub;
    }
    
    CreateShardConnection(shard_id, shard->address);
    return shard_stubs_[shard_id];
}

void ShardRouter::CreateShardConnection(const std::string& shard_id, const std::string& address) {
    // Create a gRPC channel to the shard
    // Using InsecureChannelCredentials for simplicity (use SSL in production!)
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    
    // Create a stub (client) for the KeyValueStore service
    auto stub = KeyValueStore::NewStub(channel);
    
    shard_stubs_[shard_id] = std::move(stub);
    
    std::cout << "Created connection to shard '" << shard_id << "' at " << address << std::endl;
}

void ShardRouter::RemoveShardConnection(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    shard_stubs_.erase(shard_id);
    std::cout << "Removed connection to shard '" << shard_id << "'" << std::endl;
}

ShardRouter::RoutingStats ShardRouter::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void ShardRouter::ResetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_requests = 0;
    stats_.successful_requests = 0;
    stats_.failed_requests = 0;
    stats_.per_shard_requests.clear();
}

} // namespace kvstore
