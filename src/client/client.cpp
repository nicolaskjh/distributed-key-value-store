#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"
#include <iostream>
#include <memory>
#include <string>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class KeyValueStoreClient {
public:
    KeyValueStoreClient(std::shared_ptr<Channel> channel)
        : stub_(kvstore::KeyValueStore::NewStub(channel)) {}

    bool Set(const std::string& key, const std::string& value) {
        kvstore::SetRequest request;
        request.set_key(key);
        request.set_value(value);

        kvstore::SetResponse response;
        ClientContext context;

        Status status = stub_->Set(&context, request, &response);

        if (status.ok()) {
            return response.success();
        } else {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    std::pair<bool, std::string> Get(const std::string& key) {
        kvstore::GetRequest request;
        request.set_key(key);

        kvstore::GetResponse response;
        ClientContext context;

        Status status = stub_->Get(&context, request, &response);

        if (status.ok()) {
            return {response.found(), response.value()};
        } else {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return {false, ""};
        }
    }

    bool Delete(const std::string& key) {
        kvstore::DeleteRequest request;
        request.set_key(key);

        kvstore::DeleteResponse response;
        ClientContext context;

        Status status = stub_->Delete(&context, request, &response);

        if (status.ok()) {
            return response.found();
        } else {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    bool Contains(const std::string& key) {
        kvstore::ContainsRequest request;
        request.set_key(key);

        kvstore::ContainsResponse response;
        ClientContext context;

        Status status = stub_->Contains(&context, request, &response);

        if (status.ok()) {
            return response.exists();
        } else {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<kvstore::KeyValueStore::Stub> stub_;
};

int main(int argc, char** argv) {
    std::string server_address = "localhost:50051";
    
    if (argc > 1) {
        server_address = argv[1];
    }

    std::cout << "Connecting to " << server_address << std::endl;

    KeyValueStoreClient client(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials())
    );

    std::cout << "\nTesting SET..." << std::endl;
    client.Set("name", "Alice");
    client.Set("age", "30");
    std::cout << "SET name=Alice, age=30" << std::endl;

    std::cout << "\nTesting GET..." << std::endl;
    auto [found1, value1] = client.Get("name");
    std::cout << "GET name -> " << (found1 ? value1 : "NOT FOUND") << std::endl;
    
    auto [found2, value2] = client.Get("age");
    std::cout << "GET age -> " << (found2 ? value2 : "NOT FOUND") << std::endl;

    std::cout << "\nTesting CONTAINS..." << std::endl;
    std::cout << "CONTAINS name -> " << (client.Contains("name") ? "true" : "false") << std::endl;
    std::cout << "CONTAINS missing -> " << (client.Contains("missing") ? "true" : "false") << std::endl;

    std::cout << "\nTesting DELETE..." << std::endl;
    bool deleted = client.Delete("age");
    std::cout << "DELETE age -> " << (deleted ? "deleted" : "not found") << std::endl;
    
    auto [found3, value3] = client.Get("age");
    std::cout << "GET age -> " << (found3 ? value3 : "NOT FOUND") << std::endl;

    std::cout << "\nTests completed" << std::endl;

    return 0;
}
