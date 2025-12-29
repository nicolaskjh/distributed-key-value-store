#include "replication_manager.h"
#include <iostream>

namespace kvstore {

ReplicationManager::ReplicationManager(NodeRole role)
    : role_(role) {
}

ReplicationManager::~ReplicationManager() {
}

void ReplicationManager::SetRole(NodeRole role) {
    role_ = role;
    std::cout << "Node role set to: " << (role == NodeRole::MASTER ? "MASTER" : "REPLICA") << std::endl;
}

void ReplicationManager::AddReplica(const std::string& replica_address) {
    if (!IsMaster()) {
        std::cerr << "Only master nodes can add replicas" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    auto replica = std::make_unique<ReplicaConnection>();
    replica->address = replica_address;
    replica->channel = grpc::CreateChannel(replica_address, grpc::InsecureChannelCredentials());
    replica->stub = KeyValueStore::NewStub(replica->channel);
    
    replicas_.push_back(std::move(replica));
    
    std::cout << "Added replica: " << replica_address << std::endl;
}

void ReplicationManager::RemoveReplica(const std::string& replica_address) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    replicas_.erase(
        std::remove_if(replicas_.begin(), replicas_.end(),
            [&replica_address](const std::unique_ptr<ReplicaConnection>& r) {
                return r->address == replica_address;
            }),
        replicas_.end()
    );
    
    std::cout << "Removed replica: " << replica_address << std::endl;
}

void ReplicationManager::ReplicateSet(const std::string& key, const std::string& value) {
    if (!IsMaster()) return;
    
    ReplicationCommand command;
    command.set_type(ReplicationCommand::SET);
    command.set_key(key);
    command.set_value(value);
    command.set_sequence_id(GetNextSequenceId());
    
    ReplicateCommand(command);
}

void ReplicationManager::ReplicateDelete(const std::string& key) {
    if (!IsMaster()) return;
    
    ReplicationCommand command;
    command.set_type(ReplicationCommand::DELETE);
    command.set_key(key);
    command.set_sequence_id(GetNextSequenceId());
    
    ReplicateCommand(command);
}

void ReplicationManager::ReplicateExpire(const std::string& key, int seconds) {
    if (!IsMaster()) return;
    
    ReplicationCommand command;
    command.set_type(ReplicationCommand::EXPIRE);
    command.set_key(key);
    command.set_seconds(seconds);
    command.set_sequence_id(GetNextSequenceId());
    
    ReplicateCommand(command);
}

void ReplicationManager::ReplicateCommand(const ReplicationCommand& command) {
    std::lock_guard<std::mutex> lock(replicas_mutex_);
    
    for (const auto& replica : replicas_) {
        grpc::ClientContext context;
        ReplicationResponse response;
        
        grpc::Status status = replica->stub->ReplicateCommand(&context, command, &response);
        
        if (!status.ok()) {
            std::cerr << "Failed to replicate to " << replica->address 
                      << ": " << status.error_message() << std::endl;
        }
    }
}

int64_t ReplicationManager::GetNextSequenceId() {
    return sequence_counter_.fetch_add(1);
}

void ReplicationManager::SetMasterAddress(const std::string& master_address) {
    master_address_ = master_address;
    std::cout << "Master address set to: " << master_address << std::endl;
}

} // namespace kvstore
