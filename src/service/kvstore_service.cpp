#include "kvstore_service.h"

namespace kvstore {

KeyValueStoreServiceImpl::KeyValueStoreServiceImpl(std::shared_ptr<Storage> storage)
    : storage_(storage) {
}

grpc::Status KeyValueStoreServiceImpl::Get(grpc::ServerContext* context,
                                          const GetRequest* request,
                                          GetResponse* response) {
    if (request->key().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Key cannot be empty");
    }

    auto value = storage_->Get(request->key());
    if (value.has_value()) {
        response->set_found(true);
        response->set_value(value.value());
    } else {
        response->set_found(false);
    }
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::Set(grpc::ServerContext* context,
                                          const SetRequest* request,
                                          SetResponse* response) {
    if (request->key().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Key cannot be empty");
    }

    storage_->Set(request->key(), request->value());
    response->set_success(true);
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::Contains(grpc::ServerContext* context,
                                               const ContainsRequest* request,
                                               ContainsResponse* response) {
    if (request->key().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Key cannot be empty");
    }

    bool exists = storage_->Contains(request->key());
    response->set_exists(exists);
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::Delete(grpc::ServerContext* context,
                                             const DeleteRequest* request,
                                             DeleteResponse* response) {
    if (request->key().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Key cannot be empty");
    }

    bool found = storage_->Delete(request->key());
    response->set_success(true);
    response->set_found(found);
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::Expire(grpc::ServerContext* context,
                                              const ExpireRequest* request,
                                              ExpireResponse* response) {
    if (request->key().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Key cannot be empty");
    }

    if (request->seconds() <= 0) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Seconds must be positive");
    }

    bool success = storage_->Expire(request->key(), request->seconds());
    response->set_success(success);
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::TTL(grpc::ServerContext* context,
                                           const TTLRequest* request,
                                           TTLResponse* response) {
    if (request->key().empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Key cannot be empty");
    }

    int ttl = storage_->TTL(request->key());
    response->set_seconds(ttl);
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::ReplicateCommand(grpc::ServerContext* context,
                                                        const ReplicationCommand* request,
                                                        ReplicationResponse* response) {
    switch (request->type()) {
        case ReplicationCommand::SET:
            storage_->SetFromReplication(request->key(), request->value());
            break;
        
        case ReplicationCommand::DELETE:
            storage_->DeleteFromReplication(request->key());
            break;
        
        case ReplicationCommand::EXPIRE:
            storage_->ExpireFromReplication(request->key(), request->seconds());
            break;
        
        default:
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Unknown command type");
    }
    
    response->set_success(true);
    response->set_last_applied_sequence(request->sequence_id());
    
    return grpc::Status::OK;
}

grpc::Status KeyValueStoreServiceImpl::StreamReplication(grpc::ServerContext* context,
                                                         const ReplicationStreamRequest* request,
                                                         grpc::ServerWriter<ReplicationCommand>* writer) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Streaming replication not yet implemented");
}

} // namespace kvstore
