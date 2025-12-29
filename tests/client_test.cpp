#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

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

    bool Expire(const std::string& key, int seconds) {
        kvstore::ExpireRequest request;
        request.set_key(key);
        request.set_seconds(seconds);

        kvstore::ExpireResponse response;
        ClientContext context;

        Status status = stub_->Expire(&context, request, &response);

        if (status.ok()) {
            return response.success();
        } else {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return false;
        }
    }

    int TTL(const std::string& key) {
        kvstore::TTLRequest request;
        request.set_key(key);

        kvstore::TTLResponse response;
        ClientContext context;

        Status status = stub_->TTL(&context, request, &response);

        if (status.ok()) {
            return response.seconds();
        } else {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return -2;
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

    std::cout << "\nTesting TTL/EXPIRE..." << std::endl;
    client.Set("temp_key", "temp_value");
    std::cout << "SET temp_key=temp_value" << std::endl;
    
    bool expired = client.Expire("temp_key", 5);
    std::cout << "EXPIRE temp_key 5s -> " << (expired ? "success" : "failed") << std::endl;
    
    int ttl = client.TTL("temp_key");
    std::cout << "TTL temp_key -> " << ttl << " seconds" << std::endl;
    std::cout.flush();
    
    std::cout << "Waiting 3 seconds..." << std::endl;
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    ttl = client.TTL("temp_key");
    std::cout << "TTL temp_key -> " << ttl << " seconds" << std::endl;
    std::cout.flush();
    
    std::cout << "Waiting 3 more seconds..." << std::endl;
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "Checking if key expired..." << std::endl;
    std::cout.flush();
    auto [found_temp, value_temp] = client.Get("temp_key");
    std::cout << "GET temp_key -> " << (found_temp ? value_temp : "EXPIRED/NOT FOUND") << std::endl;
    std::cout.flush();

    std::cout << "\nTests completed" << std::endl;
    std::cout.flush();

    return 0;
}
