#include "server.h"
#include "../storage/storage.h"
#include "../service/kvstore_service.h"
#include <iostream>

namespace kvstore {

Server::Server(const std::string& address)
    : server_address_(address),
      storage_(std::make_shared<Storage>()),
      service_(std::make_unique<KeyValueStoreServiceImpl>(storage_)) {
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

} // namespace kvstore
