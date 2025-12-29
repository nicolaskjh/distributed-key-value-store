#include "storage.h"

namespace kvstore {

using namespace std::chrono;

void Storage::Set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_[key] = value;
}

std::optional<std::string> Storage::Get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (IsExpired(key)) {
        RemoveExpired(key);
        return std::nullopt;
    }
    
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool Storage::Contains(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (IsExpired(key)) {
        RemoveExpired(key);
        return false;
    }
    
    return data_.find(key) != data_.end();
}

bool Storage::Delete(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    expiration_.erase(key);
    return data_.erase(key) > 0;
}

size_t Storage::Size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return data_.size();
}

bool Storage::Expire(const std::string& key, int seconds) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (data_.find(key) == data_.end()) {
        return false;
    }
    
    auto expiry_time = steady_clock::now() + std::chrono::seconds(seconds);
    expiration_[key] = expiry_time;
    return true;
}

int Storage::TTL(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (data_.find(key) == data_.end()) {
        return -2;
    }
    
    auto it = expiration_.find(key);
    if (it == expiration_.end()) {
        return -1;
    }
    
    auto now = steady_clock::now();
    if (it->second <= now) {
        return 0;
    }
    
    auto remaining = duration_cast<seconds>(it->second - now);
    return static_cast<int>(remaining.count());
}

bool Storage::IsExpired(const std::string& key) const {
    auto it = expiration_.find(key);
    if (it == expiration_.end()) {
        return false;
    }
    
    return it->second <= steady_clock::now();
}

void Storage::RemoveExpired(const std::string& key) const {
    std::shared_lock<std::shared_mutex> shared_lock(mutex_);
    shared_lock.unlock();
    
    std::unique_lock<std::shared_mutex> unique_lock(mutex_);
    
    auto exp_it = expiration_.find(key);
    if (exp_it != expiration_.end() && exp_it->second <= steady_clock::now()) {
        const_cast<std::unordered_map<std::string, TimePoint>&>(expiration_).erase(key);
        const_cast<std::unordered_map<std::string, std::string>&>(data_).erase(key);
    }
}

} // namespace kvstore
