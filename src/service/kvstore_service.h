#pragma once

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"
#include "../storage/storage.h"
#include <memory>

namespace kvstore {

/**
 * gRPC service implementation for KeyValueStore
 */
class KeyValueStoreServiceImpl final : public KeyValueStore::Service {
public:
    explicit KeyValueStoreServiceImpl(std::shared_ptr<Storage> storage);

    grpc::Status Get(grpc::ServerContext* context,
                    const GetRequest* request,
                    GetResponse* response) override;

    grpc::Status Set(grpc::ServerContext* context,
                    const SetRequest* request,
                    SetResponse* response) override;

    grpc::Status Contains(grpc::ServerContext* context,
                         const ContainsRequest* request,
                         ContainsResponse* response) override;

    grpc::Status Delete(grpc::ServerContext* context,
                       const DeleteRequest* request,
                       DeleteResponse* response) override;

    grpc::Status Expire(grpc::ServerContext* context,
                       const ExpireRequest* request,
                       ExpireResponse* response) override;

    grpc::Status TTL(grpc::ServerContext* context,
                    const TTLRequest* request,
                    TTLResponse* response) override;

    grpc::Status ReplicateCommand(grpc::ServerContext* context,
                                 const ReplicationCommand* request,
                                 ReplicationResponse* response) override;

    grpc::Status StreamReplication(grpc::ServerContext* context,
                                   const ReplicationStreamRequest* request,
                                   grpc::ServerWriter<ReplicationCommand>* writer) override;

private:
    std::shared_ptr<Storage> storage_;
};

} // namespace kvstore
