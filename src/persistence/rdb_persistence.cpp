#include "rdb_persistence.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace kvstore {

using namespace std::chrono;

RDBPersistence::RDBPersistence(const std::string& filename)
    : filename_(filename) {
}

bool RDBPersistence::SaveSnapshot(
    const std::unordered_map<std::string, std::string>& data,
    const std::unordered_map<std::string, TimePoint>& expiration
) {
    std::string temp_file = filename_ + ".tmp";
    std::ofstream file(temp_file, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Failed to create snapshot: " << temp_file << std::endl;
        return false;
    }
    
    file << "REDIS0011" << "\n";
    
    auto now = steady_clock::now();
    
    for (const auto& [key, value] : data) {
        auto exp_it = expiration.find(key);
        
        if (exp_it != expiration.end()) {
            if (exp_it->second <= now) {
                continue;
            }
            
            auto remaining = duration_cast<seconds>(exp_it->second - now);
            file << "EXPIRE " << key << " " << remaining.count() << "\n";
        }
        
        std::string escaped_value = value;
        size_t pos = 0;
        while ((pos = escaped_value.find('\n', pos)) != std::string::npos) {
            escaped_value.replace(pos, 1, "\\n");
            pos += 2;
        }
        
        file << "SET " << key << " " << escaped_value << "\n";
    }
    
    file << "EOF\n";
    file.close();
    
    std::rename(temp_file.c_str(), filename_.c_str());
    
    std::cout << "Snapshot saved: " << filename_ << " (" << data.size() << " keys)" << std::endl;
    return true;
}

bool RDBPersistence::LoadSnapshot(
    std::unordered_map<std::string, std::string>& data,
    std::unordered_map<std::string, TimePoint>& expiration
) {
    std::ifstream file(filename_);
    
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::getline(file, line);
    
    if (line != "REDIS0011") {
        std::cerr << "Invalid RDB format" << std::endl;
        return false;
    }
    
    std::unordered_map<std::string, int> pending_expires;
    
    while (std::getline(file, line)) {
        if (line == "EOF") break;
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string cmd, key, value;
        
        iss >> cmd >> key;
        
        if (cmd == "SET") {
            std::getline(iss, value);
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            
            size_t pos = 0;
            while ((pos = value.find("\\n", pos)) != std::string::npos) {
                value.replace(pos, 2, "\n");
                pos += 1;
            }
            
            data[key] = value;
            
            auto exp_it = pending_expires.find(key);
            if (exp_it != pending_expires.end()) {
                auto expiry_time = steady_clock::now() + seconds(exp_it->second);
                expiration[key] = expiry_time;
            }
        } else if (cmd == "EXPIRE") {
            iss >> value;
            pending_expires[key] = std::stoi(value);
        }
    }
    
    file.close();
    
    std::cout << "Snapshot loaded: " << filename_ << " (" << data.size() << " keys)" << std::endl;
    return true;
}

} // namespace kvstore
