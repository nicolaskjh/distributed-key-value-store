#include "storage.h"

namespace kvstore {

void Storage::Set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_[key] = value;
}

std::optional<std::string> Storage::Get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool Storage::Contains(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

bool Storage::Delete(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return data_.erase(key) > 0;
}

size_t Storage::Size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return data_.size();
}

} // namespace kvstore
