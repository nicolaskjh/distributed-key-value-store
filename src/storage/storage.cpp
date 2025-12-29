#include "storage.h"
#include "aof_persistence.h"
#include "rdb_persistence.h"
#include <iostream>

namespace kvstore {

using namespace std::chrono;

Storage::Storage(const std::string& rdb_filename, const std::string& aof_filename) {
    if (!rdb_filename.empty()) {
        rdb_ = std::make_unique<RDBPersistence>(rdb_filename);
        rdb_->LoadSnapshot(data_, expiration_);
    }
    
    if (!aof_filename.empty()) {
        aof_ = std::make_unique<AOFPersistence>(aof_filename);
        
        aof_->Replay([this](const std::string& cmd, const std::string& key, const std::string& value) {
            if (cmd == "SET") {
                data_[key] = value;
            } else if (cmd == "DELETE") {
                data_.erase(key);
                expiration_.erase(key);
            } else if (cmd == "EXPIRE") {
                int seconds = std::stoi(value);
                auto expiry_time = steady_clock::now() + std::chrono::seconds(seconds);
                expiration_[key] = expiry_time;
            }
        });
        
        aof_->Enable();
    }
}

Storage::~Storage() {
    StopBackgroundSnapshot();
}

void Storage::Set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_[key] = value;
    
    // Log to AOF if enabled
    if (aof_ && aof_->IsEnabled()) {
        aof_->LogSet(key, value);
    }
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
    bool found = data_.erase(key) > 0;
    
    // Log to AOF if enabled and key was found
    if (found && aof_ && aof_->IsEnabled()) {
        aof_->LogDelete(key);
    }
    
    return found;
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
    
    // Log to AOF if enabled
    if (aof_ && aof_->IsEnabled()) {
        aof_->LogExpire(key, seconds);
    }
    
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

void Storage::SaveSnapshot() {
    if (!rdb_) return;
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    rdb_->SaveSnapshot(data_, expiration_);
}

void Storage::StartBackgroundSnapshot(int interval_seconds) {
    if (!rdb_ || snapshot_running_) return;
    
    snapshot_interval_ = interval_seconds;
    snapshot_running_ = true;
    snapshot_thread_ = std::make_unique<std::thread>(&Storage::SnapshotLoop, this);
}

void Storage::StopBackgroundSnapshot() {
    snapshot_running_ = false;
    if (snapshot_thread_ && snapshot_thread_->joinable()) {
        snapshot_thread_->join();
    }
}

void Storage::SnapshotLoop() {
    while (snapshot_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(snapshot_interval_));
        if (snapshot_running_) {
            SaveSnapshot();
        }
    }
}

} // namespace kvstore
