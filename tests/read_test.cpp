#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"
#include <iostream>

int main(int argc, char** argv) {
    std::string server_address = "localhost:50051";
    
    if (argc > 1) {
        server_address = argv[1];
    }

    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = kvstore::KeyValueStore::NewStub(channel);

    std::cout << "Reading from " << server_address << std::endl;

    kvstore::GetRequest request;
    request.set_key("name");
    
    kvstore::GetResponse response;
    grpc::ClientContext context;
    
    grpc::Status status = stub->Get(&context, request, &response);
    
    if (status.ok() && response.found()) {
        std::cout << "GET name -> " << response.value() << std::endl;
    } else {
        std::cout << "GET name -> NOT FOUND" << std::endl;
    }

    return 0;
}
