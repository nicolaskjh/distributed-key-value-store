#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

namespace kvstore {

class KeyValueStoreServiceImpl;
class Storage;

/**
 * gRPC server wrapper for the key-value store
 */
class Server {
public:
    /**
     * Create a server with the specified configuration
     * @param address Server address (e.g., "0.0.0.0:50051")
     */
    explicit Server(const std::string& address);
    ~Server();

    /**
     * Start the server (blocking call)
     */
    void Run();

    /**
     * Shutdown the server gracefully
     */
    void Shutdown();

private:
    std::string server_address_;
    std::shared_ptr<Storage> storage_;
    std::unique_ptr<KeyValueStoreServiceImpl> service_;
    std::unique_ptr<grpc::Server> grpc_server_;
};

} // namespace kvstore
