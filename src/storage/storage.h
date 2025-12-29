#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>

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

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

} // namespace kvstore
