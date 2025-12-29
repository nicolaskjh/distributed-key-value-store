#include <grpcpp/grpcpp.h>
#include "kvstore.pb.h"
#include "kvstore.grpc.pb.h"
#include <iostream>
#include <thread>
#include <chrono>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class KeyValueStoreClient {
public:
    KeyValueStoreClient(std::shared_ptr<Channel> channel)
        : stub_(kvstore::KeyValueStore::NewStub(channel)) {}

    void Set(const std::string& key, const std::string& value) {
        kvstore::SetRequest request;
        request.set_key(key);
        request.set_value(value);
        
        kvstore::SetResponse response;
        ClientContext context;
        
        Status status = stub_->Set(&context, request, &response);
        if (status.ok()) {
            std::cout << "SET " << key << "=" << value << std::endl;
        } else {
            std::cout << "SET failed: " << status.error_message() << std::endl;
        }
    }

    void Get(const std::string& key) {
        kvstore::GetRequest request;
        request.set_key(key);
        
        kvstore::GetResponse response;
        ClientContext context;
        
        Status status = stub_->Get(&context, request, &response);
        if (status.ok() && response.found()) {
            std::cout << "GET " << key << " -> " << response.value() << std::endl;
        } else {
            std::cout << "GET " << key << " -> NOT FOUND" << std::endl;
        }
    }

private:
    std::unique_ptr<kvstore::KeyValueStore::Stub> stub_;
};

int main() {
    std::string server_address("localhost:50051");
    
    KeyValueStoreClient client(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::cout << "Adding test data..." << std::endl;
    client.Set("user:1:name", "Alice");
    client.Set("user:1:email", "alice@example.com");
    client.Set("user:2:name", "Bob");
    client.Set("user:2:email", "bob@example.com");
    client.Set("config:version", "1.0.0");
    
    std::cout << "\nWaiting 65 seconds for snapshot to be created..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(65));
    
    std::cout << "\nReading data back..." << std::endl;
    client.Get("user:1:name");
    client.Get("user:1:email");
    client.Get("user:2:name");
    client.Get("user:2:email");
    client.Get("config:version");
    
    std::cout << "\nTest complete. Check for kvstore.rdb file." << std::endl;
    
    return 0;
}
