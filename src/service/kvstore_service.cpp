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

} // namespace kvstore
