#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>

namespace kvstore {

class AOFPersistence;
class RDBPersistence;
class ReplicationManager;

class Storage {
public:
    explicit Storage(const std::string& rdb_filename = "", const std::string& aof_filename = "");
    ~Storage();

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    void Set(const std::string& key, const std::string& value);
    void SetFromReplication(const std::string& key, const std::string& value);

    std::optional<std::string> Get(const std::string& key) const;
    bool Contains(const std::string& key) const;
    
    bool Delete(const std::string& key);
    bool DeleteFromReplication(const std::string& key);
    
    size_t Size() const;
    
    bool Expire(const std::string& key, int seconds);
    bool ExpireFromReplication(const std::string& key, int seconds);
    
    int TTL(const std::string& key) const;

    void SaveSnapshot();
    void StartBackgroundSnapshot(int interval_seconds);
    void StopBackgroundSnapshot();
    
    void SetReplicationManager(std::shared_ptr<ReplicationManager> replication_manager);

private:
    using TimePoint = std::chrono::steady_clock::time_point;
    
    bool IsExpired(const std::string& key) const;
    void RemoveExpired(const std::string& key) const;
    void SnapshotLoop();

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
    std::unordered_map<std::string, TimePoint> expiration_;
    std::unique_ptr<AOFPersistence> aof_;
    std::unique_ptr<RDBPersistence> rdb_;
    std::shared_ptr<ReplicationManager> replication_manager_;
    
    std::atomic<bool> snapshot_running_{false};
    std::unique_ptr<std::thread> snapshot_thread_;
    int snapshot_interval_{0};
};

} // namespace kvstore
