#include <grpcpp/grpcpp.h>
#include "kvstore.pb.h"
#include "kvstore.grpc.pb.h"
#include <iostream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class KeyValueStoreClient {
public:
    KeyValueStoreClient(std::shared_ptr<Channel> channel)
        : stub_(kvstore::KeyValueStore::NewStub(channel)) {}

    void Get(const std::string& key) {
        kvstore::GetRequest request;
        request.set_key(key);
        
        kvstore::GetResponse response;
        ClientContext context;
        
        Status status = stub_->Get(&context, request, &response);
        if (status.ok() && response.found()) {
            std::cout << "✓ GET " << key << " -> " << response.value() << std::endl;
        } else {
            std::cout << "✗ GET " << key << " -> NOT FOUND" << std::endl;
        }
    }

private:
    std::unique_ptr<kvstore::KeyValueStore::Stub> stub_;
};

int main() {
    std::string server_address("localhost:50051");
    
    KeyValueStoreClient client(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    std::cout << "Verifying data persisted from previous session..." << std::endl;
    client.Get("name");
    client.Get("temp_key");
    
    return 0;
}
