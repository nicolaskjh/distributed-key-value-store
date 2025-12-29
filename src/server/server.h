#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <vector>

namespace kvstore {

class KeyValueStoreServiceImpl;
class Storage;
class ReplicationManager;

class Server {
public:
    explicit Server(const std::string& address, bool is_master = true);
    ~Server();

    void Run();
    void Shutdown();
    
    void AddReplica(const std::string& replica_address);
    void SetMaster(const std::string& master_address);

private:
    std::string server_address_;
    bool is_master_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<ReplicationManager> replication_manager_;
    std::unique_ptr<KeyValueStoreServiceImpl> service_;
    std::unique_ptr<grpc::Server> grpc_server_;
};

} // namespace kvstore
