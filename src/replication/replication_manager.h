#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"

namespace kvstore {

enum class NodeRole {
    MASTER,
    REPLICA
};

class ReplicationManager {
public:
    explicit ReplicationManager(NodeRole role);
    ~ReplicationManager();

    void SetRole(NodeRole role);
    NodeRole GetRole() const { return role_; }
    bool IsMaster() const { return role_ == NodeRole::MASTER; }
    bool IsReplica() const { return role_ == NodeRole::REPLICA; }

    void AddReplica(const std::string& replica_address);
    void RemoveReplica(const std::string& replica_address);
    
    void ReplicateSet(const std::string& key, const std::string& value);
    void ReplicateDelete(const std::string& key);
    void ReplicateExpire(const std::string& key, int seconds);

    void SetMasterAddress(const std::string& master_address);
    std::string GetMasterAddress() const { return master_address_; }

private:
    struct ReplicaConnection {
        std::string address;
        std::shared_ptr<grpc::Channel> channel;
        std::unique_ptr<KeyValueStore::Stub> stub;
    };

    void ReplicateCommand(const ReplicationCommand& command);
    int64_t GetNextSequenceId();

    NodeRole role_;
    std::string master_address_;
    std::vector<std::unique_ptr<ReplicaConnection>> replicas_;
    std::mutex replicas_mutex_;
    std::atomic<int64_t> sequence_counter_{0};
};

} // namespace kvstore
