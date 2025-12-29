#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

namespace kvstore {

class RDBPersistence {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    
    explicit RDBPersistence(const std::string& filename);
    
    bool SaveSnapshot(
        const std::unordered_map<std::string, std::string>& data,
        const std::unordered_map<std::string, TimePoint>& expiration
    );
    
    bool LoadSnapshot(
        std::unordered_map<std::string, std::string>& data,
        std::unordered_map<std::string, TimePoint>& expiration
    );
    
private:
    std::string filename_;
};

} // namespace kvstore
