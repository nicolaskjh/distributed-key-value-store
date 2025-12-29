#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <chrono>

namespace kvstore {

/**
 * Thread-safe in-memory key-value storage
 */
class Storage {
public:
    Storage() = default;
    ~Storage() = default;

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    /**
     * Set a key-value pair
     * @param key The key to set
     * @param value The value to associate with the key
     */
    void Set(const std::string& key, const std::string& value);

    /**
     * Get the value associated with a key
     * @param key The key to look up
     * @return The value if found, std::nullopt otherwise
     */
    std::optional<std::string> Get(const std::string& key) const;

    /**
     * Check if a key exists in the storage
     * @param key The key to check
     * @return true if the key exists, false otherwise
     */
    bool Contains(const std::string& key) const;

    /**
     * Delete a key-value pair
     * @param key The key to delete
     * @return true if the key existed and was deleted, false otherwise
     */
    bool Delete(const std::string& key);

    /**
     * Get the number of keys stored
     * @return The number of key-value pairs
     */
    size_t Size() const;

    /**
     * Set expiration time for a key (in seconds from now)
     * @param key The key to set expiration for
     * @param seconds Number of seconds until expiration
     * @return true if key exists and expiration was set, false otherwise
     */
    bool Expire(const std::string& key, int seconds);

    /**
     * Get remaining time to live for a key (in seconds)
     * @param key The key to check
     * @return Seconds remaining, or -1 if no expiration, or -2 if key doesn't exist
     */
    int TTL(const std::string& key) const;

private:
    using TimePoint = std::chrono::steady_clock::time_point;
    
    bool IsExpired(const std::string& key) const;
    void RemoveExpired(const std::string& key) const;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
    std::unordered_map<std::string, TimePoint> expiration_;
};

} // namespace kvstore
