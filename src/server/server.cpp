#include "server.h"
#include "../storage/storage.h"
#include "../service/kvstore_service.h"
#include "../replication/replication_manager.h"
#include <iostream>

namespace kvstore {

Server::Server(const std::string& address, bool is_master)
    : server_address_(address),
      is_master_(is_master),
      storage_(std::make_shared<Storage>("kvstore.rdb", "kvstore.aof")),
      replication_manager_(std::make_shared<ReplicationManager>(
          is_master ? NodeRole::MASTER : NodeRole::REPLICA)),
      service_(std::make_unique<KeyValueStoreServiceImpl>(storage_)) {
    
    storage_->SetReplicationManager(replication_manager_);
    storage_->StartBackgroundSnapshot(60);
    
    std::cout << "Server initialized as " << (is_master ? "MASTER" : "REPLICA") << std::endl;
}

Server::~Server() {
    Shutdown();
}

void Server::Run() {
    grpc::ServerBuilder builder;
    
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
    
    grpc_server_ = builder.BuildAndStart();
    
    if (grpc_server_) {
        std::cout << "Server listening on " << server_address_ << std::endl;
        grpc_server_->Wait();
    } else {
        std::cerr << "Failed to start server" << std::endl;
    }
}

void Server::Shutdown() {
    if (grpc_server_) {
        grpc_server_->Shutdown();
    }
}

void Server::AddReplica(const std::string& replica_address) {
    if (!is_master_) {
        std::cerr << "Only master can add replicas" << std::endl;
        return;
    }
    replication_manager_->AddReplica(replica_address);
}

void Server::SetMaster(const std::string& master_address) {
    if (is_master_) {
        std::cerr << "This node is configured as master" << std::endl;
        return;
    }
    replication_manager_->SetMasterAddress(master_address);
}

} // namespace kvstore
